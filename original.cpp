#include <iostream>
#include <string.h>
#include <stdio.h>
#include "optlist.h"
#include <stdlib.h>
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

using namespace std;
using namespace cv;

/***************************************************************************
*              Sample Program Using Arithmetic Encoding Library
*
*   File    : sample.c
*   Purpose : Demonstrate usage of arithmetic encoding library
*   Author  : Michael Dipperstein
*   Date    : March 10, 2004
*
****************************************************************************
*
* SAMPLE: Sample usage of the arcode Arithmetic Encoding Library
* Copyright (C) 2004, 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the arcode library.
*
* The arcode library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The arcode library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "optlist.h"
#include "arcode.h"

/***************************************************************************
*                                FUNCTIONS DCT and IDCT
***************************************************************************/
#include <math.h>
#include <stdlib.h>

void dct(float **DCTMatrix, float **Matrix, int N, int M);
void write_mat(FILE *fp, float **testRes, int N, int M);
void idct(float **Matrix, float **DCTMatrix, int N, int M);
float **calloc_mat(int dimX, int dimY);
void free_mat(float **p);

float **calloc_mat(int dimX, int dimY)
{
    float **m = (float**)calloc(dimX, sizeof(float*));
    float *p  = (float*)calloc(dimX*dimY, sizeof(float));
    int i;
    for(i=0; i <dimX;i++){
    m[i] = &p[i*dimY];

    }
   return m;
}

void free_mat(float **m){
  free(m[0]);
  free(m);
}

void write_mat(FILE *fp, float **m, int N, int M){

   int i, j;
   for(i =0; i< N; i++){
    fprintf(fp, "%f", m[i][0]);
    for(j = 1; j < M; j++){
       fprintf(fp, "\t%f", m[i][j]);
        }
    fprintf(fp, "\n");
   }
   fprintf(fp, "\n");
}

void dct(float **DCTMatrix, float **Matrix, int N, int M){

    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
        DCTMatrix[u][v] = 0;
            for (i = 0; i < N; i++) {
                for (j = 0; j < M; j++) {
                    DCTMatrix[u][v] += Matrix[i][j] * cos(M_PI/((float)N)*(i+1./2.)*u)*cos(M_PI/((float)M)*(j+1./2.)*v);
                }
            }
        }
    }
 }

struct dct_struct{
    float **DCTMatrix;
    float **Matrix;
    int N; //dimX
    int M; //dimY
};
void* mydct(void *arguments) //(float **DCTMatrix, float **Matrix, int N, int M)
{
    struct dct_struct *args = (dct_struct*)arguments;
    float **DCTMatrix   = args->DCTMatrix;
    float **Matrix      = args->Matrix;
    int N               = args->N;
    int M               = args->M;
    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
        DCTMatrix[u][v] = 0;
            for (i = 0; i < N; i++) {
                for (j = 0; j < M; j++) {
                    DCTMatrix[u][v] += Matrix[i][j] * cos(M_PI/((float)N)*(i+1./2.)*u)*cos(M_PI/((float)M)*(j+1./2.)*v);
                }
            }
        }
    }
    return NULL;
 }

void idct(float **Matrix, float **DCTMatrix, int N, int M){
    int i, j, u, v;

    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
          Matrix[u][v] = 1/4.*DCTMatrix[0][0];
          for(i = 1; i < N; i++){
          Matrix[u][v] += 1/2.*DCTMatrix[i][0];
           }
           for(j = 1; j < M; j++){
          Matrix[u][v] += 1/2.*DCTMatrix[0][j];
           }

           for (i = 1; i < N; i++) {
                for (j = 1; j < M; j++) {
                    Matrix[u][v] += DCTMatrix[i][j] * cos(M_PI/((float)N)*(u+1./2.)*i)*cos(M_PI/((float)M)*(v+1./2.)*j);
                }
            }
        Matrix[u][v] *= 2./((float)N)*2./((float)M);
        }
    }
 }

void* myidct(void* arguments) //(float **Matrix, float **DCTMatrix, int N, int M)
{
    struct dct_struct *args = (dct_struct*)arguments;
    float **DCTMatrix   = args->DCTMatrix;
    float **Matrix      = args->Matrix;
    int N               = args->N;
    int M               = args->M;
    int i, j, u, v;
    for (u = 0; u < N; ++u) {
        for (v = 0; v < M; ++v) {
          Matrix[u][v] = 1/4.*DCTMatrix[0][0];
          for(i = 1; i < N; i++){
          Matrix[u][v] += 1/2.*DCTMatrix[i][0];
           }
           for(j = 1; j < M; j++){
          Matrix[u][v] += 1/2.*DCTMatrix[0][j];
           }

           for (i = 1; i < N; i++) {
                for (j = 1; j < M; j++) {
                    Matrix[u][v] += DCTMatrix[i][j] * cos(M_PI/((float)N)*(u+1./2.)*i)*cos(M_PI/((float)M)*(v+1./2.)*j);
                }
            }
        Matrix[u][v] *= 2./((float)N)*2./((float)M);
        }
    }
    return NULL;
 }


//***********************************************************************************************************
//new gmpr code developed by Marcos Rodrigues [October 2016] starts here....

vector<int> string2vec( string sMsg )
{
    vector<int> vOut;
    istringstream is( sMsg );
    int n;
    while( is >> n )
    {
    	vOut.push_back( n );
    }
    return vOut;
}


//Some global variables to facilitate thread functions
std::vector<vector<int> > nDecoded;  //2D decoded vector by multiple threads
int nThreads;          //how many threads were ever spawn
int nThreadCount;      //how many threads are there at execution time
int nThreadRange;      //range of each thread (how many numbers each thread decodes)

std::vector<int> sort_limited_by_counts( std::vector<int> vnLimited, std::vector<int> vnData) //not used
{
    std::vector<int> counts( vnLimited.size(), 0);

    for (int i=0; i<vnLimited.size(); i++)
    {
        for (int j=0; j<vnData.size(); j++)
        {
            if (vnLimited[i] == vnData[j])
            {
                counts[i]++;
            }
        }
    }
    std::vector<int> counts_sorted = counts;
    std::sort( counts_sorted.rbegin(), counts_sorted.rend() ); //sort descending
    std::vector<int> limited_sorted( vnLimited.size() );
    //nLimitedCounts = counts_sorted;

    int k=0;                                        //find the indices of sorted in vnLimited
    for (int i=0; i<counts_sorted.size(); i++)
    {
        for (int j=0; j<counts.size(); j++)
        {
            if (counts_sorted[i] == counts[j])
            {
                limited_sorted[k] = vnLimited[j];   //j is the index I am looking for
                counts[j] = -1;                     //make j invalid as there is only one occurrence
                k++;
            }
        }
    }

    return limited_sorted;
}

void generate_key(std::vector<int> vnData, int nStartValue=1, int nFactor=2) //not used
{
    int fMaxValue = 2*max_in_vector(vnData);
    K[0]=nStartValue;
    K[1]=((nStartValue*fMaxValue)+1);
    K[2]=((K[1]*fMaxValue)+1)* nFactor;
}

std::vector<int> open_coded_file(FILE *fp) //fp is already open
{
    long lSize; //number of characters in the file
    char *cbuffer;

    //fopen ( fp ); //the file is already open
    if( !fp ) perror("Error file does not exist."),exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    cbuffer = (char*)calloc( 1, lSize+1 ); // allocate memory for entire content
    if( !cbuffer ) fclose(fp),fputs("Memory alloc fails.",stderr),exit(1);

    if( 1!=fread( cbuffer , lSize, 1 , fp) ) // copy the file into the buffer
      fclose(fp),free(cbuffer),fputs("Entire fread fails.",stderr),exit(1);

    rewind( fp );

    std::vector<int>tmp(lSize);
    int k=0;
    while (!feof (fp))
    {
      fscanf (fp, "%d", &tmp[k]);
      k++;
    }
    fclose (fp);

    std::vector<int>data(k); //k is the correct size of the input vector
    data = tmp;
    return data;
}


void save_coded_file(FILE *fp, std::vector<int> K, std::vector<int> vnLimited, std::vector<int> vnCoded) //not used
{
    //not usedd at the moment
    //save keys, size of limited, size of coded, limited, coded:
    fprintf(fp,"%i\n",K[0]);
    fprintf(fp,"%i\n",K[1]);
    fprintf(fp,"%i\n",K[2]);
    fprintf(fp, "%u\n", (int)vnLimited.size());
    fprintf(fp, "%u\n", (int)vnCoded.size());
    for (int i=0; i<vnLimited.size(); i++) fprintf(fp,"%d ",vnLimited[i]);
    fprintf(fp,"\n");
    for (int i=0; i<vnCoded.size(); i++) fprintf(fp,"%u ",vnCoded[i]);
    fclose(fp);
}

void *decode_vector_segment(void* argument)
{
    int nThisThread = *((int*) argument);
    int nLastIndex  = nCoded.size();
    int nLow        = nThisThread * nThreadRange;
    int nHigh       = (nLow+nThreadRange)>nLastIndex?nLastIndex:(nLow+nThreadRange);

    for (int r=nLow; r<nHigh; r++) //decode nRange numbers
    {
        for (int i=0; i<nLimited.size(); i++)
        {
            for (int j=0; j<nLimited.size(); j++)
            {
                for (int k=0; k<nLimited.size(); k++)
                {
                    if (r != nLastIndex-1)
                    {
                        if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k] ) //found it
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            nDecoded[nThisThread][3*(r-nLow)+2]  = nLimited[k];
                            break;
                        }
                    }
                    else    // There are three possibilities:
                    {
                        if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j] + K[2]*nLimited[k] ) //3 entries
                        {
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            nDecoded[nThisThread][3*(r-nLow)+2]  = nLimited[k];
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            break;
                        }
                         else if (nCoded[r] == K[0]*nLimited[i] + K[1]*nLimited[j]) //this is the last number coded into 2.
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            nDecoded[nThisThread][3*(r-nLow)+1]  = nLimited[j];
                            break;
                        }
                        else if  (nCoded[r] == K[0]*nLimited[i]) //this is the last number coded into 1.
                        {
                            nDecoded[nThisThread][3*(r-nLow)  ]  = nLimited[i];
                            break;
                        }
                    }
                }
            }
        }
    }
    __sync_fetch_and_sub(&nThreadCount,1); //counter is decremented as each thread exits
    return 0L;
}


void decode_vector(std::vector<int> cdata, int nRange=100)
{
    nThreadRange    = nRange; //update global variable
    nThreadCount    = cdata.size()%nThreadRange==0?cdata.size()/nThreadRange:cdata.size()/nThreadRange+1;//how many threads
    int nLastEntry  = cdata.size()%nThreadRange>0?cdata.size()%nThreadRange:nThreadRange;
    nThreads        = nThreadCount; //how many threads will be created
    nDecoded.resize(nThreads);
    for (int i=0; i<nThreads; i++)
    {
        if ( i!= nThreads-1 )
            nDecoded[i].resize(3*nThreadRange);
        else
            nDecoded[i].resize(3*nLastEntry);
    }

    pthread_t *tr[nThreads]; //array of thread ids
    for (int i=0; i<nThreads; i++)
    {
        pthread_create((pthread_t *)&tr[i], NULL, &decode_vector_segment, (void*)&i);
        waitKey(1);
    }
    do
    {
        __sync_synchronize();
    } while (nThreadCount);
    return;
}


int batch_encode(char* dirin, char* dirout, const model_t model)
{
    char filein[256];
    char fileout[256];
    int  counter = 0;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) != NULL)
    {
        while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
        {
            char* name= ent->d_name;
            if (name[0]==46) //igore filenames starting with . (46='.')
            {
                counter+=0; //do nothing
            }
            else
            {
                memset(filein, 0, sizeof filein);//clear array
                strcpy(filein,dirin);            // copy string one into the result.
                strcat(filein,"/");              // append string two to the result.
                strcat(filein,name);
                FILE *fpin;
                fpin = fopen(filein, "rb");
                nData    = open_raw_file( fpin ); //this file is closed within open_file function
                nLimited = generate_limited_data(nData);
                nLimited = sort_limited_by_counts( nLimited, nData);
                generate_random_key(nData);
                nCoded   = generate_coded_vector(nData);

                std::string s = name;
                int Len = s.length();
                name[Len-1]='c'; //add c to file extention
                name[Len]='.';
                memset(fileout, 0, sizeof fileout); //clear array
                strcpy(fileout,dirout);             // copy string one into the result.
                strcat(fileout,"/");                // append string two to the result.
                strcat(fileout,name);
                FILE *fpout;
                fpout = fopen(fileout, "w");
                //save_coded_file(fpout, K, nLimited, nCoded);//this file is closed within save_coded function

                vector<int> coded_vector = K;
                coded_vector.push_back( nLimited.size());
                coded_vector.push_back( nCoded.size());
                coded_vector.insert(coded_vector.end(), nLimited.begin(), nLimited.end());
                coded_vector.insert(coded_vector.end(), nCoded.begin(), nCoded.end());
                string sMsg = vec2string( coded_vector );
                //printf("%s", sMsg.c_str());
                myArEncodeString(sMsg, fpout, model);
                fclose(fpout);
                counter++;
            }
        }
        closedir (dir);
        //cout<<"\nEncoded "<<counter<<" files."<<"\n";
        //cout<<"source     : "<<dirin<<"\n";
        //cout<<"destination: "<<dirout<<"\n";
        return 0;
    }
    else
    {
        return -1; //failure
    }
}

int batch_decode(char* dirin, char* dirout, const model_t model)
{
    char filein[256];
    char fileout[256];
    int  counter = 0;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ( dirin )) != NULL)
    {
        while ((ent = readdir (dir)) != NULL) //read all file names, open, compress, and save compressed
        {
            char* name= ent->d_name;
            if (name[0]==46) //igore filenames starting with . (46='.')
            {
                //do nothing
            }
            else
            {
                memset(filein, 0, sizeof filein);//clear array
                strcpy(filein,dirin);            // copy string one into the result.
                strcat(filein,"/");              // append string two to the result.
                strcat(filein,name);
                FILE *fpin;
                fpin = fopen(filein, "rb");
                string sMsg = myArDecodeFile(fpin, model);
                fclose(fpin);
                std::vector<int> data = string2vec( sMsg );
                //std::vector<int>data = open_coded_file( fpin );    //this file is closed within open_file function
                K[0]     = data[0];
                K[1]     = data[1];
                K[2]     = data[2];
                int nL   = data[3];
                int nC   = data[4];
                //cout<<"\nK[0],K[1],K[2]: "<<K[0]<<" "<<K[1]<<" "<<K[2]<<" nL="<<nL<<" nC="<<nC<<"\n";
                int iFirst = 5;    //first index to copy to limited data
                int iLast  = 5+nL; //last index + 1
                int iLen   = iLast - iFirst;

                nLimited.resize(iLen); //pre-allocate the space needed to write the data directly
                memcpy(&nLimited[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer
                //cout<<"\nnlimite[0]: "<<nLimited[0];
                //cout<<"\nnLimited[last]: "<<nLimited[nLimited.size()-1];

                iFirst = 5+nL;
                iLast  = 5+nL+nC;
                iLen   = iLast - iFirst;
                nCoded.resize(iLen);
                //cod.resize(iLen); //pre-allocate the space needed to write the data directly
                memcpy(&nCoded[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer
                //for (int i=0; i<cdata.size(); i++) printf("%d ",cdata[i]);
                decode_vector( nCoded );

                std::string s = name;
                int Len = s.length();
                name[Len-2]='d'; //add d to file extention
                memset(fileout, 0, sizeof fileout); //clear array
                strcpy(fileout,dirout);             // copy string one into the result.
                strcat(fileout,"/");                // append string two to the result.
                strcat(fileout,name);
                //cout<<"\nfile:"<<name<<" nThreads:"<<nThreads;

                int N=nThreads;
                //cout<<"\nfileout: "<<fileout;
                FILE *fpout;
                fpout = fopen(fileout, "w");
                for (int i=0; i<N; i++)
                {
                    for (int j=0; j<nDecoded[i].size(); j++)
                    {
                        fprintf(fpout,"%c", nDecoded[i][j]);
                        //printf("%d", nDecoded[i][j]);
                    }
                }
                fclose(fpout);

                counter++;
            }
        }
        closedir (dir);
        //cout<<"\nDecoded "<<counter<<" files."<<"\n";
        //cout<<"source     : "<<dirin<<"\n";
        //cout<<"destination: "<<dirout<<"\n";
        return 0;
    }
    else
    {
        return -1; //failure
    }
}


//*******************************************************************************
int main(int argc, char *argv[])
{
    else //decode using multiple threads
    {
        struct timeval tic, toc; gettimeofday(&tic,NULL);
        string sMsg = myArDecodeFile(inFile, model);
        fclose(inFile);
        std::vector<int> data = string2vec( sMsg );
        K[0]     = data[0];
        K[1]     = data[1];
        K[2]     = data[2];
        int nL   = data[3];
        int nC   = data[4];
        int iFirst = 5;    //first index to copy to limited data
        int iLast  = 5+nL; //last index + 1
        int iLen   = iLast - iFirst;

        nLimited.resize(iLen); //pre-allocate the space needed to write the data directly
        memcpy(&nLimited[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer

        iFirst = iLast; //5+nL
        iLast  = iFirst+nC;//5+nL+nC;
        iLen   = iLast - iFirst;
        nCoded.resize(iLen); //pre-allocate the space needed to write the data directly
        memcpy(&nCoded[0], &data[iFirst], iLen*sizeof(int)); //write directly to destination buffer from source buffer

        decode_vector( nCoded, 25 ); //multiple threads here each thread decodes 100 numbers
        for (int i=0; i<nThreads; i++)
        {
            for (int j=0; j<nDecoded[i].size(); j++)
            {
                fprintf(outFile,"%c",nDecoded[i][j]); //save decoded file
            }
        }
        fclose(outFile);

        gettimeofday(&toc,NULL);
        printf ("Total time = %f seconds\n",
            (double) (toc.tv_usec - tic.tv_usec) / 1000000 +
            (double) (toc.tv_sec - tic.tv_sec));
    }

    return 0;
}
