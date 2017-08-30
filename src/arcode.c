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
* Copyright (C) 2004, 2006-2007, 2014, 2017 by
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
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "../includes/arcode.h"
#include "../includes/bitfile.h"

#if !(USHRT_MAX < ULONG_MAX)
#error "Implementation requires USHRT_MAX < ULONG_MAX"
#endif

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* read write file headers */
static int ReadHeader(bit_file_t *bfpIn, stats_t *stats);

/* routines for decoding */
static void InitializeDecoder(bit_file_t *bfpOut, stats_t *stats);
static probability_t GetUnscaledCode(stats_t *stats);
static int GetSymbolFromProbability(probability_t probability, stats_t *stats);
static void ReadEncodedBits(bit_file_t *bfpIn, stats_t *stats);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

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

    PrintDebug(("HEADER:\n"));
    stats->cumulativeProb = 0;

    memset(stats->ranges, 0, sizeof(stats->ranges));

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

        PrintDebug(("%02X\t%d\n", c, count));

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
