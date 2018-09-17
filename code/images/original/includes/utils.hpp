#ifndef UTILS_HPP
#define UTILS_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <algorithm>
#include <sstream>
#include <iterator>
#include <omp.h>
#include "consts.hpp"
/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
std::string vec2string(std::vector<int> vec);
std::string vec2compactstring(std::vector<int> vec);
std::vector<int> string2vec(std::string sMsg);
std::vector<int> compactstring2vec(std::string szString, int nDigits);
std::vector<int> decode_vector_binary(std::vector<int> cdata, std::vector<int> nUnique, std::vector<int> K);
std::vector<std::vector<int> > code_vector(std::vector<int> vdata);

#endif
