// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "machine.h"
#include "translate.h"
#include <stdio.h>
#include "openfile.h"

#define LRU

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    }

    else if ((which == SyscallException) && (type == SC_Exit)) {
        for (int i = 0; i < machine->pageTableSize; i++) {
            if (machine->pageTable[i].valid == TRUE) {
                int idx = machine->pageTable[i].physicalPage;
                machine->bitmap->clear(idx);
            }
        }

        //printf("TLB hits: %d\n", machine->TLBhit);
        //printf("TLB miss: %d\n", machine->TLBmiss);
        //float rate = (float)machine->TLBhit / (machine->TLBmiss + machine->TLBhit);
        //printf("TLB hit rate: %f\n", rate);
        //machine->end = 1;
        //currentThread->Finish();
        interrupt->Halt();
    }

    else if (which == PageFaultException) {
        int method = 0;
        if (machine->tlb == NULL) {
            OpenFile *openfile = fileSystem->Open("virtual_memory");
            if (openfile == NULL) ASSERT(false);
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned) virtAddr / PageSize;
            unsigned int offset = (unsigned) virtAddr % PageSize;
            int pos = machine->bitmap->find();

            if (pos == -1) {
                pos = 0;
                for (int j = 0; j < machine->pageTableSize; j++) {
                    if (machine->pageTable[j].physicalPage == 0) {
                        if (machine->pageTable[j].dirty == TRUE) {
                            openfile->WriteAt(&(machine->mainMemory[pos * PageSize]),
                                PageSize, machine->pageTable[j].virtualPage * PageSize);
                            machine->pageTable[j].valid = FALSE;
                            break;
                        }
                    }
                }
            }
            printf("PageFault vpn %d pos %d\n", vpn, pos);
            openfile->ReadAt(&(machine->mainMemory[pos * PageSize]), PageSize, vpn * PageSize);
            machine->pageTable[pos].valid = TRUE;
            machine->pageTable[pos].virtualPage = vpn;
            //machine->pageTable[vpn].physicalPage = pos;
            machine->pageTable[pos].use = FALSE;
            machine->pageTable[pos].dirty = FALSE;
            machine->pageTable[pos].readOnly = FALSE;
            delete openfile;
        }
        else {
            TranslationEntry *entry, *tlb;
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned) virtAddr / PageSize;
            unsigned int offset = (unsigned) virtAddr % PageSize;
            entry = &machine->pageTable[vpn];
            tlb = machine->tlb;
            bool flag = false;

            for (int i = 0; i < TLBSize; i++) {
                if (tlb[i].valid) {
                    tlb[i].cnt++;
                }
            }

            for (int i = 0; i < TLBSize; i++) {
                if (!tlb[i].valid) {
                    tlb[i].valid = true;
                    tlb[i].virtualPage = entry->virtualPage;
                    tlb[i].physicalPage = entry->physicalPage;
                    tlb[i].readOnly = entry->readOnly;
                    tlb[i].use = false;
                    tlb[i].dirty = false;
                    tlb[i].cnt = 0;

                    flag = true;
                    break;
                }
            }
            if (!flag) {
                int maxcnt = 0;
                int maxidx = -1;
                for (int i = 0; i < TLBSize; i++) {
                    if (tlb[i].cnt > maxcnt) {
                        maxcnt = tlb[i].cnt;
                        maxidx = i;
                    }
                }
                tlb[maxidx].virtualPage = entry->virtualPage;
                tlb[maxidx].physicalPage = entry->physicalPage;
                tlb[maxidx].readOnly = entry->readOnly;
                tlb[maxidx].use = false;
                tlb[maxidx].dirty = false;
                tlb[maxidx].cnt = 0;
            }
        }
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
