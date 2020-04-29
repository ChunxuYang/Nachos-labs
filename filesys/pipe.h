#ifndef PIPE_H
#define PIPE_H

#include "filesys.h"
#include "openfile.h"
#include "synch.h"
#include "disk.h"


class Pipe
{
private:
    char* name;             // Name of pipe
    static int pipeNumber;
    OpenFile* pipeFile;
    int readIndex;
    int writeIndex;
    int bytesToSave;
    Lock *lock;

public:
    Pipe(int size = SectorSize);
    ~Pipe();
    int Read(char* buf, int readlength);
    int Write(char* buf, int writelength);
};

#endif // PIPE_H