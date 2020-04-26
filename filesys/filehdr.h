// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"
#include <time.h>
#define NumOfIntHeaderInfo 2
#define NumOfTimeHeaderInfo 3

#define FILE_TYPE_LENGTH 5
#define FILE_TIME_LENGTH 26
#define FILE_ALL_LENGTH FILE_TYPE_LENGTH + FILE_TIME_LENGTH *NumOfTimeHeaderInfo

#define NumDataSectors ((SectorSize - (NumOfIntHeaderInfo * sizeof(int) + FILE_ALL_LENGTH * sizeof(char))) / sizeof(int))
#define NumDirect (NumDataSectors-1)
#define MaxFileSize ((NumDirect * SectorSize) + ((SectorSize / sizeof(int))*SectorSize))

#define IndirectSectorIdx (NumDirect)

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader
{
public:
  bool Allocate(BitMap *bitMap, int fileSize); // Initialize a file header,
                                               //  including allocating space
                                               //  on disk for the file data
  void Deallocate(BitMap *bitMap);             // De-allocate this file's
                                               //  data blocks

  void FetchFrom(int sectorNumber); // Initialize file header from disk
  void WriteBack(int sectorNumber); // Write modifications to file header
                                    //  back to disk

  int ByteToSector(int offset); // Convert a byte offset into the file
                                // to the disk sector containing
                                // the byte

  int FileLength(); // Return the length of the file
                    // in bytes

  void Print(); // Print the contents of the file.

  void setFileType(char *ext) { strcmp(ext, "") ? strcpy(fileType, ext) : strcpy(fileType, "None"); }
  void setCreateTime(char *t)
  {
    strcpy(createdTime, t);
    createdTime[24] = '\0';
  }
  void setModifyTime(char *t)
  {
    strcpy(modifiedTime, t);
    modifiedTime[24] = '\0';
  }
  void setVisitTime(char *t)
  {
    strcpy(lastVisitedTime, t);
    lastVisitedTime[24] = '\0';
  }
  void setHeader(char *ext);

  void setHeaderSector(int sector) { headerSector = sector; }
  int getHeaderSector() { return headerSector; }

private:
  int numBytes;   // Number of bytes in the file
  int numSectors; // Number of data sectors in the file

  char fileType[FILE_TYPE_LENGTH];
  char createdTime[FILE_TIME_LENGTH];
  char modifiedTime[FILE_TIME_LENGTH];
  char lastVisitedTime[FILE_TIME_LENGTH];

  int dataSectors[NumDataSectors]; // Disk sector numbers for each data
                              // block in the file
  int headerSector;
};
char *getFileType(char *filename);

char *getCurrentTime();
#endif // FILEHDR_H
