// fstest.cc
//	Simple test routines for the file system.
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"
#include "console.h"
#include "pipe.h"

#define TransferSize 10 // make it small, just to be difficult
extern FileSystem *fileSystem;
//extern Pipe *pipe;// = new Pipe();
//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile *openFile;
    int amountRead, fileLength;
    char *buffer;

    // Open UNIX file
    if ((fp = fopen(from, "r")) == NULL)
    {
        printf("Copy: couldn't open input file %s\n", from);
        return;
    }

    // Figure out length of UNIX file
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

    // Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength))
    { // Create Nachos file
        printf("Copy: couldn't create output file %s\n", to);
        fclose(fp);
        return;
    }

    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);

    // Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
    {

        openFile->Write(buffer, amountRead);
    }
    delete[] buffer;

    // Close the UNIX and the Nachos files
    delete openFile;

    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void Print(char *name)
{
    OpenFile *openFile;
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL)
    {
        printf("Print: unable to open file %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
        for (i = 0; i < amountRead; i++)
            printf("%c", buffer[i]);
    delete[] buffer;

    delete openFile; // close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName "TestFile"
#define Contents "1234567890"
#define ContentSize strlen(Contents)
#define FileSize ((int)(ContentSize))

static void
FileWrite()
{
    OpenFile *openFile;
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n",
           FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0))
    {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10)
        {
            printf("Perf test: unable to write %s\n", FileName);
            delete openFile;
            return;
        }
    }
    delete openFile; // close file
}

static void
FileRead()
{

    OpenFile *openFile;
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n",
           FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL)
    {
        printf("Perf test: unable to open file %s\n", FileName);
        delete[] buffer;
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Read(buffer, ContentSize);
        if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize))
        {
            printf("Perf test: unable to read %s\n", FileName);
            delete openFile;
            delete[] buffer;
            return;
        }
    }
    delete[] buffer;
    delete openFile; // close file
}

void MyReaderThread(int loopnum)
{
    OpenFile *openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("%s: Can't open file %s.\n", currentThread->getName(), FileName);
    }
    char *buffer = new char[ContentSize];
    for (int i = 1; i <= loopnum; i++)
    {

        openFile->Read(buffer, 4);
        printf("==> Thread %s Read %s.\n", currentThread->getName(), buffer);
        currentThread->Yield();
    }
    delete buffer;
    delete openFile;
}

void MyWriterThread(int loopnum)
{
    OpenFile *openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("%s: Can't open file %s.\n", currentThread->getName(), FileName);
    }
    char *buffer = new char[ContentSize];
    for (int i = 1; i <= loopnum; i++)
    {
        //printf("==> Thread %s begin writing.\n", currentThread->getName());
        openFile->Write(Contents, ContentSize);
        printf("==> Thread %s Write %s.\n", currentThread->getName(), Contents);
        currentThread->Yield();
    }
    fileSystem->Remove(FileName);
    delete buffer;
    delete openFile;
}

void PerformanceTest()
{
    printf("Starting file system performance test:\n");
    Thread *reader = Thread::createThread("reader");
    //Thread *writer = Thread::createThread("writer");
    if (!fileSystem->Create(FileName, 3000))
    {
        printf("Create fail!\n");
    }
    OpenFile *openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("Open fail!\n");
    }
    else
    {
        printf("Creating file success.\n");
    }
    printf("Write %d\n", openFile->Write(Contents, ContentSize));
    //writer->Fork(MyWriterThread, 10);
    reader->Fork(MyReaderThread, 10);

    MyWriterThread(5);

    currentThread->Yield();

    stats->Print();
}

void MakeDir(char *dirname)
{
    printf("Making Dir %s.\n", dirname);
    fileSystem->Create(dirname, -1);
}

static SynchConsole *synchConsole;

void SynchConsoleTest(char *in, char *out)
{
    char ch;
    synchConsole = new SynchConsole(in, out);
    while (TRUE)
    {
        ch = synchConsole->GetChar();
        synchConsole->PutChar(ch);
        if (ch == 'q')
        {
            printf("\n");
            return;
        }
    }
}

Pipe *pipe;
void pipereader(int dummy)
{
    int cnt = 255;
    char buf[32];
    int n = pipe->Read(buf, 32);
    printf("n = %d\n", n);
    printf("%s", buf);
    currentThread->Yield();
    putchar('\n');
}
void pipewriter(int dummy)
{
    int cnt = 255;
    char buf[20];
    scanf("%s", buf);
    int n = pipe->Write(buf, 10);
    currentThread->Yield();
}
void PipeTest()
{
    pipe = new Pipe();
    Thread *r = Thread::createThread("pipereader");
    Thread *w = Thread::createThread("pipewriter");
    r->Fork(pipewriter, 0);
    w->Fork(pipereader, 0);
}
