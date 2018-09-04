#ifndef ENCODE_HPP
#define ENCODE_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "consts.hpp"
#include <string>
/***************************************************************************
*                               FUNCTIONS
***************************************************************************/
static void WriteHeader(bit_file_t *bfpOut, stats_t *stats);
static void ApplySymbolRange(int symbol, stats_t *stats, char model);
static void SymbolCountToProbabilityRanges(stats_t *stats);
static int BuildProbabilityRangeList(std::string sMsg, stats_t *stats);
static void WriteEncodedBits(bit_file_t *bfpOut, stats_t *stats);
static void WriteRemaining(bit_file_t *bfpOut, stats_t *stats);
static void InitializeAdaptiveProbabilityRangeList(stats_t *stats);
int ArEncodeString(std::string sMsg, FILE* outFile, const model_t model);

#endif
