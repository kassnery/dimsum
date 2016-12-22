#include <stdlib.h>
#include <stdio.h>
#include "alosum.h"
#include "prng.h"
#include "math.h"
#include <windows.h>


#define ALS_NULLITEM 0x7FFFFFFF
#define swap(x,y) do{int t=x; x=y; y=t;} while(0)

void ALS_InitPassive(ALS_type *ALS) {
	ALS->passiveCounters =
		(ALSCounter*)calloc(ALS->size, sizeof(ALSCounter));
	ALS->passiveHashtable =
		(ALSCounter **)calloc(ALS->hashsize, sizeof(ALSCounter*));
	for (int i = 0; i < ALS->hashsize; ++i) {
		ALS->passiveHashtable[i] = NULL;
	}
	for (int i = 0; i < ALS->size; i++)
	{
		ALS->passiveCounters[i].next = NULL;
		ALS->passiveCounters[i].prev = NULL;
		ALS->passiveCounters[i].item = ALS_NULLITEM;
		// initialize items and counters to zero
	}
	ALS->nPassive = 0;
}

ALS_type * ALS_Init(float fPhi, float gamma)
{
	int i;
	int k = 1 + (int) 1.0 / fPhi;

	ALS_type *result = (ALS_type *)calloc(1, sizeof(ALS_type));
	// needs to be odd so that the heap always has either both children or 
	// no children present in the data structure
	result->epsilon = fPhi;
	// nitems in large table
	result->nActive = 0;
	result->size = int(ceil(gamma / fPhi) + ceil(1 / fPhi) - 1);// - 1);
	result->maxMaintenanceTime = int(ceil(gamma / fPhi));
	result->hashsize = ALS_HASHMULT*result->size;
	
	result->hasha = 151261303;
	result->hashb = 6722461; // hard coded constants for the hash table,
							 //should really generate these randomly
	result->n = (ALSweight_t)0;

	result->activeHashtable =
		(ALSCounter **)calloc(result->hashsize, sizeof(ALSCounter*));
	for (i = 0; i < result->hashsize; ++i) {
		result->activeHashtable[i] = NULL;
	}
	result->activeCounters =
		(ALSCounter*)calloc(result->size, sizeof(ALSCounter));
	for (i = 0; i < result->size; i++)
	{
		result->activeCounters[i].next = NULL;
		result->activeCounters[i].prev = NULL;
		result->activeCounters[i].item = ALS_NULLITEM;
		// initialize items and counters to zero
	}

	ALS_InitPassive(result);
	result->extra = result->size;
	result->quantile = 0;
	result->buffer =
		(int*)calloc(result->size, sizeof(int));
	result->handle = NULL;
	return(result);
}

void ALS_DestroyPassive(ALS_type* ALS) {
	free(ALS->passiveHashtable);
	free(ALS->passiveCounters);
}
void ALS_Destroy(ALS_type * ALS)
{
	std::cerr << "Destroy A" << std::endl;
	free(ALS->activeHashtable);
	free(ALS->activeCounters);
	free(ALS->buffer);
	ALS_DestroyPassive(ALS);
	free(ALS);
}

void ALS_RebuildHash(ALS_type * ALS)
{
	// rebuild the hash table and linked list pointers based on current
	// contents of the counters array
	int i;
	ALSCounter * pt;

	for (i = 0; i<ALS->hashsize; i++)
		ALS->activeHashtable[i] = 0;
		ALS->passiveHashtable[i] = 0;
	// first, reset the hash table
	for (i = 0; i < ALS->size; i++) {
		ALS->activeCounters[i].next = NULL;
		ALS->activeCounters[i].prev = NULL;
		ALS->passiveCounters[i].next = NULL;
		ALS->passiveCounters[i].prev = NULL;
	}
	// empty out the linked list
	for (i = 0; i < ALS->size; i++) { // for each item in the data structure
		pt = &ALS->activeCounters[i];
		pt->next = ALS->activeHashtable[ALS->activeCounters[i].hash];
		if (pt->next)
			pt->next->prev = pt;
		ALS->activeHashtable[ALS->activeCounters[i].hash] = pt;
		pt = &ALS->passiveCounters[i];
		pt->next = ALS->passiveHashtable[ALS->passiveCounters[i].hash];
		if (pt->next)
			pt->next->prev = pt;
		ALS->passiveHashtable[ALS->passiveCounters[i].hash] = pt;
	}
}


ALSCounter * ALS_FindItemInActive(ALS_type * ALS, ALSitem_t item)
{ // find a particular item in the date structure and return a pointer to it
	ALSCounter * hashptr;
	int hashval;
	
	hashval = (int)hash31(ALS->hasha, ALS->hashb, item) % ALS->hashsize;
	if (hashval == 15) {
		//ALS_CheckHash(ALS,0,0);
	}
	
	hashptr = ALS->activeHashtable[hashval];
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

ALSCounter * ALS_FindItemInPassive(ALS_type * ALS, ALSitem_t item)
{ // find a particular item in the date structure and return a pointer to it
	ALSCounter * hashptr;
	int hashval;

	hashval = (int)(hash31(ALS->hasha, ALS->hashb, item) % (ALS->hashsize));
	hashptr = ALS->passiveHashtable[hashval];
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


ALSCounter * ALS_FindItem(ALS_type * ALS, ALSitem_t item)
{ // find a particular item in the data structure and return a pointer to it
	ALSCounter * hashptr;
	int hashval;
	hashptr = ALS_FindItemInActive(ALS, item);
	if (!hashptr) {
		hashptr = ALS_FindItemInPassive(ALS, item);
	}
	return hashptr;
	// returns NULL if we do not find the item
}

void ALS_AddItem(ALS_type *ALS, ALSitem_t item, ALSweight_t value) {

	int hashval = (int)hash31(ALS->hasha, ALS->hashb, item) % ALS->hashsize;
	ALSCounter* hashptr = ALS->activeHashtable[hashval];
	// so, overwrite smallest heap item and reheapify if necessary
	// fix up linked list from hashtable
	if (ALS->nActive >= ALS->size) {
		std::cerr << "Error! Not enough room in table."<<std::endl;
		std::cerr << "Size:"<<ALS->size<< " Active: " << ALS->nActive << " Passive:" << ALS->nPassive 
			<< " Extra:" << ALS->extra << " From passive:" << ALS->movedFromPassive 
			<< std::endl;
	}
	assert(ALS->nActive < ALS->size);
	ALSCounter* counter = &(ALS->activeCounters[(ALS->nActive)++]);
	// slot new item into hashtable
	// counter goes to the beginning of the list.
	// The current head of the list becomes the second item in the list.
	counter->next = hashptr;
	// If the list was not empty, 
	if (hashptr)
		// point the second item's previous item to the new counter.
		hashptr->prev = counter;
	// Now put the counter as the new head of the list.
	ALS->activeHashtable[hashval] = counter;
	// The head of the list has no previous item
	counter->prev = NULL;
	// save the current item
	counter->item = item;
	// save the current hash
	counter->hash = hashval; 
	// update the upper bound on the items frequency
	counter->count = value; 	
}




int ALS_in_place_find_kth(int *v, int n, int k, int jump, int pivot) {
	assert(k < n);
	if ((n == 1) && (k == 0)) return v[0];
	else if (n == 2) {
		return (v[k*jump] < v[(1 - k)*jump]) ? v[k*jump] : v[(1 - k)*jump];
	}
	if (pivot == 0) {
		int m = (n + 4) / 5; // number of medians
								//allocate space for medians.
		for (int i = 0; i < m; i++) {
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
		pivot = ALS_in_place_find_kth(v + 2*jump, (n+2)/5, (n + 2) / 5 / 2, jump * 5);
	}
	// put smaller items in the beginning
	int store = 0;
	for (int i = 0; i < n; i++) {
		if (v[i*jump] < pivot) {
			swap(v[i*jump], v[store*jump]);
			store++;
		}
	}
	// put pivots next
	int store2 = store;
	for (int i = store; i < n; i++) {
		if (v[i*jump] == pivot) {
			swap(v[i*jump], v[store2*jump]);
			store2++;
		}
	}
	// Then put the pivot
	// if k is small, search for it in the beginning.
	if (store > k) {
		return ALS_in_place_find_kth(v, store, k, jump);
	}
	// if k is large, search for it at the end.
	else if (k >= store2){
		return ALS_in_place_find_kth(v + store2*jump, n - store2,
			k - store2, jump);
	}
	else {
		return pivot;
	}
}

DWORD WINAPI ALS_Maintenance(LPVOID lpParam) {
	// FINISH MAINTENANCE	
	// dnd quantile
	ALS_type* ALS = (ALS_type*) lpParam;
	int k = ALS->nPassive - ceil(1 / ALS->epsilon)+1;
	if (k >= 0) {
		for (int i = 0; i < ALS->nPassive; ++i) {
			ALS->buffer[i] = ALS->passiveCounters[i].count;
		}
		int median = ALS_in_place_find_kth(ALS->buffer, ALS->nPassive, k, 1, ALS->quantile+1);
		int test = 0;
		if (median > ALS->quantile) {
			ALS->quantile = median;
		}
	}
	// Copy passive to active
	ALS->movedFromPassive = 0;
	for (int i = 0; i < ALS->nPassive; i++) {
		if (ALS->passiveCounters[i].count > ALS->quantile) {
			ALSCounter* c = ALS_FindItemInActive(ALS, ALS->passiveCounters[i].item);
			if (!c) {
				++(ALS->movedFromPassive);
				ALS_AddItem(ALS, ALS->passiveCounters[i].item,
					ALS->passiveCounters[i].count);
			}
			else {
				// counter was already moved. We earned an extra addition.
				++(ALS->extra);
			}
		}
	}
	ALS_DestroyPassive(ALS);
	ALS_InitPassive(ALS);
	ALS->extra += ALS->maxMaintenanceTime;
	return 0;
}

void ALS_RestartMaintenance(ALS_type* ALS) {
	// switch counter arrays
	ALSCounter* tmp = ALS->activeCounters;
	ALS->activeCounters = ALS->passiveCounters;
	ALS->passiveCounters = tmp;
	int t = ALS->nActive;
	ALS->nActive = ALS->nPassive;
	ALS->nPassive = t;
	// switch tables
	ALSCounter** tmpTable = ALS->activeHashtable;
	ALS->activeHashtable = ALS->passiveHashtable;
	ALS->passiveHashtable = tmpTable;
	ALS->extra = ALS->size
		- (ALS->nPassive < floor(1 / ALS->epsilon) ?
			ALS->nPassive : floor(1 / ALS->epsilon))
		- ALS->maxMaintenanceTime - ALS->nActive;
	ALS->movedFromPassive = 0;
	assert(ALS->extra >= 0);
	ALS_Maintenance(ALS);
}



void ALS_Update(ALS_type * ALS, ALSitem_t item, ALSweight_t value)
{
	int hashval;
	ALSCounter * hashptr;
	// find whether new item is already stored, if so store it and add one
	// update heap property if necessary
	ALS->n += value;
	
	hashptr = ALS_FindItemInActive(ALS, item);
	if (hashptr) {
		hashptr->count += value; // increment the count of the item
		return;
	}
	else {
		// if control reaches here, then we have failed to find the item in the active table.
		// so, search for it in the passive table
		hashptr = ALS_FindItemInPassive(ALS, item);
		if (hashptr) {
			value += hashptr->count;
		}
		else {
			value += ALS->quantile;
		}
		//if (ALS->newItems == ALS->bucketSize) {
		if (ALS->extra <= 0) {
			// finish maintenance if started
			//if (ALS->handle)
			//	WaitForSingleObject(ALS->handle, INFINITE);
			// start the maintenance thread anew.
			ALS_RestartMaintenance(ALS);
		}
		// Now add the item to the active hash table.
		--(ALS->extra);
		ALS_AddItem(ALS, item, value);
		
	}
	//ALS_CheckHash(ALS, item, 0);
}

int ALS_Size(ALS_type * ALS)
{ // return the size of the data structure in bytes
	return sizeof(ALS_type) + ALS->size*sizeof(int) // size of median buffer
		+ 2*(ALS->hashsize * sizeof(ALSCounter*)) // two hash tables
		+ 2*(ALS->size*sizeof(ALSCounter)); // two counter arrays
}

ALSweight_t ALS_PointEst(ALS_type * ALS, ALSitem_t item)
{ // estimate the count of a particular item
	ALSCounter * i;
	i = ALS_FindItem(ALS, item);
	if (i)
		return(i->count);
	else
		return ALS->quantile;
}

ALSweight_t ALS_PointErr(ALS_type * ALS, ALSitem_t item)
{ // estimate the worst case error in the estimate of a particular item
	return ALS->quantile;
}

int ALS_cmp(const void * a, const void * b) {
	ALSCounter * x = (ALSCounter*)a;
	ALSCounter * y = (ALSCounter*)b;
	if (x->count<y->count) return -1;
	else if (x->count>y->count) return 1;
	else return 0;
}

void ALS_Output(ALS_type * ALS) { // prepare for output
}

std::map<uint32_t, uint32_t> ALS_Output(ALS_type * ALS, uint64_t thresh)
{
	std::map<uint32_t, uint32_t> res;

	for (int i = 0; i < ALS->nActive; ++i)
	{
		if (ALS->activeCounters[i].count >= thresh)
			res.insert(std::pair<uint32_t, uint32_t>(ALS->activeCounters[i].item, ALS->activeCounters[i].count));
	}
	for (int i = 0; i < ALS->nPassive; ++i) {
		if (ALS_FindItemInActive(ALS, ALS->passiveCounters[i].item) == NULL) {
			if (ALS->passiveCounters[i].count >= thresh) {
				res.insert(std::pair<uint32_t, uint32_t>(ALS->passiveCounters[i].item, ALS->passiveCounters[i].count));
			}
		}
	}
	return res;
}

void ALS_CheckHash(ALS_type * ALS, int item, int hash)
{ // debugging routine to validate the hash table
	int i;
	ALSCounter *hashptr, *prev;

	for (i = 0; i<ALS->hashsize; i++)
	{
		prev = NULL;
		hashptr = ALS->activeHashtable[i];
		while (hashptr) {
			if (hashptr->hash != i)
			{
				printf("\n Hash violation! hash = %d, should be %d \n",
					hashptr->hash, i);
				printf("after inserting item %d with hash %d\n", item, hash);
			}
			if (hashptr->prev != prev)
			{
				printf("\n Previous violation! prev = %d, should be %d\n",
					(int)hashptr->prev, (int)prev);
				printf("after inserting item %d with hash %d\n", item, hash);
				exit(EXIT_FAILURE);
			}
			prev = hashptr;
			hashptr = hashptr->next;
		}
	}
}

void ALS_ShowHash(ALS_type * ALS)
{ // debugging routine to show the hashtable
	int i;
	ALSCounter * hashptr;

	for (i = 0; i<ALS->hashsize; i++)
	{
		printf("%d:", i);
		hashptr = ALS->activeHashtable[i];
		while (hashptr) {
			printf(" %d [h(%u) = %d, prev = %d] ---> ", (int)hashptr,
				(unsigned int)hashptr->item,
				hashptr->hash,
				(int)hashptr->prev);
			hashptr = hashptr->next;
		}
		printf(" *** \n");
	}
}


void ALS_ShowHeap(ALS_type * ALS)
{ // debugging routine to show the heap
	int i, j;

	j = 1;
	for (i = 1; i <= ALS->size; i++)
	{
		printf("%d ", (int)ALS->activeCounters[i].count);
		if (i == j)
		{
			printf("\n");
			j = 2 * j + 1;
		}
	}
	printf("\n\n");
}
