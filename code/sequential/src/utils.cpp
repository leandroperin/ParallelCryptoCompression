#include "../includes/utils.hpp"

std::vector<unsigned int> open_raw_file(FILE *fp) {
    long lSize;
    char *cbuffer;

    if (!fp) {
      perror("Error file does not exist.");
      exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    cbuffer = (char*) calloc(1, lSize+1);
    if (!cbuffer) {
      fclose(fp);
      fputs("Memory alloc fails.",stderr);
      exit(1);
    }

    if (fread(cbuffer, lSize, 1, fp) != 1) {
      fclose(fp);
      free(cbuffer);
      fputs("Entire fread function fails. Something wrong with the file.",stderr);
      exit(1);
    }

    fclose(fp);

    std::vector<unsigned int>tmp(lSize);

    int k = 0;
    for (unsigned int i = 0; i < lSize; i++) {
        if (cbuffer[i]>0) {
            tmp[k] = cbuffer[i];
            k++;
        }
    }

    std::vector<unsigned int>data(k);
    data = tmp;
    return data;
}

bool is_in(unsigned int n, std::vector<unsigned int> vnData) {
    bool bResult = false;

    for (unsigned int i = 0; i < vnData.size(); i++) {
        if (n == vnData[i])
          bResult = true;
    }

    return bResult;
}

std::vector<unsigned int> generate_limited_data(std::vector<unsigned int> vnData) {
    std::vector<unsigned int>tmp(256);
    tmp[0] = vnData[0];
    unsigned int k = 1;

    for (unsigned int i = 1; i<vnData.size(); i++) {
        if (!is_in(vnData[i], tmp)) {
            tmp[k]=vnData[i];
            k++;
        }
    }

    std::vector<unsigned int>vnLimited(k);
    for (unsigned int i = 0; i < k; i++)
      vnLimited[i] = tmp[i];

    return vnLimited;
}

std::vector<unsigned int> generate_random_key(std::vector<unsigned int> vnData) {
    unsigned int nSeed1 = 10;
    unsigned int nSeed2 = 1000;
    std::vector<unsigned int> K(3);

    srand(time(NULL));

    unsigned int nStartValue = rand() % nSeed1 + 1;
    unsigned int nFactor = rand() % nSeed1 + 1;
    unsigned int nMaxValue = 2 * max_in_vector(vnData);

    K[0] = rand() % nSeed2 + 1;
    K[1] = ((nStartValue * nMaxValue) + 1);
    K[2] = ((K[1] * nMaxValue) + 1) * nFactor;

    return K;
}

unsigned int max_in_vector(std::vector<unsigned int> vnData)
{
    int nMaximum = -1;

    for (unsigned int i=0; i<vnData.size(); i++) {
        if (vnData[i]>nMaximum) nMaximum = vnData[i];
	  }

    return nMaximum;
}

std::vector<unsigned int> generate_coded_vector(std::vector<unsigned int> vnData, std::vector<unsigned int> K) {
    unsigned int rem = vnData.size() % 3;
    unsigned int ext = vnData.size() % 3 > 0 ? 1 : 0;

    std::vector<unsigned int> vnResult(vnData.size() / 3 + ext);
    unsigned int k = 0;

    if (rem == 0) {
        for (unsigned int i = 0; i < vnData.size(); i += 3) {
            vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
            k++;
        }
    } else if (rem == 1) {
        for (unsigned int i = 0; i < vnData.size(); i += 3) {
           if (i + 1 == vnData.size()) {
               vnResult[k] = K[0] * vnData[i];
               k++;
           } else {
                vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
                k++;
           }
        }
    } else if (rem==2) {
       for (unsigned int i = 0; i < vnData.size(); i += 3) {
           if (i + 2 == vnData.size()) {
               vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1];
               k++;
           } else {
                vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
                k++;
           }
       }
    }

    return vnResult;
}

std::string vec2string(std::vector<unsigned int> vec) {
    std::stringstream ss;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<unsigned int>(ss," "));
    std::string s = ss.str();
    s.replace(s.end()-1,s.end(),1,(char)EOF_CHAR);
    return s;
}

std::vector<unsigned int> string2vec(std::string sMsg) {
    std::vector<unsigned int> vOut;
    std::istringstream is(sMsg);
    unsigned int n;
    while(is >> n)
    	vOut.push_back( n );
    return vOut;
}

std::vector<unsigned int> decode_vector(std::vector<unsigned int> cdata, std::vector<unsigned int> nLimited, std::vector<unsigned int> K) {
  std::vector<unsigned int> nDecoded;
  unsigned int nLastIndex  = cdata.size();
  unsigned int nLSize = nLimited.size();
  nDecoded.resize(3*nLastIndex-3);

  #pragma omp parallel for
  for (unsigned int r = 0; r < nLastIndex; ++r) {
    for (unsigned int i = 0; i < nLSize; ++i) {
      for (unsigned int j = 0; j < nLSize; ++j) {
        for (unsigned int k = 0; k < nLSize; ++k) {
          if (r != nLastIndex - 1) {
            if (cdata[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k]) {
              nDecoded[3*r] = nLimited[i];
              nDecoded[3*r+1] = nLimited[j];
              nDecoded[3*r+2] = nLimited[k];
              break;
            }
	        } else {
            if (cdata[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k] ) {
              nDecoded[3*r+1]  = nLimited[j];
              nDecoded[3*r+2]  = nLimited[k];
              nDecoded[3*r]  = nLimited[i];
              break;
            } else if (cdata[r] == K[0]*nLimited[i] + K[1]*nLimited[j]) {
              nDecoded[3*r]  = nLimited[i];
              nDecoded[3*r+1]  = nLimited[j];
              break;
            } else if (cdata[r] == K[0]*nLimited[i]) {
              nDecoded[3*r]  = nLimited[i];
              break;
            }
          }
        }
      }
    }
  }

  return nDecoded;
}
