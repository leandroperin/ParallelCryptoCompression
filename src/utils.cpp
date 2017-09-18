#include "../includes/utils.hpp"

std::vector<int> open_raw_file(FILE *fp) {
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

    std::vector<int>tmp(lSize);

    int k = 0;
    for (int i = 0; i < lSize; i++) {
        if (cbuffer[i]>0) {
            tmp[k] = cbuffer[i];
            k++;
        }
    }

    std::vector<int>data(k);
    data = tmp;
    return data;
}

bool is_in(int n, std::vector<int> vnData) {
    bool bResult = false;

    for (int i = 0; i < vnData.size(); i++) {
        if (n == vnData[i])
          bResult = true;
    }

    return bResult;
}

std::vector<int> generate_limited_data(std::vector<int> vnData) {
    std::vector<int>tmp(256);
    tmp[0] = vnData[0];
    int k = 1;

    for (int i = 1; i<vnData.size(); i++) {
        if (!is_in(vnData[i], tmp)) {
            tmp[k]=vnData[i];
            k++;
        }
    }

    std::vector<int>vnLimited(k);
    for (int i = 0; i < k; i++)
      vnLimited[i] = tmp[i];

    return vnLimited;
}

std::vector<int> generate_random_key(std::vector<int> vnData, int nSeed1=10, int nSeed2=1000) {
    std::vector<int> K(3);

    srand(time(NULL));

    int nStartValue = rand() % nSeed1 + 1;
    int nFactor = rand() % nSeed1 + 1;
    int nMaxValue = 2 * max_in_vector(vnData);

    K[0] = rand() % nSeed2 + 1;
    K[1] = ((nStartValue * nMaxValue) + 1);
    K[2] = ((K[1] * nMaxValue) + 1) * nFactor;

    return K;
}

std::vector<int> generate_coded_vector(std::vector<int> vnData) {
    int rem = vnData.size() % 3;
    int ext = vnData.size() % 3 > 0 ? 1 : 0;

    std::vector<int> vnResult(vnData.size() / 3 + ext);
    int k = 0;

    if (rem == 0) {
        for (int i = 0; i < vnData.size(); i += 3) {
            vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
            k++;
        }
    } else if (rem == 1) {
        for (int i = 0; i < vnData.size(); i += 3) {
           if (i + 1 == vnData.size()) {
               vnResult[k] = K[0] * vnData[i];
               k++;
           } else {
                vnResult[k] = K[0]*vnData[i] + K[1]*vnData[i+1] + K[2]*vnData[i+2];
                k++;
           }
        }
    } else if (rem==2) {
       for (int i = 0; i < vnData.size(); i += 3) {
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

string vec2string(std::vector<int> vec) {
    std::stringstream ss;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(ss," "));
    string s = ss.str();
    s.replace(s.end()-1,s.end(),1,(char)EOF_CHAR);
    return s;
}
