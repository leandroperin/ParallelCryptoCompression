#ifndef UTILS_HPP
#define UTILS_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <vector>
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
std::vector<unsigned int> open_raw_file(FILE *fp);
bool is_in(unsigned int n, std::vector<unsigned int> vnData);
std::vector<unsigned int> generate_limited_data(std::vector<unsigned int> vnData);
std::vector<unsigned int> generate_random_key(std::vector<unsigned int> vnData);
unsigned int max_in_vector(std::vector<unsigned int> vnData);
std::vector<unsigned int> generate_coded_vector(std::vector<unsigned int> vnData, std::vector<unsigned int> K);
std::string vec2string(std::vector<unsigned int> vec);
std::vector<unsigned int> string2vec(std::string sMsg);
std::vector<unsigned int> decode_vector(std::vector<unsigned int> cdata, std::vector<unsigned int> nLimited, std::vector<unsigned int> K);

#endif
