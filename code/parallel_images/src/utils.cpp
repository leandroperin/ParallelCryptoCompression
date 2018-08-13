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
        {
          bResult = true;
          return bResult;
        }
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

std::vector<int> generate_random_key(std::vector<int> vnData) {
    int nSeed1 = 10;
    int nSeed2 = 1000;
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

int max_in_vector(std::vector<int> vnData)
{
    int nMaximum = -1;

    for (int i=0; i<vnData.size(); i++) {
        if (vnData[i]>nMaximum) nMaximum = vnData[i];
	  }

    return nMaximum;
}

std::vector<int> generate_coded_vector(std::vector<int> vnData, std::vector<int> K) {
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

std::string vec2string(std::vector<int> vec) {
    std::stringstream ss;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(ss," "));
    std::string s = ss.str();
    s.replace(s.end()-1,s.end(),1,(char)EOF_CHAR);
    return s;
}

std::vector<int> string2vec(std::string sMsg) {
    std::vector<int> vOut;
    std::istringstream is(sMsg);
    int n;
    while(is >> n)
    	vOut.push_back( n );
    return vOut;
}

std::string vec2compactstring(std::vector<int> vec) {
    std::ostringstream vts;

    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(vts));

    return vts.str();
}

std::vector<int> compactstring2vec(std::string szString, int nDigits) {
    std::vector<int> vOut;

    for (int i=nDigits; i<=szString.size(); i+=nDigits)
    {
        std::string szx = szString.substr(i-nDigits,nDigits);
        std::istringstream buffer(szx);
        int value;
        buffer >> value;
        vOut.push_back( value );
    }
    return vOut;
}


std::vector<int> decode_vector(std::vector<int> cdata, std::vector<int> nLimited, std::vector<int> K) {
  std::vector<int> nDecoded;
  unsigned int nLastIndex  = cdata.size();
  unsigned int nLSize = nLimited.size();
  nDecoded.resize(3*nLastIndex);

  #pragma omp parallel for
  for (unsigned int r = 0; r < nLastIndex; ++r) {
    int cdatar = cdata[r];

    for (unsigned int i = 0; i < nLSize; ++i) {
      int nLimited_i = K[0] * nLimited[i];

      for (unsigned int j = 0; j < nLSize; ++j) {
        int nLimited_j = K[1] * nLimited[j];

        for (unsigned int k = 0; k < nLSize; ++k) {

          if (r != nLastIndex - 1) {
            if (cdatar == nLimited_i + nLimited_j + K[2]*nLimited[k]) {
              nDecoded[3*r] = nLimited[i];
              nDecoded[3*r+1] = nLimited[j];
              nDecoded[3*r+2] = nLimited[k];

              i = j = nLSize; break; // break all loops
            }
	        } else {
            if (cdatar == nLimited_i + nLimited_j + K[2]*nLimited[k]) {
              nDecoded[3*r] = nLimited[i];
              nDecoded[3*r+1] = nLimited[j];
              nDecoded[3*r+2] = nLimited[k];

              i = j = nLSize; break; // break all loops
            } else if (cdatar == nLimited_i + nLimited_j) {
              nDecoded[3*r]  = nLimited[i];
              nDecoded[3*r+1]  = nLimited[j];

              nDecoded.resize(3*nLastIndex-1);

              i = j = nLSize; break; // break all loops
            } else if (cdatar == nLimited_i) {
              nDecoded[3*r]  = nLimited[i];

              nDecoded.resize(3*nLastIndex-2);

              i = j = nLSize; break; // break all loops
            }
          }
        }
      }
    }
  }

  return nDecoded;
}

#include <iostream>
std::vector<int> decode_vector_binary(std::vector<int> cdata, std::vector<int> nUnique, std::vector<int> K) {
  std::vector<int> nDecoded;

    //this function only works for positive integers 0,1,2,3,...
    //Pre-multiply unique vector by the triple key
    std::vector<int>nUniqueK0 = nUnique;
    std::transform(nUniqueK0.begin(), nUniqueK0.end(), nUniqueK0.begin(), std::bind1st(std::multiplies<int>(),K[0]));
    std::vector<int>nUniqueK1 = nUnique;
    std::transform(nUniqueK1.begin(), nUniqueK1.end(), nUniqueK1.begin(), std::bind1st(std::multiplies<int>(),K[1]));
    std::vector<int>nUniqueK2 = nUnique;
    std::transform(nUniqueK2.begin(), nUniqueK2.end(), nUniqueK2.begin(), std::bind1st(std::multiplies<int>(),K[2]));

    for (int i=0; i<cdata.size(); i++) //for each item to be decoded by binary search
    {
        std::vector<int>::iterator low,high; //find a narrow range for low and high indices, the answer is between those
        low  = std::lower_bound (nUniqueK2.begin(), nUniqueK2.end(), cdata.at(i));
        high = std::upper_bound (nUniqueK2.begin(), nUniqueK2.end(), cdata.at(i));
        int low2, high2;
        low2  = int(low  - nUniqueK2.begin()-1);
        high2 = int(high - nUniqueK2.begin()  );
        if (low2<0) low2=0; //avoid negative indices
        if (high2>nUnique.size()-1) high2 = int(nUnique.size()-1); //avoid out of bounds
        int carryover2 = cdata.at(i) - abs(nUniqueK2.at(low2));
        for (int j=low2; j<=high2; j++)
        {
            int carryover = cdata.at(i) - abs(nUniqueK2.at(j));
            if (carryover == 0)
            {
                carryover2 = 0;
                low2 = j;
                break;
            }
        }


        low  = std::lower_bound (nUniqueK1.begin(), nUniqueK1.end(), carryover2);
        high = std::upper_bound (nUniqueK1.begin(), nUniqueK1.end(), carryover2);
        int low1, high1;
        low1  = int(low  - nUniqueK1.begin()-1);
        high1 = int(high - nUniqueK1.begin()  );
        if (low1<0) low1=0;
        if (high1>nUnique.size()-1) high1 = int(nUnique.size()-1);
        int carryover1 = carryover2 - abs(nUniqueK1.at(low1));
        for (int j=low1; j<=high1; j++)
        {
            int carryover = carryover2 - abs(nUniqueK1.at(j));
            if (carryover == 0)
            {
                carryover1 = 0;
                low1 = j;
                break;
            }
        }


        low  = std::lower_bound (nUniqueK0.begin(), nUniqueK0.end(), carryover1);
        high = std::upper_bound (nUniqueK0.begin(), nUniqueK0.end(), carryover1);
        int low0, high0;
        low0  = int(low  - nUniqueK0.begin()-1);
        high0 = int(high - nUniqueK0.begin()  );
        if (low0<0) low0=0;
        if (high0>nUnique.size()-1) high0 = int(nUnique.size()-1);


        for (int k0=low0; k0<=high0; k0++) //now we have our narrow ranges, determine which is which
        {
            for (int k1=low1; k1<=high1; k1++)
            {
                for (int k2=low2; k2<=high2; k2++)
                {
                    if ( (nUniqueK0.at(k0) + nUniqueK1.at(k1) + nUniqueK2.at(k2)) == cdata.at(i) ) //found it!
                    {
                        nDecoded.push_back( nUnique.at(k0));
                        nDecoded.push_back( nUnique.at(k1));
                        nDecoded.push_back( nUnique.at(k2));
                        break;
                    }
                }
            }
        }
    }
    return nDecoded; //decoded vector is exactly 3 times larger than cdata IFF cdata was properly coded
}


std::vector<std::vector<int> > code_vector(std::vector<int> vdata )
{
    ///return vResult[0] = K, vResult[1] = vUnique, vResult[2] = coded data vResult[3] = padding
    std::vector<std::vector<int> > vResult(4);
    ///generate key: M=max(vdata), F=1,2,3..., K1=random, K2=K1+M+F, K2=MF(K1+K2)
    std::vector<int>  K(3);
    int M = *max_element(vdata.begin(), vdata.end());
    int F   = 1;
    K.at(0) = 1;
    K.at(1) = K.at(0) + F + M;
    K.at(2) = F*M*(K.at(0) + K.at(1));

    ///cout<<"code by triple key padding if necessary"<<endl;
    std::vector<int> vcoded;
    std::vector<int> vunique;
    std::vector<int> v = vdata;
    int padding   = 0;
    int remainder = v.size()%3;
    if (remainder == 2)
    {
        v.push_back(0); //pad one value
        padding = 1;
    }
    else if (remainder == 1)
    {
        v.push_back(0); //pad two values
        v.push_back(0);
        padding = 2;
    }
    for (int j=0; j<v.size(); j+=3) //we know v is a multiple of 3 so wont go out of bounds
    {
        int u0 = v.at(j);
        int u1 = v.at(j+1);
        int u2 = v.at(j+2);
        int coded   = K.at(0)*u0 + K.at(1)*u1 + K.at(2)*u2;
        vcoded.push_back(coded); //append to vector
        vunique.push_back(u0);
        vunique.push_back(u1);
        vunique.push_back(u2);
    }
    std::sort( vunique.begin(), vunique.end() );
    vunique.erase( unique( vunique.begin(), vunique.end() ), vunique.end() );

    vResult[0] = K;
    vResult[1] = vunique;
    vResult[2] = vcoded;
    vResult[3].push_back(padding);
    return vResult;
}
