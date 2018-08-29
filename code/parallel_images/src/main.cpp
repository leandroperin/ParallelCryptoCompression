#include "../includes/encode.hpp"
#include "../includes/decode.hpp"
#include "../includes/utils.hpp"

#include <dirent.h>
#include <sys/time.h>
#include "opencv2/opencv.hpp"

using namespace cv;

FILE *inFile, *outFile;
bool toEncode;
model_t model;

struct timeval tic, toc;

char* filePathOriginals;
char* filePathCompressed;
char *filePathDecompressed;

vector<int> nDecoded;

///Function prototypes:
string read_coded_file( char* szFilePath, char* szFileName );
int  write_img_file(char* szFilePath, char* szFileName, cv::Mat img, char* szExtension="bmp");

void initializeData() {
	inFile = NULL;
	outFile = NULL;
	toEncode = true;
	model = MODEL_STATIC;

	filePathOriginals  = "/home/lperin/imgtest/originals";
	filePathCompressed = "/home/lperin/imgtest/compressed";
	filePathDecompressed = "/home/lperin/imgtest/decompressed";
}

cv::Mat generate_quantization_matrix( int Blocksize, int R)
{
    cv::Mat Q(Blocksize, Blocksize, CV_32F, Scalar::all(0)); //the quantization matrix, float, initialized to zero
    for (int r=0; r<Blocksize; r++)
    {
        for (int c=0; c<Blocksize; c++)
        {
            Q.at<float>(r,c) = (float)(r+c)*R; //Build quantization matrix
            if (r==0 & c==0) Q.at<float>(r,c) = 1;
        }
    }
    return Q;
}


string encode(cv::Mat matImg) {
    int COLS = matImg.size().width;
    int ROWS = matImg.size().height;

    vector<cv::Mat> planes;
    split(matImg, planes);

		int planesSize = planes.size();

    int R = 1;
    int Blocksize = 8;
    int bQ = 0;
    cv::Mat Q = generate_quantization_matrix(Blocksize, R);

    vector<cv::Mat> matDctData(planesSize);

		#pragma omp parallel for
    for (int i = 0; i < planes.size(); i++) {
        planes[i].convertTo(planes[i], CV_32F);

        for (int r = 0; r < ROWS - Blocksize; r += Blocksize) {
            for (int c = 0; c < COLS - Blocksize; c += Blocksize) {
                cv::Mat roi = planes[i](cv::Rect(c, r, Blocksize, Blocksize));
                cv::Mat dst, dst_half;

                cv::dct(roi, dst);

                if (bQ)
									cv::divide(dst, Q, dst);

								dst_half = dst(cv::Rect(0, 0, Blocksize / 2, Blocksize / 2));
                dst_half.convertTo(dst_half, CV_32S, 1.0, 0.0);

                cv::Mat row(1, (Blocksize / 2) * (Blocksize / 2), CV_32S);

                int idx = 0;

                for (int j = 0; j < dst_half.size().height; j++) {
                    for (int k = 0; k < dst_half.size().width; k++) {
                        row.at<int>(0, idx) = dst_half.at<int>(j, k);
                        idx++;
                    }
                }

                matDctData[i].push_back(row);
            }
        }
    }

    vector<cv::Mat> matNonZeroLocXY(planesSize);
    vector<cv::Mat> matZeroLocXY(planesSize);
    vector<cv::Mat> matOneLocXY(planesSize);
    vector<cv::Mat> matTwoLocXY(planesSize);
    vector<cv::Mat> matThreeLocXY(planesSize);
    vector<cv::Mat> matNegLocXY(planesSize);

    vector<cv::Mat> matDctDC(planesSize);
    vector<cv::Mat> matDctAC(planesSize);
    vector<cv::Mat> matDctACcol(planesSize);
    vector<cv::Mat> matNonZeroData(planesSize);

    vector<vector<int>> vnDctACcol(planesSize);
    vector<vector<int>> vnNonZeroData(planesSize);
    vector<vector<int>> vnDC(planesSize);
    vector<vector<int>> vnZeroLoc(planesSize);
    vector<vector<int>> vnTwoLoc(planesSize);
    vector<vector<int>> vnThreeLoc(planesSize);
    vector<vector<int>> vnNegLoc(planesSize);

    vector<vector<int>> vnCodedDC(planesSize);
    vector<vector<int>> vnCodedNegLoc(planesSize);
    vector<vector<int>> vnUniqueNegLoc(planesSize);

    vector<vector<int>> vnOneData(planesSize);
    vector<vector<int>> vnTwoData(planesSize);
    vector<vector<int>> vnThreeData(planesSize);

    vector<string> szCompactOneData(planesSize);
    vector<string> szCompactTwoData(planesSize);
    vector<string> szCompactThreeData(planesSize);

    vector<vector<int>> KNegLoc(planesSize);

		#pragma omp parallel for
    for (int i = 0; i < planesSize; i++) {
        matDctDC[i] = matDctData[i](cv::Rect(0, 0, 1, matDctData[i].rows)).clone();
        matDctAC[i] = matDctData[i](cv::Rect(1, 0, matDctData[i].cols - 1, matDctData[i].rows)).clone();

        int DC = matDctDC[i].row(0).at<int>(0, 0);
        vnDC[i].push_back(DC);

        for (int j = 1; j < matDctDC[i].size().height; j++) {
            cv::Mat row = matDctDC[i].row(j);
            int value = row.at<int>(0, 0);
            vnDC[i].push_back(value - DC);
            DC = value;
        }

        for (int j = 0; j < matDctAC[i].size().height; j++) {
            cv::Mat row = matDctAC[i].row(j);
            cv::Mat rowt = row.t();
            matDctACcol[i].push_back(rowt);

            for (int k = 0; k < row.size().width; k++)
                vnDctACcol[i].push_back(row.at<int>(0, k));
        }

        cv::Mat matBoolean;
				matBoolean = matDctACcol[i] != 0;

        cv::findNonZero(matBoolean, matNonZeroLocXY[i]);
        matBoolean.release();

				matBoolean = matDctACcol[i] == 0;
        cv::findNonZero(matBoolean, matZeroLocXY[i]);

        cv::Point pt = matZeroLocXY[i].at<Point>(0);
        int index = pt.y;
        vnZeroLoc[i].push_back(index);

        for (int j = 1; j < matZeroLocXY[i].size().height; j++) {
            cv::Point pnt = matZeroLocXY[i].at<Point>(j);
            vnZeroLoc[i].push_back(pnt.y - index);
            index = pnt.y;
        }

        for (int j = 0; j < matNonZeroLocXY[i].size().height; j++) {
            cv::Point pnt = matNonZeroLocXY[i].at<Point>(j);
            int nValue = matDctACcol[i].at<int>(pnt.y, pnt.x);
            vnNonZeroData[i].push_back(nValue);

            cv::Mat row(1, 1, CV_32S);
            row.at<int>(0, 0) = nValue;
            matNonZeroData[i].push_back(row);
        }

        matBoolean.release();
				matBoolean = matNonZeroData[i] < 0;
        cv::findNonZero(matBoolean, matNegLocXY[i]);

        pt = matNegLocXY[i].at<Point>(0);
        index = pt.y;
        vnNegLoc[i].push_back(index);

        for (int j = 1; j < matNegLocXY[i].size().height; j++) {
            cv::Point pnt = matNegLocXY[i].at<Point>(j);
            vnNegLoc[i].push_back(pnt.y - index);
            index = pnt.y;
        }

        cv::Mat matAbsNonZeroAC = abs(matNonZeroData[i].clone());

        matBoolean.release();
				matBoolean = (matAbsNonZeroAC < 10);

        cv::Mat matOneLocXY;
        cv::findNonZero(matBoolean, matOneLocXY);

        for (int j = 0; j < matOneLocXY.size().height; j++) {
            cv::Point pnt = matOneLocXY.at<Point>(j);
            int nValue = matAbsNonZeroAC.at<int>(pnt.y, pnt.x);
            vnOneData[i].push_back(nValue);
        }

        matBoolean.release();
				matBoolean = (matAbsNonZeroAC > 9 & matAbsNonZeroAC < 100);
        cv::findNonZero(matBoolean, matTwoLocXY[i]);

        if (matTwoLocXY[i].size().height > 0) {
            pt = matTwoLocXY[i].at<Point>(0);
            index = pt.y;
            vnTwoLoc[i].push_back(index);

            int value = matAbsNonZeroAC.at<int>(index, pt.x);
            vnTwoData[i].push_back(value);

            for (int j = 1; j < matTwoLocXY[i].size().height; j++) {
                cv::Point pnt = matTwoLocXY[i].at<Point>(j);
                vnTwoLoc[i].push_back(pnt.y - index);
                index = pnt.y;

                int nValue = matAbsNonZeroAC.at<int>(index, pnt.x);
                vnTwoData[i].push_back(nValue);
            }
        }

        matBoolean.release();
				matBoolean = (matAbsNonZeroAC > 99 & matAbsNonZeroAC < 1000);
        cv::findNonZero(matBoolean, matThreeLocXY[i]);

        if (matThreeLocXY[i].size().height > 0) {
            pt = matThreeLocXY[i].at<Point>(0);
            index = pt.y;
            vnThreeLoc[i].push_back(index);

            int value = matAbsNonZeroAC.at<int>(index, pt.x);
            vnThreeData[i].push_back(value);

            for (int j = 1; j < matThreeLocXY[i].size().height; j++) {
                cv::Point pnt = matThreeLocXY[i].at<Point>(j);
                vnThreeLoc[i].push_back(pnt.y - index);
                index = pnt.y;

                int nValue = matAbsNonZeroAC.at<int>(index, pnt.x);
                vnThreeData[i].push_back(nValue);
            }
        }

        szCompactOneData[i] = vec2compactstring(vnOneData[i]);
        szCompactTwoData[i] = vec2compactstring(vnTwoData[i]);
        szCompactThreeData[i] = vec2compactstring(vnThreeData[i]);

        vector<vector<int>> vcodednloc = code_vector(vnNegLoc[i]);

        KNegLoc[i] = vcodednloc[0];
        vnUniqueNegLoc[i] = vcodednloc[1];
        vnCodedNegLoc[i] = vcodednloc[2];
    }

    vector<int> vnSerialized;
    string szCompact;

    vnSerialized.push_back(matImg.size().width);
    vnSerialized.push_back(matImg.size().height);
    vnSerialized.push_back(planesSize);
    vnSerialized.push_back(R);
    vnSerialized.push_back(Blocksize);
    vnSerialized.push_back(bQ);
    vnSerialized.push_back(matDctDC[0].size().height);
    vnSerialized.push_back(matDctAC[0].size().width);
    vnSerialized.push_back(matDctAC[0].size().height);

    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnZeroLoc[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnTwoLoc[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnThreeLoc[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnNegLoc[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnCodedNegLoc[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnUniqueNegLoc[i].size() );
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(szCompactOneData[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(szCompactTwoData[i].size());
    for (int i = 0; i < planesSize; i++) vnSerialized.push_back(szCompactThreeData[i].size());

    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), KNegLoc[i].begin(), KNegLoc[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnDC[i].begin(), vnDC[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnZeroLoc[i].begin(), vnZeroLoc[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnTwoLoc[i].begin(), vnTwoLoc[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnThreeLoc[i].begin(), vnThreeLoc[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnCodedNegLoc[i].begin(), vnCodedNegLoc[i].end());
    for (int i = 0; i < planesSize; i++) vnSerialized.insert(vnSerialized.end(), vnUniqueNegLoc[i].begin(), vnUniqueNegLoc[i].end());

    for (int i = 0; i < planesSize; i++) szCompact +=  szCompactOneData[i];
    for (int i = 0; i < planesSize; i++) szCompact +=  szCompactTwoData[i];
    for (int i = 0; i < planesSize; i++) szCompact +=  szCompactThreeData[i];

    string szSerialized = vec2string(vnSerialized);
    szSerialized += " " + szCompact;

    return szSerialized;
}

cv::Mat decode(char* filePathIn, char* fileName )
{
	string szSerialized = read_coded_file( filePathIn, fileName );
    string szTmp, szCompact, szNumbers;
    for ( string::iterator it=szSerialized.begin(); it!=szSerialized.end(); ++it) //parse string
    {
        string szSpace;
        szTmp += *it;
        szCompact += *it;
        szSpace = *it;
        if (szSpace == " ") //we reached a separator
        {
            if (szTmp.size() < 10) //if a few characters then it is a number
            {
                //std::cout << szTmp<<endl;
                szNumbers += szTmp;
                szTmp      = "";
                szCompact  = "";
            }
            else
            {
                //it is part of the compact string, do nothing
            }
        }
    }

	vector<int> data = string2vec(szNumbers);

    int COLS        = data.at(0);
    int ROWS        = data.at(1);
    int planes      = data.at(2);
    int R           = data.at(3);
    int Blocksize   = data.at(4);
    int bQ          = data.at(5);
    int DCheight    = data.at(6);
    int ACwidth     = data.at(7);
    int ACheight    = data.at(8);

    int ZeroLocSize[]       = {data.at(9),  data.at(10), data.at(11)};
    int TwoLocSize[]        = {data.at(12), data.at(13), data.at(14)};
    int ThreeLocSize[]      = {data.at(15), data.at(16), data.at(17)};
    int NegLocSize[]        = {data.at(18), data.at(19), data.at(20)};
    int CodedNegLocSize[]   = {data.at(21), data.at(22), data.at(23)};
    int UniqueNegSize[]     = {data.at(24), data.at(25), data.at(26)};
    int CompactOneSize[]    = {data.at(27), data.at(28), data.at(29)};
    int CompactTwoSize[]    = {data.at(30), data.at(31), data.at(32)};
    int CompactThreeSize[]  = {data.at(33), data.at(34), data.at(35)};

    std::vector<vector<int> > KNeg(planes);
    int nStart = 36;
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + 3;
        vector<int> newVec(first, last);
        KNeg[i] = newVec;
        nStart += 3;
    }

    std::vector<vector<int> > vnDiffDC(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + DCheight;
        vector<int> newVec(first, last);
        vnDiffDC[i] = newVec;
        nStart += DCheight;
    }

    std::vector<vector<int> > vnDiffZeroLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + ZeroLocSize[i];
        vector<int> newVec(first, last);
        vnDiffZeroLoc[i] = newVec;
        nStart += ZeroLocSize[i];
    }

    std::vector<vector<int> > vnDiffTwoLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + TwoLocSize[i];
        vector<int> newVec(first, last);
        vnDiffTwoLoc[i] = newVec;
        nStart += TwoLocSize[i];
    }

    std::vector<vector<int> > vnDiffThreeLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + ThreeLocSize[i];
        vector<int> newVec(first, last);
        vnDiffThreeLoc[i] = newVec;
        nStart += ThreeLocSize[i];
    }

    std::vector<vector<int> > vnCodedDiffNegLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + CodedNegLocSize[i];
        vector<int> newVec(first, last);
        vnCodedDiffNegLoc[i] = newVec;
        nStart += CodedNegLocSize[i];
    }

    std::vector<vector<int> > vnUniqueNeg(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + UniqueNegSize[i];
        vector<int> newVec(first, last);
        vnUniqueNeg[i] = newVec;
        nStart += UniqueNegSize[i];
    }

    std::vector<string> szCompactOneData(planes);
    std::vector<string> szCompactTwoData(planes);
    std::vector<string> szCompactThreeData(planes);
    std::vector<vector<int> > vnOneData(planes);
    std::vector<vector<int> > vnTwoData(planes);
    std::vector<vector<int> > vnThreeData(planes);

    nStart=0;
    for (int i = 0; i < planes; i++)
    {
        szCompactOneData[i]= szCompact.substr (nStart, CompactOneSize[i]);
        vnOneData[i] = compactstring2vec(szCompactOneData[i], 1);
        nStart += CompactOneSize[i];
    }
    for (int i = 0; i < planes; i++)
    {
        szCompactTwoData[i]= szCompact.substr (nStart, CompactTwoSize[i]);
        vnTwoData[i] = compactstring2vec(szCompactTwoData[i], 2);
        nStart += CompactTwoSize[i];
    }
    for (int i = 0; i < planes; i++)
    {
        szCompactThreeData[i]= szCompact.substr (nStart, CompactThreeSize[i]);
        vnThreeData[i] = compactstring2vec(szCompactThreeData[i], 3);
        nStart += CompactThreeSize[i];
    }

    ///Build the nonZeroData vectors and DC components
    std::vector<vector<int> > vnNonZeroData(planes);
    std::vector<vector<int> > vnTwoLoc(planes);
    std::vector<vector<int> > vnThreeLoc(planes);
    std::vector<vector<int> > vSize(planes);
    std::vector<vector<int> > vnDC(planes);


    for (int i = 0; i < planes; i++)
    {
        ///build the location of 2 and 3 digits
        int index2 = vnDiffTwoLoc[i].at(0);
        int index3 = vnDiffThreeLoc[i].at(0);
        vnTwoLoc[i].push_back( index2 );
        vnThreeLoc[i].push_back( index3 );
        for (int j=1; j<vnDiffTwoLoc[i].size(); j++) //index of two digit data
        {
            int diff = vnDiffTwoLoc[i].at(j);
            index2+=diff;
            vnTwoLoc[i].push_back( index2 );
        }
        for (int j=1; j<vnDiffThreeLoc[i].size(); j++)
        {
            int diff = vnDiffThreeLoc[i].at(j);
            index3+=diff;
            vnThreeLoc[i].push_back( index3 );
        }

        int sizeNonZero = vnOneData[i].size() + vnTwoData[i].size() + vnThreeData[i].size();
        std::vector<int> vec( sizeNonZero, -1 ); //initialized to -1
        vnNonZeroData[i] = vec;

        ///update two digit values
        for (int j=0; j<vnTwoLoc[i].size(); j++)
        {
            int index = vnTwoLoc[i].at(j);
            vnNonZeroData[i].at(index) = vnTwoData[i].at(j);
        }
        ///update three digit values
        for (int j=0; j<vnThreeLoc[i].size(); j++)
        {
            int index = vnThreeLoc[i].at(j);
            vnNonZeroData[i].at(index) = vnThreeData[i].at(j);
        }
        ///update one digit values
        int idx=0;
        for (int j=0; j<vnNonZeroData[i].size(); j++)
        {
            if (vnNonZeroData[i].at(j) == -1)
            {
                vnNonZeroData[i].at(j) = vnOneData[i].at(idx);
                idx++;
            }
        }
        ///update DC components  vnDiffDC(planes);
        int diff = vnDiffDC[i].at(0);
        vnDC[i].push_back( diff ); //save first value as is
        for (int j=1; j<vnDiffDC[i].size(); j++)
        {
            diff += vnDiffDC[i].at(j);
            vnDC[i].push_back( diff );
        }
    }

    //cout<<"\nDecoding negative locations..... note that original vector may have been padded"<<endl;
    std::vector<vector<int> > vnDiffNegLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        vector<int> newVec = vnCodedDiffNegLoc[i];
        nDecoded = decode_vector_binary(newVec, vnUniqueNeg[i], KNeg[i]);

        ///check the size of decoded vector
        if (nDecoded.size() > NegLocSize[i]) //need to delete padded entries at the end
        {
            int diff = nDecoded.size() - NegLocSize[i];
            vector<int>::const_iterator first = nDecoded.begin();
            vector<int>::const_iterator last  = nDecoded.begin() + nDecoded.size() - diff;
            vector<int> nVec( first, last);
            nDecoded = nVec;
        }
        vnDiffNegLoc[i] = nDecoded; //update vector

        ///rebuild the negative indices
        std::vector<int> vNegIndices;
        int index = vnDiffNegLoc[i].at(0);
        vNegIndices.push_back(index);
        for (int j=1; j<vnDiffNegLoc[i].size(); j++)
        {
            index += vnDiffNegLoc[i].at(j);
            vNegIndices.push_back( index );
        }

        ///fix the negative values in vnNonZeroData
        for (int j=0; j<vNegIndices.size(); j++)
        {
            index = vNegIndices.at(j);
            vnNonZeroData[i].at(index) = -1*vnNonZeroData[i].at(index);
        }
    }

    ///Add zeros to data
    std::vector<vector<int> > vnDctACcol(planes);
    std::vector<vector<int> > vnZeroLoc(planes);
    for (int i = 0; i < planes; i++)
    {
        int vsize = vnNonZeroData[i].size() + vnDiffZeroLoc[i].size();
        int index0 = 0;
        std::vector<int> vec( vsize, -1); //initialized to -1
        for (int j=0; j<vnDiffZeroLoc[i].size(); j++)
        {
            int diff = vnDiffZeroLoc[i].at(j);
            index0  += diff;
            vec.at(index0) = 0;
        }
        int index=0;
        for (int j=0; j<vec.size(); j++)
        {
            if ( vec.at(j) == -1)
            {
                vec.at(j) = vnNonZeroData[i].at(index);
                index++;
            }
        }
        vnDctACcol[i] = vec;
    }


    vector<Mat> vmatPlanes(planes); //a vector of matrices
    vector<Mat> vmatOutPlanes(planes); //a vector of matrices
		#pragma omp parallel for schedule(dynamic, 10)
    for (int i = 0; i < planes; i++)
    {
        Mat img2( ROWS, COLS, CV_32F, Scalar(0));
        int start   = 0;
        int step    = (Blocksize/2)*(Blocksize/2)-1; //steps in the vnDctACcol vector
        int dcIndex = 0;
        int acIndex = 0;
				// #pragma omp parallel for
        for (int r=0; r<ROWS-Blocksize; r+=Blocksize)
        {
            for (int c=0; c<COLS-Blocksize; c+=Blocksize)
            {
                vector<int>::const_iterator first = vnDctACcol[i].begin() + start;
                vector<int>::const_iterator last  = vnDctACcol[i].begin() + start + step;
                vector<int> vec(first, last);

                ///Reshape vector vec into a matrix
                Mat roi (Blocksize/2, Blocksize/2, CV_32F);
                int idx=0;

                for (int j=0; j<Blocksize/2; j++)
                {
                    for (int k=0; k<Blocksize/2; k++)
                    {
                        if (j==0 & k==0)
                        {
                            roi.at<float>(j,k) = float(vnDC[i].at(dcIndex));
                            dcIndex++;
                        }
                        else
                        {
                            roi.at<float>(j,k) = float(vec.at(idx));
                            idx++;
                        }
                    }
                }
                if (bQ)
                {
                     Mat Q  = generate_quantization_matrix( Blocksize, R );
                     multiply(roi, Q, roi);
                }
                roi.copyTo(img2(Rect(c, r, roi.cols, roi.rows)));
                Mat roi2 = img2( Rect(c,r,Blocksize,Blocksize) ); //x,y,w,h
                Mat dst;
                idct(roi2, dst);
                dst.convertTo(dst, CV_8UC1);
                dst.copyTo(img2(Rect(c, r, dst.cols, dst.rows)));
                start+=step;
            }
        }
        vmatPlanes[i] = img2.clone();
        vmatPlanes[i].convertTo(vmatPlanes[i], CV_8UC1);
    }

    Mat mergedImg;
    merge(vmatPlanes, mergedImg);
        ///display for debugging purposes only
        ///namedWindow("Reconstructed Image after compression", CV_WINDOW_AUTOSIZE);
        ///imshow("Reconstructed Image after compression", mergedImg);
        ///waitKey(1);
        ///destroyAllWindows();
    return mergedImg;
}




string read_coded_file( char* szFilePath, char* szFileName )
{
    char cFileName[256];
    memset(cFileName, 0, sizeof cFileName);   //clear array
    strcpy(cFileName,szFilePath);             // copy string into the result
    strcat(cFileName,"/");                    // append /
    strcat(cFileName,szFileName);             // copy string into the result

    inFile = fopen(cFileName, "rb");
    if (inFile == NULL)
    {
        std::cout << "Cannot Open File to Decode!" << std::endl;
        return NULL;
    }

	string szSerialized = ArDecodeFile(inFile, model);
	fclose(inFile);
	return szSerialized;
}


void write_coded_file(char* szFilePath, char* szFileName, string szSerialized )
{
    //Apply arithmetic coding and save the coded serialized string szSerialized to file
    char fileout[256];
    model_t model=MODEL_STATIC;             //static/adaptive model for arithmetic coding
    memset(fileout, 0, sizeof fileout);     //clear array
    strcpy(fileout,szFilePath);             // copy string into the result
    strcat(fileout,"/");                    // append /
    strcat(fileout,szFileName);             // copy string into the result
    strcat(fileout,".cry");                 // append extension "cry" for crypto-compressed

    DIR *dir;
    if ((dir = opendir ( szFilePath )) == NULL)
    {
        std::cout << "\nDestination directory does not exist! " << szFilePath <<std::endl<< "File not saved. EXIT_FAILURE"<< std::endl;
        exit(EXIT_FAILURE);
    }

    FILE *fpout = fopen( fileout, "wb" );
    if ( fpout != NULL)
    {
        myArEncodeString(szSerialized, fpout, model); //arithmetic endcode the serialized vector & save to file
        fclose(fpout);
    }
}

int write_img_file(char* szFilePath, char* szFileName, cv::Mat img, char* szExtension)
{
    char fileout[256];
    memset(fileout, 0, sizeof fileout);     //clear array
    strcpy(fileout,szFilePath);             // copy string into the result
    strcat(fileout,"/");                    // append /
    strcat(fileout,szFileName);             // copy string into the result
    //fileout[ strlen(fileout)-4]='\0';     // remove extension .cry
    strcat(fileout,".");                    // append extension
    strcat(fileout,szExtension);            // append extension

    DIR *dir;
    if ((dir = opendir ( szFilePath )) == NULL)
    {
        std::cout << "\nDestination directory does not exist! " << szFilePath <<std::endl<< "File not saved. EXIT_FAILURE"<< std::endl;
        exit(EXIT_FAILURE);
    }
    if (szExtension == "jpg")
    {
        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(100);
        cv::imwrite( fileout, img, compression_params );
    }
    else
    {
        cv::imwrite( fileout, img );
    }
    int sizeInBytes = img.step[0] * img.rows; //size in bytes
    return sizeInBytes;
}

cv::Mat read_img_file(char* szFilePath, char* szFileName) {
		string cFileName = szFilePath;
		cFileName.append("/");
		cFileName.append(szFileName);

    return cv::imread(cFileName);
}

int batch_write_img_file(char* dirin, char* dirout, char* szExtension) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) == NULL) return -1;

    while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
    {
        char* name= ent->d_name;
        if (name[0]!=46) //igore filenames starting with . (46='.')
        {
            cv::Mat img = read_img_file( dirin, name );
            write_img_file( dirout, name, img, szExtension);
        }
    }
    closedir (dir);
    return 0;
}

void batch_img_decode() {
    int sizeInBytes = 0;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(filePathCompressed)) != NULL) {
			while ((ent = readdir(dir)) != NULL) {
	        char* name = ent->d_name;

	        if (name[0]!=46) {
	            cv::Mat imgout = decode(filePathCompressed, name);
	            sizeInBytes += write_img_file(filePathDecompressed, name, imgout);
	        }
	    }

	    closedir(dir);

	    std::cout << "Size of decoded directory in Bytes: " << sizeInBytes << std::endl;
		}
}

void batch_img_encode() {
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(filePathOriginals)) != NULL) {
			while ((ent = readdir(dir)) != NULL) {
	        char* name = ent->d_name;

	        if (name[0] != 46) {
	            cv::Mat img = read_img_file(filePathOriginals, name);
	            string szSerialized = encode(img);
	            write_coded_file(filePathCompressed, name, szSerialized);
	        }
	    }

	    closedir(dir);
		}
}

int main(int argc, char *argv[]) {
	initializeData();

  ///load pics from a directory, compress and save
	gettimeofday(&tic, NULL);
  batch_img_encode();

  gettimeofday(&toc, NULL);
	std::cout << "Total time for encoding = " << (double) (toc.tv_usec - tic.tv_usec) / 1000000 +	(double) (toc.tv_sec - tic.tv_sec) << " seconds" << std::endl;

  ///load encoded images, decompress and save
  gettimeofday(&tic, NULL);
  batch_img_decode();

  gettimeofday(&toc, NULL);
	std::cout << "Total time for decoding = " << (double) (toc.tv_usec - tic.tv_usec) / 1000000 +	(double) (toc.tv_sec - tic.tv_sec) << " seconds" << std::endl;

	return 0;
}
