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

void SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        Thread::TS();
        printf("*** thread %d with TID %d and UID %d priority %d looped %d times\n", which, currentThread->getTid(), currentThread->getUid(), currentThread->getPriority(), num);
        //currentThread->Sleep();
    }
}

void ThreadTest_Priority()
{
    Thread *t1 = Thread::Create_thread("Thread 1",5);
    Thread *t2 = Thread::Create_thread("Thread 2",3);
    Thread *t3 = Thread::Create_thread("Thread 3",1);

    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t3->Fork(SimpleThread, 3);

    //Thread::TS();
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest_Priority();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
