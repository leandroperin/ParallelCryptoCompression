#ifndef UTILS_HPP
#define UTILS_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <vector>
#include <string.h>
#include "bitfile.hpp"
/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
std::vector<int> open_raw_file(FILE *fp);
bool is_in(int n, std::vector<int> vnData);
std::vector<int> generate_limited_data(std::vector<int> vnData);
std::vector<int> generate_random_key(std::vector<int> vnData, int nSeed1=10, int nSeed2=1000);
std::vector<int> generate_coded_vector(std::vector<int> vnData);
string vec2string(std::vector<int> vec);

#endif
