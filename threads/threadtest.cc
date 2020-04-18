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
#include <string.h>
#include <stdio.h>
#include "synch.h"

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

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

void
BarrierTest(int which) {
    Barrier(5);
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = Thread::createThread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

/*
const int buffer_size = 4;
Semaphore* empty = new Semaphore("empty", buffer_size);
Semaphore* full = new Semaphore("full", 0);
Condition* p_cond = new Condition("producer_condition");
Condition* c_cond = new Condition("consumer_condition");
Lock* mutex = new Lock("mutex");
int buffer = 0;


void Producer(int id) {
    int cnt = 0;
    while(cnt < 20) {
        empty->P();
        mutex->Acquire();
        buffer++;
        printf("$$$ I am producer %d. Buffer: %d\n", id, buffer);
        mutex->Release();
        full->V();
        cnt++;
    }
}

void Consumer(int id) {
    int cnt = 0;
    while(cnt < 10) {
        full->P();
        mutex->Acquire();
        buffer--;
        printf("### I am consumer %d. Buffer: %d\n", id, buffer);
        mutex->Release();
        empty->V();
        cnt++;
    }
}


void Producer(int id) {
    int cnt = 0;
    while(cnt < 20) {
        mutex->Acquire();
        while (buffer >= buffer_size) {
            p_cond->Wait(mutex);
        }
        buffer++;
        printf("$$$ I am producer %d. Buffer: %d\n", id, buffer);

        c_cond->Signal(mutex);
        mutex->Release();
        cnt++;
    }
}

void Consumer(int id) {
    int cnt = 0; 
    while(cnt < 10) {
        mutex->Acquire();
        while(buffer <= 0) {
            c_cond->Wait(mutex);
        }
        buffer--;
        printf("### I am consumer %d. Buffer: %d\n", id, buffer);

        p_cond->Signal(mutex);
        mutex->Release();
        cnt++;
    }
}

void
ThreadTest2() {
    DEBUG('t', "Entering ThreadTest2");

    Thread* p1 = Thread::createThread("Producer1");
    //Thread* p2 = Thread::createThread("Producer2");
    p1->Fork(Producer, 1);
    //p2->Fork(Producer, 2);
    Thread* c1 = Thread::createThread("Consumer1");
    Thread* c2 = Thread::createThread("Consumer2");
    c1->Fork(Consumer, 1);
    c2->Fork(Consumer, 2);

    currentThread->Yield();
}

void
ThreadTest3() {
    DEBUG('t', "Entering ThreadTest3");
    Thread *t[5];
    for (int i = 0; i < 5; i++) {
        t[i] = Thread::createThread("testBarrier");
        t[i]->Fork(BarrierTest, i);
    }
    currentThread->Yield();
}

RWLock *rwlock = new RWLock("RW lock");

void Reader(int id) {
    for (int i = 0; i < 5; i++) {
        rwlock->rP();
        if (buffer == 0) {
            printf("Reader %d, no data to read\n", id);
        }
        else {
            printf("Reader %d, read successfully!\n", id);
        }
        rwlock->rV();
        currentThread->Yield();
    }
}

void Writer(int id) {
    for (int i = 0; i < 5; i++) {
        rwlock->wP();
        buffer++;
        printf("Writer %d, write successfully!\n", id);
        rwlock->wV();
        currentThread->Yield();
    }
}


void
ThreadTest4() {
    DEBUG('t', "Entering ThreadTest4");
    Thread* r1 = Thread::createThread("Reader1");
    Thread* w1 = Thread::createThread("Writer1");
    Thread* r2 = Thread::createThread("Reader2");
    //Thread* w2 = Thread::createThread("Writer2");
    r1->Fork(Reader, 1);
    w1->Fork(Writer, 1);
    r2->Fork(Reader, 2);
    //w2->Fork(Writer, 2);
    currentThread->Yield();
}
*/
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	//ThreadTest4();
    //Thread::ts();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}
