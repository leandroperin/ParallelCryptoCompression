#include "../includes/bitfile.hpp"

static endian_t DetermineEndianess()
{
    endian_test_t endianTest;
    endianTest.word = 1;

    if (endianTest.bytes[0] == 1)
        return BF_LITTLE_ENDIAN;
    if (endianTest.bytes[sizeof(unsigned long) - 1] == 1)
        return BF_BIG_ENDIAN;

    return BF_UNKNOWN_ENDIAN;
}

static int BitFilePutBitsLE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
	(void)size;

    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;
    offset = 0;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
            return EOF;

        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        tmp = bytes[offset];
        tmp <<= (8 - remaining);

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
                return EOF;

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

static int BitFilePutBitsBE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
	if (count > (size * 8))
        return EOF;

    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;
    offset = size - 1;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
            return EOF;

        remaining -= 8;
        offset--;
    }

    if (remaining != 0)
    {
        tmp = bytes[offset];
        tmp <<= (8 - remaining);

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
                return EOF;

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

static int BitFileGetBitsLE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
	(void)size;

    unsigned char *bytes;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;
    offset = 0;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
            return EOF;

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
                return EOF;

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

    }

    return count;
}

static int BitFileGetBitsBE(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
	if (count > (size * 8))
        return EOF;

    unsigned char *bytes;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;
    offset = size - 1;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
            return EOF;

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset--;
    }

    if (remaining != 0)
    {
        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
                return EOF;

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

    }

    return count;
}

static int BitFileNotSupported(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
    (void)stream;
    (void)bits;
    (void)count;
    (void)size;

    return -ENOTSUP;
}

bit_file_t *BitFileOpen(const char *fileName, const BF_MODES mode)
{
    const char modes[3][3] = {"rb", "wb", "ab"};
    bit_file_t *bf;

    bf = (bit_file_t *) malloc(sizeof(bit_file_t));

    if (bf == NULL)
        errno = ENOMEM;
    else
    {
        bf->fp = fopen(fileName, modes[mode]);

        if (bf->fp == NULL)
        {
            free(bf);
            bf = NULL;
        }
        else
        {
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

                default:
                    bf->PutBitsNumFunc = BitFileNotSupported;
                    bf->GetBitsNumFunc = BitFileNotSupported;
                    break;
            }
        }
    }

    return bf;
}

bit_file_t *MakeBitFile(FILE *stream, const BF_MODES mode)
{
    bit_file_t *bf;

    if (stream == NULL)
    {
        errno = EBADF;
        bf = NULL;
    }
    else
    {
        bf = (bit_file_t *) malloc(sizeof(bit_file_t));

        if (bf == NULL)
            errno = ENOMEM;
        else
        {
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

                default:
                    bf->PutBitsNumFunc = BitFileNotSupported;
                    bf->GetBitsNumFunc = BitFileNotSupported;
                    break;
            }
        }
    }

    return bf;
}

int BitFileClose(bit_file_t *stream)
{
    int returnValue = 0;

    if (stream == NULL)
        return EOF;

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);
        }
    }

    returnValue = fclose(stream->fp);

    free(stream);

    return returnValue;
}

FILE *BitFileToFILE(bit_file_t *stream)
{
    FILE *fp = NULL;

    if (stream == NULL)
        return NULL;

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);
        }
    }

    fp = stream->fp;

    free(stream);

    return fp;
}

int BitFileByteAlign(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
        return EOF;

    returnValue = stream->bitBuffer;

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);
        }
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return returnValue;
}

int BitFileFlushOutput(bit_file_t *stream, const unsigned char onesFill)
{
    int returnValue;

    if (stream == NULL)
        return EOF;

    returnValue = -1;

    if (stream->bitCount != 0)
    {
        stream->bitBuffer <<= (8 - stream->bitCount);

        if (onesFill)
            stream->bitBuffer |= (0xFF >> stream->bitCount);

        returnValue = fputc(stream->bitBuffer, stream->fp);
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return (returnValue);
}

int BitFileGetChar(bit_file_t *stream)
{
    int returnValue;
    unsigned char tmp;

    if (stream == NULL)
        return EOF;

    returnValue = fgetc(stream->fp);

    if (stream->bitCount == 0)
        return returnValue;

    if (returnValue != EOF)
    {
        tmp = ((unsigned char)returnValue) >> (stream->bitCount);
        tmp |= ((stream->bitBuffer) << (8 - (stream->bitCount)));

        stream->bitBuffer = returnValue;

        returnValue = tmp;
    }

    return returnValue;
}

int BitFilePutChar(const int c, bit_file_t *stream)
{
    unsigned char tmp;

    if (stream == NULL)
        return EOF;

    if (stream->bitCount == 0)
        return fputc(c, stream->fp);

    tmp = ((unsigned char)c) >> (stream->bitCount);
    tmp = tmp | ((stream->bitBuffer) << (8 - stream->bitCount));

    if (fputc(tmp, stream->fp) != EOF)
        stream->bitBuffer = c;
    else
        return EOF;

    return tmp;
}

int BitFileGetBit(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
        return EOF;

    if (stream->bitCount == 0)
    {
        if ((returnValue = fgetc(stream->fp)) == EOF)
            return EOF;
        else
        {
            stream->bitCount = 8;
            stream->bitBuffer = returnValue;
        }
    }

    stream->bitCount--;
    returnValue = (stream->bitBuffer) >> (stream->bitCount);

    return returnValue & 0x01;
}

int BitFilePutBit(const int c, bit_file_t *stream)
{
    int returnValue = c;

    if (stream == NULL)
        return EOF;

    stream->bitCount++;
    stream->bitBuffer <<= 1;

    if (c != 0)
        stream->bitBuffer |= 1;

    if (stream->bitCount == 8)
    {
        if (fputc(stream->bitBuffer, stream->fp) == EOF)
            returnValue = EOF;

        stream->bitCount = 0;
        stream->bitBuffer = 0;
    }

    return returnValue;
}

int BitFileGetBits(bit_file_t *stream, void *bits, const unsigned int count)
{
    unsigned char *bytes, shifts;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;

    if ((stream == NULL) || (bits == NULL))
        return EOF;

    offset = 0;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
            return EOF;

        bytes[offset] = (unsigned char)returnValue;
        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        shifts = 8 - remaining;
        bytes[offset] = 0;

        while (remaining > 0)
        {
            returnValue = BitFileGetBit(stream);

            if (returnValue == EOF)
                return EOF;

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
            remaining--;
        }

        bytes[offset] <<= shifts;
    }

    return count;
}

int BitFilePutBits(bit_file_t *stream, void *bits, const unsigned int count)
{
    unsigned char *bytes, tmp;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;

    if ((stream == NULL) || (bits == NULL))
        return EOF;

    offset = 0;
    remaining = count;

    while (remaining >= 8)
    {
        returnValue = BitFilePutChar(bytes[offset], stream);

        if (returnValue == EOF)
            return EOF;

        remaining -= 8;
        offset++;
    }

    if (remaining != 0)
    {
        tmp = bytes[offset];

        while (remaining > 0)
        {
            returnValue = BitFilePutBit((tmp & 0x80), stream);

            if (returnValue == EOF)
                return EOF;

            tmp <<= 1;
            remaining--;
        }
    }

    return count;
}

int BitFileGetBitsNum(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
    if ((stream == NULL) || (bits == NULL))
        return EOF;

    if (NULL == stream->GetBitsNumFunc)
        return -ENOTSUP;

    return (stream->GetBitsNumFunc)(stream, bits, count, size);
}

int BitFilePutBitsNum(bit_file_t *stream, void *bits, const unsigned int count, const size_t size)
{
    if ((stream == NULL) || (bits == NULL))
        return EOF;

    if (NULL == stream->PutBitsNumFunc)
        return ENOTSUP;

    return (stream->PutBitsNumFunc)(stream, bits, count, size);
}