#ifndef DECODE_HPP
#define DECODE_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "consts.hpp"
/***************************************************************************
*                               FUNCTIONS
***************************************************************************/
static int ReadHeader(bit_file_t *bfpIn, stats_t *stats);
static void ApplySymbolRange(int symbol, stats_t *stats, char model);
static void SymbolCountToProbabilityRanges(stats_t *stats);
static void InitializeDecoder(bit_file_t *bfpOut, stats_t *stats);
static probability_t GetUnscaledCode(stats_t *stats);
static int GetSymbolFromProbability(probability_t probability, stats_t *stats);
static void ReadEncodedBits(bit_file_t *bfpIn, stats_t *stats);
static void InitializeAdaptiveProbabilityRangeList(stats_t *stats);
std::string ArDecodeFile(FILE *inFile, const model_t model);

#endif
