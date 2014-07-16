// os345p3.c - Jurassic Park
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include "os345.h"
#include "os345park.h"

#include "DClock.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over

extern DClock* dc;
extern Semaphore* deltaClockMutex;

Semaphore* getPassenger;
Semaphore* seatTaken;
Semaphore* rideDone[MAX_TASKS];
Semaphore* timeEvent[MAX_TASKS];
Semaphore* passengerSeated;
int curVisitor;
Semaphore* visitorMutex;
Semaphore* spotInPark;
Semaphore* spotInGiftShop;
Semaphore* spotInMuseum;
Semaphore* carTicket;

Semaphore* driverDoneSemaphore;
Semaphore* driverReady;
Semaphore* driverMutex;
Semaphore* driver;
Semaphore* needDriver;
Semaphore* needTicket;
Semaphore* takeTicket;
int currCarID;
int driverID = 0;

#define MAX_ENTRANCE_TIME 10			// in seconds
#define MAX_LINE_TIME 3			// in seconds
#define MAX_GIFT_SHOP_TIME 10			// in seconds
#define MAX_MUSEUM_TIME 10			// in seconds


// ***********************************************************************
// project 3 functions and tasks
int P3_carTask(int argc, char* argv[]);
int P3_visitorTask(int argc, char* argv[]);
int P3_driverTask(int argc, char* argv[]);
void ticketBooth(int visitorId);
void museum(int visitorId);
void tourCar(int visitorId);
void giftShop(int visitorId);


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	int visitors;																	SWAP;

	if (argc == 1) {
		visitors = 45;																SWAP;
	}
	else {
		visitors = atoi(argv[1]);													SWAP;
	}

	// start park
	char buf[32];																	SWAP;
	char* newArgv[2];																SWAP;
	sprintf(buf, "jurassicPark");													SWAP;
	newArgv[0] = buf;																SWAP;
	createTask(buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,								// task count
		newArgv);																	SWAP;		// task argument

	getPassenger = createSemaphore("getPassenger", BINARY, 0);						SWAP;
	seatTaken = createSemaphore("seatTaken", BINARY, 0);							SWAP;
	passengerSeated = createSemaphore("passengerSeated", BINARY, 1);				SWAP;
	visitorMutex = createSemaphore("visitorMutex", BINARY, 1);						SWAP;
	spotInPark = createSemaphore("spotInPark", COUNTING, MAX_IN_PARK);				SWAP;
	spotInGiftShop = createSemaphore("spotInGiftShop", COUNTING, MAX_IN_GIFTSHOP);	SWAP;
	spotInMuseum = createSemaphore("spotInMuseum", COUNTING, MAX_IN_MUSEUM);		SWAP;
	carTicket = createSemaphore("museumTicket", COUNTING, MAX_TICKETS);				SWAP;

	driverReady = createSemaphore("Driver Ready", BINARY, 0);						SWAP;
	driverMutex = createSemaphore("Driver Mutex", BINARY, 1);						SWAP;
	driver = createSemaphore("Driver", BINARY, 0);									SWAP;
	needDriver = createSemaphore("Need Driver", BINARY, 0);							SWAP;
	needTicket = createSemaphore("Need Ticket", BINARY, 0);							SWAP;
	takeTicket = createSemaphore("Take Ticket", BINARY, 0);							SWAP;

	// wait for park to get initialized...
	while (!parkMutex)																SWAP;
	printf("\nStart Jurassic Park...");												SWAP;

	// create car, driver, and visitor tasks here
	int id;																			SWAP;
	sprintf(buf, "carTask");														SWAP;
	id = 0;																			SWAP;
	newArgv[0] = buf;																SWAP;
	newArgv[1] = (char*)&id;														SWAP;
	for (id = 0; id < NUM_CARS; id++) {
		createTask(buf, P3_carTask, MED_PRIORITY, 2, newArgv);						SWAP;
	}

	sprintf(buf, "visitorTask");													SWAP;
	for (id = 0; id < visitors; id++) {
		createTask(buf, P3_visitorTask, MED_PRIORITY, 2, newArgv);					SWAP;
	}

	for (id = 1; id <= NUM_DRIVERS; id++)
	{
		char name[32];																SWAP;
		sprintf(name, "Driver %d", id);												SWAP;
		createTask(name, P3_driverTask, MED_PRIORITY, 0, 0);						SWAP;
		driverID++;																	SWAP;
	}

	return 0;
} // end project3



// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	// Display the current delta clock contents
	printf("\nDelta Clock\n");														SWAP;
	printClock(dc);																	SWAP;

	return 0;
} // end CL3_dc


// ***********************************************************************
// ***********************************************************************
// Car Task
int P3_carTask(int argc, char* argv[]) {
	int i;																			
	Semaphore* carRideDone[NUM_SEATS+1];											
	int carId = argv[1][0];															

	while (1) {
		for (i = 0; i < NUM_SEATS; i++) {
			semWait(fillSeat[carId]);												SWAP;

			semSignal(getPassenger);												SWAP;	// signal for visitor
			semWait(seatTaken);														SWAP;	// wait for visitor to reply

			// save passenger ride over semaphore
			carRideDone[i] = rideDone[curVisitor];									SWAP;

			semSignal(passengerSeated);												SWAP;	// signal visitor in seat

			if (i == 2)
			{
				SEM_WAIT(driverMutex);												SWAP;
				SEM_SIGNAL(needDriver);												SWAP;
				currCarID = carId;													SWAP;
				SEM_SIGNAL(driver);													SWAP;
				SEM_WAIT(driverReady);												SWAP;
				carRideDone[3] = driverDoneSemaphore;								SWAP;
				SEM_SIGNAL(driverMutex);											SWAP;
			}

			semSignal(seatFilled[carId]);											SWAP;
			semWait(parkMutex);														SWAP;
			myPark.numInCarLine--;													SWAP;
			myPark.numInCars++;														SWAP;
			semSignal(parkMutex);													SWAP;
		}

		semWait(rideOver[carId]);													SWAP;

		// Release passengers
		for (i = 0; i < NUM_SEATS; i++) {
			semWait(parkMutex);														SWAP;
			myPark.numInCars--;														SWAP;
			myPark.numInGiftLine++;													SWAP;
			semSignal(parkMutex);													SWAP;
			semSignal(carRideDone[i]);												SWAP;
		}

		//Release driver
		semSignal(carRideDone[3]);													SWAP;
	}
}

int P3_driverTask(int argc, char* argv[])
{
	char name[32];																	
	int id = driverID;																
	sprintf(name, "Driver Done %d", id);											
	Semaphore* driverDone = createSemaphore(name, BINARY, 0);						

	while (1)
	{

		SEM_WAIT(parkMutex);														SWAP;
		myPark.drivers[id] = 0;														SWAP;
		SEM_SIGNAL(parkMutex);														SWAP;
		SEM_WAIT(driver);															SWAP;

		if (semTryLock(needDriver))
		{
			driverDoneSemaphore = driverDone;										SWAP;

			SEM_WAIT(parkMutex);													SWAP;
			myPark.drivers[id] = currCarID + 1;										SWAP;
			SEM_SIGNAL(parkMutex);													SWAP;

			SEM_SIGNAL(driverReady);												SWAP;
			SEM_WAIT(driverDone);													SWAP;
		}
		else if (semTryLock(needTicket))
		{
			SEM_WAIT(parkMutex);													SWAP;
			myPark.drivers[id] = -1;												SWAP;
			SEM_SIGNAL(parkMutex);													SWAP;
			SEM_SIGNAL(takeTicket);													SWAP;
		}
		else break;
	}
	return 0;
}


// ***********************************************************************
// ***********************************************************************
// Visitor Task
int P3_visitorTask(int argc, char* argv[]) {
	int visitorId = argv[1][0];														SWAP;

	// Set up visitor-specific semaphores
	char buf[32];																	SWAP;
	sprintf(buf, "timeEvent%d", visitorId);											SWAP;
	timeEvent[visitorId] = createSemaphore(buf, BINARY, 0);							SWAP;
	sprintf(buf, "rideDone%d", visitorId);											SWAP;
	rideDone[visitorId] = createSemaphore(buf, BINARY, 0);							SWAP;

	semWait(parkMutex);																SWAP;
	myPark.numOutsidePark++;														SWAP;
	semSignal(parkMutex);															SWAP;

	// Wait random time before attempting to enter park
	int waitTime = rand() % (MAX_ENTRANCE_TIME * 10) + 1;							SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;
	semWait(spotInPark);															SWAP;

	// Enter park and get in line for museum ticket
	semWait(parkMutex);																SWAP;
	myPark.numOutsidePark--;														SWAP;
	myPark.numInPark++;																SWAP;
	myPark.numInTicketLine++;														SWAP;
	semSignal(parkMutex);															SWAP;

	ticketBooth(visitorId);															SWAP;

	museum(visitorId);																SWAP;

	tourCar(visitorId);																SWAP;

	giftShop(visitorId);															SWAP;
}

void ticketBooth(int visitorId){
	//Wait randomly in line for a ticket
	int waitTime = rand() % (MAX_LINE_TIME * 10) + 1;								SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;
	SEM_WAIT(carTicket);															SWAP;

	//Wake up the driver and ask for a ticket, then wait for the ticket
	SEM_WAIT(driverMutex);															SWAP;
	SEM_SIGNAL(needTicket);															SWAP;
	SEM_SIGNAL(driver);																SWAP;
	//Wait randomly before taking ticket and giving to visitor
	//waitTime = rand() % (MAX_LINE_TIME * 10) + 1;			SWAP;
	//insert(dc, waitTime, timeEvent[visitorId]);			SWAP;
	//semWait(timeEvent[visitorId]);			SWAP;
	SEM_WAIT(takeTicket);															SWAP;
	SEM_SIGNAL(driverMutex);														SWAP;

	//Update park stats
	SEM_WAIT(parkMutex);															SWAP;
	myPark.numInTicketLine--;														SWAP;
	myPark.numTicketsAvailable--;													SWAP;
	myPark.numInMuseumLine++;														SWAP;
	SEM_SIGNAL(parkMutex);															SWAP;
}

void museum(int visitorId) {

	// Wait random time before attempting to enter museum
	int waitTime = rand() % (MAX_LINE_TIME * 10) + 1;								SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;
	semWait(spotInMuseum);															SWAP;

	semWait(parkMutex);																SWAP;
	myPark.numInMuseumLine--;														SWAP;
	myPark.numInMuseum++;															SWAP;
	semSignal(parkMutex);															SWAP;

	// Browse museum for random time
	waitTime = rand() % (MAX_MUSEUM_TIME * 10) + 1;									SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;

	semWait(parkMutex);																SWAP;
	myPark.numInMuseum--;															SWAP;
	myPark.numInCarLine++;															SWAP;
	semSignal(parkMutex);															SWAP;
	semSignal(spotInMuseum);														SWAP;

}

void tourCar(int visitorId) {
	// Wait random time before attempting to ride tour car
	int waitTime = rand() % (MAX_LINE_TIME * 10) + 1;								SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;
	semWait(getPassenger);															SWAP;

	semWait(visitorMutex);															SWAP;
	curVisitor = visitorId;															SWAP;
	semSignal(seatTaken);															SWAP;
	semWait(passengerSeated);														SWAP;

	//Update park stats
	SEM_WAIT(parkMutex);															SWAP;
	myPark.numTicketsAvailable++;													SWAP;
	SEM_SIGNAL(parkMutex);															SWAP;

	//Release passenger car ticket
	SEM_SIGNAL(carTicket);															SWAP;
	semSignal(visitorMutex);														SWAP;

	semWait(rideDone[visitorId]);													SWAP;
}

void giftShop(int visitorId) {
	// Wait random time before attempting to enter gift shop
	int waitTime = rand() % (MAX_LINE_TIME * 10) + 1;								SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;
	semWait(spotInGiftShop);														SWAP;

	semWait(parkMutex);																SWAP;
	myPark.numInGiftLine--;															SWAP;
	myPark.numInGiftShop++;															SWAP;
	semSignal(parkMutex);															SWAP;

	// Browse gift shop for random time, then leave park
	waitTime = rand() % (MAX_GIFT_SHOP_TIME * 10) + 1;								SWAP;
	insert(dc, waitTime, timeEvent[visitorId]);										SWAP;
	semWait(timeEvent[visitorId]);													SWAP;

	semWait(parkMutex);																SWAP;
	myPark.numInGiftShop--;															SWAP;
	myPark.numInPark--;																SWAP;
	myPark.numExitedPark++;															SWAP;
	semSignal(parkMutex);															SWAP;
	semSignal(spotInGiftShop);														SWAP;
	semSignal(spotInPark);															SWAP;
}
