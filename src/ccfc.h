// ccfc.h -- header file for Adaptive Group Testing, Graham Cormode, 2003
// using Count Sketches, proposed in CCFC 2002.

#ifndef CCFC_h
#define CCFC_h

#include "prng.h"

typedef struct CCFC_type{
  int tests;
  int logn;
  int gran;
  int buckets;
  int count;
  int ** counts;
  int64_t *testa, *testb, *testc, *testd;
} CCFC_type;

extern CCFC_type * CCFC_Init(int, int, int, int);
extern void CCFC_Update(CCFC_type *, int, int); 
extern int CCFC_Count(CCFC_type *, int, int);
extern std::map<uint32_t, uint32_t> CCFC_Output(CCFC_type *, int);
extern int64_t CCFC_F2Est(CCFC_type *);
extern void CCFC_Destroy(CCFC_type *);
extern int CCFC_Size(CCFC_type *);

#endif

