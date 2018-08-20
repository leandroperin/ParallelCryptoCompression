#include "../includes/utils.hpp"

std::string vec2string(std::vector<int> vec) {
    std::stringstream vts;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(vts, " "));
    return vts.str();
}

std::string vec2compactstring(std::vector<int> vec) {
    std::ostringstream vts;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(vts));
    return vts.str();
}

std::vector<int> string2vec(std::string sMsg) {
    std::vector<int> vOut;
    std::istringstream is(sMsg);
    int n;
    while(is >> n)
    	vOut.push_back(n);
    return vOut;
}

std::vector<int> compactstring2vec(std::string szString, int nDigits) {
    std::vector<int> vOut;
    for (int i = nDigits; i <= szString.size(); i += nDigits) {
        std::string szx = szString.substr(i - nDigits, nDigits);
        std::istringstream buffer(szx);
        int value;
        buffer >> value;
        vOut.push_back(value);
    }
    return vOut;
}

std::vector<int> decode_vector_binary(std::vector<int> cdata, std::vector<int> nUnique, std::vector<int> K) {
    std::vector<int> nDecoded;

    std::vector<int>nUniqueK0 = nUnique;
    std::transform(nUniqueK0.begin(), nUniqueK0.end(), nUniqueK0.begin(), std::bind1st(std::multiplies<int>(), K[0]));
    std::vector<int>nUniqueK1 = nUnique;
    std::transform(nUniqueK1.begin(), nUniqueK1.end(), nUniqueK1.begin(), std::bind1st(std::multiplies<int>(), K[1]));
    std::vector<int>nUniqueK2 = nUnique;
    std::transform(nUniqueK2.begin(), nUniqueK2.end(), nUniqueK2.begin(), std::bind1st(std::multiplies<int>(), K[2]));

    for (int i = 0; i < cdata.size(); i++) {
        auto low  = std::lower_bound(nUniqueK2.begin(), nUniqueK2.end(), cdata.at(i));
        auto high = std::upper_bound(nUniqueK2.begin(), nUniqueK2.end(), cdata.at(i));

        int low2  = int(low  - nUniqueK2.begin()-1);
        int high2 = int(high - nUniqueK2.begin());

        if (low2 < 0)
          low2 = 0;

        if (high2 > nUnique.size() - 1)
          high2 = int(nUnique.size() - 1);

        int carryover2 = cdata.at(i) - abs(nUniqueK2.at(low2));

        for (int j = low2; j <= high2; j++) {
            int carryover = cdata.at(i) - abs(nUniqueK2.at(j));
            if (carryover == 0) {
                carryover2 = 0;
                low2 = j;
                break;
            }
        }

        low  = std::lower_bound(nUniqueK1.begin(), nUniqueK1.end(), carryover2);
        high = std::upper_bound(nUniqueK1.begin(), nUniqueK1.end(), carryover2);

        int low1  = int(low  - nUniqueK1.begin()-1);
        int high1 = int(high - nUniqueK1.begin());

        if (low1 < 0)
          low1 = 0;

        if (high1 > nUnique.size() - 1)
          high1 = int(nUnique.size() - 1);

        int carryover1 = carryover2 - abs(nUniqueK1.at(low1));
        for (int j=low1; j<=high1; j++) {
            int carryover = carryover2 - abs(nUniqueK1.at(j));
            if (carryover == 0) {
                carryover1 = 0;
                low1 = j;
                break;
            }
        }

        low  = std::lower_bound (nUniqueK0.begin(), nUniqueK0.end(), carryover1);
        high = std::upper_bound (nUniqueK0.begin(), nUniqueK0.end(), carryover1);

        int low0  = int(low  - nUniqueK0.begin()-1);
        int high0 = int(high - nUniqueK0.begin());

        if (low0 < 0)
          low0 = 0;

        if (high0 > nUnique.size() - 1)
          high0 = int(nUnique.size() - 1);

        for (int k0 = low0; k0 <= high0; k0++) {
            int nUnique_k0 = nUniqueK0.at(k0);

            for (int k1 = low1; k1 <= high1; k1++) {
                int nUnique_k1 = nUniqueK1.at(k1);

                for (int k2 = low2; k2 <= high2; k2++) {
                    if ((nUnique_k0 + nUnique_k1 + nUniqueK2.at(k2)) == cdata.at(i)) {
                        nDecoded.push_back( nUnique.at(k0));
                        nDecoded.push_back( nUnique.at(k1));
                        nDecoded.push_back( nUnique.at(k2));

                        k0 = high0 + 1;
                        k1 = high1 + 1;
                        break;
                    }
                }
            }
        }

    }

    return nDecoded;
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
