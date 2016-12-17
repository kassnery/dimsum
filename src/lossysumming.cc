Based on:
Implementation of Lazy Lossy Counting algorithm to Find Frequent Items
Based on the paper of Manku and Motwani, 2002
And Metwally, Agrwawal and El Abbadi, 2005
Implementation by G. Cormode 2002, 2003, 2005
This implements the space saving algorithm, which 
guarantees 1/epsilon space. 
This implementation uses a heap to track which is the current smallest count

Original Code: 2002-11
This version: 2002,2003,2005,2008

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA.
*********************************************************************/

#define LCL_NULLITEM 0x7FFFFFFF
	// 2^31 -1 as a special character

LS_type * LS_Init(float fPhi)
{
	int i;
	int k = 1 + (int) 1.0/fPhi;

	LS_type *result = (LS_type *) calloc(1,sizeof(LS_type));
	
	result->size = (1 + k); // TODO WHAT IS K?
	result->hashsize = LS_HASHMULT*result->size;
	result->hashtable=(LSCounter **) calloc(result->hashsize,sizeof(LSCounter*));
	result->counters=(LSCounter*) calloc(1+result->size,sizeof(LSCounter));
	// indexed from 1, so add 1

	result->hasha=151261303;
	result->hashb=6722461; // hard coded constants for the hash table,
	//should really generate these randomly
	result->n=(LSweight_t) 0;

	for (i=1; i<=result->size;i++)
	{
		result->counters[i].next=NULL;
		result->counters[i].prev=NULL;
		result->counters[i].item=LS_NULLITEM;
		// initialize items and counters to zero
	}
	result->root=&result->counters[1]; // put in a pointer to the top of the heap
	return(result);
}

void LS_Destroy(LS_type * ls)
{
	free(ls->hashtable);
	free(ls->counters);
	free(ls);
}

LCLCounter * LS_FindItem(LCL_type * ls, LCLitem_t item)
{ // find a particular item in the date structure and return a pointer to it
	LCLCounter * hashptr;
	int hashval;

	hashval=(int) hash31(lcl->hasha, lcl->hashb,item) % lcl->hashsize;
	hashptr=lcl->hashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table

	while (hashptr) {
		if (hashptr->item==item)
			break;
		else hashptr=hashptr->next;
	}
	return hashptr;
	// returns NULL if we do not find the item
}

void LS_Update(LS_type * ls, LSitem_t item, LSweight_t value)
{
	int hashval;
	LSCounter * hashptr;
	// find whether new item is already stored, if so store it and add one
	// update heap property if necessary

	ls->n+=value;
	//lcl->counters->item=0; // mark data structure as 'dirty'

	hashval=(int) hash31(ls->hasha, ls->hashb,item) % ls->hashsize;
	hashptr=ls->hashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table

	while (hashptr) {
		if (hashptr->item==item) {
			hashptr->count+=value; // increment the count of the item
			//Heapify(lcl,hashptr-lcl->counters); // and fix up the heap
			return;
		}
		else hashptr=hashptr->next;
	}
	// if control reaches here, then we have failed to find the item
	// so, overwrite smallest heap item and reheapify if necessary
	// fix up linked list from hashtable
	if (!ls->root->prev) // if it is first in its list
		ls->hashtable[ls->root->hash]=ls->root->next;
	else
		lcl->root->prev->next=lcl->root->next;
	if (lcl->root->next) // if it is not last in the list
		lcl->root->next->prev=lcl->root->prev;
	// update the hash table appropriately to remove the old item

	// slot new item into hashtable
	hashptr=lcl->hashtable[hashval];
	lcl->root->next=hashptr;
	if (hashptr)
		hashptr->prev=lcl->root;
	lcl->hashtable[hashval]=lcl->root;
	// we overwrite the smallest item stored, so we look in the root
	lcl->root->prev=NULL;
	lcl->root->item=item;
	lcl->root->hash=hashval;
	lcl->root->delta=lcl->root->count;
	// update the implicit lower bound on the items frequency
	//  value+=lcl->root->delta;
	// update the upper bound on the items frequency
	lcl->root->count=value+lcl->root->delta;
	Heapify(lcl,1); // restore heap property if needed
	// return value;
}

int LCL_Size(LCL_type * lcl)
{ // return the size of the data structure in bytes
	return sizeof(LCL_type) + (lcl->hashsize * sizeof(int)) + 
		(lcl->size*sizeof(LCLCounter));
}

LCLweight_t LCL_PointEst(LCL_type * lcl, LCLitem_t item)
{ // estimate the count of a particular item
	LCLCounter * i;
	i=LCL_FindItem(lcl,item);
	if (i)
		return(i->count);
	else
		return 0;
}

LCLweight_t LCL_PointErr(LCL_type * lcl, LCLitem_t item)
{ // estimate the worst case error in the estimate of a particular item
	LCLCounter * i;
	i=LCL_FindItem(lcl,item);
	if (i)
		return(i->delta);
	else
		return lcl->root->delta;
}

int LCL_cmp( const void * a, const void * b) {
	LCLCounter * x = (LCLCounter*) a;
	LCLCounter * y = (LCLCounter*) b;
	if (x->count<y->count) return -1;
	else if (x->count>y->count) return 1;
	else return 0;
}

void LCL_Output(LCL_type * lcl) { // prepare for output
	if (lcl->counters->item==0) {
		qsort(&lcl->counters[1],lcl->size,sizeof(LCLCounter),LCL_cmp);
		LCL_RebuildHash(lcl);
		lcl->counters->item=1;
	}
}

std::map<uint32_t, uint32_t> LCL_Output(LCL_type * lcl, int thresh)
{
	std::map<uint32_t, uint32_t> res;

	for (int i=1;i<=lcl->size;++i)
	{
		if (lcl->counters[i].count>=thresh)
			res.insert(std::pair<uint32_t, uint32_t>(lcl->counters[i].item, lcl->counters[i].count));
	}

	return res;
}

void LCL_CheckHash(LCL_type * lcl, int item, int hash)
{ // debugging routine to validate the hash table
	int i;
	LCLCounter * hashptr, * prev;

	for (i=0; i<lcl->hashsize;i++)
	{
		prev=NULL;
		hashptr=lcl->hashtable[i];
		while (hashptr) {
			if (hashptr->hash!=i)
			{
				printf("\n Hash violation! hash = %d, should be %d \n", 
					hashptr->hash,i);
				printf("after inserting item %d with hash %d\n", item, hash);
			}
			if (hashptr->prev!=prev)
			{
				printf("\n Previous violation! prev = %d, should be %d\n",
					(int) hashptr->prev, (int) prev);
				printf("after inserting item %d with hash %d\n",item, hash);
				exit(EXIT_FAILURE);
			}
			prev=hashptr;
			hashptr=hashptr->next;
		}
	}
}

void LCL_ShowHash(LCL_type * lcl)
{ // debugging routine to show the hashtable
	int i;
	LCLCounter * hashptr;

	for (i=0; i<lcl->hashsize;i++)
	{
		printf("%d:",i);
		hashptr=lcl->hashtable[i];
		while (hashptr) {
			printf(" %d [h(%u) = %d, prev = %d] ---> ",(int) hashptr,
				(unsigned int) hashptr->item,
				hashptr->hash,
				(int) hashptr->prev);
			hashptr=hashptr->next;
		}
		printf(" *** \n");
	}
}


void LCL_ShowHeap(LCL_type * lcl)
{ // debugging routine to show the heap
	int i, j;

	j=1;
	for (i=1; i<=lcl->size; i++)
	{
		printf("%d ",(int) lcl->counters[i].count);
		if (i==j) 
		{ 
			printf("\n");
			j=2*j+1;
		}
	}
	printf("\n\n");
}
