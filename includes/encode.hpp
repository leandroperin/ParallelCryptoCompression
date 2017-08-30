#ifndef ENCODE_HPP
#define ENCODE_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <limits.h>
#include <string.h>
#include <assert.h>
#include "bitfile.hpp"

#ifdef NDEBUG
#define PrintDebug(ARGS) do {} while (0)
#else
#define PrintDebug(ARGS) printf ARGS
#endif
/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define EOF_CHAR (UCHAR_MAX + 1)
#define PRECISION (8 * sizeof(probability_t))
#define MAX_PROBABILITY (1 << (PRECISION - 2))
/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef unsigned short probability_t; 

typedef enum
{
    MODEL_ADAPTIVE = 0,
    MODEL_STATIC = 1
} 	model_t;

typedef struct
{
    probability_t ranges[EOF_CHAR + 2];
    probability_t cumulativeProb;
    probability_t lower;
    probability_t upper;
    probability_t code;
    unsigned char underflowBits;
} 	stats_t;
/***************************************************************************
*                                  MACROS
***************************************************************************/
#define MASK_BIT(x) (probability_t)(1 << (PRECISION - (1 + (x))))
#define LOWER(c) (c)
#define UPPER(c) ((c) + 1)
/***************************************************************************
*                               FUNCTIONS
***************************************************************************/
static void WriteHeader(bit_file_t *bfpOut, stats_t *stats);
static void ApplySymbolRange(int symbol, stats_t *stats, char model);
static void SymbolCountToProbabilityRanges(stats_t *stats);
static void WriteEncodedBits(bit_file_t *bfpOut, stats_t *stats);
static void WriteRemaining(bit_file_t *bfpOut, stats_t *stats);
static int BuildProbabilityRangeList(FILE *fpIn, stats_t *stats);
static void InitializeAdaptiveProbabilityRangeList(stats_t *stats);
int ArEncodeFile(FILE *inFile, FILE *outFile, const model_t model);

#endif