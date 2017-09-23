#include "../includes/encode.hpp"

static void WriteHeader(bit_file_t *bfpOut, stats_t *stats)
{
    int c;
    probability_t previous = 0;

    PrintDebug(("HEADER:\n"));

    for(c = 0; c <= (EOF_CHAR - 1); c++)
    {
        if (stats->ranges[UPPER(c)] > previous)
        {
            BitFilePutChar((char)c, bfpOut);
            previous = (stats->ranges[UPPER(c)] - previous);
            PrintDebug(("%02X\t%d\n", c, previous));

            BitFilePutBitsNum(bfpOut, &previous, (PRECISION - 2), sizeof(probability_t));

            previous = stats->ranges[UPPER(c)];
        }
    }

    BitFilePutChar(0x00, bfpOut);
    previous = 0;
    BitFilePutBits(bfpOut, (void *)&previous, PRECISION - 2);
}

static void ApplySymbolRange(int symbol, stats_t *stats, char model)
{
    unsigned long range;
    unsigned long rescaled;
    int i;
    probability_t original;
    probability_t delta;

    range = (unsigned long)(stats->upper - stats->lower) + 1;

    rescaled = (unsigned long)(stats->ranges[UPPER(symbol)]) * range;
    rescaled /= (unsigned long)(stats->cumulativeProb);

    stats->upper = stats->lower + (probability_t)rescaled - 1;

    rescaled = (unsigned long)(stats->ranges[LOWER(symbol)]) * range;
    rescaled /= (unsigned long)(stats->cumulativeProb);

    stats->lower = stats->lower + (probability_t)rescaled;

    if (!model)
    {
        stats->cumulativeProb++;

        for (i = UPPER(symbol); i <= UPPER(EOF_CHAR); i++)
            stats->ranges[i] += 1;

        if (stats->cumulativeProb >= MAX_PROBABILITY)
        {
            stats->cumulativeProb = 0;
            original = 0;

            for (i = 1; i <= UPPER(EOF_CHAR); i++)
            {
                delta = stats->ranges[i] - original;
                original = stats->ranges[i];

                if (delta <= 2)
                    stats->ranges[i] = stats->ranges[i - 1] + 1;
                else
                    stats->ranges[i] = stats->ranges[i - 1] + (delta / 2);

                stats->cumulativeProb += (stats->ranges[i] - stats->ranges[i - 1]);
            }
        }
    }

    assert(stats->lower <= stats->upper);
}

static void SymbolCountToProbabilityRanges(stats_t *stats)
{
    int c;

    stats->ranges[0] = 0;
    stats->ranges[UPPER(EOF_CHAR)] = 1;
    stats->cumulativeProb++;

    for (c = 1; c <= UPPER(EOF_CHAR); c++)
        stats->ranges[c] += stats->ranges[c - 1];

    PrintDebug(("Ranges:\n"));
    for (c = 0; c < UPPER(EOF_CHAR); c++)
        PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)], stats->ranges[UPPER(c)]));

    return;
}

static int BuildProbabilityRangeList(std::string sMsg, stats_t *stats) {
    int c;
    unsigned long countArray[EOF_CHAR];
    unsigned long totalCount = 0;
    unsigned long rescaleValue;

    for (c = 0; c < EOF_CHAR; c++) {
        countArray[c] = 0;
    }

    for (std::string::iterator it = sMsg.begin(); it < sMsg.end(); ++it) {
        if (totalCount == ULONG_MAX) {
            fprintf(stderr, "Error: file too large\n");
            return -1;
        }

        countArray[(int)*it]++;
        totalCount++;
    }

    if (totalCount >= MAX_PROBABILITY) {
        rescaleValue = (totalCount / MAX_PROBABILITY) + 1;

        for (c = 0; c < EOF_CHAR; c++) {
            if (countArray[c] > rescaleValue)
                countArray[c] /= rescaleValue;
            else if (countArray[c] != 0)
                countArray[c] = 1;
        }
    }

    stats->ranges[0] = 0;
    stats->cumulativeProb = 0;

    for (c = 0; c < EOF_CHAR; c++) {
        stats->ranges[UPPER(c)] = countArray[c];
        stats->cumulativeProb += countArray[c];
    }

    SymbolCountToProbabilityRanges(stats);
    return 0;
}

static void WriteEncodedBits(bit_file_t *bfpOut, stats_t *stats)
{
    while (true)
    {
        if ((stats->upper & MASK_BIT(0)) == (stats->lower & MASK_BIT(0)))
        {
            BitFilePutBit((stats->upper & MASK_BIT(0)) != 0, bfpOut);

            while (stats->underflowBits > 0)
            {
                BitFilePutBit((stats->upper & MASK_BIT(0)) == 0, bfpOut);
                stats->underflowBits--;
            }
        }
        else if ((stats->lower & MASK_BIT(1)) && !(stats->upper & MASK_BIT(1)))
        {
            stats->underflowBits += 1;
            stats->lower &= ~(MASK_BIT(0) | MASK_BIT(1));
            stats->upper |= MASK_BIT(1);
        }
        else
            return;

        stats->lower <<= 1;
        stats->upper <<= 1;
        stats->upper |= 1;
    }
}

static void WriteRemaining(bit_file_t *bfpOut, stats_t *stats)
{
    BitFilePutBit((stats->lower & MASK_BIT(1)) != 0, bfpOut);

    for (stats->underflowBits++; stats->underflowBits > 0; stats->underflowBits--)
        BitFilePutBit((stats->lower & MASK_BIT(1)) == 0, bfpOut);
}

static void InitializeAdaptiveProbabilityRangeList(stats_t *stats)
{
    int c;

    stats->ranges[0] = 0;

    for (c = 1; c <= UPPER(EOF_CHAR); c++)
        stats->ranges[c] = stats->ranges[c - 1] + 1;

    stats->cumulativeProb = UPPER(EOF_CHAR);

    PrintDebug(("Ranges:\n"));
    for (c = 0; c < UPPER(EOF_CHAR); c++)
        PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)], stats->ranges[UPPER(c)]));

    return;
}

int ArEncodeString(std::string sMsg, FILE* outFile, const model_t model)
{
    int c;
    bit_file_t *bOutFile;
    stats_t stats;

    if (outFile == NULL)
        bOutFile = MakeBitFile(stdout, BF_WRITE);
    else
        bOutFile = MakeBitFile(outFile, BF_WRITE);

    if (NULL == bOutFile) {
        fprintf(stderr, "Error: Creating binary output file\n");
        return -1;
    }

    if (MODEL_STATIC == model) {
        if (0 != BuildProbabilityRangeList(sMsg, &stats))
        {
            BitFileClose(bOutFile);
            fprintf(stderr, "Error determining frequency ranges.\n");
            return -1;
        }

        WriteHeader(bOutFile, &stats);
    }
    else
        InitializeAdaptiveProbabilityRangeList(&stats);

    stats.lower = 0;
    stats.upper = ~0;
    stats.underflowBits = 0;

    for (std::string::iterator it = sMsg.begin(); it < sMsg.end(); ++it) {
        c=*it;
        ApplySymbolRange(c, &stats, model);
        WriteEncodedBits(bOutFile, &stats);
    }

    ApplySymbolRange(EOF_CHAR, &stats, model);
    WriteEncodedBits(bOutFile, &stats);
    WriteRemaining(bOutFile, &stats);
    outFile = BitFileToFILE(bOutFile);

    return 0;
}
