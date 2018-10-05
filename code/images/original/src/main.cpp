#include "../includes/encode.hpp"
#include "../includes/decode.hpp"
#include "../includes/optlist.hpp"
#include "../includes/utils.hpp"
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <iterator>
#include <vector>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <thread>
#include <algorithm>    // std::sort

#include "opencv2/opencv.hpp" //for image processing later on
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ml/ml.hpp"

// Example showing how to read and write images
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <ctime>


using namespace std;
using namespace cv;

option_t *optList, *thisOpt;
FILE *inFile, *outFile;
bool toEncode;
model_t model;

vector<int> nData;
vector<int> nLimited;
vector<int> nCoded;
vector<int> nDecoded;
vector<int> K(3);

void initializeData() {
	inFile = NULL;
	outFile = NULL;
	toEncode = true;
	model = MODEL_STATIC;
}

void getOpt(int argc, char *argv[]) {
	optList = GetOptList(argc, argv, (char*)"acdi:o:h?");
	thisOpt = optList;

	while (thisOpt != NULL) {
			switch (thisOpt->option) {
					case 'a':
							model = MODEL_ADAPTIVE;
							break;
					case 'c':
							toEncode = true;
							break;
					case 'd':
							toEncode = false;
							break;
					case 'i':
							if (inFile != NULL)	{
									printf("Multiple input files not allowed.\n");
									fclose(inFile);

									if (outFile != NULL)
											fclose(outFile);

									FreeOptList(optList);
									exit(EXIT_FAILURE);
							}
							else if ((inFile = fopen(thisOpt->argument, "rb")) == NULL) {
									perror("Opening Input File");

									if (outFile != NULL)
											fclose(outFile);

									FreeOptList(optList);
									exit(EXIT_FAILURE);
							}
							break;
					case 'o':
							if (outFile != NULL) {
									printf("Multiple output files not allowed.\n");
									fclose(outFile);

									if (inFile != NULL)
											fclose(inFile);

									FreeOptList(optList);
									exit(EXIT_FAILURE);
							}
							else if ((outFile = fopen(thisOpt->argument, "wb")) == NULL) {
									perror("Opening Output File");

									if (inFile != NULL)
											fclose(inFile);

									FreeOptList(optList);
									exit(EXIT_FAILURE);
							}
							break;
					case 'h':
					case '?':
						printf("Usage: %s <options>\n\n", FindFileName(argv[0]));
						printf("options:\n");
						printf("  -c : Encode input file to output file.\n");
						printf("  -d : Decode input file to output file.\n");
						printf("  -i <filename> : Name of input file.\n");
						printf("  -o <filename> : Name of output file.\n");
						printf("  -a : Use adaptive model instead of static.\n");
						printf("  -h | ?  : Print out command line options.\n\n");
						printf("Default: %s -c\n", FindFileName(argv[0]));

					FreeOptList(optList);
					exit(EXIT_SUCCESS);
			}

			optList = thisOpt->next;
			free(thisOpt);
			thisOpt = optList;
	}
}

void validateCommandLine(char *argv[]) {
	if (inFile == NULL) {
			fprintf(stderr, "Input file must be provided\n");
			fprintf(stderr, "Enter \"%s -?\" for help.\n", FindFileName(argv[0]));

			if (outFile != NULL)
					fclose(outFile);

			exit (EXIT_FAILURE);
	}	else if (outFile == NULL) {
			fprintf(stderr, "Output file must be provided\n");
			fprintf(stderr, "Enter \"%s -?\" for help.\n", FindFileName(argv[0]));

			fclose(inFile);

			exit (EXIT_FAILURE);
	}
}

///Function prototypes:
std::string read_coded_file( char* szFilePath, char* szFileName );
void write_coded_file(char* szFilePath, char* szFileName, std::string szSerialized );
cv::Mat read_img_file( char* szFilePath, char* szFileName );
size_t  write_img_file(char* szFilePath, char* szFileName, cv::Mat img, char* szExtension="bmp");
int batch_write_img_file(char* dirin, char* dirout, char* szExtension="bmp");
int batch_img_encode(char* dirin, char* dirout);
int batch_Img_decode(char* dirin, char* dirout);


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


std::string encode(cv::Mat matImg)
{
    /********************** The algorithm: ***************************************************
    The DCT transform generates a matrix (say 8x8) where the element at(0,0) is the DC component
    and all other elements are called the AC coefficients. We need to separate the DC component
    from the rest of AC, then apply arithmetic coding to the DC and we are free to manipulate the
    AC components as we please but, at the end, we also apply arithmetic coding to the AC data.

    An example of DCT applied to a 4x4 block (original image data not shown):
        [ 1670  1  -1   2
           -12  0  45 -32
             0 -1  0  123
          -439  2 -1    3 ]
    The task is first to reshape the matrix into a row vector and concatenate into a larger matrix:
        [ 1670  1  -1   2  -12  0  45 -32  0 -1  0  123 -439  2 -1  3
           ...  ... other blocks go here, one block per row   ... .. ]

    Then separate all DC (first column of the matrix above) from the AC coefficients. Now organize
    the AC coefficients into a single column matrix (or row, we prefer column). On the AC data,
    find the y location of all zeros, two and three digits, and the location of negative numbers.
    Suppose an AC as above:
        [ 1;  -1;   2;  -12;  0;  45; -32;  0; -1;  0;  123; -439;  2; -1;  3; ]

    zeroLoc  = [ 4 7 9 ]
    Now we remove the zeros from the AC data and we are left with nonzerodata:
        [ 1;  -1;   2;  -12;  45; -32;  -1;  123; -439;  2; -1;  3; ]

    twoLoc   = [ 3 4 5 ]            with diffTwoLoc   = [ 3 1 1 ]
    threeLoc = [ 7 8 ]              with diffThreeLoc = [ 7 1 ]
    negLoc   = [ 1 3 5 6 8 10 ]     with diffNegLoc   = [ 1 2 2 1 2 2 ]
    (oneLoc is superfluous because we already know all the other locations)

    Then:
    1. Make all data positive
    2. Code the diff negative locations with the triple key.
    3. Code diff one-digit, two-digit, and three-digit data as compact string
    4. Apply arithmetic coding to all data, save to file
    5. To decode, follow the inverse path, and use the binary search for decoding the negative locations.
    **/


    //cout<<"image size:"<<matImg.size()<<endl;
    int COLS = matImg.size().width;
    int ROWS = matImg.size().height;

    vector<Mat> planes; // Split the image into planes
    split(matImg, planes);

        ///Indented code starting with /// for debubbing purposes only
        ///vector<Mat> outplanes(planes.size());
        ///for (size_t i = 0; i < planes.size(); i++)
        ///{
        ///    Mat img( ROWS, COLS, CV_32F, Scalar(0)); //create a matrix and initialize to zeros
        ///    outplanes[i] = img; //assign the zero matrix to outplanes[i]
        ///}

    ///R, Blocksize and bQ are fixed in this implementation. Will make these as parameters in version 2.
    int R = 1;          //2; //parameter used in quantization R=1,2,3,...
    int Blocksize = 8;  //Blocksize = 2,4,8,16,... means 2x2,4x4,8x8,...
    int bQ = 0;         //boolean value, either to apply quantization or not
    Mat Q  = generate_quantization_matrix( Blocksize, R );

    ///run dct per block and place into a row vector then push into matDctData matrix
    vector<Mat> matDctData(planes.size());     ///to keep the transformed data, one matrix per plane
    for (size_t i = 0; i < planes.size(); i++) //for each plane
    {
        planes[i].convertTo(planes[i], CV_32F); //32F as dct only works with float/double
                ///indented code starting with /// for debugging purposes only, delete later on
                ///Mat img( ROWS, COLS, CV_32F);

        for (int r=0; r<ROWS-Blocksize; r+=Blocksize)
        {
            for (int c=0; c<COLS-Blocksize; c+=Blocksize)
            {
                Mat roi = planes[i]( Rect(c,r,Blocksize,Blocksize) ); //x,y,w,h
                Mat dst, dst_half;
                dct(roi, dst);
                    ///indented code starting with /// for debugging purposes only, delete later on
                    ///Mat dst2, dst3;
                    ///idct(dst, dst3);
                    ///dst3.convertTo(dst3, CV_8UC1);

                if (bQ) divide(dst, Q, dst); //Quantization, divide by Q element by element
                dst_half = dst( Rect(0,0,Blocksize/2,Blocksize/2)); //x,y,w,h
                dst_half.convertTo(dst_half, CV_32S, 1.0, 0.0); //convert to int
                    ///indented code starting with /// for debugging purposes only, delete later on
                    ///dst_half.copyTo(img(Rect(c, r, dst_half.cols, dst_half.rows)));
                    ///cout<<"img half_dst:"<< img(Rect(c,r,Blocksize/2,Blocksize/2));
                    ///Mat roi2 = img(Rect(c,r,Blocksize,Blocksize));
                    ///if (c==0 & r==0) cout<<"roi2 original"<<roi2;
                    ///idct(roi2, dst2);
                    ///dst2.copyTo(img(Rect(c, r, dst2.cols, dst2.rows)));
                    ///if (c==0 & r==0) cout<<"idct(roi2,dst2)"<<dst2;
                    ///cout<<"roi2 reconstructed:"<<endl<<dst2<<endl;
                    ///cout<<"dst_half float:"<<endl<<dst_half<<endl;

                ///Reshape matrix dst_half into a single row
                Mat row( 1, (Blocksize/2)*(Blocksize/2), CV_32S); //row of int, 32-bit Signed
                int idx = 0;
                for (int j=0; j<dst_half.size().height; j++)
                {
                    for (int k=0; k<dst_half.size().width; k++)
                    {
                        row.at<int>(0,idx) = dst_half.at<int>(j,k);
                        idx++;
                    }
                }
                matDctData[i].push_back(row); //concatenate with existing data
            }
        }
            ///outplanes[i] = img;
            ///outplanes[i].convertTo(outplanes[i], CV_8UC1);
    }
        ///Mat merged;
        ///merge(outplanes, merged);
        ///namedWindow("Original image with half block size", CV_WINDOW_AUTOSIZE);
        ///imshow("Original image with half block size", merged);
        ///waitKey(1);

    vector<Mat> matNonZeroLocXY(planes.size());    ///this is a 2xN matrix of [x,y] points, note that is x,y
    vector<Mat> matZeroLocXY(planes.size());
    vector<Mat> matOneLocXY(planes.size());
    vector<Mat> matTwoLocXY(planes.size());
    vector<Mat> matThreeLocXY(planes.size());
    vector<Mat> matNegLocXY(planes.size());

    vector<Mat> matDctDC(planes.size());            //mat of DC components (one column matrix)
    vector<Mat> matDctAC(planes.size());            //mat of AC coefficients (mxn matrix)
    vector<Mat> matDctACcol(planes.size());         //one column matrix version of matDctAC
    vector<Mat> matNonZeroData(planes.size());      //one column matDctACcol without zeros

    std::vector<vector<int> > vnDctACcol(planes.size());        //vector version of AC data, all data including zeros
    std::vector<vector<int> > vnNonZeroData(planes.size());     //vector version of AC data, exluding zeros
    std::vector<vector<int> > vnDC(planes.size());              //vector DC data differences
    std::vector<vector<int> > vnZeroLoc(planes.size());         //vector of zero locations. Need this
    std::vector<vector<int> > vnTwoLoc(planes.size());
    std::vector<vector<int> > vnThreeLoc(planes.size());
    std::vector<vector<int> > vnNegLoc(planes.size());

    std::vector<vector<int> > vnCodedDC(planes.size()); //DC differences, this is the only coding applied to these
    std::vector<vector<int> > vnCodedNegLoc(planes.size());
    std::vector<vector<int> > vnUniqueNegLoc(planes.size());

    std::vector<vector<int> > vnOneData(planes.size());       //one-digit data
    std::vector<vector<int> > vnTwoData(planes.size());       //two digit data
    std::vector<vector<int> > vnThreeData(planes.size());     //three digit data

    std::vector<std::string> szCompactOneData(planes.size());     //remove all spaces between values
    std::vector<std::string> szCompactTwoData(planes.size());
    std::vector<std::string> szCompactThreeData(planes.size());

    std::vector<vector<int> > KNegLoc(planes.size()); //coding triple key

    for (size_t i = 0; i < planes.size(); i++)
    {
        ///cout<<"\n1. Plane ["<<i<<"]: Create matrices for DC components and AC coefficients"<<endl;
        matDctDC[i] = matDctData[i]( Rect(0,0,1,matDctData[i].rows)).clone(); //extract the DC components, first column
        matDctAC[i] = matDctData[i]( Rect(1,0, matDctData[i].cols-1, matDctData[i].rows)).clone(); //extract the AC coefficients

        ///cout<<"2. Extract the DC components and code their differences"<<endl;
        int DC = matDctDC[i].row(0).at<int>(0,0);
        vnDC[i].push_back(DC); //save first value as is
        for (int j=1; j<matDctDC[i].size().height; j++) //difference starts at index 1, or second value
        {
            Mat row = matDctDC[i].row(j);
            int value = row.at<int>(0,0);
            vnDC[i].push_back(value - DC);
            DC=value;
        }

        ///cout<<"3. Now deal with AC coefficients. First transpose all AC coefficients into a single column matrix"<<endl;
        for (int j=0; j<matDctAC[i].size().height; j++)
        {
            Mat row = matDctAC[i].row(j);
            Mat rowt = row.t();             //transpose a row to a one-column
            matDctACcol[i].push_back(rowt); //append the column to one-column matrix
            for (int k=0; k<row.size().width; k++)
            {
                float value = row.at<int>(0,k);
                vnDctACcol[i].push_back(value); //vector version of matDctAC[i]
            }
        }

        ///cout<<"4. Find the locations of zeros within matDctACcol"<<endl;
        Mat matBoolean; matBoolean = matDctACcol[i] != 0; //this is a binary matrix required by function findNonZero
        findNonZero(matBoolean, matNonZeroLocXY[i]);
        matBoolean.release(); matBoolean = matDctACcol[i] == 0;
        findNonZero(matBoolean, matZeroLocXY[i]);

        ///cout<<"5. Extract the location of zeros and code their differences"<<endl;
        Point pt = matZeroLocXY[i].at<Point>(0); //retrieve the first point (x,y)
        int index = pt.y; //first index
        vnZeroLoc[i].push_back(pt.y); //save first index as is
        for (int j=1; j<matZeroLocXY[i].size().height; j++)
        {
            Point pnt = matZeroLocXY[i].at<Point>(j);    //access each zero location
            vnZeroLoc[i].push_back( pnt.y - index);
            index = pnt.y;
        }

        ///cout<<"6. Extract all non-zero data and place into a vector and into a matrix"<<endl;
        for (int j=0; j<matNonZeroLocXY[i].size().height; j++)
        {
            Point pnt = matNonZeroLocXY[i].at<Point>(j);    //access each non-zero location
            int nValue = matDctACcol[i].at<int>(pnt.y,pnt.x);  //extract its value
            vnNonZeroData[i].push_back(nValue);   //append to vector

            Mat row( 1, 1, CV_32S); //create a row matrix 1 row, 1 column
            row.at<int>(0,0) = nValue; //set the value at index 00
            matNonZeroData[i].push_back(row); //append to matrix
        }

        ///From now on, all operations are on non-zero data matrix. We already know where the zeros are in the original matrix
        ///cout<<"7. Find the location of negatives and code their differences"<<endl;
        matBoolean.release();  matBoolean = matNonZeroData[i] < 0;
        findNonZero(matBoolean, matNegLocXY[i]);
        pt = matNegLocXY[i].at<Point>(0); //retrieve the first point (x,y)
        index = pt.y; //first index
        vnNegLoc[i].push_back(pt.y); //save first index
        for (int j=1; j<matNegLocXY[i].size().height; j++)
        {
            Point pnt = matNegLocXY[i].at<Point>(j);    //access each non-zero location
            vnNegLoc[i].push_back( pnt.y - index);
            index = pnt.y;
        }

        ///cout<<"8. Find the location of one digit data and keep their values"<<endl;
        Mat matAbsNonZeroAC = abs(matNonZeroData[i].clone()); //copy the contents to a new matrix, abs it
        matBoolean.release();  matBoolean = (matAbsNonZeroAC<10); //one digit
        Mat matOneLocXY;
        findNonZero(matBoolean, matOneLocXY);
        for (int j=0; j<matOneLocXY.size().height; j++)
        {
            Point pnt = matOneLocXY.at<Point>(j);    //access each non-zero location
            int nValue = matAbsNonZeroAC.at<int>(pnt.y,pnt.x);  //extract its value
            vnOneData[i].push_back(nValue);  //append to vector
        }

        ///cout<<"9. Extract the location of two digit data and code their differences. Keep the actual data values"<<endl;
        matBoolean.release();  matBoolean = (matAbsNonZeroAC > 9 & matAbsNonZeroAC <100); //two digits
        findNonZero(matBoolean, matTwoLocXY[i]);
        if (matTwoLocXY[i].size().height > 0) //it is possible that there are no 2 digit data
        {
            pt = matTwoLocXY[i].at<Point>(0); //retrieve the first point (x,y)
            index = pt.y; //first index
            vnTwoLoc[i].push_back(pt.y); //save first inde
            int value = matAbsNonZeroAC.at<int>(pt.y,pt.x);  //extract its value
            vnTwoData[i].push_back(value);  //save it
            for (int j=1; j<matTwoLocXY[i].size().height; j++)
            {
                Point pnt = matTwoLocXY[i].at<Point>(j);    //access each non-zero location
                vnTwoLoc[i].push_back( pnt.y - index);
                index = pnt.y;
                int nValue = matAbsNonZeroAC.at<int>(pnt.y,pnt.x);  //extract its value
                vnTwoData[i].push_back(nValue);  //append to vector
            }
        }

        ///cout<<"10. Extract the location of three digit data and code their differences. Keep the actual data values."<<endl;
        matBoolean.release();  matBoolean = (matAbsNonZeroAC > 99 & matAbsNonZeroAC < 1000); //three digits
        findNonZero(matBoolean, matThreeLocXY[i]);
        if (matThreeLocXY[i].size().height > 0)
        {
            pt = matThreeLocXY[i].at<Point>(0); //retrieve the first point (x,y)
            index = pt.y; //first index
            vnThreeLoc[i].push_back(pt.y); //save first index
            int value = matAbsNonZeroAC.at<int>(pt.y,pt.x);  //extract its value
            vnThreeData[i].push_back(value);  //save it
            for (int j=1; j<matThreeLocXY[i].size().height; j++)
            {
                Point pnt = matThreeLocXY[i].at<Point>(j);    //access each non-zero location
                vnThreeLoc[i].push_back( pnt.y - index);
                index = pnt.y;
                int nValue = matAbsNonZeroAC.at<int>(pnt.y,pnt.x);  //extract its value
                vnThreeData[i].push_back(nValue);  //append to vector
            }
        }

        ///cout<<"11. Convert data to compact string (without spaces between numbers)"<<endl;
        szCompactOneData[i]   = vec2compactstring(vnOneData[i]);
        szCompactTwoData[i]   = vec2compactstring(vnTwoData[i]);
        szCompactThreeData[i] = vec2compactstring(vnThreeData[i]);

        ///cout<<"12. Apply the MM algorithm: code negative loc by triple key "<<endl;
        std::vector<std::vector<int> > vcodednloc = code_vector(vnNegLoc[i]); //return key, unique, data, padding
        KNegLoc[i]        = vcodednloc[0];          //key
        vnUniqueNegLoc[i] = vcodednloc[1];          //unique data
        vnCodedNegLoc[i]  = vcodednloc[2];          //coded data
        int paddingnloc   = vcodednloc[3].at(0);    //padding
    }

    ///cout<<"13. Serialize the data as follows."<<endl;
    std::vector<int> vnSerialized;
    std::string szCompact;
    vnSerialized.push_back( matImg.size().width );
    vnSerialized.push_back( matImg.size().height );
    vnSerialized.push_back( planes.size() );
    vnSerialized.push_back( R );
    vnSerialized.push_back( Blocksize );
    vnSerialized.push_back( bQ );
    vnSerialized.push_back( matDctDC[0].size().height );
    vnSerialized.push_back( matDctAC[0].size().width );
    vnSerialized.push_back( matDctAC[0].size().height );

    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnZeroLoc[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnTwoLoc[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnThreeLoc[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnNegLoc[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnCodedNegLoc[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( vnUniqueNegLoc[i].size() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( szCompactOneData[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( szCompactTwoData[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.push_back( szCompactThreeData[i].size());
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), KNegLoc[i].begin(), KNegLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnDC[i].begin(), vnDC[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnZeroLoc[i].begin(), vnZeroLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnTwoLoc[i].begin(), vnTwoLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnThreeLoc[i].begin(), vnThreeLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnCodedNegLoc[i].begin(), vnCodedNegLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) vnSerialized.insert(vnSerialized.end(), vnUniqueNegLoc[i].begin(), vnUniqueNegLoc[i].end() );
    for (size_t i = 0; i < planes.size(); i++) szCompact +=  szCompactOneData[i];
    for (size_t i = 0; i < planes.size(); i++) szCompact +=  szCompactTwoData[i];
    for (size_t i = 0; i < planes.size(); i++) szCompact +=  szCompactThreeData[i];
    std::string szSerialized = vec2string( vnSerialized );
    szSerialized+=" "+szCompact; //to separate compact string from the rest

    //cout<<"\nData encoded!\n\n"<<endl;
    return szSerialized;
}

cv::Mat decode(char* filePathIn, char* fileName )
{
	std::string szSerialized = read_coded_file( filePathIn, fileName );
    std::string szTmp, szCompact, szNumbers;
    for ( std::string::iterator it=szSerialized.begin(); it!=szSerialized.end(); ++it) //parse string
    {
        std::string szSpace;
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
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + 3;
        vector<int> newVec(first, last);
        KNeg[i] = newVec;
        nStart += 3;
    }

    std::vector<vector<int> > vnDiffDC(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + DCheight;
        vector<int> newVec(first, last);
        vnDiffDC[i] = newVec;
        nStart += DCheight;
    }

    std::vector<vector<int> > vnDiffZeroLoc(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + ZeroLocSize[i];
        vector<int> newVec(first, last);
        vnDiffZeroLoc[i] = newVec;
        nStart += ZeroLocSize[i];
    }

    std::vector<vector<int> > vnDiffTwoLoc(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + TwoLocSize[i];
        vector<int> newVec(first, last);
        vnDiffTwoLoc[i] = newVec;
        nStart += TwoLocSize[i];
    }

    std::vector<vector<int> > vnDiffThreeLoc(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + ThreeLocSize[i];
        vector<int> newVec(first, last);
        vnDiffThreeLoc[i] = newVec;
        nStart += ThreeLocSize[i];
    }

    std::vector<vector<int> > vnCodedDiffNegLoc(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + CodedNegLocSize[i];
        vector<int> newVec(first, last);
        vnCodedDiffNegLoc[i] = newVec;
        nStart += CodedNegLocSize[i];
    }

    std::vector<vector<int> > vnUniqueNeg(planes);
    for (size_t i = 0; i < planes; i++)
    {
        vector<int>::const_iterator first = data.begin() + nStart;
        vector<int>::const_iterator last  = data.begin() + nStart + UniqueNegSize[i];
        vector<int> newVec(first, last);
        vnUniqueNeg[i] = newVec;
        nStart += UniqueNegSize[i];
    }

    std::vector<std::string> szCompactOneData(planes);
    std::vector<std::string> szCompactTwoData(planes);
    std::vector<std::string> szCompactThreeData(planes);
    std::vector<vector<int> > vnOneData(planes);
    std::vector<vector<int> > vnTwoData(planes);
    std::vector<vector<int> > vnThreeData(planes);

    nStart=0;
    for (size_t i = 0; i < planes; i++)
    {
        szCompactOneData[i]= szCompact.substr (nStart, CompactOneSize[i]);
        vnOneData[i] = compactstring2vec(szCompactOneData[i], 1);
        nStart += CompactOneSize[i];
    }
    for (size_t i = 0; i < planes; i++)
    {
        szCompactTwoData[i]= szCompact.substr (nStart, CompactTwoSize[i]);
        vnTwoData[i] = compactstring2vec(szCompactTwoData[i], 2);
        nStart += CompactTwoSize[i];
    }
    for (size_t i = 0; i < planes; i++)
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


    for (size_t i = 0; i < planes; i++)
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
    for (size_t i = 0; i < planes; i++)
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
    for (size_t i = 0; i < planes; i++)
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
    for (size_t i = 0; i < planes; i++)
    {
        Mat img2( ROWS, COLS, CV_32F, Scalar(0));
        int start   = 0;
        int step    = (Blocksize/2)*(Blocksize/2)-1; //steps in the vnDctACcol vector
        int dcIndex = 0;
        int acIndex = 0;

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




std::string read_coded_file( char* szFilePath, char* szFileName )
{
    char cFileName[256];
    memset(cFileName, 0, sizeof cFileName);   //clear array
    strcpy(cFileName,szFilePath);             // copy string into the result
    strcat(cFileName,"/");                    // append /
    strcat(cFileName,szFileName);             // copy string into the result

    inFile = fopen(cFileName, "rb");
    if (inFile == NULL)
    {
        cout << "Cannot Open File to Decode!" << endl;
        return NULL;
    }

	string szSerialized = ArDecodeFile(inFile, model);
	fclose(inFile);
	return szSerialized;
}


void write_coded_file(char* szFilePath, char* szFileName, std::string szSerialized )
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
        cout << "\nDestination directory does not exist! " << szFilePath <<endl<< "File not saved. EXIT_FAILURE"<< endl;
        exit(EXIT_FAILURE);
    }

    FILE *fpout = fopen( fileout, "wb" );
    if ( fpout != NULL)
    {
        myArEncodeString(szSerialized, fpout, model); //arithmetic endcode the serialized vector & save to file
        fclose(fpout);
    }
}

cv::Mat read_img_file( char* szFilePath, char* szFileName )
{
    char cFileName[256];
    memset(cFileName, 0, sizeof cFileName);     //clear array
    strcpy(cFileName,szFilePath);             // copy string into the result
    strcat(cFileName,"/");                    // append /
    strcat(cFileName,szFileName);             // copy string into the result

    Mat img = imread(cFileName, 1);
    if (img.empty() || (img.data == NULL))
    {
        cout << "\nCan't load image from " << cFileName << "!" << endl<< "EXIT_FAILURE."<< endl;
        exit(EXIT_FAILURE);
    }
    return img;
}

size_t write_img_file(char* szFilePath, char* szFileName, cv::Mat img, char* szExtension)
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
        cout << "\nDestination directory does not exist! " << szFilePath <<endl<< "File not saved. EXIT_FAILURE"<< endl;
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
    size_t sizeInBytes = img.step[0] * img.rows; //size in bytes
    return sizeInBytes;
}


int batch_write_img_file(char* dirin, char* dirout, char* szExtension)
{
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


int batch_img_encode(char* dirin, char* dirout)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) == NULL) return -1;

    while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
    {
        char* name= ent->d_name;
        if (name[0]!=46) //igore filenames starting with . (46='.')
        {
            cv::Mat img = read_img_file( dirin, name );
            std::string szSerialized = encode( img );
            write_coded_file( dirout, name, szSerialized );
        }
    }
    closedir (dir);
    return 0;
}

int batch_img_decode(char* dirin, char* dirout)
{
    int sizeInBytes=0;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) == NULL) return -1;

    while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
    {
        char* name= ent->d_name;
        if (name[0]!=46) //igore filenames starting with . (46='.')
        {
            cv::Mat imgout = decode( dirin, name );
            sizeInBytes += write_img_file( dirout, name, imgout );
        }
    }
    closedir (dir);
    return 0;
}


int main(int argc, char *argv[]) {
	initializeData();

	//getOpt(argc, argv);
	//validateCommandLine(argv);
    struct timeval tic, toc;

	///load pics from a directory and save into another in a specified format
    gettimeofday(&tic,NULL);
    char* filePathIn  = "/Users/mar/Caco/Compress/IAB/ParallelCryptoCompression-master-image/parallel/pics";
    char* filePathOut = "/Users/mar/Caco/Compress/IAB/ParallelCryptoCompression-master-image/parallel/picsjpg";
    batch_write_img_file( filePathIn, filePathOut, "jpg");
    gettimeofday(&toc,NULL);
    // printf ("Total time for saving as bmp (from arbitrary formats) = %f seconds\n",
            // (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            // (double) (toc.tv_sec - tic.tv_sec));


    ///load pics from a directory, compress and save
    gettimeofday(&tic,NULL);
    filePathIn  = "/home/lperin/imgtest/originals";
    filePathOut = "/home/lperin/imgtest/compressed";
    batch_img_encode(filePathIn, filePathOut);
    gettimeofday(&toc,NULL);
    printf ("Total time for encoding = %f seconds\n",
            (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            (double) (toc.tv_sec - tic.tv_sec));


    ///load encoded images, decompress and save
    gettimeofday(&tic,NULL);
    filePathIn  = "/home/lperin/imgtest/compressed";
    filePathOut = "/home/lperin/imgtest/decompressed";
    batch_img_decode(filePathIn, filePathOut);
    gettimeofday(&toc,NULL);
    printf ("Total time for decoding = %f seconds\n",
            (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            (double) (toc.tv_sec - tic.tv_sec));


	return 0;
}
