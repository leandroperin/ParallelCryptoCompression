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
std::vector<int> open_raw_file(FILE *fp);
bool is_in(int n, std::vector<int> vnData);
std::vector<int> generate_limited_data(std::vector<int> vnData);
std::vector<int> generate_random_key(std::vector<int> vnData);
int max_in_vector(std::vector<int> vnData);
std::vector<int> generate_coded_vector(std::vector<int> vnData, std::vector<int> K);
std::string vec2string(std::vector<int> vec);
std::vector<int> string2vec(std::string sMsg);
std::vector<int> decode_vector(std::vector<int> cdata, std::vector<int> nLimited, std::vector<int> K);

#endif
