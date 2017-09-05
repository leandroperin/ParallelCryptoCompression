#ifndef ENCODE_HPP
#define ENCODE_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "consts.hpp"
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