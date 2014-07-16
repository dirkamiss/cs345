#include "os345.h"
extern Semaphore* DClockMutex;

#ifdef TESTING
typedef int Semaphore;
#endif

struct _DClockItem {
	int tics;
	Semaphore* event;
	struct _DClockItem* next;
};

typedef struct _DClockItem DClockItem;

typedef struct {
	DClockItem* head;
} DClock;

DClock* newDeltaClock();
void deleteDeltaClock(DClock* dc);
void printClock(DClock* dc);
void insert(DClock* dc, int tics, Semaphore* event);
void tic(DClock* dc);
