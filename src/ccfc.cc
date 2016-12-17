/********************************************************************
Adaptive Group Testing to Find Frequent Items using CCFC sketches
G. Cormode 2003

This version: 2003-10

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA. 
*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "ccfc.h"
#include "prng.h"

CCFC_type * CCFC_Init(int buckets, int tests, int lgn, int gran)
{
  // Create the data structure for Adaptive Group Testing
  // Keep T tests.  Each test has buckets buckets
  // lgn is the bit depth of the items which will arrive
  // gran is the granularity at which to perform the testing
  // gran = 1 means to do one bit at a time,
  // gran = 8 means to do one quad at a time, etc. 

  int i;
  CCFC_type * result;
  prng_type * prng;

  prng=prng_Init(-632137,2);

  result = (CCFC_type*) calloc(1,sizeof(CCFC_type));
  if (result==NULL) exit(1);
  result->tests=tests;
  result->logn=lgn;
  result->gran=gran;
  result->buckets=buckets;
  result->count=0;
  result->testa=(int64_t*) calloc(tests,sizeof(int64_t));
  result->testb=(int64_t*) calloc(tests,sizeof(int64_t));
  result->testc=(int64_t*) calloc(tests,sizeof(int64_t));
  result->testd=(int64_t*) calloc(tests,sizeof(int64_t));
  // create space for the hash functions

  //  printf("Creating with %d buckets, %d subbuckets\n",
  // buckets,result->subbuckets);

  result->counts=(int **) calloc(1+lgn,sizeof(int *));
  if (result->counts==NULL) exit(1); 
  // create space for the counts
  for (i=0;i<=lgn;i+=gran)
    {
      result->counts[i]=(int *) calloc(buckets*tests, sizeof(int));
      if (result->counts[i]==NULL) exit(1); 
    }

  for (i=0;i<tests;i++)
    {
      result->testa[i]=(int64_t) prng_int(prng);
      if (result->testa[i]<0) result->testa[i]= -result->testa[i];
      result->testb[i]=(int64_t) prng_int(prng);
      if (result->testb[i]<0) result->testb[i]= -result->testb[i];
      result->testc[i]=(int64_t) prng_int(prng);
      if (result->testc[i]<0) result->testc[i]= -result->testc[i];
      result->testd[i]=(int64_t) prng_int(prng);
      if (result->testd[i]<0) result->testd[i]= -result->testd[i];
      // initialise the hash functions
      // prng_int() should return a random integer
      // uniformly distributed in the range 0..2^31
    }
  prng_Destroy(prng);
  return (result);
}

void CCFC_Update(CCFC_type * ccfc, int item, int diff)
{
  int i,j;
  unsigned int hash;
  int mult, offset;

  ccfc->count+=diff;
  for (i=0;i<ccfc->logn;i+=ccfc->gran)
    {
      offset=0;
      for (j=0;j<ccfc->tests;j++)
	{
	  hash=hash31(ccfc->testa[j],ccfc->testb[j],item);
	  hash=hash % (ccfc->buckets); 
	  mult=hash31(ccfc->testc[j],ccfc->testd[j],item);
	  if ((mult&1)==1)
	    ccfc->counts[i][offset+hash]+=diff;
	  else
	    ccfc->counts[i][offset+hash]-=diff;
	  /*
	  printf("inserting item %d into level %d, bucket %d\n",
		 item,i,offset+hash);
	  */
	  offset+=ccfc->buckets;
	}
      item>>=ccfc->gran;
    }
}

int CCFC_Count(CCFC_type * ccfc, int depth, int item)
{
	int i;
	int offset;
	int * estimates;
	unsigned int hash;
	int mult;

	if (depth==ccfc->logn) return(ccfc->count);
	estimates=(int *) calloc(1+ccfc->tests, sizeof(int));
	offset=0;
	for (i=1;i<=ccfc->tests;i++)
	{
		hash=hash31(ccfc->testa[i-1],ccfc->testb[i-1],item);
		hash=hash % (ccfc->buckets); 
		mult=hash31(ccfc->testc[i-1],ccfc->testd[i-1],item);
		if ((mult&1)==1)
			estimates[i]=ccfc->counts[depth][offset+hash];
		else
			estimates[i]=-ccfc->counts[depth][offset+hash];
		offset+=ccfc->buckets;
	}
	if (ccfc->tests==1) i=estimates[1];
	else if (ccfc->tests==2) i=(estimates[1]+estimates[2])/2; 
	else i=MedSelect(1+ccfc->tests/2,ccfc->tests,estimates);
	free(estimates);
	return(i);
}

void ccfc_recursive(CCFC_type * ccfc, int depth, int start, int thresh, std::map<uint32_t, uint32_t>& res)
{
	int i;
	int blocksize;
	int estcount;
	int itemshift;

	estcount = CCFC_Count(ccfc,depth,start);

	if (estcount>=thresh) 
	{ 
		if (depth==0)
		{
			if (res.size() < ccfc->buckets) res[start] = estcount;
		}
		else
		{
			blocksize=1<<ccfc->gran;
			itemshift=start<<ccfc->gran;
			// assumes that gran is an exact multiple of the bit dept
			for (i=0;i<blocksize;i++)
				ccfc_recursive(ccfc,depth-ccfc->gran,itemshift+i,thresh,res);
		}
	}
}

std::map<uint32_t, uint32_t> CCFC_Output(CCFC_type * ccfc, int thresh)
{
	std::map<uint32_t, uint32_t> res;
	ccfc_recursive(ccfc,ccfc->logn,0,thresh,res);
	return res;
}

int64_t CCFC_F2Est(CCFC_type * ccfc)
{
  int i,j, r;
  int64_t * estimates;
  int64_t result, z;

  estimates=(int64_t *) calloc(1+ccfc->tests, sizeof(int64_t));
  r=0;
  for (i=1;i<=ccfc->tests;i++)
    {
      z=0;
      for (j=0;j<ccfc->buckets;j++)
	{
	  z+=((int64_t) ccfc->counts[0][r]* (int64_t) ccfc->counts[0][r]);
	  r++;
	}
      estimates[i]=z;
    }
  if (ccfc->tests==1) result=estimates[1];
  else if (ccfc->tests==2) result=(estimates[1]+estimates[2])/2; 
  else
    result=LLMedSelect(1+ccfc->tests/2,ccfc->tests,estimates);
  free(estimates);
  return(result);
}

int CCFC_Size(CCFC_type * ccfc){
    int size;

    size=(ccfc->logn+1)*(sizeof(int *))+ 
      (1+ccfc->logn/ccfc->gran)*(ccfc->buckets*ccfc->tests)*sizeof(int)+
      ccfc->tests*4*sizeof(uint32_t)+
      sizeof(CCFC_type);
    return size;
}

void CCFC_Destroy(CCFC_type * ccfc)
{
  int i;

  free(ccfc->testa);
  free(ccfc->testb);
  free(ccfc->testc);
  free(ccfc->testd);

  for (i=0;i<=ccfc->logn;i+=ccfc->gran)
    free(ccfc->counts[i]);
  free(ccfc->counts);
  free(ccfc);
}
