// lossycount.h -- header file for Lossy Counting
// see Manku & Motwani, VLDB 2002 for details
// implementation by Graham Cormode, 2002,2003

#ifndef LOSSYCOUNTING_h
#define LOSSYCOUNTING_h

#include "prng.h"

typedef struct lccounter
{
  int item;
  int count;
} LCCounter;

typedef struct LC_type
{
  LCCounter *bucket;
  LCCounter *holder;
  LCCounter *newcount;
  int buckets;
  int holdersize;
  int maxholder;
  int window;
  int epoch;
} LC_type;

extern LC_type * LC_Init(float);
extern void LC_Destroy(LC_type *);
extern void LC_Update(LC_type *, int);
extern int LC_Size(LC_type *);
extern int LC_PointEst(LC_type *, int);
extern std::map<uint32_t, uint32_t> LC_Output(LC_type *,int);

// lossycount.h -- header file for Lossy Counting
// see Manku & Motwani, VLDB 2002 for details
// implementation by Graham Cormode, 2002,2003

typedef struct lcdcounter
{
  int item;
  int count;
  int delta;
} LCDCounter;

typedef struct LCD_type
{
  LCDCounter *bucket;
  LCDCounter *holder;
  LCDCounter *newcount;
  int buckets;
  int holdersize;
  int maxholder;
  int window;
  int epoch;
} LCD_type;

extern LCD_type * LCD_Init(float);
extern void LCD_Destroy(LCD_type *);
extern void LCD_Update(LCD_type *, int);
extern int LCD_Size(LCD_type *);
extern int LCD_PointEst(LCD_type *, int);
extern std::map<uint32_t, uint32_t> LCD_Output(LCD_type *,int);

// lclazy.h -- header file for Lazy Lossy Counting
// see Manku & Motwani, VLDB 2002 for details
// implementation by Graham Cormode, 2002,2003, 2005

/////////////////////////////////////////////////////////
#define LCLweight_t int
//#define LCL_SIZE 101 // size of k, for the summary
// if not defined, then it is dynamically allocated based on user parameter
////////////////////////////////////////////////////////

#define LCLitem_t uint32_t

typedef struct lclcounter_t LCLCounter;

struct lclcounter_t
{
  LCLitem_t item; // item identifier
  int hash; // its hash value
  LCLweight_t count; // (upper bound on) count for the item
  LCLweight_t delta; // max possible error in count for the value
  LCLCounter *prev, *next; // pointers in doubly linked list for hashtable
}; // 32 bytes

#define LCL_HASHMULT 3  // how big to make the hashtable of elements:
  // multiply 1/eps by this amount
  // about 3 seems to work well

#ifdef LCL_SIZE
#define LCL_SPACE (LCL_HASHMULT*LCL_SIZE)
#endif

typedef struct LCL_type
{
  LCLweight_t n;
  int hasha, hashb, hashsize;
  int size;
  LCLCounter *root;
#ifdef LCL_SIZE
  LCLCounter counters[LCL_SIZE+1]; // index from 1
  LCLCounter *hashtable[LCL_SPACE]; // array of pointers to items in 'counters'
  // 24 + LCL_SIZE*(32 + LCL_HASHMULT*4) + 8
            // = 24 + 102*(32+12)*4 = 4504
            // call it 4520 for luck...
#else
  LCLCounter *counters;
  LCLCounter ** hashtable; // array of pointers to items in 'counters'
#endif
} LCL_type;

extern LCL_type * LCL_Init(float fPhi);
extern void LCL_Destroy(LCL_type *);
extern void LCL_Update(LCL_type *, LCLitem_t, int);
extern int LCL_Size(LCL_type *);
extern int LCL_PointEst(LCL_type *, LCLitem_t);
extern int LCL_PointErr(LCL_type *, LCLitem_t);
extern std::map<uint32_t, uint32_t> LCL_Output(LCL_type *,int);

//////////////////////////////////////////////////////
typedef int LCUWT;
//#define LCU_SIZE 101
// if not defined, then it is dynamically allocated based on user parameter
//////////////////////////////////////////////////////

#define LCU_HASHMULT 3
#ifdef LCU_SIZE
#define LCU_TBLSIZE (LCU_HASHMULT*LCU_SIZE)
#endif

typedef struct lcu_itemlist LCUITEMLIST;
typedef struct lcu_group LCUGROUP;

typedef struct lcu_item LCUITEM;
typedef struct lcu_group LCUGROUP;

struct lcu_group 
{
  LCUWT count;
  LCUITEM *items;
  LCUGROUP *previousg, *nextg;
};

struct lcu_item 
{
  unsigned int item;
  int hash;
  LCUWT delta;
  LCUGROUP *parentg;
  LCUITEM *previousi, *nexti;
  LCUITEM *nexting, *previousing;
};

typedef struct LCU_type{

  LCUWT n;
  int gpt;
  int k;
  int tblsz;
  long long a,b;
  LCUGROUP * root;
#ifdef LCU_SIZE
  LCUITEM items[LCU_SIZE];
  LCUGROUP groups[LCU_SIZE];
  LCUGROUP *freegroups[LCU_SIZE];
  LCUITEM* hashtable[LCU_TBLSIZE];
#else
  LCUITEM * items;
  LCUGROUP *groups;
  LCUGROUP **freegroups;
  LCUITEM **hashtable;

#endif

} LCU_type;

extern LCU_type * LCU_Init(float fPhi);
extern void LCU_Destroy(LCU_type *);
extern void LCU_Update(LCU_type *, int);
extern int LCU_Size(LCU_type *);
extern std::map<uint32_t, uint32_t> LCU_Output(LCU_type *,int);

#endif
