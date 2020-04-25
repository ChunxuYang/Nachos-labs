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

    if (numSectors < NumDirect)
    {
        DEBUG('f', "Allocating using direct indexing only\n");
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else
    {
        if (numSectors < (NumDirect + SectorCount))
        {
            DEBUG('f', "Allocating using single indirect indexing\n");
            // direct
            for (int i = 0; i < NumDirect; i++)
                dataSectors[i] = freeMap->Find();
            // indirect
            dataSectors[IndirectSectorIdx] = freeMap->Find();
            int indirectIndex[SectorCount];
            for (int i = 0; i < numSectors - NumDirect; i++)
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

    int i, ii; // For direct / single indirect
    DEBUG('f', "Deallocating direct indexing table\n");
    for (i = 0; (i < numSectors) && (i < NumDirect); i++)
    {
        ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }
    if (numSectors > NumDirect)
    {
        DEBUG('f', "Deallocating single indirect indexing table\n");
        int singleIndirectIndex[SectorCount]; // used to restore the indexing map
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < SectorCount); i++, ii++)
        {
            ASSERT(freeMap->Test((int)singleIndirectIndex[ii])); // ought to be marked!
            freeMap->Clear((int)singleIndirectIndex[ii]);
        }
        // Free the sector of the single indirect indexing table
        ASSERT(freeMap->Test((int)dataSectors[IndirectSectorIdx]));
        freeMap->Clear((int)dataSectors[IndirectSectorIdx]);
    }

    // if (numSectors < NumDirect)
    // {
    //     for (int i = 0; i < numSectors; i++)
    //     {
    //         ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
    //         freeMap->Clear((int)dataSectors[i]);
    //     }
    // }
    // else
    // {
    //     char *indirect_index = new char[SectorSize];
    //     synchDisk->ReadSector(dataSectors[IndirectSectorIdx], indirect_index);
    //     for (int i = 0; i < numSectors - IndirectSectorIdx; i++)
    //     {
    //         ASSERT(freeMap->Test((int)dataSectors[i * 4])); // ought to be marked!
    //         freeMap->Clear((int)dataSectors[i * 4]);
    //     }
    //     for (int i = 0; i < NumDirect; i++)
    //     {
    //         ASSERT(freeMap->Test((int)dataSectors[i]));
    //         freeMap->Clear((int)dataSectors[i]);
    //     }
    // }
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
    const int singleIndirectMapSize = directMapSize + SectorCount * SectorSize;
    //const int doubleIndirectMapSize = singleIndirectMapSize +  SectorCount * SectorCount * SectorSize;
    //printf("offset: %d, directMapSize: %d, singleIndirectMapSize: %d\n",offset, directMapSize, singleIndirectMapSize);
    if (offset < directMapSize)
    {
        //printf("offset: %d, xxx %d\n", offset, dataSectors[offset / SectorSize]);
        return (dataSectors[offset / SectorSize]);
    }
    else if (offset < singleIndirectMapSize)
    {
        const int sectorNum = (offset - directMapSize) / SectorSize;
        int singleIndirectIndex[SectorCount]; // used to restore the indexing map

        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)singleIndirectIndex);
        printf("offset: %d, xxx %d\n", offset, singleIndirectIndex[sectorNum]);
        return singleIndirectIndex[sectorNum];
    }
    else
    {
        ASSERT(FALSE);
    }
    // const int directMapSize = NumDirect * SectorSize;
    // if (offset < directMapSize)
    // {
    //     return dataSectors[offset / SectorSize];
    // }
    // else
    // {
    //     const int sectorNum = (offset - directMapSize) / SectorSize;
    //     char *singleIndirectIndex = new char[SectorSize]; // used to restore the indexing map
    //     synchDisk->ReadSector(dataSectors[IndirectSectorIdx], singleIndirectIndex);
    //     return int(singleIndirectIndex[sectorNum * 4]);
    // }
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

    int ii;                               // For single / double indirect indexing
    int singleIndirectIndex[SectorCount]; // used to restore the indexing map
    int doubleIndirectIndex[SectorCount]; // used to restore the indexing map
    printf("  Direct indexing:\n    ");
    for (i = 0; (i < numSectors) && (i < NumDirect); i++)
        printf("%d ", dataSectors[i]);
    if (numSectors > NumDirect)
    {
        printf("\n  Indirect indexing: (mapping table sector: %d)\n    ", dataSectors[IndirectSectorIdx]);
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < SectorCount); i++, ii++)
            printf("%d ", singleIndirectIndex[ii]);
    }
    printf("\nFile contents:\n");
    for (i = k = 0; (i < numSectors) && (i < NumDirect); i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            printChar(data[j]);
        printf("\n");
    }
    if (numSectors > NumDirect)
    {
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < SectorCount); i++, ii++)
        {
            synchDisk->ReadSector(singleIndirectIndex[ii], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                printChar(data[j]);
            printf("\n");
        }
    }
    printf("----------------------------------------------\n");
    delete[] data;
    // for (i = 0; i < numSectors; i++)
    //     printf("\t%d ", dataSectors[i]);
    // if (numSectors < NumDirect)
    // {
    //     for (i = 0; i < numSectors; i++)
    //     {
    //         printf("%d ", dataSectors[i]);
    //     }
    // }
    // else
    // {
    //     printf("Using indirect index : %d\n", dataSectors[IndirectSectorIdx]);
    //     for (i = 0; i < NumDirect - 1; i++)
    //     {
    //         printf("%d ", dataSectors[i]);
    //     }
    //     char *indirect_index = new char[SectorSize];
    //     synchDisk->ReadSector(dataSectors[IndirectSectorIdx], indirect_index);
    //     j = 0;
    //     for (i = 0; i < numSectors - IndirectSectorIdx; i++)
    //     {
    //         printf("%d ", int(indirect_index[j]));
    //         j = j + 4;
    //     }
    //     printf("\n");
    // }
    // printf("\nFile contents:\n");
    // if (numSectors < NumDirect)
    // {
    //     for (i = k = 0; i < numSectors; i++)
    //     {
    //         synchDisk->ReadSector(dataSectors[i], data);
    //         for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //         {
    //             if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
    //                 printf("%c", data[j]);
    //             else
    //                 printf("\\%x", (unsigned char)data[j]);
    //         }
    //         printf("\n");
    //     }
    // }
    // else
    // {
    //     for (i = k = 0; i < numSectors; i++)
    //     {
    //         printf("Sector %d:\n", dataSectors[i]);
    //         synchDisk->ReadSector(dataSectors[i], data);
    //         for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //         {
    //             if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
    //                 printf("%c", data[j]);
    //             else
    //                 printf("\\%x", (unsigned char)data[j]);
    //         }
    //         printf("\n");
    //     }
    //     char *indirect_index = new char[SectorSize];
    //     synchDisk->ReadSector(dataSectors[IndirectSectorIdx], indirect_index);
    //     for (i = 0; i < numSectors - IndirectSectorIdx; i++)
    //     {
    //         printf("Sector %d:\n", int(indirect_index[i * 4]));
    //         synchDisk->ReadSector(int(indirect_index[i * 4]), data);
    //         for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //         {
    //             if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
    //                 printf("%c", data[j]);
    //             else
    //                 printf("\\%x", (unsigned char)data[j]);
    //         }
    //         printf("\n");
    //     }
    // }
    // delete[] data;
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