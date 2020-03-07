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

    for (num = 0; num < 3; num++)
    {
        Thread::TS();
        printf("*** thread %d with TID %d and UID %d looped %d times\n", which, currentThread->getTid(), currentThread->getUid(), num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = Thread::Create_thread("Forked Thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
    Thread::TS();
}

void ThreadTest2_limit()
{
    DEBUG('t', "Entering limit_ThreadTest");
    for (int i = 0; i <= 128; i++)
    {
        Thread *t = Thread::Create_thread("Forked Thread");
        if(t!=NULL) t->Print();
        //SimpleThread(currentThread->getTid());
    }
    
}

void ThreadTest3_TS()
{
    DEBUG('t', "Entering ThreadTest3_TS");
    Thread *t1 = Thread::Create_thread("thread1");
    Thread *t2 = Thread::Create_thread("thread2");
    Thread *t3 = Thread::Create_thread("thread3");

    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 1);
    t3->Fork(SimpleThread, 1);
    

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
        ThreadTest1();
        break;
    case 2:
        ThreadTest2_limit();
        break;
    case 3:
        ThreadTest3_TS();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
