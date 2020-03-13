// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
void Do_Nothing(int which)
{
    for (int num = 1; num <= 5; num++)
    {
        printf("*** %s with TID %d and UID %d priority %d looped %d times\n", currentThread->getName(), currentThread->getTid(), currentThread->getUid(), currentThread->getPriority(), num);
    }
    currentThread->Yield();
}

void SimpleThread(int which)
{
    int num;
    for (num = 1; num <= 5; num++)
    {
        
        printf("*** %s with TID %d and UID %d priority %d looped %d times\n", 
            currentThread->getName(), 
            currentThread->getTid(), 
            currentThread->getUid(), 
            currentThread->getPriority(), 
            num);
        if (num == 2)
        {
            //interrupt->OneTick();
            //printf("Created Novel\n");
            Thread *t0 = Thread::Create_thread("Novel Thread", 0);
            t0->Fork(Do_Nothing, 0);
        }
    }
}



void ThreadTest_Priority()
{
    Thread *t1 = Thread::Create_thread("Thread 1", 5);
    Thread *t2 = Thread::Create_thread("Thread 2", 3);
    Thread *t3 = Thread::Create_thread("Thread 3", 2);

    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t3->Fork(SimpleThread, 3);

    //Thread::TS();
}

void TestRR(int runningTime)
{
    for (int i = 0; i < runningTime*10; i++)
    {
        printf(">> thread %s running for %d, looped %d times | totalTicks: %d \n",
               currentThread->getName(),
               runningTime,
               i + 1,
               stats->totalTicks);
        interrupt->OneTick();
    }
    currentThread->Finish();
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void ThreadTest2()
{
    DEBUG('t', "Entering RR test");

    printf("Initial Tick:\tSystem=%d, user=%d, total=%d\n", stats->systemTicks, stats->userTicks, stats->totalTicks);

    Thread *t1 = Thread::Create_thread("p7");
    Thread *t2 = Thread::Create_thread("p2");
    Thread *t3 = Thread::Create_thread("p5");

    printf("Create Tick:\tSystem=%d, user=%d, total=%d\n", stats->systemTicks, stats->userTicks, stats->totalTicks);

    t1->Fork(TestRR, 7);
    t2->Fork(TestRR, 2);
    t3->Fork(TestRR, 5);

    printf("Fork Tick:\tSystem=%d, user=%d, total=%d\n", stats->systemTicks, stats->userTicks, stats->totalTicks);

    scheduler->setLastSwitchTick(stats->totalTicks);
    currentThread->Yield();
}
void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest_Priority();
        break;
    case 2:
        ThreadTest2();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
