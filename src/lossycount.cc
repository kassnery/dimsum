#include <stdlib.h>
#include <stdio.h>
#include "lossycount.h"
#include "prng.h"
/********************************************************************
Implementation of Lossy Counting algorithm to Find Frequent Items
Based on the paper of Manku and Motwani, 2002
Implementation by G. Cormode 2002, 2003

Original Code: 2002-11
This version: 2003-10

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA.
*********************************************************************/

LC_type * LC_Init(float phi)
{
	LC_type * result;

	result=(LC_type *) calloc(1,sizeof(LC_type));
	result->buckets=0;
	result->holdersize=0;
	result->epoch=0;

	result->window=(int) 1.0/phi;
	result->maxholder=result->window*4;
	result->bucket=(LCCounter*) calloc(result->window+2,sizeof(LCCounter));
	result->holder=(LCCounter*) calloc(result->maxholder,sizeof(LCCounter));
	result->newcount=(LCCounter*) calloc(result->maxholder,sizeof(LCCounter));
	return(result);
}

void LC_Destroy(LC_type * lc)
{
	free(lc->bucket);
	free(lc->holder);
	free(lc->newcount);
	free(lc);
}

int lccmp(const void *x, const void *y)
{
	// used in quicksort to sort lists to get the exact results for comparison
	const LCCounter * h1= (LCCounter*) x; 
	const LCCounter * h2= (LCCounter*) y;

	if ((h1->item)<(h2->item)) 
		return -1; 
	else if ((h1->item)>(h2->item)) 
		return 1;
	else return 0;
}

void LCShowCounters(LCCounter * counts, int length, int delta)
{
	int i;

	for (i=0;i<length;i++)
		printf("  (item %d, count %d, delta %d)\n",
		counts[i].item,counts[i].count,delta);
}


int lccountermerge(LCCounter *newcount, LCCounter *left, LCCounter *right,
				   int l, int r, int maxholder)
{  // merge up two lists of counters. returns the size of the lists.
	int i,j,m;

	if (l+r>maxholder)
	{ // a more advanced implementation would do a realloc here...
		printf("Out of memory -- trying to allocate %d counters\n",l+r);
		exit(1);
	}
	i=0;
	j=0;
	m=0;

	while (i<l && j<r)
	{ // merge two lists
		if (left[i].item==right[j].item)
		{ // sum the counts of identical items
			newcount[m].item=left[i].item;
			newcount[m].count=right[j].count;
			while (left[i].item==right[j].item)
			{
				newcount[m].count+=left[i].count;
				i++;
			}
			j++;
		}
		else if (left[i].item<right[j].item)
		{ // else take the left item, creating counts appropriately
			newcount[m].item=left[i].item;
			newcount[m].count=0;
			while (left[i].item==newcount[m].item)
			{
				newcount[m].count+=left[i].count;
				i++;
			}
		}
		else {
			newcount[m].item=right[j].item;
			newcount[m].count=right[j].count;
			j++;
		}
		newcount[m].count--;
		if (newcount[m].count>0) m++;
		else
		{ // adjust for items which may have negative or zero counts
			newcount[m].item=-1;
			newcount[m].count=0;
		}
	}

	// now that the main part of the merging has been done
	// need to copy over what remains of whichever list is not used up

	if (j<r)
	{
		while (j<r)
		{
			if (right[j].count > 1)
			{
				newcount[m].item=right[j].item;
				newcount[m].count=right[j].count-1;
				m++;
			}
			j++;
		}
	}
	else
		if (i<l)
		{
			while(i<l)
			{
				newcount[m].item=left[i].item;
				newcount[m].count=-1;
				while ((newcount[m].item==left[i].item) && (i<l))
				{
					newcount[m].count+=left[i].count;
					i++;
				}
				if (newcount[m].count>0)
					m++;
				else
				{
					newcount[m].item=-1;
					newcount[m].count=0;
				}
			}
		}

		return(m);
}


void LC_Update(LC_type * lc, int val)
{
	LCCounter *tmp;

	// interpret a negative item identifier as a removal
	if (val>0)
	{
		lc->bucket[lc->buckets].item=val;
		lc->bucket[lc->buckets].count=1;
	}
	else
	{
		lc->bucket[lc->buckets].item=-val;
		lc->bucket[lc->buckets].count=-1;
	}

	lc->buckets++;
	if (lc->buckets==lc->window)
	{
		qsort(lc->bucket,lc->window,sizeof(LCCounter),lccmp);
		lc->holdersize=lccountermerge(lc->newcount,lc->bucket,lc->holder,
			lc->window,lc->holdersize,lc->maxholder);
		tmp=lc->newcount;
		lc->newcount=lc->holder;
		lc->holder=tmp;
		lc->buckets=0;
		lc->epoch++;
	}
}

int LC_Size(LC_type * lc)
{
	int size;
	size=(lc->maxholder+lc->window)*sizeof(LCCounter)+sizeof(LC_type);
	return size;
}

int LC_PointEst(LC_type * lc, int item)
{
	int i;

	for (i=0;i<lc->holdersize;i++)
		if (lc->holder[i].item==item)
			return(lc->holder[i].count + lc->epoch);
	return 0;
}

std::map<uint32_t, uint32_t> LC_Output(LC_type * lc, int thresh)
{
	std::map<uint32_t, uint32_t> res;

	// should do a countermerge here.

	for (int i=0;i<lc->holdersize;i++)
	{
		if (lc->holder[i].count+lc->epoch>=thresh)
			res.insert(std::pair<uint32_t, uint32_t>(lc->holder[i].item, lc->holder[i].count+lc->epoch));
	}

	return res;
}

/********************************************************************
Implementation of Lossy Counting algorithm to Find Frequent Items
Based on the paper of Manku and Motwani, 2002
Implementation by G. Cormode 2002, 2003
Includes the use of 'delta' values to give better estimates

Original Code: 2002-11
This version: 2003-10

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA.
*********************************************************************/

#define LCDMULTIPLE 2

LCD_type * LCD_Init(float phi)
{
	LCD_type * result;

	result=(LCD_type *) calloc(1,sizeof(LCD_type));
	result->buckets=0;
	result->holdersize=0;
	result->epoch=0;

	result->window=1 + (int) 1.0/phi;
	result->maxholder=result->window*LCDMULTIPLE;
	result->bucket=(LCDCounter*) calloc(result->window+2,sizeof(LCDCounter));
	result->holder=(LCDCounter*) calloc(result->maxholder,sizeof(LCDCounter));
	result->newcount=(LCDCounter*) calloc(result->maxholder,sizeof(LCDCounter));
	return(result);
}

void LCD_Destroy(LCD_type * lc)
{
	free(lc->bucket);
	free(lc->holder);
	free(lc->newcount);
	free(lc);
}

void LCDShowCounters(LCDCounter * counts, int length)
{
	int i;

	for (i=0;i<length;i++)
		printf("  (item %d, count %d, delta %d)\n",
		counts[i].item,counts[i].count,counts[i].delta);
}

int ccmp(const void *x, const void *y)
{
	// used in quicksort to sort lists to get the exact results for comparison
	const LCDCounter * h1= (LCDCounter *) x;
	const LCDCounter * h2= (LCDCounter *) y;

	if ((h1->item)<(h2->item)) 
		return -1; 
	else if ((h1->item)>(h2->item)) 
		return 1;
	else return 0;
}

int lcdcountermerge(LCDCounter *newcount, LCDCounter *left, LCDCounter *right,
					int l, int r, int maxholder, int epoch)
{  // merge up two lists of counters. returns the size of the lists.
	int i,j,m;

	if (l+r>maxholder)
	{ // a more advanced implementation would do a realloc here...
		printf("Out of memory -- trying to allocate %d counters\n",l+r);
		exit(1);
	}
	i=0;
	j=0;
	m=0;

	// interpretation: left is the new items, right is the existing list

	while (i<l && j<r)
	{ // merge two lists
		if (left[i].item==right[j].item)
		{ // sum the counts of identical items
			newcount[m].item=left[i].item;
			newcount[m].count=right[j].count;
			newcount[m].delta=right[j].delta;
			while (left[i].item==right[j].item)
			{ // merge in multiple copies of the same new item
				newcount[m].count+=left[i].count;
				++i;
			}
			++j;
		}
		else if (left[i].item<right[j].item)
		{ // else take the left item, creating counts appropriately
			newcount[m].item=left[i].item;
			newcount[m].count=0;
			newcount[m].delta=left[i].delta;
			while (left[i].item==newcount[m].item)
			{ // merge in multiple new copies of the same item
				newcount[m].count+=left[i].count;
				// need to update deltas here? 
				++i;
			}
		}
		else {
			newcount[m].item=right[j].item;
			newcount[m].count=right[j].count;
			newcount[m].delta=right[j].delta;
			++j;
		}
		if (newcount[m].count+newcount[m].delta > epoch) ++m;
		else
		{ // overwrite an item that is not worth keeping
			newcount[m].item=-1;
			newcount[m].count=0;
			newcount[m].delta=0;
		}
	}

	// now that the main part of the merging has been done
	// need to copy over what remains of whichever list is not used up

	if (j<r)
	{
		while (j<r)
		{
			if (right[j].count > 1)
			{
				newcount[m].item=right[j].item;
				newcount[m].count=right[j].count;
				newcount[m].delta=right[j].delta;
				if (newcount[m].count+newcount[m].delta > epoch) ++m;
			}
			++j;
		}
	}
	else if (i<l)
	{
		while(i<l)
		{
			newcount[m].item=left[i].item;
			newcount[m].delta=left[i].delta;
			newcount[m].count=0;
			while ((newcount[m].item==left[i].item) && (i<l))
			{
				newcount[m].count+=left[i].count;
				i++;
			}
			if (newcount[m].count+newcount[m].delta > epoch)
				m++;
			else
			{
				newcount[m].item=-1;
				newcount[m].count=0;
				newcount[m].delta=0;
			}
		}
	}
	return(m);
}


void LCD_Update(LCD_type * lc, int val)
{
	LCDCounter *tmp;
	// interpret a negative item identifier as a removal
	if (val>0)
	{
		lc->bucket[lc->buckets].item=val;
		lc->bucket[lc->buckets].count=1;
		lc->bucket[lc->buckets].delta=lc->epoch;
	}
	else
	{
		lc->bucket[lc->buckets].item=-val;
		lc->bucket[lc->buckets].count=-1;
		lc->bucket[lc->buckets].delta=lc->epoch;
	}

	lc->buckets++;

	if (lc->buckets==lc->window)
	{

		lc->epoch++;
		qsort(lc->bucket,lc->window,sizeof(LCDCounter),ccmp);
		lc->holdersize=lcdcountermerge(lc->newcount,lc->bucket,lc->holder,
			lc->window,
			lc->holdersize,lc->maxholder,lc->epoch);
		tmp=lc->newcount;
		lc->newcount=lc->holder;
		lc->holder=tmp;
		lc->buckets=0;
	}
}

int LCD_Size(LCD_type * lc)
{
	int size;
	size=(lc->maxholder+lc->window)*sizeof(LCDCounter)+sizeof(LCD_type);
	return size;
}

int LCD_PointEst(LCD_type * lcd, int item)
{
	int i;

	for (i=0;i<lcd->holdersize;i++)
		if (lcd->holder[i].item==item)
			return(lcd->holder[i].count + lcd->holder[i].delta);
	return 0;
}

std::map<uint32_t, uint32_t> LCD_Output(LCD_type * lc, int thresh)
{
	std::map<uint32_t, uint32_t> res;

	// should do a countermerge here.

	for (int i=0;i<lc->holdersize;i++)
	{
		if (lc->holder[i].count+lc->holder[i].delta>=thresh)
			res.insert(std::pair<uint32_t, uint32_t>(lc->holder[i].item, lc->holder[i].count+lc->holder[i].delta));
	}

	return res;
}

/********************************************************************
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

LCL_type * LCL_Init(float fPhi)
{
	int i;
	int k = 1 + (int) 1.0/fPhi;

	LCL_type *result = (LCL_type *) calloc(1,sizeof(LCL_type));
	// needs to be odd so that the heap always has either both children or 
	// no children present in the data structure

	result->size = (1 + k) | 1; // ensure that size is odd
	result->hashsize = LCL_HASHMULT*result->size;
	result->hashtable=(LCLCounter **) calloc(result->hashsize,sizeof(LCLCounter*));
	result->counters =(LCLCounter*) calloc(1+result->size,sizeof(LCLCounter));
	// indexed from 1, so add 1

	result->hasha=151261303;
	result->hashb=6722461; // hard coded constants for the hash table,
	//should really generate these randomly
	result->n=(LCLweight_t) 0;

	for (i=1; i<=result->size;i++)
	{
		result->counters[i].next=NULL;
		result->counters[i].prev=NULL;
		result->counters[i].item=LCL_NULLITEM;
		// initialize items and counters to zero
	}
	result->root=&result->counters[1]; // put in a pointer to the top of the heap
	return(result);
}

void LCL_Destroy(LCL_type * lcl)
{
	free(lcl->hashtable);
	free(lcl->counters);
	free(lcl);
}

void LCL_RebuildHash(LCL_type * lcl)
{
	// rebuild the hash table and linked list pointers based on current
	// contents of the counters array
	int i;
	LCLCounter * pt;

	for (i=0; i<lcl->hashsize;i++)
		lcl->hashtable[i]=0;
	// first, reset the hash table
	for (i=1; i<=lcl->size;i++) {
		lcl->counters[i].next=NULL;
		lcl->counters[i].prev=NULL;
	}
	// empty out the linked list
	for (i=1; i<=lcl->size;i++) { // for each item in the data structure
		pt=&lcl->counters[i];
		pt->next=lcl->hashtable[lcl->counters[i].hash];
		if (pt->next)
			pt->next->prev=pt;
		lcl->hashtable[lcl->counters[i].hash]=pt;
	}
}

void Heapify(LCL_type * lcl, int ptr)
{ // restore the heap condition in case it has been violated
	LCLCounter tmp;
	LCLCounter * cpt, *minchild;
	int mc;

	while(1)
	{
		if ((ptr<<1) + 1>lcl->size) break;
		// if the current node has no children

		cpt=&lcl->counters[ptr]; // create a current pointer
		mc=(ptr<<1)+
			((lcl->counters[ptr<<1].count<lcl->counters[(ptr<<1)+1].count)? 0 : 1);
		minchild=&lcl->counters[mc];
		// compute which child is the lesser of the two

		if (cpt->count < minchild->count) break;
		// if the parent is less than the smallest child, we can stop

		tmp=*cpt;
		*cpt=*minchild;
		*minchild=tmp;
		// else, swap the parent and child in the heap

		if (cpt->hash==minchild->hash)
			// test if the hash value of a parent is the same as the 
			// hash value of its child
		{ 
			// swap the prev and next pointers back. 
			// if the two items are in the same linked list
			// this avoids some nasty buggy behaviour
			minchild->prev=cpt->prev;
			cpt->prev=tmp.prev;
			minchild->next=cpt->next;
			cpt->next=tmp.next;
		} else { // ensure that the pointers in the linked list are correct
			// check: hashtable has correct pointer (if prev ==0)
			if (!cpt->prev) { // if there is no previous pointer
				if (cpt->item!=LCL_NULLITEM)
					lcl->hashtable[cpt->hash]=cpt; // put in pointer from hashtable
			} else
				cpt->prev->next=cpt;
			if (cpt->next) 
				cpt->next->prev=cpt; // place in linked list

			if (!minchild->prev) // also fix up the child
				lcl->hashtable[minchild->hash]=minchild;
			else
				minchild->prev->next=minchild; 
			if (minchild->next)
				minchild->next->prev=minchild;
		}
		ptr=mc;
		// continue on with the heapify from the child position
	} 
}

LCLCounter * LCL_FindItem(LCL_type * lcl, LCLitem_t item)
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

void LCL_Update(LCL_type * lcl, LCLitem_t item, LCLweight_t value)
{
	int hashval;
	LCLCounter * hashptr;
	// find whether new item is already stored, if so store it and add one
	// update heap property if necessary

	lcl->n+=value;
	lcl->counters->item=0; // mark data structure as 'dirty'

	hashval=(int) hash31(lcl->hasha, lcl->hashb,item) % lcl->hashsize;
	hashptr=lcl->hashtable[hashval];
	// compute the hash value of the item, and begin to look for it in 
	// the hash table

	while (hashptr) {
		if (hashptr->item==item) {
			hashptr->count+=value; // increment the count of the item
			Heapify(lcl,hashptr-lcl->counters); // and fix up the heap
			return;
		}
		else hashptr=hashptr->next;
	}
	// if control reaches here, then we have failed to find the item
	// so, overwrite smallest heap item and reheapify if necessary
	// fix up linked list from hashtable
	if (!lcl->root->prev) // if it is first in its list
		lcl->hashtable[lcl->root->hash]=lcl->root->next;
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

/********************************************************************
Implementation of Frequent algorithm to Find Frequent Items
Based on papers by:
Misra and Gries, 1982
Demaine, Lopez-Ortiz, Munroe, 2002
Karp, Papadimitriou and Shenker, 2003
Implementation by G. Cormode 2002, 2003

Original Code: 2002-11
This version: 2003-10

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA. 
*********************************************************************/

LCU_type * LCU_Init(float fPhi)
{
	int i;
	int k = 1 + (int) 1.0/fPhi;

	LCU_type* result = (LCU_type*) calloc(1,sizeof(LCU_type));

	result->a= (long long) 698124007;
	result->b= (long long) 5125833;
	if (k<1) k=1;
	result->k=k;
	result->n=0;  

	result->tblsz=LCU_HASHMULT*k;  
	result->hashtable=(LCUITEM**) calloc(result->tblsz,sizeof(LCUITEM *));
	result->groups=(LCUGROUP *)calloc(k,sizeof(LCUGROUP));
	result->items=(LCUITEM *) calloc(k,sizeof(LCUITEM));
	result->freegroups=(LCUGROUP **) calloc(k,sizeof(LCUGROUP*));

	for (i=0; i<result->tblsz;i++) 
		result->hashtable[i]=NULL;

	result->root=result->groups;
	result->groups->count=0;
	result->groups->nextg=NULL;
	result->groups->previousg=NULL;

	result->groups->items=result->items;
	for (i=0; i<k;i++)
		result->freegroups[i]=&result->groups[i];
	result->gpt=1; // initialize list of free groups

	for (i=0;i<k;i++) 
	{
		result->items[i].item=0;
		result->items[i].delta=0;
		result->items[i].hash=0;
		result->items[i].nexti=NULL;
		result->items[i].previousi=NULL;  // initialize values

		result->items[i].parentg=result->groups;
		result->items[i].nexting=&(result->items[i+1]);
		result->items[i].previousing=&(result->items[i-1]); 
		// create doubly linked list
	}
	result->items[0].previousing=&(result->items[k-1]);
	result->items[k-1].nexting=&(result->items[0]);
	// fix start and end of linked list

	return(result);
}  

void LCU_ShowGroups(LCU_type * lcu) {
	LCUGROUP *g;
	LCUITEM *i,*first;
	int n, wt;

	g=lcu->groups;
	wt=0;
	n=0;
	while (g!=NULL) 
	{
		printf("Group %d :",g->count);
		first=g->items;
		i=first;
		if (i!=NULL)
			do 
			{
				printf("%d -> ",i->item);
				i=i->nexting;
				wt+=g->count;
				n++;
			}
			while (i!=first);
		else printf(" empty");
		printf(")");
		g=g->nextg;
		if ((g!=NULL) && (g->previousg->nextg!=g))
			printf("Badly linked");
		printf("\n");
	}
	printf("In total, %d items, with a total count of %d\n",n,wt);
}

void LCU_InsertIntoHashtable(LCU_type *lcu, LCUITEM *newi, int i, int newitem){
	newi->nexti=lcu->hashtable[i];
	newi->item=newitem; // overwrite the old item
	newi->hash=i;
	newi->previousi=NULL;
	// insert item into the hashtable
	if (lcu->hashtable[i])
		lcu->hashtable[i]->previousi=newi;
	lcu->hashtable[i]=newi;
}

int LCU_cmp( const void * a, const void * b) {
	LCUITEM * x = (LCUITEM*) a;
	LCUITEM * y = (LCUITEM*) b;
	if (x->parentg->count<y->parentg->count) return -1;
	else if (x->parentg->count>y->parentg->count) return 1;
	else return 0;
}

std::map<uint32_t, uint32_t> LCU_Output(LCU_type * lcu, int thresh)
{
	std::map<uint32_t, uint32_t> res;

	for (int i=0; i<lcu->k; ++i) 
		if (lcu->items[i].parentg->count>=thresh) 
			res.insert(std::pair<uint32_t, uint32_t>(lcu->items[i].item, lcu->items[i].parentg->count));

	return res;
}

LCUITEM * LCU_GetNewCounter(LCU_type * lcu) {
	LCUITEM * newi;
	int j;

	newi=lcu->root->items;  // take a counter from the first group
	// but currently it remains in the same group

	newi->nexting->previousing=newi->previousing;
	newi->previousing->nexting=newi->nexting;
	// unhook the new item from the linked list in the hash table	    

	// need to remove this item from the hashtable
	j = newi->hash;
	if (lcu->hashtable[j]==newi)
		lcu->hashtable[j]=newi->nexti;

	if (newi->nexti!=NULL)
		newi->nexti->previousi=newi->previousi;
	if (newi->previousi!=NULL)
		newi->previousi->nexti=newi->nexti;

	return (newi);
}

void LCU_PutInNewGroup(LCU_type * lcu, LCUITEM *newi, LCUGROUP * tmpg){ 
	LCUGROUP * oldgroup;

	oldgroup=newi->parentg;  
	// put item in the tmpg group
	newi->parentg=tmpg;

	if (newi->nexting!=newi) // if the group does not have size 1
	{ // remove the item from its current group
		newi->nexting->previousing=newi->previousing;
		newi->previousing->nexting=newi->nexting;
		oldgroup->items=oldgroup->items->nexting;
	}
	else { // group will be empty
		if (oldgroup->nextg!=NULL) // there is another group
			oldgroup->nextg->previousg=oldgroup->previousg;
		if (lcu->root==oldgroup) // this is the first group
			lcu->root=oldgroup->nextg;
		else
			oldgroup->previousg->nextg=oldgroup->nextg;
		lcu->freegroups[--lcu->gpt]=oldgroup;
		// if we have created an empty group, remove it 
	}	
	newi->nexting=tmpg->items;
	newi->previousing=tmpg->items->previousing;
	newi->previousing->nexting=newi;
	newi->nexting->previousing=newi;
}

void LCU_AddNewGroupAfter(LCU_type * lcu, LCUITEM *newi, LCUGROUP *oldgroup) {
	LCUGROUP *newgroup;

	// remove item from old group...
	newi->nexting->previousing=newi->previousing;
	newi->previousing->nexting=newi->nexting;
	oldgroup->items=newi->nexting;
	//get new group
	newgroup=lcu->freegroups[lcu->gpt++];
	newgroup->count=oldgroup->count+1; // set count to be one more the prev group
	newgroup->items=newi;
	newgroup->previousg=oldgroup;
	newgroup->nextg=oldgroup->nextg;
	oldgroup->nextg=newgroup;
	if (newgroup->nextg!=NULL) // if there is another group
		newgroup->nextg->previousg=newgroup;
	newi->parentg=newgroup;
	newi->nexting=newi;
	newi->previousing=newi;
}

void LCU_IncrementCounter(LCU_type * lcu, LCUITEM *newi)
{
	LCUGROUP *oldgroup;

	oldgroup=newi->parentg;
	if ((oldgroup->nextg!=NULL) && 
		(oldgroup->nextg->count - oldgroup->count==1))
		LCU_PutInNewGroup(lcu, newi,oldgroup->nextg);
	// if the next group exists
	else { // need to create a new group with a differential of one
		if (newi->nexting==newi) // if there is only one item in the group...
			newi->parentg->count++;
		else      
			LCU_AddNewGroupAfter(lcu,newi,oldgroup);
	}
}

void LCU_Update(LCU_type * lcu, int newitem) {
	int h;
	LCUITEM *il;

	lcu->n++;
	h=hash31(lcu->a,lcu->b,newitem) % lcu->tblsz;
	il=lcu->hashtable[h];
	while (il) {
		if (il->item ==newitem) 
			break;
		il=il->nexti;
	}
	if (il==NULL) // item is not monitored (not in hashtable) 
	{
		il=LCU_GetNewCounter(lcu);
		/// and put it into the hashtable for the new item 
		il->delta=lcu->root->count;
		// initialize delta with count of first group
		LCU_InsertIntoHashtable(lcu,il,h,newitem);
		// put the new counter into the first group
		// counter is already in first group by defn of how we got it
		LCU_IncrementCounter(lcu, il);
	}
	else 
		LCU_IncrementCounter(lcu, il);
	// if we have an item, we need to increment its counter 
}

int LCU_Size(LCU_type * lcu) {
	return sizeof(LCU_type)+(lcu->tblsz)*sizeof(LCUITEM*) + 
		(lcu->k)*(sizeof(LCUITEM) + sizeof(LCUGROUP) + sizeof(LCUITEM*));
}

void LCU_Destroy(LCU_type * lcu)
{
	free(lcu->freegroups);
	free(lcu->items);
	free(lcu->groups);
	free(lcu->hashtable);
	free (lcu);
}  
