#include <stdlib.h>
#include <stdio.h>
#include "losum.h"
#include "prng.h"
#include "math.h"
#include <windows.h>

#define STEPS_AT_A_TIME 1
#define BLOCK_SIZE 1
#define LS_NULLITEM 0x7FFFFFFF
#define swap(x,y) do{int t=x; x=y; y=t;} while(0)

inline void LS_FinishStep(LS_type* LS) {
	--(LS->blocksLeft);
	if ((--(LS->blocksLeftThisUpdate)) == 0) {
		if (ReleaseSemaphore(LS->finishUpdateSemaphore, 1, NULL)==0) {
			int err = GetLastError();
		}
	}
}
void LS_InitPassive(LS_type *LS) {
	for (int i = 0; i < LS->hashsize; ++i) {
		LS_FinishStep(LS);
		LS->passiveHashtable[i] = NULL;
	}
	LS->nPassive = 0;
}

LS_type * LS_Init(float fPhi, float gamma)
{
	fPhi = (float) (1. / (1. / fPhi + 1));
	int i;
	int k = 1 + (int) (1.0 / fPhi);
	
	LS_type *result = (LS_type *)calloc(1, sizeof(LS_type));
	// needs to be odd so that the heap always has either both children or 
	// no children present in the data structure
	result->epsilon = fPhi;
	// nitems in large table
	result->nActive = 0;
	result->size = int(ceil(gamma / fPhi) + ceil(1 / fPhi) - 1);
	result->hashsize = LS_HASHMULT*result->size;
	result->maxMaintenanceTime = 24*result->size + result->hashsize + 1;

	result->hasha = 151261303;
	result->hashb = 6722461; // hard coded constants for the hash table,
							 //should really generate these randomly
	result->n = (LSweight_t)0;

	result->activeHashtable =
		(LSCounter **)calloc(result->hashsize, sizeof(LSCounter*));
	for (i = 0; i < result->hashsize; ++i) {
		result->activeHashtable[i] = NULL;
	}
	result->activeCounters =
		(LSCounter*)calloc(result->size, sizeof(LSCounter));
	for (i = 0; i < result->size; i++)
	{
		result->activeCounters[i].next = NULL;
		result->activeCounters[i].item = LS_NULLITEM;
	}
	result->passiveCounters =
		(LSCounter*)calloc(result->size, sizeof(LSCounter));
	result->passiveHashtable =
		(LSCounter **)calloc(result->hashsize, sizeof(LSCounter*));
	for (int i = 0; i < result->hashsize; ++i) {
		result->passiveHashtable[i] = NULL;
	}
	result->nPassive = 0;
	result->quantile = 0;
	result->buffer =
		(int*)calloc(result->size, sizeof(int));
	result->handle = NULL;	
	DWORD threadID;
	result->handle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size
		LS_Maintenance,         // thread function name
		result,					// argument to thread function
		0,                      // use default creation flags
		&threadID);				// returns the thread identifier

	result->maintenanceStepSemaphore = CreateSemaphore(NULL, 0, 
		1, NULL);
	if (result->maintenanceStepSemaphore == NULL) {
		std::cout << "CreateSemaphore error: " << GetLastError() << std::endl;
		ExitProcess(3);
	}
	result->finishUpdateSemaphore = CreateSemaphore(NULL, 0, 
		1, NULL);
	if (result->finishUpdateSemaphore == NULL) {
		std::cout << "CreateSemaphore error: " << GetLastError() << std::endl;
		ExitProcess(3);
	}
	if (result->handle == NULL)
	{
		std::cerr << "Error! Could not create thread!" << std::endl;
		std::cerr << GetLastError() << std::endl;
		ExitProcess(3);
	}

	result->blocksLeft = 0;
	result->left2Move = 0;
	result->done = false;
	result->finishedMedian = false;
	result->stepsLeft = 0;
	result->movedFromPassive = 0;
	result->clearedFromPassive = result->hashsize;
	result->copied2Buffer = 0;
	return(result);
}

void LS_DestroyPassive(LS_type* LS) {
	LS->done = true;
	ReleaseSemaphore(LS->maintenanceStepSemaphore,1,NULL);
	free(LS->passiveHashtable);
	free(LS->passiveCounters);
}
void LS_Destroy(LS_type * LS)
{
	std::cerr << "Destroy A" << std::endl;
	free(LS->activeHashtable);
	free(LS->activeCounters);
	free(LS->buffer);
	LS_DestroyPassive(LS);
	free(LS);
}


/*
If item was present when the function was called, find it.
If item was not present when the function finished, return NULL.
Else, do either.
Assume: No items are deleted during the runtime of the function.
*/
LSCounter * LS_FindItemInActive(LS_type * LS, LSitem_t item)
{ // find a particular item in the date structure and return a pointer to it
	LSCounter * hashptr;
	int hashval;
	hashval = (int)hash31(LS->hasha, LS->hashb, item) % LS->hashsize;
	hashptr = LS->activeHashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table
	while (hashptr) {
		if (hashptr->item == item)
			break;
		else hashptr = hashptr->next;
	}
	
	return hashptr;
	// returns NULL if we do not find the item
}


LSCounter** LS_CalculateLocation(LS_type* LS, LSitem_t item) {
	int hashval;
	hashval = (int)hash31(LS->hasha, LS->hashb, item) % LS->hashsize;
	return &(LS->activeHashtable[hashval]);
}

/*
If item was present when the function was called, find it.
If item was not present when the function finished, return NULL.
Else, do either.
Assume: No items are deleted during the runtime of the function.
*/
LSCounter * LS_FindItemInLocation(LS_type * LS, LSitem_t item, LSCounter** location)
{ // find a particular item in the date structure and return a pointer to it
	LSCounter * hashptr;
	// TODO: Make sure that the head of the list is updated only after the rest of the chain is complete.
	hashptr = *location;
	// compute the hash value of the item, and begin to look for it in 
	// the hash table
	while (hashptr) {
		if (hashptr->item == item)
			break;
		else hashptr = hashptr->next;
	}

	return hashptr;
}


LSCounter * LS_FindItemInPassive(LS_type * LS, LSitem_t item)
{ // find a particular item in the data structure and return a pointer to it
	LSCounter * hashptr;
	int hashval;

	hashval = (int)(hash31(LS->hasha, LS->hashb, item) % (LS->hashsize));
	hashptr = LS->passiveHashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table

	while (hashptr) {
		if (hashptr->item == item)
			break;
		else hashptr = hashptr->next;
	}

	return hashptr;
	// returns NULL if we do not find the item
}

LSCounter * LS_FindItemInPassive(LS_type * LS, LSitem_t item, int hashval)
{ // find a particular item in the data structure and return a pointer to it
	LSCounter * hashptr;
	hashptr = LS->passiveHashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table

	while (hashptr) {
		if (hashptr->item == item)
			break;
		else hashptr = hashptr->next;
	}

	return hashptr;
	// returns NULL if we do not find the item
}


LSCounter * LS_FindItem(LS_type * LS, LSitem_t item)
{ // find a particular item in the data structure and return a pointer to it
	LSCounter * hashptr;
	hashptr = LS_FindItemInActive(LS, item);
	if (!hashptr) {
		hashptr = LS_FindItemInPassive(LS, item);
	}
	return hashptr;
	// returns NULL if we do not find the item
}

/*
Can be executed while FindItem or AddItem is running.
*/
void LS_AddItem(LS_type *LS, LSitem_t item, LSweight_t value) {

	int hashval = (int)hash31(LS->hasha, LS->hashb, item) % LS->hashsize;
	// Function should not have been called if there is not enough room in table to insert the item
	// This applies both to if it's called from maintenance thread and update.
	assert(LS->nActive < LS->size);
	LSCounter* counter = &(LS->activeCounters[(LS->nActive)++]);
	// slot new item into hashtable
	// counter goes to the beginning of the list.
	// save the current item
	counter->item = item;
	// save the current hash
	counter->count = value;
	// If we add another item to the same chain we could have a problem here.
	LSCounter* hashptr = LS->activeHashtable[hashval];
	// The current head of the list becomes the second item in the list.
	counter->next = hashptr;
	// Now put the counter as the new head of the list.
	LS->activeHashtable[hashval] = counter;
}

/*
Can be executed while FindItem or AddItem is running.
*/
void LS_AddItem2Location(LS_type *LS, LSitem_t item, LSweight_t value, LSCounter** location) {
	// Function should not have been called if there is not enough room in table to insert the item
	// This applies both to if it's called from maintenance thread and update.
	assert(LS->nActive < LS->size);
	LSCounter* counter = &(LS->activeCounters[(LS->nActive)++]);
	// slot new item into hashtable
	// counter goes to the beginning of the list.
	// save the current item
	counter->item = item;
	// save the current hash
	counter->count = value;
	// If we add another item to the same chain we could have a problem here.
	LSCounter* hashptr = *location;
	// The current head of the list becomes the second item in the list.
	counter->next = hashptr;
	// Now put the counter as the new head of the list.
	*location = counter;
}

int in_place_find_kth(int *v, int n, int k, int jump, int pivot, LS_type* LS) {
	assert(k < n);
	if ((n == 1) && (k == 0)) return v[0];
	else if (n == 2) {
		return (v[k*jump] < v[(1 - k)*jump]) ? v[k*jump] : v[(1 - k)*jump];
	}
	if (pivot == 0) {
		int m = (n + 4) / 5; // number of medians
								//allocate space for medians.
		for (int i = 0; i < m; i++) {
			LS_FinishStep(LS);
			// if quintet is full
			int to_sort = (n - 5 * i < 3)? (n - 5 * i): 3;
			int quintet_size = (n - 5 * i < 5) ? (n - 5 * i) : 5;
			int *w = &v[5 * i * jump];
			// find 3 smallest items
			for (int j0 = 0; j0 < to_sort; j0++) {
				int jmin = j0;
				for (int j = j0 + 1; j < quintet_size; j++) {
					if (w[j*jump] < w[jmin*jump]) jmin = j;
				}
				swap(w[j0*jump], w[jmin*jump]);
			}
		
		}
		pivot = in_place_find_kth(v + 2*jump, (n+2)/5, (n + 2) / 5 / 2, jump * 5, 0, LS);
	}
	// put smaller items in the beginning
	int store = 0;
	for (int i = 0; i < n; i++) {
		LS_FinishStep(LS);
		if (v[i*jump] < pivot) {
			swap(v[i*jump], v[store*jump]);
			store++;
		}
	}
	// put pivots next
	int store2 = store;
	for (int i = store; i < n; i++) {
		LS_FinishStep(LS);
		if (v[i*jump] == pivot) {
			swap(v[i*jump], v[store2*jump]);
			store2++;
		}
	}
	// if k is small, search for it in the beginning.
	if (store > k) {
		//std::cerr << "Beginning:" << std::endl;
		return in_place_find_kth(v, store, k, jump, 0, LS);
	}
	// if k is large, search for it at the end.
	else if (k >= store2){
		//std::cerr << "End:" << std::endl;
		return in_place_find_kth(v + store2*jump, n - store2,
			k - store2, jump, 0, LS);
	}
	else {
		return pivot;
	}
}

DWORD WINAPI LS_Maintenance(LPVOID lpParam) {
	// FINISH MAINTENANCE
	LS_type* LS = (LS_type*) lpParam;
	while (true) {
		//std::cerr << "Waiting for maintenance semaphore" << std::endl;
		WaitForSingleObject(LS->maintenanceStepSemaphore, INFINITE);
		//std::cerr << "Acquired maintenance semaphore" << std::endl;
		if (LS->done)
			return 0;
		//std::cerr << "Calculating median..." << std::endl;
		int k = LS->nPassive - ceil(1 / LS->epsilon);// + 1;// TODO Added + 1
		if (k >= 0) {
			int median = in_place_find_kth(LS->buffer, LS->nPassive, k, 1, LS->quantile + 1, LS);
			if (median > LS->quantile) {
				LS->quantile = median;
			}
		}
		else {
			LS->blocksLeft = (LS->hashsize + LS->nPassive) / STEPS_AT_A_TIME + 1;
		}
		// Copy passive to active
		//std::cerr << "Copying P to A..." << std::endl;
		assert(LS->blocksLeft >= (LS->hashsize + LS->nPassive)/STEPS_AT_A_TIME + 1);
		LS->blocksLeft = (LS->hashsize + LS->nPassive)/STEPS_AT_A_TIME + 1;
		
		LS->finishedMedian = true;
		// Release update if it is waiting
		LS->blocksLeftThisUpdate = 0;
		//std::cerr << "Finished maintenance..." << std::endl;
		ReleaseSemaphore(LS->finishUpdateSemaphore, 1, NULL);
		
	}
	return 0;
}



void LS_RestartMaintenance(LS_type* LS) {
	// switch counter arrays
	LSCounter* tmp = LS->activeCounters;
	LS->activeCounters = LS->passiveCounters;
	LS->passiveCounters = tmp;
	LS->nPassive = (int) LS->nActive;
	LS->nActive = 0;
	// switch tables
	LSCounter** tmpTable = LS->activeHashtable;
	LS->activeHashtable = LS->passiveHashtable;
	LS->passiveHashtable = tmpTable;
	LS->blocksLeft = (LS->hashsize + 24*LS->nPassive )/STEPS_AT_A_TIME+1;
	
	int temp = LS->nPassive;
	LS->left2Move = min(temp, floor(1 / LS->epsilon));
	LS->finishedMedian = false;
	LS->clearedFromPassive = 0;
	LS->movedFromPassive = 0;
	LS->copied2Buffer = 0;
}

void LS_DoUpdate(LS_type * LS, LSitem_t item, LSweight_t value) {
	LSCounter * hashptr;
	// find whether new item is already stored, if so store it and add one
	// update heap property if necessary
	LS->n += value;
	int hashval = (int)hash31(LS->hasha, LS->hashb, item) % LS->hashsize;
	LSCounter** location = &(LS->activeHashtable[hashval]);
	hashptr = LS_FindItemInLocation(LS, item, location);
	if (hashptr) {
		hashptr->count += value; // increment the count of the item
	}
	else {
		// if control reaches here, then we have failed to find the item in the active table.
		// so, search for it in the passive table
		hashptr = LS_FindItemInPassive(LS, item, hashval);
		if (hashptr) {
			value += hashptr->count;
		}
		else {
			value += LS->quantile;
		}
		// Now add the item to the active hash table.
		// This could race with adding the item from the passive table.
		// What if first we check for the item in active and it's missing, 
		// then the item is copied from the passive table
		// and then the item is added here?
		LS_AddItem2Location(LS, item, value, location);
	}
}

void LS_DoSomeCopying(LS_type * LS) {
	int updatesLeft = LS->size - LS->nActive;
	assert(LS->movedFromPassive == 0);
	assert(updatesLeft >= 0);
	LS->stepsLeft = (LS->hashsize + 24 * LS->nPassive) + 1 - LS->copied2Buffer;
	int stepsLeftThisUpdate = LS->stepsLeft / (updatesLeft + 1);
	int k = LS->nPassive - ceil(1 / LS->epsilon);// + 1;// TODO Added + 1
	if (k >= 0) {
		for (int i = 0; i < stepsLeftThisUpdate; ++i) {
			if (LS->copied2Buffer >= LS->nPassive) {
				break;
			}
			--(LS->blocksLeft);
			LS->buffer[LS->copied2Buffer] = LS->passiveCounters[LS->copied2Buffer].count;
			++(LS->copied2Buffer);
		}
	}
	else {
		LS->copied2Buffer = LS->nPassive;
	}
	if (LS->copied2Buffer == LS->nPassive) {
		LS->blocksLeft = (LS->hashsize + 23 * LS->nPassive) / STEPS_AT_A_TIME + 1;
		ReleaseSemaphore(LS->maintenanceStepSemaphore, 1, NULL);
	}
	
}

void LS_DoSomeClearing(LS_type * LS) {
	int updatesLeft = LS->size - LS->nActive;
	assert(LS->movedFromPassive == LS->nPassive);
	assert(LS->left2Move == 0);
	assert(updatesLeft >= 0);
	LS->stepsLeft = LS->hashsize - LS->clearedFromPassive;
	int stepsLeftThisUpdate = LS->stepsLeft / (updatesLeft + 1);
	for (int i = 0; i < stepsLeftThisUpdate; ++i) {
		LS->passiveHashtable[LS->clearedFromPassive++] = NULL;
	}
}

void LS_DoSomeMoving(LS_type * LS) {
	int updatesLeft = LS->size - LS->nActive - LS->left2Move;
	LS->stepsLeft = LS->hashsize + LS->nPassive - LS->movedFromPassive;
	int stepsLeftThisUpdate = LS->stepsLeft / (updatesLeft+1);
	int largerThanQuantile = 0;
	for (int i = 0; i < stepsLeftThisUpdate; ++i) {
		if (LS->passiveCounters[LS->movedFromPassive].count > LS->quantile) {
			LSCounter* c = LS_FindItemInActive(LS, LS->passiveCounters[LS->movedFromPassive].item);
			if (!c) {
				LS_AddItem(LS, LS->passiveCounters[LS->movedFromPassive].item,
					LS->passiveCounters[LS->movedFromPassive].count);
			}
			--LS->left2Move;
			++largerThanQuantile;
		}
		(LS->movedFromPassive)++;
		if (LS->movedFromPassive >= LS->nPassive) {
			LS->left2Move = 0;
			LS->clearedFromPassive = 0;
			break;
		}
	}
	if (LS->nPassive == LS->movedFromPassive) {
		// If finished moving
		LS->left2Move = 0;
	}
}


void LS_Update(LS_type * LS, LSitem_t item, LSweight_t value)
{
	int updatesLeft = LS->size - LS->nActive - LS->left2Move;
	if (updatesLeft <= 0) {
		// Maintenance must be restarted
		// The iteration should be finished by now.
		// assert(LS->stepsLeft == 0);
		// Check that the clearing is done.
		assert(LS->movedFromPassive == LS->nPassive);
		if (LS->clearedFromPassive != LS->hashsize) {
			std::cerr << LS->clearedFromPassive << " " << LS->hashsize<<std::endl;
		}
		assert(LS->clearedFromPassive == LS->hashsize);
		LS_RestartMaintenance(LS);
		updatesLeft = LS->size - LS->nActive - LS->left2Move;		
	}
	// This is the number of steps maintenance must run
	int blocksLeftThisUpdate = (LS->blocksLeft / updatesLeft);
	if ((blocksLeftThisUpdate > 0) && (blocksLeftThisUpdate < BLOCK_SIZE)) {
		blocksLeftThisUpdate = BLOCK_SIZE;
	}
	LS->blocksLeftThisUpdate = blocksLeftThisUpdate;
	// Do actual update
	LS_DoUpdate(LS, item, value);
	// Wait for maintenance to finish running if needed
	if (!(LS->finishedMedian)) {
		if (LS->copied2Buffer < LS->nPassive) {
			LS_DoSomeCopying(LS);
		}
		else if (blocksLeftThisUpdate > 0) {
			int res;
			do {
				res = WaitForSingleObject(LS->finishUpdateSemaphore, 1000);
				if (res == WAIT_TIMEOUT) {
					std::cerr << "Waiting for maintenance? DUSHBAG!" << std::endl;
				}
				if (res == WAIT_FAILED) {
					std::cerr << "Error! Wait failed! Code " << GetLastError() << std::endl;
				}
			} while (res == WAIT_TIMEOUT);
		}
	}
	else {
		if (LS->movedFromPassive < LS->nPassive) {
			LS_DoSomeMoving(LS);
		}
		else {
			LS_DoSomeClearing(LS);
		}
	}
}


int LS_Size(LS_type * LS)
{ // return the size of the data structure in bytes
	return sizeof(LS_type) + LS->size*sizeof(int) // size of median buffer
		+ 2*(LS->hashsize * sizeof(LSCounter*)) // two hash tables
		+ 2*(LS->size*sizeof(LSCounter)); // two counter arrays
}

LSweight_t LS_PointEst(LS_type * LS, LSitem_t item)
{ // estimate the count of a particular item
	LSCounter * i;
	i = LS_FindItem(LS, item);
	if (i)
		return(i->count);
	else
		return LS->quantile;
}

LSweight_t LS_PointErr(LS_type * LS, LSitem_t item)
{ // estimate the worst case error in the estimate of a particular item
	return LS->quantile;
}

int LS_cmp(const void * a, const void * b) {
	LSCounter * x = (LSCounter*)a;
	LSCounter * y = (LSCounter*)b;
	if (x->count<y->count) return -1;
	else if (x->count>y->count) return 1;
	else return 0;
}

void LS_Output(LS_type * LS) { // prepare for output
}

std::map<uint32_t, uint32_t> LS_Output(LS_type * LS, uint64_t thresh)
{
	std::map<uint32_t, uint32_t> res;

	for (int i = 0; i < LS->nActive; ++i)
	{
		if (LS->activeCounters[i].count >= thresh)
			res.insert(std::pair<uint32_t, uint32_t>(LS->activeCounters[i].item, 
				LS->activeCounters[i].count));
	}
	for (int i = 0; i < LS->nPassive; ++i) {
		if ((LS_FindItemInActive(LS, LS->passiveCounters[i].item) == NULL) 
				&& (LS->passiveCounters[i].count >= thresh)) {
			res.insert(std::pair<uint32_t, uint32_t>(
				LS->passiveCounters[i].item, LS->passiveCounters[i].count));
		}
	}
	return res;
}

void LS_CheckHash(LS_type * LS, int item, int hash)
{ // debugging routine to validate the hash table
	int i;
	LSCounter *hashptr, *prev;

	for (i = 0; i<LS->hashsize; i++)
	{
		prev = NULL;
		hashptr = LS->activeHashtable[i];
		while (hashptr) {
			prev = hashptr;
			hashptr = hashptr->next;
		}
	}
}

void LS_ShowHash(LS_type * LS)
{ // debugging routine to show the hashtable
	int i;
	LSCounter * hashptr;

	for (i = 0; i<LS->hashsize; i++)
	{
		printf("%d:", i);
		hashptr = LS->activeHashtable[i];
		while (hashptr) {
			printf(" %d [h(%u) = ?, prev = ?] ---> ", (int)hashptr,
				(unsigned int)hashptr->item);
				//hashptr->hash);//,
				//hashptr->prev);
			hashptr = hashptr->next;
		}
		printf(" *** \n");
	}
}


void LS_ShowHeap(LS_type * LS)
{ // debugging routine to show the heap
	int i, j;

	j = 1;
	for (i = 1; i <= LS->size; i++)
	{
		printf("%d ", (int)LS->activeCounters[i].count);
		if (i == j)
		{
			printf("\n");
			j = 2 * j + 1;
		}
	}
	printf("\n\n");
}
