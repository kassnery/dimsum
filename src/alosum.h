#pragma once
#include "prng.h"
#include <windows.h>
// losum.h -- header file for Lossy Summing

/////////////////////////////////////////////////////////
#define ALSweight_t int
//#define ALS_SIZE 101 // size of k, for the summary
// if not defined, then it is dynamically allocated based on user parameter
////////////////////////////////////////////////////////

#define ALSitem_t uint32_t
#define GAMMA 1.0
typedef struct ALScounter_t ALSCounter;

struct ALScounter_t
{
	ALSitem_t item; // item identifier
	int hash; // its hash value
	ALSweight_t count; // (upper bound on) count for the item
	ALSCounter *prev, *next; // pointers in doubly linked list for hashtable
};

#define ALS_HASHMULT 3  // how big to make the hashtable of elements:

#ifdef ALS_SIZE
#define ALS_SPACE (ALS_HASHMULT*ALS_SIZE)
#endif

typedef struct ALS_type
{
	ALSweight_t n;
	int hasha, hashb, hashsize;
	int size, maxMaintenanceTime;
	int nActive, nPassive, extra, movedFromPassive;
	int* buffer;
	int quantile;
	float epsilon;
	float gamma;
	HANDLE handle;
	ALSCounter *activeCounters;
	ALSCounter *passiveCounters;
	ALSCounter ** activeHashtable; // array of pointers to items in 'counters'
	ALSCounter ** passiveHashtable; // array of pointers to items in 'counters'
} ALS_type;

extern ALS_type * ALS_Init(float fPhi, float gamma = GAMMA);
extern void ALS_Destroy(ALS_type *);
extern void ALS_Update(ALS_type *, ALSitem_t, int);
extern int ALS_Size(ALS_type *);
extern int ALS_PointEst(ALS_type *, ALSitem_t);
extern int ALS_PointErr(ALS_type *, ALSitem_t);
extern void ALS_CheckHash(ALS_type * ALS, int item, int hash);
extern std::map<uint32_t, uint32_t> ALS_Output(ALS_type *, uint64_t thresh);
extern int ALS_in_place_find_kth(int *v, int n, int k, int jump=1, int pivot=0);
