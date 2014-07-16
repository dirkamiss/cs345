#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "DClock.h"

DClock* newDeltaClock() {
	DClock* dc = (DClock*)malloc(sizeof(DClock));
	dc->head = NULL;
	return dc;
}

void deleteDeltaClock(DClock* dc) {
	assert("NULL DeltaClock*" && dc);
	DeltaClockItem* cur = dc->head;
	DeltaClockItem* temp;

	while (cur) {
		temp = cur;
		cur = cur->next;
		free(temp);
	}

	free(dc);
}

void printClock(DClock* dc) {
	printf("**");			SWAP;

	DeltaClockItem* cur = dc->head;			SWAP;

	while (cur) {
		printf("{%d} ", cur->tics);			SWAP;
		cur = cur->next;			SWAP;
	}

	printf("**\n");			SWAP;
}

void insert(DClock* dc, int tics, Semaphore* event) {
	assert("Invalid tic count" && tics > 0);
	semWait(deltaClockMutex);
	DeltaClockItem* item = (DeltaClockItem*)malloc(sizeof(DeltaClockItem));			SWAP;
	item->event = event;			SWAP;
	item->next = NULL;			SWAP;

	if (!dc->head) {
		item->tics = tics;			SWAP;
		dc->head = item;			SWAP;
	}
	else if (tics < dc->head->tics) {
		item->tics = tics;			SWAP;
		dc->head->tics -= tics;			SWAP;
		item->next = dc->head;			SWAP;
		dc->head = item;			SWAP;
	}
	else {
		int diff = tics - dc->head->tics;			SWAP;
		DeltaClockItem* prev = dc->head;			SWAP;
		DeltaClockItem* cur = dc->head->next;			SWAP;

		while (1) {
			if (!cur) {
				prev->next = item;			SWAP;
				item->tics = diff;			SWAP;
				break;			SWAP;
			}

			diff -= cur->tics;			SWAP;

			if (diff < 0) {
				item->tics = diff + cur->tics;			SWAP;
				cur->tics = -diff;			SWAP;
				item->next = cur;			SWAP;
				prev->next = item;			SWAP;
				break;			SWAP;
			}

			prev = prev->next;			SWAP;
			cur = cur->next;			SWAP;
		}
	}
	semSignal(deltaClockMutex);
}

void tic(DClock* dc) {
	if (dc->head) {
		dc->head->tics--;
		while (dc->head && dc->head->tics == 0) {
			semSignal(dc->head->event);
			DeltaClockItem* oldHead = dc->head;
			dc->head = dc->head->next;
			free(oldHead);
		}
	}
}
