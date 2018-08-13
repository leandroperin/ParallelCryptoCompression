#ifndef UTILS_HPP
#define UTILS_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iterator>
#include <stdlib.h>
#include <time.h>
#include "bitfile.hpp"
#include "consts.hpp"
#include <omp.h>
/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
// std::vector<int> decode_vector(std::vector<int> cdata, std::vector<int> nLimited, std::vector<int> K);
int max_in_vector(std::vector<int> vnData);
std::string vec2string(std::vector<int> vec);
std::vector<int> string2vec(std::string sMsg);
std::string vec2compactstring(std::vector<int> vec);
std::vector<int> compactstring2vec(std::string szString, int nDigits);
std::vector<int> decode_vector_binary(std::vector<int> cdata, std::vector<int> nUnique, std::vector<int> K);
std::vector<std::vector<int> > code_vector(std::vector<int> vdata);

#endif
