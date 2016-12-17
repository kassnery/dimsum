#pragma once
#include "prng.h"
#include <windows.h>
#include<atomic>
// losum.h -- header file for Lossy Summing

/////////////////////////////////////////////////////////
#define LSweight_t int
#define LSAtomicWeight_t std::atomic<LSweight_t>
//#define LS_SIZE 101 // size of k, for the summary
// if not defined, then it is dynamically allocated based on user parameter
////////////////////////////////////////////////////////

#define LSitem_t uint32_t
#define LSAtomicItem_t std::atomic<LSitem_t>

typedef struct LScounter_t LSCounter;

struct LScounter_t
{
	LSitem_t item; // item identifier
	LSweight_t count; // (upper bound on) count for the item
	LSCounter* next;
	// Table does not support removals therefore does not define prev,
}; // 32 bytes

#define LS_HASHMULT 3  // how big to make the hashtable of elements:
   // multiply 1/eps by this amount
   // about 3 seems to work well

#ifdef LS_SIZE
#define LS_SPACE (LS_HASHMULT*LS_SIZE)
#endif

typedef struct LS_type
{
	LSweight_t n;
	std::atomic_int blocksLeftThisUpdate, blocksLeft, quantile;
	int nActive, nPassive, left2Move;
	int hasha, hashb, hashsize;
	int size, maxMaintenanceTime;
	int* buffer;
	int clearedFromPassive, movedFromPassive, stepsLeft, copied2Buffer;
	float epsilon;
	HANDLE handle, 
		maintenanceStepSemaphore, finishUpdateSemaphore;
	std::atomic_bool loosa, done, finishedMedian;
	LSCounter *activeCounters;
	LSCounter *passiveCounters;
	LSCounter ** activeHashtable; // array of pointers to items in 'counters'
	LSCounter ** passiveHashtable; // array of pointers to items in 'counters'
} LS_type;

extern LS_type * LS_Init(float fPhi, float gamma);
extern void LS_Destroy(LS_type *);
extern void LS_Update(LS_type *, LSitem_t, int);
extern int LS_Size(LS_type *);
extern int LS_PointEst(LS_type *, LSitem_t);
extern int LS_PointErr(LS_type *, LSitem_t);
extern void LS_CheckHash(LS_type * LS, int item, int hash);
extern std::map<uint32_t, uint32_t> LS_Output(LS_type *, uint64_t thresh);
extern int in_place_find_kth(int *v, int n, int k, int jump=1, int pivot=0);
extern DWORD WINAPI LS_Maintenance(LPVOID lpParam);
extern void LS_FinishStep(LS_type* LS);