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
#include "openfile.h"

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
extern Machine *machine;
extern FileSystem *fileSystem;
int TLB_FIFO();
int TLB_Clock();
int TLB_LRU();
int TLB_RAND();
void PrintTLB();
void PrintTLBStatus();
int PageReplacement(int vpn);
enum TLB_ALGO
{
    FIFO,
    LRU,
    RAND
};
enum TLB_ALGO tlb_algo = RAND;
TranslationEntry PageFaultHandler(int vpn)
{
    int pageFrame = machine->allocateFrame();
    if (pageFrame == -1)
    {
        pageFrame = PageReplacement(vpn);
    }
    machine->pageTable[vpn].physicalPage = pageFrame;
    
    DEBUG('a', "Loading page from Virtual Memory\n");
    OpenFile *vm = fileSystem->Open("VirtualMemory");
    if(vm == NULL) printf("vm NULL\n");
    ASSERT(vm != NULL);
    
    vm->ReadAt(&(machine->mainMemory[pageFrame * PageSize]), PageSize, vpn * PageSize);
    
    delete vm; // close the file

    // Set the page attributes
    machine->pageTable[vpn].valid = TRUE;
    machine->pageTable[vpn].use = FALSE;
    machine->pageTable[vpn].dirty = FALSE;
    machine->pageTable[vpn].readOnly = FALSE;

    //currentThread->space->PrintState(); // debug with -d M to show bitmap
}
void TLBHandler(int addr)
{
    my_miss_count++;
    int vpn = (unsigned)addr / PageSize;
    TranslationEntry page = machine->pageTable[vpn];
    if (!page.valid)
    {
        DEBUG('S', "==> Page Miss\n");
        page = PageFaultHandler(vpn);
    }
    int pos = -1;
    for (int i = 0; i < TLBSize; i++)
    {
        if (machine->tlb[i].valid == FALSE)
        {
            pos = i;
            break;
        }
    }
    if (pos == -1)
    {
        switch (tlb_algo)
        {
        case FIFO:
            pos = TLB_FIFO();
            break;
        case LRU:
            pos = TLB_LRU();
            break;
        case RAND:
            pos = TLB_RAND();
            break;
        default:
            break;
        }
    }
    machine->tlb[pos].valid = TRUE;
    machine->tlb[pos].virtualPage = vpn;
    machine->tlb[pos].physicalPage = machine->pageTable[vpn].physicalPage;
    machine->tlb[pos].use = FALSE;
    machine->tlb[pos].dirty = FALSE;
    machine->tlb[pos].readOnly = FALSE;
}

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == PageFaultException)
    {
        if (machine->tlb == NULL)
        {
            DEBUG('S', "=> Page table fault.\n");
        }
        else
        {
            DEBUG('S', "=> TLB Miss.\n");
            int missAddress = machine->ReadRegister(BadVAddrReg);
            TLBHandler(missAddress);
        }
        return;
    }

    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            DEBUG('a', "Shutdown, initiated by user program.\n");
            PrintTLBStatus();
            interrupt->Halt();
        }
        else if (type == SC_Exit || type == SC_Exec || type == SC_Join)
        {
            AddressSpaceControlHandler(type);
        }
        else if (type == SC_Create || type == SC_Open || type == SC_Write || type == SC_Read || type == SC_Close)
        {
            // File System System Calls
            FileSystemHandler(type);
        }
        else if (type == SC_Fork || type == SC_Yield)
        {
            // User-level Threads System Calls
            UserLevelThreadsHandler(type);
        }
        IncrementPCRegs();
        return;
    }

    printf("Unexpected user mode exception %d %d\n", which, type);
    ASSERT(FALSE);
}

int TLB_FIFO()
{
    int index = TLBSize - 1;
    for (int i = 0; i < TLBSize - 1; i++)
    {
        machine->tlb[i] = machine->tlb[i + 1];
    }
    return index;
}
int TLB_LRU()
{
    int min = 65535;
    int index = -1;
    for (int i = 0; i < TLBSize; i++)
    {
        if (machine->tlb[i].lastUsedTime < min)
        {
            min = machine->tlb[i].lastUsedTime;
            index = i;
        }
    }
    return index;
}
int TLB_RAND()
{
    int index = ((13 + stats->totalTicks) * 23) % TLBSize;
    return index;
}
void PrintTLBStatus()
{
#ifdef USE_TLB
    printf("==> Total reference: %d\n", my_ref_count);
    printf("==> Hits: %d\n", my_ref_count - my_miss_count);
    printf("==> Miss: %d\n", my_miss_count);
    printf("==> Hit Rate: %.3lf%%\n", ((double)(my_ref_count - my_miss_count) * 100) / (my_ref_count));
#endif
}
void PrintTLB()
{
    for (int i = 0; i < TLBSize; i++)
    {
        printf("(%d, %d, %d, %d)\n", i, machine->tlb[i].valid, machine->tlb[i].use, machine->tlb[i].lastUsedTime);
    }
    printf("_______\n");
}

void AddressSpaceControlHandler(int type)
{
    if (type = SC_Exit)
    {
        PrintTLBStatus();
        int status = machine->ReadRegister(4);
        currentThread->setExitStatus(status);
        if (status == 0)
        {
            DEBUG('S', "User program exit normally.\n");
        }
        else
        {
            DEBUG('S', "User program exit with status %d.\n", status);
        }
        if (currentThread->space != NULL)
        {
            machine->freeMem();

            delete currentThread->space;
            currentThread->space = NULL;
        }
        currentThread->Finish();
    }
}
void FileSystemHandler(int type) {}       // Create, Open, Write, Read, Close
void UserLevelThreadsHandler(int type) {} // Fork, Yield

void IncrementPCRegs()
{
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}
int
PageReplacement(int vpn)
{
    int pageFrame = -1;
    for (int temp_vpn = 0; temp_vpn < machine->pageTableSize, temp_vpn != vpn; temp_vpn++) {
        if (machine->pageTable[temp_vpn].valid) {
            if (!machine->pageTable[temp_vpn].dirty) {
                pageFrame = machine->pageTable[temp_vpn].physicalPage;
                break;
            }
        }
    }
    if (pageFrame == -1) { // No non-dirty entry
        for (int replaced_vpn = 0; replaced_vpn < machine->pageTableSize, replaced_vpn != vpn; replaced_vpn++) {
            if (machine->pageTable[replaced_vpn].valid) {
                machine->pageTable[replaced_vpn].valid = FALSE;
                pageFrame = machine->pageTable[replaced_vpn].physicalPage;

                // Store the page back to disk
                OpenFile *vm = fileSystem->Open("VirtualMemory");
                //ASSERT_MSG(vm != NULL, "fail to open virtual memory");
                vm->WriteAt(&(machine->mainMemory[pageFrame*PageSize]), PageSize, replaced_vpn*PageSize);
                delete vm; // close file
                break;
            }
        }
    }
    return pageFrame;
}
// int PageReplacement(int vpn)
// {
//     int pageFrame = -1;
//     for (int i = 0; i < machine->pageTableSize, i != vpn; i++)
//     {
//         if (machine->pageTable[i].valid)
//         {
//             if (!machine->pageTable[i].dirty)
//             {
//                 pageFrame = machine->pageTable[i].physicalPage;
//                 break;
//             }
//         }
//     }
//     if(pageFrame == -1)
//     {
//         for (int i = 0; i < machine->pageTableSize, i != vpn; i++)
//         {
//             if (machine->pageTable[i].valid)
//             {
//                 machine->pageTable[i].valid = FALSE;
//                 pageFrame = machine->pageTable[i].physicalPage;

//                 OpenFile *vm = fileSystem->Open("VirtualMemory");
//                 if(vm == NULL) printf("vm NULL\n");
//                 ASSERT(vm != NULL);
//                 vm->WriteAt(&(machine->mainMemory[pageFrame*PageSize]), PageSize, i*PageSize);
//                 delete vm; // close file
//                 break;
//             }
            
//         }
        
//     }
//     printf(">>>>> Page Frame %d\n", pageFrame);
//     return pageFrame;
    
// }