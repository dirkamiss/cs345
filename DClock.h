#include "os345.h"
extern Semaphore* deltaClockMutex;

#ifdef TESTING
typedef int Semaphore;
#endif

struct _DeltaClockItem {
	int tics;
	Semaphore* event;
	struct _DeltaClockItem* next;
};

typedef struct _DeltaClockItem DeltaClockItem;

typedef struct {
	DeltaClockItem* head;
} DClock;

DClock* newDeltaClock();
void deleteDeltaClock(DClock* dc);
void printClock(DClock* dc);
void insert(DClock* dc, int tics, Semaphore* event);
void tic(DClock* dc);
