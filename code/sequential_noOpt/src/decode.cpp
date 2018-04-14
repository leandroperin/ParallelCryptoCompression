#include "../includes/decode.hpp"

static int ReadHeader(bit_file_t *bfpIn, stats_t *stats)
{
    int c;
    probability_t count;

    // PrintDebug(("HEADER:\n"));
    stats->cumulativeProb = 0;

    memset(stats->ranges, 0, sizeof(stats->ranges));

    while (true)
    {
        c = BitFileGetChar(bfpIn);
        count = 0;

        if (BitFileGetBitsNum(bfpIn, &count, (PRECISION - 2), sizeof(probability_t)) == EOF)
        {
            fprintf(stderr, "Error: unexpected EOF\n");
            return -1;
        }

        // PrintDebug(("%02X\t%d\n", c, count));

        if (count == 0)
            break;

        stats->ranges[UPPER(c)] = count;
        stats->cumulativeProb += count;
    }

    SymbolCountToProbabilityRanges(stats);
    return 0;
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

    // PrintDebug(("Ranges:\n"));
    // for (c = 0; c < UPPER(EOF_CHAR); c++)
        // PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)], stats->ranges[UPPER(c)]));

    return;
}

static void InitializeDecoder(bit_file_t *bfpIn, stats_t *stats)
{
    int i;

    stats->code = 0;

    for (i = 0; i < (int)PRECISION; i++)
    {
        stats->code <<= 1;

        if(BitFileGetBit(bfpIn) == 1)
            stats->code |= 1;
    }

    stats->lower = 0;
    stats->upper = ~0;
}

static probability_t GetUnscaledCode(stats_t *stats)
{
    unsigned long range;
    unsigned long unscaled;

    range = (unsigned long)(stats->upper - stats->lower) + 1;

    unscaled = (unsigned long)(stats->code - stats->lower) + 1;
    unscaled = unscaled * (unsigned long)(stats->cumulativeProb) - 1;
    unscaled /= range;

    return ((probability_t)unscaled);
}

static int GetSymbolFromProbability(probability_t probability, stats_t *stats)
{
    int first, last, middle;

    first = 0;
    last = UPPER(EOF_CHAR);
    middle = last / 2;

    while (last >= first)
    {
        if (probability < stats->ranges[LOWER(middle)])
        {
            last = middle - 1;
            middle = first + ((last - first) / 2);
            continue;
        }

        if (probability >= stats->ranges[UPPER(middle)])
        {
            first = middle + 1;
            middle = first + ((last - first) / 2);
            continue;
        }

        return middle;
    }

    fprintf(stderr, "Unknown Symbol: %d (max: %d)\n", probability, stats->ranges[UPPER(EOF_CHAR)]);
    return -1;
}

static void ReadEncodedBits(bit_file_t *bfpIn, stats_t *stats)
{
    int nextBit;

    while (true)
    {
      if ((stats->upper & MASK_BIT(0)) != (stats->lower & MASK_BIT(0))) {
    		if ((stats->lower & MASK_BIT(1)) && !(stats->upper & MASK_BIT(1)))
    		{
    			stats->lower &= ~(MASK_BIT(0) | MASK_BIT(1));
            	stats->upper |= MASK_BIT(1);
            	stats->code ^= MASK_BIT(1);
    		}
    		else
    			return;
      }

        stats->lower <<= 1;
        stats->upper <<= 1;
        stats->upper |= 1;
        stats->code <<= 1;

        if ((nextBit = BitFileGetBit(bfpIn)) != EOF)
            stats->code |= nextBit;
    }
}

static void InitializeAdaptiveProbabilityRangeList(stats_t *stats)
{
    int c;

    stats->ranges[0] = 0;

    for (c = 1; c <= UPPER(EOF_CHAR); c++)
        stats->ranges[c] = stats->ranges[c - 1] + 1;

    stats->cumulativeProb = UPPER(EOF_CHAR);

    // PrintDebug(("Ranges:\n"));
    // for (c = 0; c < UPPER(EOF_CHAR); c++)
        // PrintDebug(("%02X\t%d\t%d\n", c, stats->ranges[LOWER(c)], stats->ranges[UPPER(c)]));

    return;
}

std::string ArDecodeFile(FILE *inFile, const model_t model) {
    std::string sOut = "";
    int c;
    probability_t unscaled;
    bit_file_t *bInFile;
    stats_t stats;

    if (NULL == inFile) {
        fprintf(stderr, "Error: Invalid input file\n");
        return sOut;
    }

    bInFile = MakeBitFile(inFile, BF_READ);

    if (NULL == bInFile) {
        fprintf(stderr, "Error: Unable to create binary input file\n");
        return sOut;
    }

    if (MODEL_STATIC == model)
    {
        if (0 != ReadHeader(bInFile, &stats))
        {
            BitFileClose(bInFile);
            return sOut;
        }
    }
    else
        InitializeAdaptiveProbabilityRangeList(&stats);

    InitializeDecoder(bInFile, &stats);

    while (true) {
        unscaled = GetUnscaledCode(&stats);

        if ((c = GetSymbolFromProbability(unscaled, &stats)) == -1)
            break;

        if (c == EOF_CHAR)
            break;

        sOut += c;

        ApplySymbolRange(c, &stats, model);
        ReadEncodedBits(bInFile, &stats);
    }

    inFile = BitFileToFILE(bInFile);

    return sOut;
}
