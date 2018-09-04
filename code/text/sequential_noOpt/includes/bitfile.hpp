#ifndef BITFILE_HPP
#define BITFILE_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct bit_file_t bit_file_t;
typedef int (*num_func_t)(bit_file_t*, void*, const unsigned int, const size_t);

typedef enum
{
    BF_READ = 0,
    BF_WRITE = 1,
    BF_APPEND= 2,
    BF_NO_MODE
} 	BF_MODES;

typedef enum
{
    BF_UNKNOWN_ENDIAN,
    BF_LITTLE_ENDIAN,
    BF_BIG_ENDIAN
} 	endian_t;

typedef union
{
    unsigned long word;
    unsigned char bytes[sizeof(unsigned long)];
} 	endian_test_t;

struct bit_file_t
{
    FILE *fp;
    unsigned char bitBuffer;
    unsigned char bitCount;
    num_func_t PutBitsNumFunc;
    num_func_t GetBitsNumFunc;
    BF_MODES mode;
};
/***************************************************************************
*                               FUNCTIONS
***************************************************************************/
static endian_t DetermineEndianess();
static int BitFilePutBitsLE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
static int BitFilePutBitsBE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
static int BitFileGetBitsLE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
static int BitFileGetBitsBE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
static int BitFileNotSupported(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
bit_file_t *BitFileOpen(const char *fileName, const BF_MODES mode);
bit_file_t *MakeBitFile(FILE *stream, const BF_MODES mode);
int BitFileClose(bit_file_t *stream);
FILE *BitFileToFILE(bit_file_t *stream);
int BitFileByteAlign(bit_file_t *stream);
int BitFileFlushOutput(bit_file_t *stream, const unsigned char onesFill);
int BitFileGetChar(bit_file_t *stream);
int BitFilePutChar(const int c, bit_file_t *stream);
int BitFileGetBit(bit_file_t *stream);
int BitFilePutBit(const int c, bit_file_t *stream);
int BitFileGetBits(bit_file_t *stream, void *bits, const unsigned int count);
int BitFilePutBits(bit_file_t *stream, void *bits, const unsigned int count);
int BitFileGetBitsNum(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);
int BitFilePutBitsNum(bit_file_t *stream, void *bits, const unsigned int count, const size_t size);

#endif