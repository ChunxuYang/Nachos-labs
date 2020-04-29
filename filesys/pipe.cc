#include "pipe.h"
#include "stdio.h"
#include "utility.h"
#include "synch.h"
#include "system.h"

extern FileSystem *fileSystem;
int Pipe::pipeNumber = 0;

Pipe::Pipe(int size)
{
    size++;
    name = new char[SectorSize];
    snprintf(name, 127, "__pipe_file_%d_%d__", size, pipeNumber++);
    if (!fileSystem->Create(name, size))
    {
        printf("Pipe file %s creation failure.\n", name);
        //ASSERT(FALSE);
    }
    pipeFile = fileSystem->Open(name);
    if (pipeFile == NULL)
    {
        printf("Pipe file %s failed to open.\n", name);
        ASSERT(FALSE);
    }
    readIndex = 0;
    writeIndex = 0;
    bytesToSave = size;
    lock = new Lock("Pipe Lock");
}

Pipe::~Pipe()
{
    delete pipeFile;
    fileSystem->Remove(name);
    delete[] name;
    delete lock;
    pipeNumber--;
}

int Pipe::Read(char *buf, int readlength)
{
    if (readIndex == writeIndex)
    {
        return 0;
    }
    lock->Acquire();
    if(readIndex > writeIndex) writeIndex += bytesToSave;
    int indexAfterRead = readIndex + readlength;
    if (indexAfterRead > writeIndex)
        indexAfterRead = writeIndex;
    int total = pipeFile->ReadAt(buf, (indexAfterRead > bytesToSave ? bytesToSave - readIndex : indexAfterRead - readIndex), readIndex);
    if (indexAfterRead > bytesToSave)
    {
        total += pipeFile->ReadAt(buf + total, indexAfterRead - bytesToSave, 0);
    }
    readIndex += total;
    readIndex %= bytesToSave;
    writeIndex %= bytesToSave;
    lock->Release();
    return total;
}

int Pipe::Write(char *buf, int writeLength)
{
    if ((writeIndex + 1) % bytesToSave == readIndex)
    {
        return 0;
    }
    lock->Acquire();
    //int indexAfterRead = bytesToRead + readlength;
    if (readIndex <= writeIndex)
        readIndex += bytesToSave;
    int indexAfterWrite = writeIndex + writeLength;
    if(indexAfterWrite > readIndex-1) indexAfterWrite = readIndex-1;
    int total = pipeFile->WriteAt(buf, (indexAfterWrite > bytesToSave ? bytesToSave - writeIndex : indexAfterWrite - writeIndex), writeIndex);
    if (indexAfterWrite > bytesToSave)
    {
        total += pipeFile->WriteAt(buf + total, indexAfterWrite - bytesToSave, 0);
    }
    writeIndex += total;
    readIndex %= bytesToSave;
    writeIndex %= bytesToSave;
    lock->Release();
    return total;
}