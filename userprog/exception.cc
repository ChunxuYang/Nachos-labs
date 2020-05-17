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
#include "filesys.h"
#include "addrspace.h"
extern Machine *machine;
extern FileSystem *fileSystem;
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

void IncrementPCRegs(void)
{
    // Debug usage
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));

    // Advance program counter
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

char *getFileNameFromAddress(int address)
{
    int position = 0;
    int data;
    char name[FILENAME_MAX + 1];
    do
    {
        // each time read one byte
        bool success = machine->ReadMem(address + position, 1, &data);
        //ASSERT_MSG(success, "Fail to read memory in Create syscall");
        ASSERT(success);
        name[position++] = (char)data;
        ASSERT(position <= FILENAME_MAX);
        //ASSERT_MSG(position <= FileNameMaxLength, "Filename length too long")
    } while (data != '\0');
    name[position] = '\0';
    //printf("==> AddressName: %s", name);
    return name;
}

void FileSystemHandler(int type)
{
    //printf("==> Type: %d\n", type);
    if (type == SC_Create)
    {
        int address = machine->ReadRegister(4);
        DEBUG('S', "Recieved Syscall Create (r4 = %d): ", address);
        char *name = getFileNameFromAddress(address);
        bool success = fileSystem->Create(name, 0);
        if (success)
        {
            DEBUG('S', "File \"%s\" created.\n", name);
        }
        else
        {
            DEBUG('S', "File \"%s\" creation Failed.\n", name);
        }
    }
    else if (type == SC_Open)
    {
        int address = machine->ReadRegister(4);
        DEBUG('S', "Recieved Syscall Open (r4 = %d): ", address);
        char *name = getFileNameFromAddress(address);
        OpenFile *openFile = fileSystem->Open(name);
        DEBUG('S', "File \"%s\" opened.\n", name);
        machine->WriteRegister(2, (OpenFileId)openFile);
    }
    else if (type == SC_Write)
    {
        int address = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        OpenFileId id = machine->ReadRegister(6);
        DEBUG('S', "Recieved Syscall Write (r4 = %d, r5 = %d, r6 = %d): ", address, size, id);
        char *buffer = new char[size];
        for (int i = 0; i < size; i++)
        {
            machine->ReadMem(address + i, 1, (int *)&buffer[i]);
        }
        OpenFile *openFile = (OpenFile *)id;
        int numBytes = openFile->Write(buffer, size);
        DEBUG('S', "Write %d bytes into file.\n", numBytes);
        machine->WriteRegister(2, numBytes);
    }
    else if (type == SC_Read)
    {
        //printf("====> GET HERE\n");
        int address = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        OpenFileId id = machine->ReadRegister(6);
        DEBUG('S', "Recieved Syscall Read (r4 = %d, r5 = %d, r6 = %d): ", address, size, id);
        char *buffer = new char[size];
        OpenFile *openFile = (OpenFile *)id;
        int numBytes = openFile->Read(buffer, size);
        for (int i = 0; i < numBytes; i++)
        {
            machine->WriteMem(address + i, 1, (int)&buffer[i]);
        }
        DEBUG('S', "Write %d bytes into file.\n", numBytes);
        machine->WriteRegister(2, numBytes);
    }
}

void __exec(int arg)
{
    char *filename = new char[256];
    
    filename = (char*) arg;
    //printf("____exec filename %s", filename);
    filename = "../test/test";
    OpenFile *ex2 = fileSystem->Open(filename);
    if(ex2 == NULL)
    {
        printf("----->Can't Open File\n");
    }
    AddrSpace *space2 = new AddrSpace(ex2);
    currentThread->space = space2;
    currentThread->filename = filename;
    space2->InitRegisters();
    space2->RestoreState();
    printf("Thread %s Start Running.\n", currentThread->getName());
    machine->Run();
    // currentThread->space->InitRegisters();
    // currentThread->space->RestoreState();
    // Machine *p = (Machine *)arg;
    // p->Run();
}
void __fork(int address)
{
    currentThread->space->RestoreState();
    machine->WriteRegister(PCReg, address);
    machine->WriteRegister(NextPCReg, address + 4);
    machine->Run();
}
void ThreadHandler(int type)
{
    if (type == SC_Exit)
    {
        //Thread::ts();
        int address = machine->ReadRegister(4);
        DEBUG('S', "Recieved Syscall [Exit] (r4 = %d): ", address);
        printf("Program Exit with Status %d\n", address);
        // for (int i = 0; i < machine->pageTableSize; i++)
        // {
        //     if (machine->pageTable[i].valid == TRUE)
        //     {
        //         int idx = machine->pageTable[i].physicalPage;
        //         machine->bitmap->clear(idx);
        //     }
        // }
        // if (currentThread->getName() == "main")
        // {
        //     IncrementPCRegs();
        //     currentThread->Finish();
        // }
        // else
        // {
            if (currentThread->fatherThread != NULL)
            {
                for (int i = 0; i < 128; i++)
                {
                    if (currentThread->fatherThread->childThreads[i] == currentThread)
                    {
                        currentThread->fatherThread->childThreads[i] = NULL;
                        currentThread->Finish();
                    }
                }
            }
        // }
        IncrementPCRegs();
         currentThread->Finish();
    }
    else if (type == SC_Exec)
    {
        int address = machine->ReadRegister(4);
        //printf("-->%d\n", address);
        DEBUG('S', "Recieved Syscall [Exec] (r4 = %d): ", address);
        int value;
        int count = 0;
        do
        {
            machine->ReadMem(address++, 1, &value);
            count++;
        } while (value != 0);
        address = address - count;
        char filename[count];
        for (int i = 0; i < count; i++)
        {
            machine->ReadMem(address + i, 1, &value);
            filename[i] = char(value);
        }
        filename[count] = '\0';
        //printf("\n==>filename: %s\n", filename);

        //char* filename = getFileNameFromAddress(address);
        Thread *newThread = Thread::createThread("exec");

        bool found = FALSE;
        for (int i = 0; i < 128; i++)
        {
            if (currentThread->childThreads[i] == NULL)
            {
                currentThread->childThreads[i] = newThread;
                machine->WriteRegister(2, (int)newThread);
                found = TRUE;
                break;
            }
        }
        if (!found)
        {
            printf("Current Thread Full\n");
            IncrementPCRegs();
            return;
        }
        newThread->fatherThread = currentThread;
        //printf("==> exec filename: %s\n", filename);
        newThread->Fork(__exec, (int)filename);
        IncrementPCRegs();
    }
    else if (type == SC_Join)
    {
        int address = machine->ReadRegister(4);
        DEBUG('S', "Recieved Syscall [JOIN] (r4 = %d): ", address);
        Thread *joinThread = (Thread *)address;

        // printf("\nthread: %s.\n", joinThread->getName());
        // printf("current thread: %s.\n", currentThread->getName());
        bool found = FALSE;
        int joinNum;

        for (int i = 0; i < 128; i++)
        {

            if (!currentThread->childThreads[i])
            {
                printf("NULL");
            }
            else
            {
                printf(currentThread->childThreads[i]->getName());
            }
            //printf("\nGET HERE\n");
            //printf("%s\n", currentThread->childThreads[i]);
            if (currentThread->childThreads[i] == joinThread)
            {
                joinNum = i;
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            printf("Cannot find Thread.\n");
            return;
        }
        while (currentThread->childThreads[joinNum] != NULL)
        {
            printf("Thread %s Waiting for thread %s ends.\n", currentThread->getName(), currentThread->childThreads[joinNum]->getName());
            Thread::ts();
            currentThread->Yield();
        }
        printf("Thread %s Join Success!\n", currentThread->getName());
        IncrementPCRegs();
    }
    else if (type == SC_Yield)
    {
        DEBUG('S', "Recieved Syscall [YIELD]: %s Yield\n", currentThread->getName());
        IncrementPCRegs();
        currentThread->Yield();
    }
    else if (type == SC_Fork)
    {
        DEBUG('S', "Recieved Syscall [FORK]");
    }
}

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
        }
        else if (type == SC_Create || type == SC_Open || type == SC_Write || type == SC_Read)
        {
            FileSystemHandler(type);
            IncrementPCRegs();
        }
        else if (type == SC_Exec || type == SC_Fork || type == SC_Yield || type == SC_Join || type == SC_Exit)
        {
            ThreadHandler(type);
        }
    }

    else if (which == PageFaultException)
    {
        int method = 0;
        if (machine->tlb == NULL)
        {
            OpenFile *openfile = fileSystem->Open("virtual_memory");
            if (openfile == NULL)
                ASSERT(false);
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned)virtAddr / PageSize;
            unsigned int offset = (unsigned)virtAddr % PageSize;
            int pos = machine->bitmap->find();

            if (pos == -1)
            {
                pos = 0;
                for (int j = 0; j < machine->pageTableSize; j++)
                {
                    if (machine->pageTable[j].physicalPage == 0)
                    {
                        if (machine->pageTable[j].dirty == TRUE)
                        {
                            openfile->WriteAt(&(machine->mainMemory[pos * PageSize]),
                                              PageSize, machine->pageTable[j].virtualPage * PageSize);
                            machine->pageTable[j].valid = FALSE;
                            break;
                        }
                    }
                }
            }
            DEBUG('M', "PageFault vpn %d pos %d\n", vpn, pos);
            openfile->ReadAt(&(machine->mainMemory[pos * PageSize]), PageSize, vpn * PageSize);
            machine->pageTable[pos].valid = TRUE;
            machine->pageTable[pos].virtualPage = vpn;
            //machine->pageTable[vpn].physicalPage = pos;
            machine->pageTable[pos].use = FALSE;
            machine->pageTable[pos].dirty = FALSE;
            machine->pageTable[pos].readOnly = FALSE;
            delete openfile;
        }
        else
        {
            TranslationEntry *entry, *tlb;
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned)virtAddr / PageSize;
            unsigned int offset = (unsigned)virtAddr % PageSize;
            entry = &machine->pageTable[vpn];
            tlb = machine->tlb;
            bool flag = false;

            for (int i = 0; i < TLBSize; i++)
            {
                if (tlb[i].valid)
                {
                    tlb[i].cnt++;
                }
            }

            for (int i = 0; i < TLBSize; i++)
            {
                if (!tlb[i].valid)
                {
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
            if (!flag)
            {
                int maxcnt = 0;
                int maxidx = -1;
                for (int i = 0; i < TLBSize; i++)
                {
                    if (tlb[i].cnt > maxcnt)
                    {
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
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
