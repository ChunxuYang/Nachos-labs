// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include "synchdisk.h"

#define SectorCount (SectorSize / sizeof(int))
extern SynchDisk *synchDisk;
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
        return FALSE; // not enough space

    if (numSectors <= NumDirect)
    {
        DEBUG('f', "Allocating using direct indexing only\n");
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else
    {
        if (numSectors <= (NumDirect + SectorCount))
        {
            DEBUG('f', "Allocating using single indirect indexing\n");
            // direct
            for (int i = 0; i <= NumDirect; i++)
                dataSectors[i] = freeMap->Find();
            // indirect
            //dataSectors[IndirectSectorIdx] = freeMap->Find();
            int indirectIndex[SectorCount];
            for (int i = 0; i < numSectors - IndirectSectorIdx; i++)
            {
                indirectIndex[i] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[IndirectSectorIdx], (char *)indirectIndex);
        }
        else
        {
            ASSERT("FALSE");
        }
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{
    if (numSectors <= NumDirect)
    {
        for (int i = 0; i < numSectors; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i]));
            freeMap->Clear((int)dataSectors[i]);
        }
    }
    else
    {
        int indirect_idx[SectorSize];
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)indirect_idx);
        for (int i = 0; i < numSectors - IndirectSectorIdx; i++)
        {
            freeMap->Clear(indirect_idx[i]);
        }
        for (int i = 0; i <= NumDirect; i++)
        {
            freeMap->Clear(dataSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{

    const int directMapSize = NumDirect * SectorSize;
    if (offset < directMapSize)
    {
        return (dataSectors[offset / SectorSize]);
    }
    else
    {
        const int sectorNum = (offset - directMapSize) / SectorSize;
        int singleIndirectIndex[SectorCount];
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)singleIndirectIndex);
        return singleIndirectIndex[sectorNum];
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------
char *
printChar(char oriChar)
{
    if ('\040' <= oriChar && oriChar <= '\176') // isprint(oriChar)
        printf("%c", oriChar);                  // Character content
    else
        printf("\\%x", (unsigned char)oriChar); // Unreadable binary content
}
void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("\n");
    printf(" File type:\t%s\n", fileType);
    printf(" Created:\t%s\n", createdTime);
    printf(" Last visited:\t%s\n", lastVisitedTime);
    printf(" Modified:\t%s\n", modifiedTime);

    printf(" File size:\t%d\n File blocks:\n\t", numBytes);
    if (numSectors <= NumDirect)
    {
        for (i = 0; i < numSectors; i++)
        {
            printf("%d ", dataSectors[i]);
        }
    }
    else
    {
        for(i = 0; i < IndirectSectorIdx; i++)
        {
            printf("%d ", dataSectors[i]);
        }
                printf("\nUsing indirect index: %d\n", dataSectors[IndirectSectorIdx]);

        int indirect_index[SectorCount];
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)indirect_index);
        for (i = 0; i < numSectors - IndirectSectorIdx; i++)
        {
            printf("%d ", indirect_index[i]);
        }
        
    }
    printf("\n");
    printf("File contents: \n");
    if(numSectors <= NumDirect)
    {
        for(i = k = 0; i < numSectors; i++)
        {
            synchDisk->ReadSector(dataSectors[i], data);
            for ( j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            {
                printChar(data[j]);
            }
            printf("\n");
        }
    }
    else
    {
        for(i = k = 0; i < IndirectSectorIdx; i++)
        {
            synchDisk->ReadSector(dataSectors[i], data);
            for ( j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            {
                printChar(data[j]);
            }
            printf("\n");
        }
        int indirect_index[SectorCount];
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)indirect_index);
        for(i = 0; i < numSectors - IndirectSectorIdx; i++)
        {
            printf("In sector %d\n", indirect_index[i]);
            synchDisk->ReadSector(indirect_index[i], data);
            for(j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            {
                printChar(data[j]);
            }
            printf("\n");
        }
    }
    printf("\n");
    printf("----------------------------------------------\n");
    delete[] data;
}

char *getFileType(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

char *getCurrentTime()
{
    time_t timepre;
    time(&timepre);
    return asctime(localtime(&timepre));
}

void FileHeader::setHeader(char *ext)
{
    setFileType(ext);

    char *currentTime = getCurrentTime();
    setCreateTime(currentTime);
    setModifyTime(currentTime);
    setVisitTime(currentTime);
}

bool FileHeader::ExpandSize(BitMap *freemap, int additionalBytes)
{
    ASSERT(additionalBytes > 0);
    numBytes += additionalBytes;
    int initSector = numSectors;
    numSectors = divRoundUp(numBytes, SectorSize);
    if(initSector == numSectors)
    {
        return TRUE;
    }
    int sectorsAdded = numSectors - initSector;
    if(freemap->NumClear() < sectorsAdded)
    {
        return FALSE;
    }
    DEBUG('f', "Expanding file size for %d sectors (%d bytes)\n", sectorsAdded, additionalBytes);
    if(numSectors < NumDirect)
    {
        for(int i = initSector; i < numSectors; i++)
        {
            dataSectors[i] = freemap->Find();
        }
    }
    else
    {
        ASSERT(FALSE);
    }
    return TRUE;
}