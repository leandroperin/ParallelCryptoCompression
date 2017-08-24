#include <iostream>
#include <string.h>
#include <stdio.h>
#include "optlist.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <iterator>
#include <vector>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <thread>
#include <algorithm>    // std::sort

#include "opencv2/opencv.hpp" //for image processing later on
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ml/ml.hpp"

using namespace std;
using namespace cv;


/***************************************************************************
*                       Command Line Option Parser
*
*   File    : optlist.c
*   Purpose : Provide getopt style command line option parsing
*   Author  : Michael Dipperstein
*   Date    : August 1, 2007
*
****************************************************************************
*
* OptList: A command line option parsing library
* Copyright (C) 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the OptList library.
*
* OptList is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or (at
* your option) any later version.
*
* OptList is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

static option_t *MakeOpt(
    const char option, char *const argument, const int index);

static size_t MatchOpt(
    const char argument, char *const options);

/****************************************************************************
*   Function   : GetOptList
*   Description: This function is similar to the POSIX function getopt.  All
*                options and their corresponding arguments are returned in a
*                linked list.  This function should only be called once per
*                an option list and it does not modify argv or argc.
*   Parameters : argc - the number of command line arguments (including the
*                       name of the executable)
*                argv - pointer to the open binary file to write encoded
*                       output
*                options - getopt style option list.  A NULL terminated
*                          string of single character options.  Follow an
*                          option with a colon to indicate that it requires
*                          an argument.
*   Effects    : Creates a link list of command line options and their
*                arguments.
*   Returned   : option_t type value where the option and arguement fields
*                contain the next option symbol and its argument (if any).
*                The argument field will be set to NULL if the option is
*                specified as having no arguments or no arguments are found.
*                The option field will be set to PO_NO_OPT if no more
*                options are found.
*
*   NOTE: The caller is responsible for freeing up the option list when it
*         is no longer needed.
****************************************************************************/
option_t *GetOptList(const int argc, char *const argv[], char *const options)
{
    int nextArg;
    option_t *head, *tail;
    size_t optIndex;
    size_t argIndex;

    /* start with first argument and nothing found */
    nextArg = 1;
    head = NULL;
    tail = NULL;

    /* loop through all of the command line arguments */
    while (nextArg < argc)
    {
        argIndex = 1;

        while ((strlen(argv[nextArg]) > argIndex) && ('-' == argv[nextArg][0]))
        {
            /* attempt to find a matching option */
            optIndex = MatchOpt(argv[nextArg][argIndex], options);

            if (options[optIndex] == argv[nextArg][argIndex])
            {
                /* we found the matching option */
                if (NULL == head)
                {
                    head = MakeOpt(options[optIndex], NULL, OL_NOINDEX);
                    tail = head;
                }
                else
                {
                    tail->next = MakeOpt(options[optIndex], NULL, OL_NOINDEX);
                    tail = tail->next;
                }

                if (':' == options[optIndex + 1])
                {
                    /* the option found should have a text arguement */
                    argIndex++;

                    if (strlen(argv[nextArg]) > argIndex)
                    {
                        /* no space between argument and option */
                        tail->argument = &(argv[nextArg][argIndex]);
                        tail->argIndex = nextArg;
                    }
                    else if (nextArg < argc)
                    {
                        /* there must be space between the argument option */
                        nextArg++;
                        tail->argument = argv[nextArg];
                        tail->argIndex = nextArg;
                    }

                    break; /* done with argv[nextArg] */
                }
            }

            argIndex++;
        }

        nextArg++;
    }

    return head;
}

/****************************************************************************
*   Function   : MakeOpt
*   Description: This function uses malloc to allocate space for an option_t
*                type structure and initailizes the structure with the
*                values passed as a parameter.
*   Parameters : option - this option character
*                argument - pointer string containg the argument for option.
*                           Use NULL for no argument
*                index - argv[index] contains argument use OL_NOINDEX for
*                        no argument
*   Effects    : A new option_t type variable is created on the heap.
*   Returned   : Pointer to newly created and initialized option_t type
*                structure.  NULL if space for structure can't be allocated.
****************************************************************************/
static option_t *MakeOpt(
    const char option, char *const argument, const int index)
{
    option_t *opt;

    opt = (option_t*)malloc(sizeof(option_t));

    if (opt != NULL)
    {
        opt->option = option;
        opt->argument = argument;
        opt->argIndex = index;
        opt->next = NULL;
    }
    else
    {
        perror("Failed to Allocate option_t");
    }

    return opt;
}

/****************************************************************************
*   Function   : FreeOptList
*   Description: This function will free all the elements in an option_t
*                type linked list starting from the node passed as a
*                parameter.
*   Parameters : list - head of linked list to be freed
*   Effects    : All elements of the linked list pointed to by list will
*                be freed and list will be set to NULL.
*   Returned   : None
****************************************************************************/
void FreeOptList(option_t *list)
{
    option_t *head, *next;

    head = list;
    list = NULL;

    while (head != NULL)
    {
        next = head->next;
        free(head);
        head = next;
    }

    return;
}

/****************************************************************************
*   Function   : MatchOpt
*   Description: This function searches for an arguement in an option list.
*                It will return the index to the option matching the
*                arguement or the index to the NULL if none is found.
*   Parameters : arguement - character arguement to be matched to an
*                            option in the option list
*                options - getopt style option list.  A NULL terminated
*                          string of single character options.  Follow an
*                          option with a colon to indicate that it requires
*                          an argument.
*   Effects    : None
*   Returned   : Index of argument in option list.  Index of end of string
*                if arguement does not appear in the option list.
****************************************************************************/
static size_t MatchOpt(
    const char argument, char *const options)
{
    size_t optIndex = 0;

    /* attempt to find a matching option */
    while ((options[optIndex] != '\0') &&
        (options[optIndex] != argument))
    {
        do
        {
            optIndex++;
        }
        while ((options[optIndex] != '\0') &&
            (':' == options[optIndex]));
    }

    return optIndex;
}

/****************************************************************************
*   Function   : FindFileName
*   Description: This is function accepts a pointer to the name of a file
*                along with path information and returns a pointer to the
*                first character that is not part of the path.
*   Parameters : fullPath - pointer to an array of characters containing
*                           a file name and possible path modifiers.
*   Effects    : None
*   Returned   : Returns a pointer to the first character after any path
*                information.
****************************************************************************/
char *FindFileName(const char *const fullPath)
{
    int i;
    const char *start;                          /* start of file name */
    const char *tmp;
    const char delim[3] = {'\\', '/', ':'};     /* path deliminators */

    start = fullPath;

    /* find the first character after all file path delimiters */
    for (i = 0; i < 3; i++)
    {
        tmp = strrchr(start, delim[i]);

        if (tmp != NULL)
        {
            start = tmp + 1;
        }
    }

    return (char *)start;
}

/***************************************************************************
*                        Bit Stream File Implementation
*
*   File    : bitfile.c
*   Purpose : This file implements a simple library of I/O functions for
*             files that contain data in sizes that aren't integral bytes.
*             An attempt was made to make the functions in this library
*             analogous to functions provided to manipulate byte streams.
*             The functions contained in this library were created with
*             compression algorithms in mind, but may be suited to other
*             applications.
*   Author  : Michael Dipperstein
*   Date    : January 9, 2004
*
****************************************************************************
*
* Bitfile: Bit stream File I/O Routines
* Copyright (C) 2004-2014 by Michael Dipperstein (mdipper@alumni.cs.ucsb.edu)
*
* This file is part of the bit file library.
*
* The bit file library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The bit file library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdlib.h>
#include <errno.h>
#include "bitfile.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
* type to point to the kind of functions that put/get bits from/to numerical
* data types (short, int, long, ...)
* parameters: file pointer, data structure, number of bits, sizeof data.
***************************************************************************/
typedef int (*num_func_t)(bit_file_t*, void*, const unsigned int, const size_t);

struct bit_file_t
{
    FILE *fp;                   /* file pointer used by stdio functions */
    unsigned char bitBuffer;    /* bits waiting to be read/written */
    unsigned char bitCount;     /* number of bits in bitBuffer */
    num_func_t PutBitsNumFunc;  /* endian specific BitFilePutBitsNum */
    num_func_t GetBitsNumFunc;  /* endian specific BitFileGetBitsNum */
    BF_MODES mode;              /* open for read, write, or append */
};

typedef enum
{
    BF_UNKNOWN_ENDIAN,
    BF_LITTLE_ENDIAN,
    BF_BIG_ENDIAN
} endian_t;

/* union used to test for endianess */
typedef union
{
    unsigned long word;
    unsigned char bytes[sizeof(unsigned long)];
} endian_test_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
static endian_t DetermineEndianess(void);

static int BitFilePutBitsLE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size);
static int BitFilePutBitsBE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size);

static int BitFileGetBitsLE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size);
static int BitFileGetBitsBE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size);
static int BitFileNotSupported(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size);

/***************************************************************************
*   Function   : BitFileOpen
*   Description: This function opens a bit file for reading, writing,
*                or appending.  If successful, a bit_file_t data
*                structure will be allocated and a pointer to the
*                structure will be returned.
*   Parameters : fileName - NULL terminated string containing the name of
*                           the file to be opened.
*                mode - The mode of the file to be opened
*   Effects    : The specified file will be opened and file structure will
*                be allocated.
*   Returned   : Pointer to the bit_file_t structure for the bit file
*                opened, or NULL on failure.  errno will be set for all
*                failure cases.
***************************************************************************/
bit_file_t *BitFileOpen(const char *fileName, const BF_MODES mode)
{
    const char modes[3][3] = {"rb", "wb", "ab"};    /* binary modes for fopen */
    bit_file_t *bf;

    bf = (bit_file_t *)malloc(sizeof(bit_file_t));

    if (bf == NULL)
    {
        /* malloc failed */
        errno = ENOMEM;
    }
    else
    {
        bf->fp = fopen(fileName, modes[mode]);

        if (bf->fp == NULL)
        {
            /* fopen failed */
            free(bf);
            bf = NULL;
        }
        else
        {
            /* fopen succeeded fill in remaining bf data */
            bf->bitBuffer = 0;
            bf->bitCount = 0;
            bf->mode = mode;

            switch (DetermineEndianess())
            {
                case BF_LITTLE_ENDIAN:
                    bf->PutBitsNumFunc = &BitFilePutBitsLE;
                    bf->GetBitsNumFunc = &BitFileGetBitsLE;
                    break;

                case BF_BIG_ENDIAN:
                    bf->PutBitsNumFunc = &BitFilePutBitsBE;
                    bf->GetBitsNumFunc = &BitFileGetBitsBE;
                    break;

                case BF_UNKNOWN_ENDIAN:
                default:
                    bf->PutBitsNumFunc = BitFileNotSupported;
                    bf->GetBitsNumFunc = BitFileNotSupported;
                    break;
            }

            /***************************************************************
            * TO DO: Consider using the last byte in a file to indicate
            * the number of bits in the previous byte that actually have
            * data.  If I do that, I'll need special handling of files
            * opened with a mode of BF_APPEND.
            ***************************************************************/
        }
    }

    return (bf);
}

/***************************************************************************
*   Function   : MakeBitFile
*   Description: This function naively wraps a standard file in a
*                bit_file_t structure.  ANSI-C doesn't support file status
*                functions commonly found in other C variants, so the
*                caller must be passed as a parameter.
*   Parameters : stream - pointer to the standard file being wrapped.
*                mode - The mode of the file being wrapped.
*   Effects    : A bit_file_t structure will be created for the stream
*                passed as a parameter.
*   Returned   : Pointer to the bit_file_t structure for the bit file
*                or NULL on failure.  errno will be set for all failure
*                cases.
***************************************************************************/
bit_file_t *MakeBitFile(FILE *stream, const BF_MODES mode)
{
    bit_file_t *bf;

    if (stream == NULL)
    {
        /* can't wrapper empty steam */
        errno = EBADF;
        bf = NULL;
    }
    else
    {
        bf = (bit_file_t *)malloc(sizeof(bit_file_t));

        if (bf == NULL)
        {
            /* malloc failed */
            errno = ENOMEM;
        }
        else
        {
            /* set structure data */
            bf->fp = stream;
            bf->bitBuffer = 0;
            bf->bitCount = 0;
            bf->mode = mode;

            switch (DetermineEndianess())
            {
                case BF_LITTLE_ENDIAN:
                    bf->PutBitsNumFunc = &BitFilePutBitsLE;
                    bf->GetBitsNumFunc = &BitFileGetBitsLE;
                    break;

                case BF_BIG_ENDIAN:
                    bf->PutBitsNumFunc = &BitFilePutBitsBE;
                    bf->GetBitsNumFunc = &BitFileGetBitsBE;
                    break;

                case BF_UNKNOWN_ENDIAN:
                default:
                    bf->PutBitsNumFunc = BitFileNotSupported;
                    bf->GetBitsNumFunc = BitFileNotSupported;
                    break;
            }
        }
    }

    return (bf);
}

/***************************************************************************
*   Function   : DetermineEndianess
*   Description: This function determines the endianess of the current
*                hardware architecture.  An unsigned long is set to 1.  If
*                the 1st byte of the unsigned long gets the 1, this is a
*                little endian machine.  If the last byte gets the 1, this
*                is a big endian machine.
*   Parameters : None
*   Effects    : None
*   Returned   : endian_t for current machine architecture
***************************************************************************/
static endian_t DetermineEndianess(void)
{
    endian_t endian;
    endian_test_t endianTest;

    endianTest.word = 1;

    if (endianTest.bytes[0] == 1)
    {
        /* LSB is 1st byte (little endian)*/
        endian = BF_LITTLE_ENDIAN;
    }
    else if (endianTest.bytes[sizeof(unsigned long) - 1] == 1)
    {
        /* LSB is last byte (big endian)*/
        endian = BF_BIG_ENDIAN;
    }
    else
    {
        endian = BF_UNKNOWN_ENDIAN;
    }

    return endian;
}

/***************************************************************************
*   Function   : BitFileClose
*   Description: This function closes a bit file and frees all associated
*                data.
*   Parameters : stream - pointer to bit file stream being closed
*   Effects    : The specified file will be closed and the file structure
*                will be freed.
*   Returned   : 0 for success or EOF for failure.
***************************************************************************/
int BitFileClose(bit_file_t *stream)
{
    int returnValue = 0;

    if (stream == NULL)
    {
        return(EOF);
    }

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    /***********************************************************************
    *  TO DO: Consider writing an additional byte indicating the number of
    *  valid bits (bitCount) in the previous byte.
    ***********************************************************************/

    /* close file */
    returnValue = fclose(stream->fp);

    /* free memory allocated for bit file */
    free(stream);

    return(returnValue);
}

/***************************************************************************
*   Function   : BitFileToFILE
*   Description: This function flushes and frees the bitfile structure,
*                returning a pointer to a stdio file.
*   Parameters : stream - pointer to bit file stream being closed
*   Effects    : The specified bitfile will be made usable as a stdio
*                FILE.
*   Returned   : Pointer to FILE.  NULL for failure.
***************************************************************************/
FILE *BitFileToFILE(bit_file_t *stream)
{
    FILE *fp = NULL;

    if (stream == NULL)
    {
        return(NULL);
    }

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    /***********************************************************************
    *  TO DO: Consider writing an additional byte indicating the number of
    *  valid bits (bitCount) in the previous byte.
    ***********************************************************************/

    /* close file */
    fp = stream->fp;

    /* free memory allocated for bit file */
    free(stream);

    return(fp);
}

/***************************************************************************
*   Function   : BitFileByteAlign
*   Description: This function aligns the bitfile to the nearest byte.  For
*                output files, this means writing out the bit buffer with
*                extra bits set to 0.  For input files, this means flushing
*                the bit buffer.
*   Parameters : stream - pointer to bit file stream to align
*   Effects    : Flushes out the bit buffer.
*   Returned   : EOF if stream is NULL or write fails.  Writes return the
*                byte aligned contents of the bit buffer.  Reads returns
*                the unaligned contents of the bit buffer.
***************************************************************************/
int BitFileByteAlign(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = stream->bitBuffer;

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return (returnValue);
}

/***************************************************************************
*   Function   : BitFileFlushOutput
*   Description: This function flushes the output bit buffer.  This means
*                left justifying any pending bits, and filling spare bits
*                with the fill value.
*   Parameters : stream - pointer to bit file stream to align
*                onesFill - non-zero if spare bits are filled with ones
*   Effects    : Flushes out the bit buffer, filling spare bits with ones
*                or zeros.
*   Returned   : EOF if stream is NULL or not writeable.  Otherwise, the
*                bit buffer value written. -1 if no data was written.
***************************************************************************/
int BitFileFlushOutput(bit_file_t *stream, const unsigned char onesFill)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = -1;

    /* write out any unwritten bits */
    if (stream->bitCount != 0)
    {
        stream->bitBuffer <<= (8 - stream->bitCount);

        if (onesFill)
        {
            stream->bitBuffer |= (0xFF >> stream->bitCount);
        }

        returnValue = fputc(stream->bitBuffer, stream->fp);
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return (returnValue);
}

/***************************************************************************
*   Function   : BitFileGetChar
*   Description: This function returns the next byte from the file passed as
*                a parameter.
*   Parameters : stream - pointer to bit file stream to read from
*   Effects    : Reads next byte from file and updates buffer accordingly.
*   Returned   : EOF if a whole byte cannot be obtained.  Otherwise,
*                the character read.
***************************************************************************/
int BitFileGetChar(bit_file_t *stream)
{
    int returnValue;
    unsigned char tmp;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = fgetc(stream->fp);

    if (stream->bitCount == 0)
    {
        /* we can just get byte from file */
        return returnValue;
    }

    /* we have some buffered bits to return too */
    if (returnValue != EOF)
    {
        /* figure out what to return */
        tmp = ((unsigned char)returnValue) >> (stream->bitCount);
        tmp |= ((stream->bitBuffer) << (8 - (stream->bitCount)));

        /* put remaining in buffer. count shouldn't change. */
        stream->bitBuffer = returnValue;

        returnValue = tmp;
    }

    return returnValue;
}

/***************************************************************************
*   Function   : BitFilePutChar
*   Description: This function writes the byte passed as a parameter to the
*                file passed a parameter.
*   Parameters : c - the character to be written
*                stream - pointer to bit file stream to write to
*   Effects    : Writes a byte to the file and updates buffer accordingly.
*   Returned   : On success, the character written, otherwise EOF.
***************************************************************************/
int BitFilePutChar(const int c, bit_file_t *stream)
{
    unsigned char tmp;

    if (stream == NULL)
    {
        return(EOF);
    }

    if (stream->bitCount == 0)
    {
        /* we can just put byte from file */
        return fputc(c, stream->fp);
    }

    /* figure out what to write */
    tmp = ((unsigned char)c) >> (stream->bitCount);
    tmp = tmp | ((stream->bitBuffer) << (8 - stream->bitCount));

    if (fputc(tmp, stream->fp) != EOF)
    {
        /* put remaining in buffer. count shouldn't change. */
        stream->bitBuffer = c;
    }
    else
    {
        return EOF;
    }

    return tmp;
}

/***************************************************************************
*   Function   : BitFileGetBit
*   Description: This function returns the next bit from the file passed as
*                a parameter.  The bit value returned is the msb in the
*                bit buffer.
*   Parameters : stream - pointer to bit file stream to read from
*   Effects    : Reads next bit from bit buffer.  If the buffer is empty,
*                a new byte will be read from the file.
*   Returned   : 0 if bit == 0, 1 if bit == 1, and EOF if operation fails.
***************************************************************************/
int BitFileGetBit(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    if (stream->bitCount == 0)
    {
        /* buffer is empty, read another character */
        if ((returnValue = fgetc(stream->fp)) == EOF)
        {
            return EOF;
        }
        else
        {
            stream->bitCount = 8;
            stream->bitBuffer = returnValue;
        }
    }

    /* bit to return is msb in buffer */
    stream->bitCount--;
    returnValue = (stream->bitBuffer) >> (stream->bitCount);

    return (returnValue & 0x01);
}

/***************************************************************************
*   Function   : BitFilePutBit
*   Description: This function writes the bit passed as a parameter to the
*                file passed a parameter.
*   Parameters : c - the bit value to be written
*                stream - pointer to bit file stream to write to
*   Effects    : Writes a bit to the bit buffer.  If the buffer has a byte,
*                the buffer is written to the file and cleared.
*   Returned   : On success, the bit value written, otherwise EOF.
***************************************************************************/
int BitFilePutBit(const int c, bit_file_t *stream)
{
    int returnValue = c;

    if (stream == NULL)
    {
        return(EOF);
    }

    stream->bitCount++;
    stream->bitBuffer <<= 1;

    if (c != 0)
    {
        stream->bitBuffer |= 1;
    }

    /* write bit buffer if we have 8 bits */
    if (stream->bitCount == 8)
    {
        if (fputc(stream->bitBuffer, stream->fp) == EOF)
        {
            returnValue = EOF;
        }

        /* reset buffer */
        stream->bitCount = 0;
        stream->bitBuffer = 0;
    }

    return returnValue;
}

/***************************************************************************
*   Function   : BitFileGetBits
*   Description: This function reads the specified number of bits from the
*                file passed as a parameter and writes them to the
*                requested memory location (msb to lsb).
*   Parameters : stream - pointer to bit file stream to read from
*                bits - address to store bits read
*                count - number of bits to read
*   Effects    : Reads bits from the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.
*   Returned   : EOF for failure, otherwise the number of bits read.  If
*                an EOF is reached before all the bits are read, bits
*                will contain every bit through the last complete byte.
***************************************************************************/
int BitFileGetBits(bit_file_t *stream, void *bits, const unsigned int count)
{
    unsigned char *bytes, shifts;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;

    if ((stream == NULL) || (bits == NULL))
    {
        return(EOF);
    }

    offset = 0;
    remaining = count;

    /* read whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        /* read remaining bits */
        shifts = 8 - remaining;
        bytes[offset] = 0;

        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

        /* shift last bits into position */
        bytes[offset] <<= shifts;
    }

    return count;
}

/***************************************************************************
*   Function   : BitFilePutBits
*   Description: This function writes the specified number of bits from the
*                memory location passed as a parameter to the file passed
*                as a parameter.   Bits are written msb to lsb.
*   Parameters : stream - pointer to bit file stream to write to
*                bits - pointer to bits to write
*                count - number of bits to write
*   Effects    : Writes bits to the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.
*   Returned   : EOF for failure, otherwise the number of bits written.  If
*                an error occurs after a partial write, the partially
*                written bits will not be unwritten.
***************************************************************************/
int BitFilePutBits(bit_file_t *stream, void *bits, const unsigned int count)
{
    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;

    if ((stream == NULL) || (bits == NULL))
    {
        return(EOF);
    }

    offset = 0;
    remaining = count;

    /* write whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        /* write remaining bits */
        tmp = bytes[offset];

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

/***************************************************************************
*   Function   : BitFileGetBitsNum
*   Description: This function provides a machine independent layer that
*                allows a single function call to stuff an arbitrary number
*                of bits into an integer type variable.
*   Parameters : stream - pointer to bit file stream to read from
*                bits - address to store bits read
*                count - number of bits to read
*                size - sizeof type containing "bits"
*   Effects    : Calls a function that reads bits from the bit buffer and
*                file stream.  The bit buffer will be modified as necessary.
*                the bits will be written to "bits" from least significant
*                byte to most significant byte.
*   Returned   : EOF for failure, -ENOTSUP unsupported architecture,
*                otherwise the number of bits read by the called function.
***************************************************************************/
int BitFileGetBitsNum(bit_file_t *stream, void *bits, const unsigned int count,
    const size_t size)
{
    if ((stream == NULL) || (bits == NULL))
    {
        return EOF;
    }

    if (NULL == stream->GetBitsNumFunc)
    {
        return -ENOTSUP;
    }

    /* call function that correctly handles endianess */
    return (stream->GetBitsNumFunc)(stream, bits, count, size);
}

/***************************************************************************
*   Function   : BitFileGetBitsLE   (Little Endian)
*   Description: This function reads the specified number of bits from the
*                file passed as a parameter and writes them to the
*                requested memory location (LSB to MSB).
*   Parameters : stream - pointer to bit file stream to read from
*                bits - address to store bits read
*                count - number of bits to read
*                size - sizeof type containing "bits"
*   Effects    : Reads bits from the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.  bits is treated as
*                a little endian integer of length >= (count/8) + 1.
*   Returned   : EOF for failure, otherwise the number of bits read.  If
*                an EOF is reached before all the bits are read, bits
*                will contain every bit through the last successful read.
***************************************************************************/
static int BitFileGetBitsLE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size)
{
    unsigned char *bytes;
    int offset, remaining, returnValue;

    (void)size;
    bytes = (unsigned char *)bits;
    offset = 0;
    remaining = count;

    /* read whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        /* read remaining bits */
        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

    }

    return count;
}

/***************************************************************************
*   Function   : BitFileGetBitsBE   (Big Endian)
*   Description: This function reads the specified number of bits from the
*                file passed as a parameter and writes them to the
*                requested memory location (LSB to MSB).
*   Parameters : stream - pointer to bit file stream to read from
*                bits - address to store bits read
*                count - number of bits to read
*                size - sizeof type containing "bits"
*   Effects    : Reads bits from the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.  bits is treated as
*                a big endian integer of length size.
*   Returned   : EOF for failure, otherwise the number of bits read.  If
*                an EOF is reached before all the bits are read, bits
*                will contain every bit through the last successful read.
***************************************************************************/
static int BitFileGetBitsBE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size)
{
    unsigned char *bytes;
    int offset, remaining, returnValue;

    if (count > (size * 8))
    {
        /* too many bits to read */
        return EOF;
    }

    bytes = (unsigned char *)bits;

    offset = size - 1;
    remaining = count;

    /* read whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset--;
    }

    if (remaining != 0)
    {
        /* read remaining bits */
        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

    }

    return count;
}

/***************************************************************************
*   Function   : BitFilePutBitsNum
*   Description: This function provides a machine independent layer that
*                allows a single function call to write an arbitrary number
*                of bits from an integer type variable into a file.
*   Parameters : stream - pointer to bit file stream to write to
*                bits - pointer to bits to write
*                count - number of bits to write
*                size - sizeof type containing "bits"
*   Effects    : Calls a function that writes bits to the bit buffer and
*                file stream.  The bit buffer will be modified as necessary.
*                the bits will be written to the file stream from least
*                significant byte to most significant byte.
*   Returned   : EOF for failure, ENOTSUP unsupported architecture,
*                otherwise the number of bits written.  If an error occurs
*                after a partial write, the partially written bits will not
*                be unwritten.
***************************************************************************/
int BitFilePutBitsNum(bit_file_t *stream, void *bits, const unsigned int count,
    const size_t size)
{
    if ((stream == NULL) || (bits == NULL))
    {
        return EOF;
    }

    if (NULL == stream->PutBitsNumFunc)
    {
        return ENOTSUP;
    }

    /* call function that correctly handles endianess */
    return (stream->PutBitsNumFunc)(stream, bits, count, size);
}

/***************************************************************************
*   Function   : BitFilePutBitsLE   (Little Endian)
*   Description: This function writes the specified number of bits from the
*                memory location passed as a parameter to the file passed
*                as a parameter.   Bits are written LSB to MSB.
*   Parameters : stream - pointer to bit file stream to write to
*                bits - pointer to bits to write
*                count - number of bits to write
*                size - sizeof type containing "bits"
*   Effects    : Writes bits to the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.  bits is treated as
*                a little endian integer of length >= (count/8) + 1.
*   Returned   : EOF for failure, otherwise the number of bits written.  If
*                an error occurs after a partial write, the partially
*                written bits will not be unwritten.
***************************************************************************/
static int BitFilePutBitsLE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size)
{
    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    (void)size;
    bytes = (unsigned char *)bits;
    offset = 0;
    remaining = count;

    /* write whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        /* write remaining bits */
        tmp = bytes[offset];
        tmp <<= (8 - remaining);

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

/***************************************************************************
*   Function   : BitFilePutBitsBE   (Big Endian)
*   Description: This function writes the specified number of bits from the
*                memory location passed as a parameter to the file passed
*                as a parameter.   Bits are written LSB to MSB.
*   Parameters : stream - pointer to bit file stream to write to
*                bits - pointer to bits to write
*                count - number of bits to write
*   Effects    : Writes bits to the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.  bits is treated as
*                a big endian integer of length size.
*   Returned   : EOF for failure, otherwise the number of bits written.  If
*                an error occurs after a partial write, the partially
*                written bits will not be unwritten.
***************************************************************************/
static int BitFilePutBitsBE(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size)
{
    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    if (count > (size * 8))
    {
        /* too many bits to write */
        return EOF;
    }

    bytes = (unsigned char *)bits;
    offset = size - 1;
    remaining = count;

    /* write whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        remaining -= 8;
        offset--;
    }

    if (remaining != 0)
    {
        /* write remaining bits */
        tmp = bytes[offset];
        tmp <<= (8 - remaining);

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
            {
                return EOF;
            }

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

/***************************************************************************
*   Function   : BitFileNotSupported
*   Description: This function returns -ENOTSUP.  It is called when a
*                Get/PutBits function is called on an unsupported
*                architecture.
*   Parameters : stream - not used
*                bits - not used
*                count - not used
*   Effects    : None
*   Returned   : -ENOTSUP
***************************************************************************/
static int BitFileNotSupported(bit_file_t *stream, void *bits,
    const unsigned int count, const size_t size)
{
    (void)stream;
    (void)bits;
    (void)count;
    (void)size;

    return -ENOTSUP;
}

/***************************************************************************
*                 Arithmetic Encoding and Decoding Library
*
*   File    : arcode.c
*   Purpose : Use arithmetic coding to compress/decompress file streams
*   Author  : Michael Dipperstein
*   Date    : April 2, 2004
*
****************************************************************************
*
* Arcode: An ANSI C Arithmetic Encoding/Decoding Routines
* Copyright (C) 2004, 2006-2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the arcode library.
*
* The arcode library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The arcode library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

#include <limits.h>
#include <assert.h>
#include "arcode.h"
#include "bitfile.h"

#ifdef NDEBUG
#define PrintDebug(ARGS) do {} while (0)
#else
#define PrintDebug(ARGS) printf ARGS
#endif

#if !(USHRT_MAX < ULONG_MAX)
#error "Implementation requires USHRT_MAX < ULONG_MAX"
#endif

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
#define EOF_CHAR    (UCHAR_MAX + 1)

typedef unsigned short probability_t;       /* probability count type */

typedef struct
{
    /* probability ranges for each symbol: [ranges[LOWER(c)], ranges[UPPER(c)]) */
    probability_t ranges[EOF_CHAR + 2];
    probability_t cumulativeProb;   /* sum for all ranges */

    /* lower and upper bounds of current code range */
    probability_t lower;
    probability_t upper;

    probability_t code;             /* current MSBs of encode input stream */
    unsigned char underflowBits;    /* current underflow bit count */
} stats_t;


/***************************************************************************
*                                CONSTANTS
***************************************************************************/
/* number of bits used to compute running code values */
#define PRECISION           (8 * sizeof(probability_t))

/* 2 bits less than precision. keeps lower and upper bounds from crossing. */
#define MAX_PROBABILITY     (1 << (PRECISION - 2))

/***************************************************************************
*                                  MACROS
***************************************************************************/
/* set bit x to 1 in probability_t.  Bit 0 is MSB */
#define MASK_BIT(x) (probability_t)(1 << (PRECISION - (1 + (x))))

/* indices for a symbol's lower and upper cumulative probability ranges */
#define LOWER(c)    (c)
#define UPPER(c)    ((c) + 1)

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* read write file headers */
static void WriteHeader(bit_file_t *bfpOut, stats_t *stats);
static int ReadHeader(bit_file_t *bfpIn, stats_t *stats);

/* applies symbol's ranges to current upper and lower range bounds */
static void ApplySymbolRange(int symbol, stats_t *stats, char model);

/* routines for encoding*/
static void WriteEncodedBits(bit_file_t *bfpOut, stats_t *stats);
static void WriteRemaining(bit_file_t *bfpOut, stats_t *stats);
static int BuildProbabilityRangeList(FILE *fpIn, stats_t *stats);
static int myBuildProbabilityRangeList(string sMsg, stats_t *stats);
static void InitializeAdaptiveProbabilityRangeList(stats_t *stats);

/* routines for decoding */
static void InitializeDecoder(bit_file_t *bfpOut, stats_t *stats);
static probability_t GetUnscaledCode(stats_t *stats);
static int GetSymbolFromProbability(probability_t probability, stats_t *stats);
static void ReadEncodedBits(bit_file_t *bfpIn, stats_t *stats);

/***************************************************************************
*   Function   : ArEncodeFile
*   Description: This routine generates a list of arithmetic code ranges for
*                a file and then uses them to write out an encoded version
*                of that file.
*   Parameters : inFile - FILE stream to encode
*                outFile - FILE stream to write encoded output to
*                model - model_t type value for adaptive or static model
*   Effects    : File is arithmetically encoded
*   Returned   : 0 for success, otherwise non-zero.
***************************************************************************/
int ArEncodeFile(FILE *inFile, FILE *outFile, const model_t model)
{
    int c;
    bit_file_t *bOutFile;               /* encoded output */
    stats_t stats;                      /* statistics for symbols and file */

    /* open input and output files */
    if (NULL == inFile)
    {
        inFile = stdin;
    }

    if (outFile == NULL)
    {
        bOutFile = MakeBitFile(stdout, BF_WRITE);
    }
    else
    {
        bOutFile = MakeBitFile(outFile, BF_WRITE);
    }

    if (NULL == bOutFile)
    {
        fprintf(stderr, "Error: Creating binary output file\n");
        return -1;
    }

    if (MODEL_STATIC == model)
    {
        /* create list of probability ranges by counting symbols in file */
        if (0 != BuildProbabilityRangeList(inFile, &stats))
        {
            fclose(inFile);
            BitFileClose(bOutFile);
            fprintf(stderr, "Error determining frequency ranges.\n");
            return -1;
        }

        rewind(inFile);

        /* write information required to decode file to encoded file */
        WriteHeader(bOutFile, &stats);
    }
    else
    {
        /* initialize probability ranges assuming uniform distribution */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* initialize coder start with full probability range [0%, 100%) */
    stats.lower = 0;
    stats.upper = ~0;                                  /* all ones */
    stats.underflowBits = 0;

    /* encode symbols one at a time */
    while ((c = fgetc(inFile)) != EOF)
    {
        ApplySymbolRange(c, &stats, model);
        WriteEncodedBits(bOutFile, &stats);
    }

    ApplySymbolRange(EOF_CHAR, &stats, model);   /* encode an EOF */
    WriteEncodedBits(bOutFile, &stats);
    WriteRemaining(bOutFile, &stats);           /* write out lsbs */
    outFile = BitFileToFILE(bOutFile);          /* make file normal again */

    return 0;
}


int myArEncodeString(string sMsg, FILE* outFile, const model_t model)
{
    int c;
    bit_file_t *bOutFile;               /* encoded output */
    stats_t stats;                      /* statistics for symbols and file */

    /* open output file */
    if (outFile == NULL)
    {
        bOutFile = MakeBitFile(stdout, BF_WRITE);
    }
    else
    {
        bOutFile = MakeBitFile(outFile, BF_WRITE);
    }

    if (NULL == bOutFile)
    {
        fprintf(stderr, "Error: Creating binary output file\n");
        return -1;
    }

    if (MODEL_STATIC == model)
    {
        /* create list of probability ranges by counting symbols in file */
        if (0 != myBuildProbabilityRangeList(sMsg, &stats))
        {
            BitFileClose(bOutFile);
            fprintf(stderr, "Error determining frequency ranges.\n");
            return -1;
        }

        //rewind(inFile);

        /* write information required to decode file to encoded file */
        WriteHeader(bOutFile, &stats);
    }
    else
    {
        /* initialize probability ranges assuming uniform distribution */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* initialize coder start with full probability range [0%, 100%) */
    stats.lower = 0;
    stats.upper = ~0;                                  /* all ones */
    stats.underflowBits = 0;

    /* encode symbols one at a time */
    for (string::iterator it = sMsg.begin(); it < sMsg.end(); ++it)
    {
        c=*it;
        ApplySymbolRange(c, &stats, model);
        WriteEncodedBits(bOutFile, &stats);
    }

    ApplySymbolRange(EOF_CHAR, &stats, model);   /* encode an EOF */
    WriteEncodedBits(bOutFile, &stats);
    WriteRemaining(bOutFile, &stats);           /* write out lsbs */
    outFile = BitFileToFILE(bOutFile);          /* make file normal again */

    return 0;
}

#include <stdio.h>
#include <stdlib.h>

#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

 struct io_struct{
  FILE *inFile;
  FILE *outFile;
  model_t model;
  };

//thread version of encode
void* myArEncodeFile(void* arguments) //(FILE *inFile, FILE *outFile, const model_t model)
{
    //deprecated, for information only
    int c;
    bit_file_t *bOutFile;               /* encoded output */
    stats_t stats;                      /* statistics for symbols and file */

    //FILE *inFile, *outFile;    /* input & output files */
    //const model_t model;       /* static/adaptive model*/
    struct io_struct *args = (io_struct*)arguments;
    FILE *inFile   = args->inFile;
    FILE *outFile  = args->outFile;
    model_t model  = args->model;

    /* open input and output files */
    if (NULL == inFile)
    {
        inFile = stdin;
    }

    if (outFile == NULL)
    {
        fprintf(stdin, "outFILE==NULL\n");
        bOutFile = MakeBitFile(stdout, BF_WRITE);
    }
    else
    {
        bOutFile = MakeBitFile(outFile, BF_WRITE);
    }

    if (NULL == bOutFile)
    {
        fprintf(stderr, "Error: Creating binary output file\n");
        return NULL;
    }
    else
    {
        fprintf(stderr, "Success: Creating binary output file\n");
    }

    if (MODEL_STATIC == model)
    {
        /* create list of probability ranges by counting symbols in file */
        if (0 != BuildProbabilityRangeList(inFile, &stats))
        {
            fclose(inFile);
            BitFileClose(bOutFile);
            fprintf(stderr, "Error determining frequency ranges.\n");
            return NULL;
        }

        rewind(inFile);

        /* write information required to decode file to encoded file */
        WriteHeader(bOutFile, &stats);
    }
    else
    {
        /* initialize probability ranges assuming uniform distribution */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* initialize coder start with full probability range [0%, 100%) */
    stats.lower = 0;
    stats.upper = ~0;                                  /* all ones */
    stats.underflowBits = 0;

    /* encode symbols one at a time */
    while ((c = fgetc(inFile)) != EOF)
    {
        ApplySymbolRange(c, &stats, model);
        WriteEncodedBits(bOutFile, &stats);
    }

    ApplySymbolRange(EOF_CHAR, &stats, model);   /* encode an EOF */
    WriteEncodedBits(bOutFile, &stats);
    WriteRemaining(bOutFile, &stats);           /* write out lsbs */
    outFile = BitFileToFILE(bOutFile);          /* make file normal again */

    return NULL;
}

/***************************************************************************
*   Function   : SymbolCountToProbabilityRanges
*   Description: This routine converts the ranges array containing only
*                symbol counts to an array containing the upper and lower
*                probability ranges for each symbol.
*   Parameters : stats - structure containing data used to encode symbols
*   Effects    : ranges struct containing symbol counts in the upper field
*                for each symbol is converted to a list of upper and lower
*                probability bounds for each symbol.
*   Returned   : None
***************************************************************************/
static void SymbolCountToProbabilityRanges(stats_t *stats)
{
    int c;

    stats->ranges[0] = 0;                  /* absolute lower bound is 0 */
    stats->ranges[UPPER(EOF_CHAR)] = 1;    /* add 1 EOF character */
    stats->cumulativeProb++;

    /* assign upper and lower probability ranges */
    for (c = 1; c <= UPPER(EOF_CHAR); c++)
    {
        stats->ranges[c] += stats->ranges[c - 1];
    }

/**
    // dump list of ranges
    PrintDebug(("Ranges:\n"));
    for (c = 0; c < UPPER(EOF_CHAR); c++)
    {
        PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)],
            stats->ranges[UPPER(c)]));
    }
**/

    return;
}

/***************************************************************************
*   Function   : BuildProbabilityRangeList
*   Description: This routine reads the input file and builds the global
*                list of upper and lower probability ranges for each
*                symbol.
*   Parameters : fpIn - file to build range list for
*                stats - structure containing data used to encode symbols
*   Effects    : stats struct is made to contain probability ranges for
*                each symbol.
*   Returned   : 0 for success, otherwise non-zero.
***************************************************************************/
static int BuildProbabilityRangeList(FILE *fpIn, stats_t *stats)
{
    int c;

    /***********************************************************************
    * unsigned long is used to hold the largest counts we can have without
    * any rescaling.  Rescaling may take place before probability ranges
    * are computed.
    ***********************************************************************/
    unsigned long countArray[EOF_CHAR];
    unsigned long totalCount = 0;
    unsigned long rescaleValue;

    if (fpIn == NULL)
    {
        return -1;
    }

    /* start with no symbols counted */
    for (c = 0; c < EOF_CHAR; c++)
    {
        countArray[c] = 0;
    }

    while ((c = fgetc(fpIn)) != EOF)
    {
        if (totalCount == ULONG_MAX)
        {
            fprintf(stderr, "Error: file too large\n");
            return -1;
        }

        countArray[c]++;
        totalCount++;
    }

    /* rescale counts to be < MAX_PROBABILITY */
    if (totalCount >= MAX_PROBABILITY)
    {
        rescaleValue = (totalCount / MAX_PROBABILITY) + 1;

        for (c = 0; c < EOF_CHAR; c++)
        {
            if (countArray[c] > rescaleValue)
            {
                countArray[c] /= rescaleValue;
            }
            else if (countArray[c] != 0)
            {
                countArray[c] = 1;
            }
        }
    }

    /* copy scaled symbol counts to range list */
    stats->ranges[0] = 0;
    stats->cumulativeProb = 0;
    for (c = 0; c < EOF_CHAR; c++)
    {
        stats->ranges[UPPER(c)] = countArray[c];
        stats->cumulativeProb += countArray[c];
    }

    /* convert counts to a range of probabilities */
    SymbolCountToProbabilityRanges(stats);
    return 0;
}

static int myBuildProbabilityRangeList(string sMsg, stats_t *stats)
{
    int c;

    /***********************************************************************
    * unsigned long is used to hold the largest counts we can have without
    * any rescaling.  Rescaling may take place before probability ranges
    * are computed.
    ***********************************************************************/
    unsigned long countArray[EOF_CHAR];
    unsigned long totalCount = 0;
    unsigned long rescaleValue;

    /* start with no symbols counted */
    for (c = 0; c < EOF_CHAR; c++)
    {
        countArray[c] = 0;
    }

    for (string::iterator it = sMsg.begin(); it < sMsg.end(); ++it)
    {
        if (totalCount == ULONG_MAX)
        {
            fprintf(stderr, "Error: file too large\n");
            return -1;
        }
        countArray[(int)*it]++;
        totalCount++;
    }

    /* rescale counts to be < MAX_PROBABILITY */
    if (totalCount >= MAX_PROBABILITY)
    {
        rescaleValue = (totalCount / MAX_PROBABILITY) + 1;

        for (c = 0; c < EOF_CHAR; c++)
        {
            if (countArray[c] > rescaleValue)
            {
                countArray[c] /= rescaleValue;
            }
            else if (countArray[c] != 0)
            {
                countArray[c] = 1;
            }
        }
    }

    /* copy scaled symbol counts to range list */
    stats->ranges[0] = 0;
    stats->cumulativeProb = 0;
    for (c = 0; c < EOF_CHAR; c++)
    {
        stats->ranges[UPPER(c)] = countArray[c];
        stats->cumulativeProb += countArray[c];
    }

    /* convert counts to a range of probabilities */
    SymbolCountToProbabilityRanges(stats);
    return 0;
}


/***************************************************************************
*   Function   : WriteHeader
*   Description: This function writes each symbol contained in the encoded
*                file as well as its rescaled number of occurrences.  A
*                decoding algorithm may use these numbers to reconstruct
*                the probability range list used to encode the file.
*   Parameters : bfpOut - pointer to open binary file to write to.
*                stats - structure containing data used to encode symbols
*   Effects    : Symbol values and symbol counts are written to a file.
*   Returned   : None
***************************************************************************/
static void WriteHeader(bit_file_t *bfpOut, stats_t *stats)
{
    int c;
    probability_t previous = 0;         /* symbol count so far */

    for(c = 0; c <= (EOF_CHAR - 1); c++)
    {
        if (stats->ranges[UPPER(c)] > previous)
        {
            /* some of these symbols will be encoded */
            BitFilePutChar((char)c, bfpOut);
            previous = (stats->ranges[UPPER(c)] - previous); /* symbol count */

            //PrintDebug(("HEADER:\n"));
            //PrintDebug(("%02X\t%d\n", c, previous));

            /* write out PRECISION - 2 bit count */
            BitFilePutBitsNum(bfpOut, &previous, (PRECISION - 2),
                sizeof(probability_t));

            /* current upper range is previous for the next character */
            previous = stats->ranges[UPPER(c)];
        }
    }

    /* now write end of table (zero count) */
    BitFilePutChar(0x00, bfpOut);
    previous = 0;
    BitFilePutBits(bfpOut, (void *)&previous, PRECISION - 2);
}

/***************************************************************************
*   Function   : InitializeAdaptiveProbabilityRangeList
*   Description: This routine builds the initial global list of upper and
*                lower probability ranges for each symbol.  This routine
*                is used by both adaptive encoding and decoding.
*                Currently it provides a uniform symbol distribution.
*                Other distributions might be better suited for known data
*                types (such as English text).
*   Parameters : stats - structure containing data used to encode symbols
*   Effects    : ranges array is made to contain initial probability ranges
*                for each symbol.
*   Returned   : NONE
***************************************************************************/
static void InitializeAdaptiveProbabilityRangeList(stats_t *stats)
{
    int c;

    stats->ranges[0] = 0;      /* absolute lower range */

    /* assign upper and lower probability ranges assuming uniformity */
    for (c = 1; c <= UPPER(EOF_CHAR); c++)
    {
        stats->ranges[c] = stats->ranges[c - 1] + 1;
    }

    stats->cumulativeProb = UPPER(EOF_CHAR);

/**
    // dump list of ranges
    PrintDebug(("Ranges:\n"));
    for (c = 0; c < UPPER(EOF_CHAR); c++)
    {
        PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)],
            stats->ranges[UPPER(c)]));
    }
**/
    return;
}

/***************************************************************************
*   Function   : ApplySymbolRange
*   Description: This function is used for both encoding and decoding.  It
*                applies the range restrictions of a new symbol to the
*                current upper and lower range bounds of an encoded stream.
*                If an adaptive model is being used, the probability range
*                list will be updated after the effect of the symbol is
*                applied.
*   Parameters : symbol - The symbol to be added to the current code range
*                stats - structure containing data used to encode symbols
*                model - TRUE if encoding/decoding with a static
*                              model.
*   Effects    : The current upper and lower range bounds are adjusted to
*                include the range effects of adding another symbol to the
*                encoded stream.  If an adaptive model is being used, the
*                probability range list will be updated.
*   Returned   : None
***************************************************************************/
static void ApplySymbolRange(int symbol, stats_t *stats, char model)
{
    unsigned long range;        /* must be able to hold max upper + 1 */
    unsigned long rescaled;     /* range rescaled for range of new symbol */
                                /* must hold range * max upper */

    /* for updating dynamic models */
    int i;
    probability_t original;     /* range value prior to rescale */
    probability_t delta;        /* range for individual symbol */

    /***********************************************************************
    * Calculate new upper and lower ranges.  Since the new upper range is
    * dependant of the old lower range, compute the upper range first.
    ***********************************************************************/
    range = (unsigned long)(stats->upper - stats->lower) + 1;

    /* scale upper range of the symbol being coded */
    rescaled = (unsigned long)(stats->ranges[UPPER(symbol)]) * range;
    rescaled /= (unsigned long)(stats->cumulativeProb);

    /* new upper = old lower + rescaled new upper - 1*/
    stats->upper = stats->lower + (probability_t)rescaled - 1;

    /* scale lower range of the symbol being coded */
    rescaled = (unsigned long)(stats->ranges[LOWER(symbol)]) * range;
    rescaled /= (unsigned long)(stats->cumulativeProb);

    /* new lower = old lower + rescaled new upper */
    stats->lower = stats->lower + (probability_t)rescaled;

    if (!model)
    {
        /* add new symbol to model */
        stats->cumulativeProb++;

        for (i = UPPER(symbol); i <= UPPER(EOF_CHAR); i++)
        {
            stats->ranges[i] += 1;
        }

        /* halve current values if cumulativeProb is too large */
        if (stats->cumulativeProb >= MAX_PROBABILITY)
        {
            stats->cumulativeProb = 0;
            original = 0;

            for (i = 1; i <= UPPER(EOF_CHAR); i++)
            {
                delta = stats->ranges[i] - original;
                original = stats->ranges[i];

                if (delta <= 2)
                {
                    /* prevent probability from being 0 */
                    stats->ranges[i] = stats->ranges[i - 1] + 1;
                }
                else
                {
                    stats->ranges[i] = stats->ranges[i - 1] + (delta / 2);
                }

                stats->cumulativeProb +=
                    (stats->ranges[i] - stats->ranges[i - 1]);
            }
        }
    }

    assert(stats->lower <= stats->upper);
}

/***************************************************************************
*   Function   : WriteEncodedBits
*   Description: This function attempts to shift out as many code bits as
*                possible, writing the shifted bits to the encoded output
*                file.  Only bits that will be unchanged when additional
*                symbols are encoded may be written out.
*
*                If the n most significant bits of the lower and upper range
*                bounds match, they will not be changed when additional
*                symbols are encoded, so they may be shifted out.
*
*                Adjustments are also made to prevent possible underflows
*                that occur when the upper and lower ranges are so close
*                that encoding another symbol won't change their values.
*   Parameters : bfpOut - pointer to open binary file to write to.
*                stats - structure containing data used to encode symbols
*   Effects    : The upper and lower code bounds are adjusted so that they
*                only contain only bits that may be affected by the
*                addition of a new symbol to the encoded stream.
*   Returned   : None
***************************************************************************/
static void WriteEncodedBits(bit_file_t *bfpOut, stats_t *stats)
{
    for (;;)
    {
        if ((stats->upper & MASK_BIT(0)) == (stats->lower & MASK_BIT(0)))
        {
            /* MSBs match, write them to output file */
            BitFilePutBit((stats->upper & MASK_BIT(0)) != 0, bfpOut);

            /* we can write out underflow bits too */
            while (stats->underflowBits > 0)
            {
                BitFilePutBit((stats->upper & MASK_BIT(0)) == 0, bfpOut);
                stats->underflowBits--;
            }
        }
        else if ((stats->lower & MASK_BIT(1)) && !(stats->upper & MASK_BIT(1)))
        {
            /***************************************************************
            * Possible underflow condition: neither MSBs nor second MSBs
            * match.  It must be the case that lower and upper have MSBs of
            * 01 and 10.  Remove 2nd MSB from lower and upper.
            ***************************************************************/
            stats->underflowBits += 1;
            stats->lower &= ~(MASK_BIT(0) | MASK_BIT(1));
            stats->upper |= MASK_BIT(1);

            /***************************************************************
            * The shifts below make the rest of the bit removal work.  If
            * you don't believe me try it yourself.
            ***************************************************************/
        }
        else
        {
            /* nothing left to do */
            return ;
        }

        /*******************************************************************
        * Shift out old MSB and shift in new LSB.  Remember that lower has
        * all 0s beyond it's end and upper has all 1s beyond it's end.
        *******************************************************************/
        stats->lower <<= 1;
        stats->upper <<= 1;
        stats->upper |= 1;
    }
}

/***************************************************************************
*   Function   : WriteRemaining
*   Description: This function writes out all remaining significant bits
*                in the upper and lower ranges and the underflow bits once
*                the last symbol has been encoded.
*   Parameters : bfpOut - pointer to open binary file to write to.
*                stats - structure containing data used to encode symbols
*   Effects    : Remaining significant range bits are written to the output
*                file.
*   Returned   : None
***************************************************************************/
static void WriteRemaining(bit_file_t *bfpOut, stats_t *stats)
{
    BitFilePutBit((stats->lower & MASK_BIT(1)) != 0, bfpOut);

    /* write out any unwritten underflow bits */
    for (stats->underflowBits++; stats->underflowBits > 0;
        stats->underflowBits--)
    {
        BitFilePutBit((stats->lower & MASK_BIT(1)) == 0, bfpOut);
    }
}

/***************************************************************************
*   Function   : ArDecodeFile
*   Description: This routine opens an arithmetically encoded file, reads
*                it's header, and builds a list of probability ranges which
*                it then uses to decode the rest of the file.
*   Parameters : inFile - FILE stream to decode
*                outFile - FILE stream to write decoded output to
*                model - model_t type value for adaptive or static model
*   Effects    : Encoded file is decoded
*   Returned   : 0 for success, otherwise non-zero.
***************************************************************************/
int ArDecodeFile(FILE *inFile, FILE *outFile, const model_t model)
{
    int c;
    probability_t unscaled;
    bit_file_t *bInFile;
    stats_t stats;                      /* statistics for symbols and file */

    /* handle file pointers */
    if (NULL == outFile)
    {
        outFile = stdout;
    }

    if (NULL == inFile)
    {
        fprintf(stderr, "Error: Invalid input file\n");
        return -1;
    }

    bInFile = MakeBitFile(inFile, BF_READ);

    if (NULL == bInFile)
    {
        fprintf(stderr, "Error: Unable to create binary input file\n");
        return -1;
    }

    if (MODEL_STATIC == model)
    {
        /* build probability ranges from header in file */
        if (0 != ReadHeader(bInFile, &stats))
        {
            BitFileClose(bInFile);
            fclose(outFile);
            return -1;
        }
    }
    else
    {
        /* initialize ranges for adaptive model */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* read start of code and initialize bounds, and adaptive ranges */
    InitializeDecoder(bInFile, &stats);

    /* decode one symbol at a time */
    for (;;)
    {
        /* get the unscaled probability of the current symbol */
        unscaled = GetUnscaledCode(&stats);

        /* figure out which symbol has the above probability */
        if((c = GetSymbolFromProbability(unscaled, &stats)) == -1)
        {
            /* error: unknown symbol */
            break;
        }

        if (c == EOF_CHAR)
        {
            /* no more symbols */
            break;
        }

        fputc((char)c, outFile);

        /* factor out symbol */
        ApplySymbolRange(c, &stats, model);
        ReadEncodedBits(bInFile, &stats);
    }

    inFile = BitFileToFILE(bInFile);        /* make file normal again */

    return 0;
}

string myArDecodeFile(FILE *inFile, const model_t model)
{
    string sOut="";
    int c;
    probability_t unscaled;
    bit_file_t *bInFile;
    stats_t stats;                      /* statistics for symbols and file */


    if (NULL == inFile)
    {
        fprintf(stderr, "Error: Invalid input file\n");
        return sOut;
    }

    bInFile = MakeBitFile(inFile, BF_READ);

    if (NULL == bInFile)
    {
        fprintf(stderr, "Error: Unable to create binary input file\n");
        return sOut;
    }

    if (MODEL_STATIC == model)
    {
        /* build probability ranges from header in file */
        if (0 != ReadHeader(bInFile, &stats))
        {
            BitFileClose(bInFile);
            //fclose(outFile);
            return sOut;
        }
    }
    else
    {
        /* initialize ranges for adaptive model */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* read start of code and initialize bounds, and adaptive ranges */
    InitializeDecoder(bInFile, &stats);

    /* decode one symbol at a time */
    for (;;)
    {
        /* get the unscaled probability of the current symbol */
        unscaled = GetUnscaledCode(&stats);

        /* figure out which symbol has the above probability */
        if((c = GetSymbolFromProbability(unscaled, &stats)) == -1)
        {
            /* error: unknown symbol */
            break;
        }

        if (c == EOF_CHAR)
        {
            /* no more symbols */
            break;
        }


        //fputc((char)c, outFile);
        sOut += c;

        /* factor out symbol */
        ApplySymbolRange(c, &stats, model);
        ReadEncodedBits(bInFile, &stats);
    }
    inFile = BitFileToFILE(bInFile);        /* make file normal again */

    return sOut;
}




//thread version of decode file
void* myArDecodeFile(void* arguments) //int ArDecodeFile(FILE *inFile, FILE *outFile, const model_t model)
{
    struct io_struct *args = (io_struct*)arguments;
    FILE *inFile   = args->inFile;
    FILE *outFile  = args->outFile;
    model_t model  = args->model;

    int c;
    probability_t unscaled;
    bit_file_t *bInFile;
    stats_t stats;                      /* statistics for symbols and file */

    /* handle file pointers */
    if (NULL == outFile)
    {
        outFile = stdout;
    }

    if (NULL == inFile)
    {
        fprintf(stderr, "Error: Invalid input file\n");
        return NULL;
    }

    bInFile = MakeBitFile(inFile, BF_READ);

    if (NULL == bInFile)
    {
        fprintf(stderr, "Error: Unable to create binary input file\n");
        return NULL;
    }

    if (MODEL_STATIC == model)
    {
        /* build probability ranges from header in file */
        if (0 != ReadHeader(bInFile, &stats))
        {
            BitFileClose(bInFile);
            fclose(outFile);
            return NULL;
        }
    }
    else
    {
        /* initialize ranges for adaptive model */
        InitializeAdaptiveProbabilityRangeList(&stats);
    }

    /* read start of code and initialize bounds, and adaptive ranges */
    InitializeDecoder(bInFile, &stats);

    /* decode one symbol at a time */
    for (;;)
    {
        /* get the unscaled probability of the current symbol */
        unscaled = GetUnscaledCode(&stats);

        /* figure out which symbol has the above probability */
        if((c = GetSymbolFromProbability(unscaled, &stats)) == -1)
        {
            /* error: unknown symbol */
            break;
        }

        if (c == EOF_CHAR)
        {
            /* no more symbols */
            break;
        }

        fputc((char)c, outFile);

        /* factor out symbol */
        ApplySymbolRange(c, &stats, model);
        ReadEncodedBits(bInFile, &stats);
    }

    inFile = BitFileToFILE(bInFile);        /* make file normal again */

    return 0;
}


/****************************************************************************
*   Function   : ReadHeader
*   Description: This function reads the header information stored by
*                WriteHeader.  The header can then be used to build a
*                probability range list matching the list that was used to
*                encode the file.
*   Parameters : bfpIn - file to read from
*                stats - structure containing data used to encode symbols
*   Effects    : Probability range list is built.
*   Returned   : 0 for success, otherwise non-zero.
****************************************************************************/
static int ReadHeader(bit_file_t *bfpIn, stats_t *stats)
{
    int c;
    probability_t count;

    stats->cumulativeProb = 0;

    for (c = 0; c <= UPPER(EOF_CHAR); c++)
    {
        stats->ranges[UPPER(c)] = 0;
    }

    /* read [character, probability] sets */
    for (;;)
    {
        c = BitFileGetChar(bfpIn);
        count = 0;

        /* read (PRECISION - 2) bit count */
        if (BitFileGetBitsNum(bfpIn, &count, (PRECISION - 2),
            sizeof(probability_t)) == EOF)
        {
            /* premature EOF */
            fprintf(stderr, "Error: unexpected EOF\n");
            return -1;
        }

        //PrintDebug(("HEADER:\n"));
        //PrintDebug(("%02X\t%d\n", c, count));

        if (count == 0)
        {
            /* 0 count means end of header */
            break;
        }

        stats->ranges[UPPER(c)] = count;
        stats->cumulativeProb += count;
    }

    /* convert counts to range list */
    SymbolCountToProbabilityRanges(stats);
    return 0;
}

/****************************************************************************
*   Function   : InitializeDecoder
*   Description: This function starts the upper and lower ranges at their
*                max/min values and reads in the most significant encoded
*                bits.
*   Parameters : bfpIn - file to read from
*                stats - structure containing data used to encode symbols
*   Effects    : upper, lower, and code are initialized.  The probability
*                range list will also be initialized if an adaptive model
*                will be used.
*   Returned   : None
****************************************************************************/
static void InitializeDecoder(bit_file_t *bfpIn, stats_t *stats)
{
    int i;

    stats->code = 0;

    /* read PERCISION MSBs of code one bit at a time */
    for (i = 0; i < (int)PRECISION; i++)
    {
        stats->code <<= 1;

        /* treat EOF like 0 */
        if(BitFileGetBit(bfpIn) == 1)
        {
            stats->code |= 1;
        }
    }

    /* start with full probability range [0%, 100%) */
    stats->lower = 0;
    stats->upper = ~0;      /* all ones */
}

/****************************************************************************
*   Function   : GetUnscaledCode
*   Description: This function undoes the scaling that ApplySymbolRange
*                performed before bits were shifted out.  The value returned
*                is the probability of the encoded symbol.
*   Parameters : stats - structure containing data used to encode symbols
*   Effects    : None
*   Returned   : The probability of the current symbol
****************************************************************************/
static probability_t GetUnscaledCode(stats_t *stats)
{
    unsigned long range;        /* must be able to hold max upper + 1 */
    unsigned long unscaled;

    range = (unsigned long)(stats->upper - stats->lower) + 1;

    /* reverse the scaling operations from ApplySymbolRange */
    unscaled = (unsigned long)(stats->code - stats->lower) + 1;
    unscaled = unscaled * (unsigned long)(stats->cumulativeProb) - 1;
    unscaled /= range;

    return ((probability_t)unscaled);
}

/****************************************************************************
*   Function   : GetSymbolFromProbability
*   Description: Given a probability, this function will return the symbol
*                whose range includes that probability.  Symbol is found
*                binary search on probability ranges.
*   Parameters : probability - probability of symbol.
*                stats - structure containing data used to encode symbols
*   Effects    : None
*   Returned   : -1 for failure, otherwise encoded symbol
****************************************************************************/
static int GetSymbolFromProbability(probability_t probability, stats_t *stats)
{
    int first, last, middle;    /* indicies for binary search */

    first = 0;
    last = UPPER(EOF_CHAR);
    middle = last / 2;

    /* binary search */
    while (last >= first)
    {
        if (probability < stats->ranges[LOWER(middle)])
        {
            /* lower bound is higher than probability */
            last = middle - 1;
            middle = first + ((last - first) / 2);
            continue;
        }

        if (probability >= stats->ranges[UPPER(middle)])
        {
            /* upper bound is lower than probability */
            first = middle + 1;
            middle = first + ((last - first) / 2);
            continue;
        }

        /* we must have found the right value */
        return middle;
    }

    /* error: none of the ranges include the probability */
    fprintf(stderr, "Unknown Symbol: %d (max: %d)\n", probability,
        stats->ranges[UPPER(EOF_CHAR)]);
    return -1;
}

/***************************************************************************
*   Function   : ReadEncodedBits
*   Description: This function attempts to shift out as many code bits as
*                possible, as bits are shifted out the coded input is
*                populated with bits from the encoded file.  Only bits
*                that will be unchanged when additional symbols are decoded
*                may be shifted out.
*
*                If the n most significant bits of the lower and upper range
*                bounds match, they will not be changed when additional
*                symbols are decoded, so they may be shifted out.
*
*                Adjustments are also made to prevent possible underflows
*                that occur when the upper and lower ranges are so close
*                that decoding another symbol won't change their values.
*   Parameters : bfpOut - pointer to open binary file to read from.
*                stats - structure containing data used to encode symbols
*   Effects    : The upper and lower code bounds are adjusted so that they
*                only contain only bits that will be affected by the
*                addition of a new symbol.  Replacements are read from the
*                encoded stream.
*   Returned   : None
***************************************************************************/
static void ReadEncodedBits(bit_file_t *bfpIn, stats_t *stats)
{
    int nextBit;        /* next bit from encoded input */

    for (;;)
    {
        if ((stats->upper & MASK_BIT(0)) == (stats->lower & MASK_BIT(0)))
        {
            /* MSBs match, allow them to be shifted out*/
        }
        else if ((stats->lower & MASK_BIT(1)) && !(stats->upper & MASK_BIT(1)))
        {
            /***************************************************************
            * Possible underflow condition: neither MSBs nor second MSBs
            * match.  It must be the case that lower and upper have MSBs of
            * 01 and 10.  Remove 2nd MSB from lower and upper.
            ***************************************************************/
            stats->lower   &= ~(MASK_BIT(0) | MASK_BIT(1));
            stats->upper  |= MASK_BIT(1);
            stats->code ^= MASK_BIT(1);

            /* the shifts below make the rest of the bit removal work */
        }
        else
        {
            /* nothing to shift out */
            return;
        }

        /*******************************************************************
        * Shift out old MSB and shift in new LSB.  Remember that lower has
        * all 0s beyond it's end and upper has all 1s beyond it's end.
        *******************************************************************/
        stats->lower <<= 1;
        stats->upper <<= 1;
        stats->upper |= 1;
        stats->code <<= 1;

        if ((nextBit = BitFileGetBit(bfpIn)) == EOF)
        {
            /* either all bits are shifted out or error occurred */
        }
        else
        {
            stats->code |= nextBit;     /* add next encoded bit to code */
        }
    }

    return;
}

/***************************************************************************
*              Sample Program Using Arithmetic Encoding Library
*
*   File    : sample.c
*   Purpose : Demonstrate usage of arithmetic encoding library
*   Author  : Michael Dipperstein
*   Date    : March 10, 2004
*
****************************************************************************
*
* SAMPLE: Sample usage of the arcode Arithmetic Encoding Library
* Copyright (C) 2004, 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the arcode library.
*
* The arcode library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The arcode library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "optlist.h"
#include "arcode.h"

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

/***************************************************************************
*                                FUNCTIONS DCT and IDCT
***************************************************************************/
#include <math.h>
#include <stdlib.h>

void dct(float **DCTMatrix, float **Matrix, int N, int M);
void write_mat(FILE *fp, float **testRes, int N, int M);
void idct(float **Matrix, float **DCTMatrix, int N, int M);
float **calloc_mat(int dimX, int dimY);
void free_mat(float **p);

float **calloc_mat(int dimX, int dimY)
{
    float **m = (float**)calloc(dimX, sizeof(float*));
    float *p  = (float*)calloc(dimX*dimY, sizeof(float));
    int i;
    for(i=0; i <dimX;i++){
    m[i] = &p[i*dimY];

    }
   return m;
}

void free_mat(float **m){
  free(m[0]);
  free(m);
}

void write_mat(FILE *fp, float **m, int N, int M){

   int i, j;
   for(i =0; i< N; i++){
    fprintf(fp, "%f", m[i][0]);
    for(j = 1; j < M; j++){
       fprintf(fp, "\t%f", m[i][j]);
        }
    fprintf(fp, "\n");
   }
   fprintf(fp, "\n");
}

void dct(float **DCTMatrix, float **Matrix, int N, int M){

    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
        DCTMatrix[u][v] = 0;
            for (i = 0; i < N; i++) {
                for (j = 0; j < M; j++) {
                    DCTMatrix[u][v] += Matrix[i][j] * cos(M_PI/((float)N)*(i+1./2.)*u)*cos(M_PI/((float)M)*(j+1./2.)*v);
                }
            }
        }
    }
 }

struct dct_struct{
    float **DCTMatrix;
    float **Matrix;
    int N; //dimX
    int M; //dimY
};
void* mydct(void *arguments) //(float **DCTMatrix, float **Matrix, int N, int M)
{
    struct dct_struct *args = (dct_struct*)arguments;
    float **DCTMatrix   = args->DCTMatrix;
    float **Matrix      = args->Matrix;
    int N               = args->N;
    int M               = args->M;
    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
        DCTMatrix[u][v] = 0;
            for (i = 0; i < N; i++) {
                for (j = 0; j < M; j++) {
                    DCTMatrix[u][v] += Matrix[i][j] * cos(M_PI/((float)N)*(i+1./2.)*u)*cos(M_PI/((float)M)*(j+1./2.)*v);
                }
            }
        }
    }
    return NULL;
 }

void idct(float **Matrix, float **DCTMatrix, int N, int M){
    int i, j, u, v;

    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
          Matrix[u][v] = 1/4.*DCTMatrix[0][0];
          for(i = 1; i < N; i++){
          Matrix[u][v] += 1/2.*DCTMatrix[i][0];
           }
           for(j = 1; j < M; j++){
          Matrix[u][v] += 1/2.*DCTMatrix[0][j];
           }

           for (i = 1; i < N; i++) {
                for (j = 1; j < M; j++) {
                    Matrix[u][v] += DCTMatrix[i][j] * cos(M_PI/((float)N)*(u+1./2.)*i)*cos(M_PI/((float)M)*(v+1./2.)*j);
                }
            }
        Matrix[u][v] *= 2./((float)N)*2./((float)M);
        }
    }
 }

void* myidct(void* arguments) //(float **Matrix, float **DCTMatrix, int N, int M)
{
    struct dct_struct *args = (dct_struct*)arguments;
    float **DCTMatrix   = args->DCTMatrix;
    float **Matrix      = args->Matrix;
    int N               = args->N;
    int M               = args->M;
    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
          Matrix[u][v] = 1/4.*DCTMatrix[0][0];
          for(i = 1; i < N; i++){
          Matrix[u][v] += 1/2.*DCTMatrix[i][0];
           }
           for(j = 1; j < M; j++){
          Matrix[u][v] += 1/2.*DCTMatrix[0][j];
           }

           for (i = 1; i < N; i++) {
                for (j = 1; j < M; j++) {
                    Matrix[u][v] += DCTMatrix[i][j] * cos(M_PI/((float)N)*(u+1./2.)*i)*cos(M_PI/((float)M)*(v+1./2.)*j);
                }
            }
        Matrix[u][v] *= 2./((float)N)*2./((float)M);
        }
    }
    return NULL;
 }


//***********************************************************************************************************
//new gmpr code developed by Marcos Rodrigues [October 2016] starts here....

string vec2string( vector<int> vec)
{
    //convert input vector to a string stream
    std::stringstream ss;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(ss," "));
    string s = ss.str();
    //replace the trailing space by EOF
    s.replace(s.end()-1,s.end(),1,(char)EOF_CHAR);
    return s;
}

vector<int> string2vec( string sMsg )
{
    vector<int> vOut;
    istringstream is( sMsg );
    int n;
    while( is >> n )
    {
    	vOut.push_back( n );
    }
    return vOut;
}


//Some global variables to facilitate thread functions
std::vector<int>  K(3);              //make key int for lossless text processing. Remove this limitation later on.
std::vector<int>  nData;             //data to compress
std::vector<int>  nLimited;          //list of unique ASCII/UTF characters in the file
std::vector<int>  nCoded;            //coded vector by the triple key
std::vector<vector<int> > nDecoded;  //2D decoded vector by multiple threads
int nThreads;          //how many threads were ever spawn
int nThreadCount;      //how many threads are there at execution time
int nThreadRange;      //range of each thread (how many numbers each thread decodes)

int max_in_vector(std::vector<int> vnData)
{
    int nMaximum = -1; //data is ASCII/UTF, so it has only positive values
    for (int i=0; i<vnData.size(); i++)
    {
        if (vnData[i]>nMaximum) nMaximum = vnData[i];
	 }
    return(nMaximum);
}

bool is_in(int n, std::vector<int> vnData)
{
    bool bResult = false;
    for (int i=0; i<vnData.size(); i++)
    {
        if (n == vnData[i]) bResult = true;
    }
    return bResult;
}

std::vector<int> sort_limited_by_counts( std::vector<int> vnLimited, std::vector<int> vnData) //not used
{
    std::vector<int> counts( vnLimited.size(), 0);

    for (int i=0; i<vnLimited.size(); i++)
    {
        for (int j=0; j<vnData.size(); j++)
        {
            if (vnLimited[i] == vnData[j])
            {
                counts[i]++;
            }
        }
    }
    std::vector<int> counts_sorted = counts;
    std::sort( counts_sorted.rbegin(), counts_sorted.rend() ); //sort descending
    std::vector<int> limited_sorted( vnLimited.size() );
    //nLimitedCounts = counts_sorted;

    int k=0;                                        //find the indices of sorted in vnLimited
    for (int i=0; i<counts_sorted.size(); i++)
    {
        for (int j=0; j<counts.size(); j++)
        {
            if (counts_sorted[i] == counts[j])
            {
                limited_sorted[k] = vnLimited[j];   //j is the index I am looking for
                counts[j] = -1;                     //make j invalid as there is only one occurrence
                k++;
            }
        }
    }

    return limited_sorted;
}

std::vector<int> generate_limited_data(std::vector<int> vnData)
{
    std::vector<int>tmp(256); //max number of possible different characters
    tmp[0]=vnData[0]; int k=1;
    for (int i=1; i<vnData.size(); i++)
    {
        if (!is_in(vnData[i], tmp))
        {
            tmp[k]=vnData[i];
            k++;
        }
    }
    std::vector<int>vnLimited(k);
    for (int i=0; i<k; i++) vnLimited[i]=tmp[i];
    return vnLimited;
}

void generate_key(std::vector<int> vnData, int nStartValue=1, int nFactor=2) //not used
{
    int fMaxValue = 2*max_in_vector(vnData);
    K[0]=nStartValue;
    K[1]=((nStartValue*fMaxValue)+1);
    K[2]=((K[1]*fMaxValue)+1)* nFactor;
}

void generate_random_key(std::vector<int> vnData, int nSeed1=10, int nSeed2=1000)
{
    srand (time(NULL));
    int nStartValue = rand()%nSeed1+1;
    int nFactor = rand()%nSeed1+1;
    int nMaxValue = 2*max_in_vector(vnData);
    K[0]=rand()%nSeed2+1;
    K[1]=((nStartValue*nMaxValue)+1);
    K[2]=((K[1]*nMaxValue)+1) * nFactor;
}

std::vector<int> generate_coded_vector(std::vector<int> vnData)
{
    //this function codes every 3 numbers into one, so need to check length as multiple of 3
    int rem = vnData.size()%3;
    int ext = vnData.size()%3>0?1:0;
    std::vector<int>vnResult(vnData.size()/3+ext);

    int k=0;
    if (rem==0)//lucky me
    {
        for (int i=0; i<vnData.size(); i+=3) //code each set of 3 entries
        {
            vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
            k++;
        }
    }
    else if (rem==1)
    {
       for (int i=0; i<vnData.size(); i+=3)
       {
           if (i+1==vnData.size())
           {
               vnResult[k] = K[0]*vnData[i]; //i is last entry, the set of 3 has only one value
               k++;
           }
            else //compute as normal, we have not reached the last set of 3 entries
            {
                vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
                k++;
            }
       }
    }
    else if (rem==2)
    {
       for (int i=0; i<vnData.size(); i+=3)
       {
           if (i+2==vnData.size())
           {
               vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1]; //last entry, set of 3 has 2 values
               k++;
           }
            else //compute as normal, not reached the last set yet
            {
                vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
                k++;
            }
       }
    }
    return vnResult;
}

std::vector<int> open_raw_file(FILE *fp) //file pointer is already open when this function is called!
{
    long lSize; //number of characters in the file
    char *cbuffer;

    //fopen ( fp ); //the file is already open
    if( !fp ) perror("Error file does not exist."),exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    cbuffer = (char*)calloc( 1, lSize+1 ); // allocate memory for entire content
    if( !cbuffer ) fclose(fp),fputs("Memory alloc fails.",stderr),exit(1);

    if( 1!=fread( cbuffer , lSize, 1 , fp) ) // copy the file into the buffer
      fclose(fp),free(cbuffer),fputs("Entire fread function fails. Something wrong with the file.",stderr),exit(1);

    fclose(fp);

    // 'conversion' to ASCII/UTF happens here, also eliminate negative numbers
    std::vector<int>tmp(lSize);
    int k=0;
    for (int i = 0; i < lSize; i++)
    {
        if (cbuffer[i]>0)
        {
            tmp[k] = cbuffer[i];
            k++;
        }
    }

    std::vector<int>data(k); //k is the correct size of the input vector
    data = tmp; //tmp is longer than data, so tmp is cut off to the size of data
    return data;
}

std::vector<int> open_coded_file(FILE *fp) //fp is already open
{
    long lSize; //number of characters in the file
    char *cbuffer;

    //fopen ( fp ); //the file is already open
    if( !fp ) perror("Error file does not exist."),exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    cbuffer = (char*)calloc( 1, lSize+1 ); // allocate memory for entire content
    if( !cbuffer ) fclose(fp),fputs("Memory alloc fails.",stderr),exit(1);

    if( 1!=fread( cbuffer , lSize, 1 , fp) ) // copy the file into the buffer
      fclose(fp),free(cbuffer),fputs("Entire fread fails.",stderr),exit(1);

    rewind( fp );

    std::vector<int>tmp(lSize);
    int k=0;
    while (!feof (fp))
    {
      fscanf (fp, "%d", &tmp[k]);
      k++;
    }
    fclose (fp);

    std::vector<int>data(k); //k is the correct size of the input vector
    data = tmp;
    return data;
}


void save_coded_file(FILE *fp, std::vector<int> K, std::vector<int> vnLimited, std::vector<int> vnCoded) //not used
{
    //not usedd at the moment
    //save keys, size of limited, size of coded, limited, coded:
    fprintf(fp,"%i\n",K[0]);
    fprintf(fp,"%i\n",K[1]);
    fprintf(fp,"%i\n",K[2]);
    fprintf(fp, "%u\n", (int)vnLimited.size());
    fprintf(fp, "%u\n", (int)vnCoded.size());
    for (int i=0; i<vnLimited.size(); i++) fprintf(fp,"%d ",vnLimited[i]);
    fprintf(fp,"\n");
    for (int i=0; i<vnCoded.size(); i++) fprintf(fp,"%u ",vnCoded[i]);
    fclose(fp);
}

void *decode_vector_segment(void* argument)
{
    int nThisThread = *((int*) argument);
    int nLastIndex  = nCoded.size();
    int nLow        = nThisThread * nThreadRange;
    int nHigh       = (nLow+nThreadRange)>nLastIndex?nLastIndex:(nLow+nThreadRange);

    for (int r=nLow; r<nHigh; r++) //decode nRange numbers
    {
        for (int i=0; i<nLimited.size(); i++)
        {
            for (int j=0; j<nLimited.size(); j++)
            {
                for (int k=0; k<nLimited.size(); k++)
                {
                    if (r != nLastIndex-1)
                    {
                        if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k] ) //found it
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            nDecoded[nThisThread][3*(r-nLow)+2]  = nLimited[k];
                            break;
                        }
                    }
                    else    // There are three possibilities:
                    {
                        if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k] ) //3 entries
                        {
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            nDecoded[nThisThread][3*(r-nLow)+2]  = nLimited[k];
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            break;
                        }
                         else if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j]) //this is the last number coded into 2.
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            break;
                        }
                        else if  (nCoded[r] == K[0]*nLimited[i]) //this is the last number coded into 1.
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            break;
                        }
                    }
                }
            }
        }
    }
    __sync_fetch_and_sub(&nThreadCount,1); //counter is decremented as each thread exits
    return 0L;
}


void decode_vector(std::vector<int> cdata, int nRange=100)
{
    nThreadRange    = nRange; //update global variable
    nThreadCount    = cdata.size()%nThreadRange==0?cdata.size()/nThreadRange:cdata.size()/nThreadRange+1;//how many threads
    int nLastEntry  = cdata.size()%nThreadRange>0?cdata.size()%nThreadRange:nThreadRange;
    nThreads        = nThreadCount; //how many threads will be created
    nDecoded.resize(nThreads);
    for (int i=0; i<nThreads; i++)
    {
        if ( i!= nThreads-1 )
            nDecoded[i].resize(3*nThreadRange);
        else
            nDecoded[i].resize(3*nLastEntry);
    }

    pthread_t *tr[nThreads]; //array of thread ids
    for (int i=0; i<nThreads; i++)
    {
        pthread_create((pthread_t *)&tr[i], NULL, &decode_vector_segment, (void*)&i);
        waitKey(1);
    }
    do
    {
        __sync_synchronize();
    } while (nThreadCount);
    return;
}


int batch_encode(char* dirin, char* dirout, const model_t model)
{
    char filein[256];
    char fileout[256];
    int  counter = 0;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) != NULL)
    {
        while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
        {
            char* name= ent->d_name;
            if (name[0]==46) //igore filenames starting with . (46='.')
            {
                counter+=0; //do nothing
            }
            else
            {
                memset(filein, 0, sizeof filein);//clear array
                strcpy(filein,dirin);            // copy string one into the result.
                strcat(filein,"/");              // append string two to the result.
                strcat(filein,name);
                FILE *fpin;
                fpin = fopen(filein, "rb");
                nData    = open_raw_file( fpin ); //this file is closed within open_file function
                nLimited = generate_limited_data(nData);
                nLimited = sort_limited_by_counts( nLimited, nData);
                generate_random_key(nData);
                nCoded   = generate_coded_vector(nData);

                std::string s = name;
                int Len = s.length();
                name[Len-1]='c'; //add c to file extention
                name[Len]='.';
                memset(fileout, 0, sizeof fileout); //clear array
                strcpy(fileout,dirout);             // copy string one into the result.
                strcat(fileout,"/");                // append string two to the result.
                strcat(fileout,name);
                FILE *fpout;
                fpout = fopen(fileout, "w");
                //save_coded_file(fpout, K, nLimited, nCoded);//this file is closed within save_coded function

                vector<int> coded_vector = K;
                coded_vector.push_back( nLimited.size());
                coded_vector.push_back( nCoded.size());
                coded_vector.insert(coded_vector.end(), nLimited.begin(), nLimited.end());
                coded_vector.insert(coded_vector.end(), nCoded.begin(), nCoded.end());
                string sMsg = vec2string( coded_vector );
                //printf("%s", sMsg.c_str());
                myArEncodeString(sMsg, fpout, model);
                fclose(fpout);
                counter++;
            }
        }
        closedir (dir);
        //cout<<"\nEncoded "<<counter<<" files."<<"\n";
        //cout<<"source     : "<<dirin<<"\n";
        //cout<<"destination: "<<dirout<<"\n";
        return 0;
    }
    else
    {
        return -1; //failure
    }
}

int batch_decode(char* dirin, char* dirout, const model_t model)
{
    char filein[256];
    char fileout[256];
    int  counter = 0;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) != NULL)
    {
        while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
        {
            char* name= ent->d_name;
            if (name[0]==46) //igore filenames starting with . (46='.')
            {
                //do nothing
            }
            else
            {
                memset(filein, 0, sizeof filein);//clear array
                strcpy(filein,dirin);            // copy string one into the result.
                strcat(filein,"/");              // append string two to the result.
                strcat(filein,name);
                FILE *fpin;
                fpin = fopen(filein, "rb");
                string sMsg = myArDecodeFile(fpin, model);
                fclose(fpin);
                std::vector<int> data = string2vec( sMsg );
                //std::vector<int>data = open_coded_file( fpin );    //this file is closed within open_file function
                K[0]     = data[0];
                K[1]     = data[1];
                K[2]     = data[2];
                int nL   = data[3];
                int nC   = data[4];
                //cout<<"\nK[0],K[1],K[2]: "<<K[0]<<" "<<K[1]<<" "<<K[2]<<" nL="<<nL<<" nC="<<nC<<"\n";
                int iFirst = 5;    //first index to copy to limited data
                int iLast  = 5+nL; //last index + 1
                int iLen   = iLast - iFirst;

                nLimited.resize(iLen); //pre-allocate the space needed to write the data directly
                memcpy(&nLimited[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer
                //cout<<"\nnlimite[0]: "<<nLimited[0];
                //cout<<"\nnLimited[last]: "<<nLimited[nLimited.size()-1];

                iFirst = 5+nL;
                iLast  = 5+nL+nC;
                iLen   = iLast - iFirst;
                nCoded.resize(iLen);
                //cod.resize(iLen); //pre-allocate the space needed to write the data directly
                memcpy(&nCoded[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer
                //for (int i=0; i<cdata.size(); i++) printf("%d ",cdata[i]);
                decode_vector( nCoded );

                std::string s = name;
                int Len = s.length();
                name[Len-2]='d'; //add d to file extention
                memset(fileout, 0, sizeof fileout); //clear array
                strcpy(fileout,dirout);             // copy string one into the result.
                strcat(fileout,"/");                // append string two to the result.
                strcat(fileout,name);
                //cout<<"\nfile:"<<name<<" nThreads:"<<nThreads;

                int N=nThreads;
                //cout<<"\nfileout: "<<fileout;
                FILE *fpout;
                fpout = fopen(fileout, "w");
                for (int i=0; i<N; i++)
                {
                    for (int j=0; j<nDecoded[i].size(); j++)
                    {
                        fprintf(fpout,"%c", nDecoded[i][j]);
                        //printf("%d", nDecoded[i][j]);
                    }
                }
                fclose(fpout);

                counter++;
            }
        }
        closedir (dir);
        //cout<<"\nDecoded "<<counter<<" files."<<"\n";
        //cout<<"source     : "<<dirin<<"\n";
        //cout<<"destination: "<<dirout<<"\n";
        return 0;
    }
    else
    {
        return -1; //failure
    }
}


//*******************************************************************************
int main(int argc, char *argv[])
{
    printf("Hello world!\n");
    //run from the command line:
    // ./ac -c -i infile -o outfile
    // ./ac -d -i infile -o outfile
    // ./ac -? for help

    option_t *optList, *thisOpt;
    FILE *inFile, *outFile; /* input & output files */
    char encode;            /* encode/decode */
    model_t model;          /* static/adaptive model*/

    /* initialize data */
    inFile = NULL;
    outFile = NULL;
    encode = TRUE;
    model = MODEL_STATIC;

//#define BATCH //run this as a batch processing, comment this out for command line
#if !defined(BATCH) //check command line
    /* parse command line */
    optList = GetOptList(argc, argv, (char*)"acdi:o:h?");
    thisOpt = optList;

    while (thisOpt != NULL)
    {
        switch(thisOpt->option)
        {
            case 'a':       /* adaptive model vs. static */
                model = MODEL_ADAPTIVE;
                break;

            case 'c':       /* compression mode */
                encode = TRUE;
                break;

            case 'd':       /* decompression mode */
                encode = FALSE;
                break;

            case 'i':       /* input file name */
                if (inFile != NULL)
                {
                    fprintf(stderr, "Multiple input files not allowed.\n");
                    fclose(inFile);

                    if (outFile != NULL)
                    {
                        fclose(outFile);
                    }

                    FreeOptList(optList);
                    exit(EXIT_FAILURE);
                }
                else if ((inFile = fopen(thisOpt->argument, "rb")) == NULL)
                {
                    perror("Opening Input File");

                    if (outFile != NULL)
                    {
                        fclose(outFile);
                    }

                    FreeOptList(optList);
                    exit(EXIT_FAILURE);
                }

                break;

            case 'o':       /* output file name */
                if (outFile != NULL)
                {
                    fprintf(stderr, "Multiple output files not allowed.\n");
                    fclose(outFile);

                    if (inFile != NULL)
                    {
                        fclose(inFile);
                    }

                    FreeOptList(optList);
                    exit(EXIT_FAILURE);
                }
                else if ((outFile = fopen(thisOpt->argument, "wb")) == NULL)
                {
                    perror("Opening Output File");

                    if (inFile != NULL)
                    {
                        fclose(inFile);
                    }

                    FreeOptList(optList);
                    exit(EXIT_FAILURE);
                }

                break;

            case 'h':
            case '?':
                printf("Usage: %s <options>\n\n", FindFileName(argv[0]));
                printf("options:\n");
                printf("  -c : Encode input file to output file.\n");
                printf("  -d : Decode input file to output file.\n");
                printf("  -i <filename> : Name of input file.\n");
                printf("  -o <filename> : Name of output file.\n");
                printf("  -a : Use adaptive model instead of static.\n");
                printf("  -h | ?  : Print out command line options.\n\n");
                printf("Default: %s -c\n", FindFileName(argv[0]));

                FreeOptList(optList);
                return(EXIT_SUCCESS);
        }

        optList = thisOpt->next;
        free(thisOpt);
        thisOpt = optList;
    }

    /* validate command line */
    if (NULL == inFile)
    {
        fprintf(stderr, "Input file must be provided\n");
        fprintf(stderr, "Enter \"%s -?\" for help.\n", FindFileName(argv[0]));

        if (outFile != NULL)
        {
            fclose(outFile);
        }

        exit (EXIT_FAILURE);
    }
    else if (NULL == outFile)
    {
        fprintf(stderr, "Output file must be provided\n");
        fprintf(stderr, "Enter \"%s -?\" for help.\n", FindFileName(argv[0]));

        if (inFile != NULL)
        {
            fclose(inFile);
        }

        exit (EXIT_FAILURE);
    }

    if (encode) //we want to encode a file, single threaded always
    {
        nData = open_raw_file(inFile);
        nLimited = generate_limited_data(nData);
        generate_random_key(nData);
        nCoded=generate_coded_vector(nData);
        vector<int> coded_vector = K;
        cout << "\nK[0], K[1], K[2] = " << K[0] << " " <<K[1] <<" " <<K[2] <<"\n";
        coded_vector.push_back( nLimited.size());
        coded_vector.push_back( nCoded.size());
        coded_vector.insert(coded_vector.end(), nLimited.begin(), nLimited.end());
        coded_vector.insert(coded_vector.end(), nCoded.begin(), nCoded.end());
        string sMsg = vec2string( coded_vector );
        myArEncodeString(sMsg, outFile, model); //arithmetic code the encoded vector, save to file
        fclose(outFile);
    }
    else //decode using multiple threads
    {
        struct timeval tic, toc; gettimeofday(&tic,NULL);
        string sMsg = myArDecodeFile(inFile, model);
        fclose(inFile);
        std::vector<int> data = string2vec( sMsg );
        K[0]     = data[0];
        K[1]     = data[1];
        K[2]     = data[2];
        int nL   = data[3];
        int nC   = data[4];
        int iFirst = 5;    //first index to copy to limited data
        int iLast  = 5+nL; //last index + 1
        int iLen   = iLast - iFirst;

        nLimited.resize(iLen); //pre-allocate the space needed to write the data directly
        memcpy(&nLimited[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer

        iFirst = iLast; //5+nL
        iLast  = iFirst+nC;//5+nL+nC;
        iLen   = iLast - iFirst;
        nCoded.resize(iLen); //pre-allocate the space needed to write the data directly
        memcpy(&nCoded[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer

        decode_vector( nCoded, 25 ); //multiple threads here each thread decodes 100 numbers
        for (int i=0; i<nThreads; i++)
        {
            for (int j=0; j<nDecoded[i].size(); j++)
            {
                fprintf(outFile,"%c",nDecoded[i][j]); //save decoded file
            }
        }
        fclose(outFile);

        gettimeofday(&toc,NULL);
        printf ("Total time = %f seconds\n",
            (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            (double) (toc.tv_sec - tic.tv_sec));
    }

    return 0;
#endif //if !defined(BATCH)

    struct timeval tic, toc;
    gettimeofday(&tic,NULL);

    //if we are here, then we will be  coding and decoding in batch, all files from directory dirin
    char *dirin  = (char*)"/Users/mar/Caco/Projects/Compress/ce/test";
    char *dirout = (char*)"/Users/mar/Caco/Projects/Compress/ce/testc";
    batch_encode( dirin, dirout, model);
    dirin  = (char*)"/Users/mar/Caco/Projects/Compress/ce/testc";
    dirout = (char*)"/Users/mar/Caco/Projects/Compress/ce/testd";
    batch_decode( dirin, dirout, model );

    gettimeofday(&toc,NULL);
    printf ("Total time = %f seconds\n", (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            (double) (toc.tv_sec - tic.tv_sec));
    return 0;


}
