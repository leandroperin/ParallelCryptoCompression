#include "../includes/utils.hpp"

// std::vector<int> open_raw_file(FILE *fp) //file pointer is already open when this function is called!
// {
//     // long lSize; //number of characters in the file
//     // char *cbuffer;
//     //
//     // //fopen ( fp ); //the file is already open
//     // if( !fp ) perror("Error file does not exist."),exit(1);
//     //
//     // fseek( fp , 0L , SEEK_END);
//     // lSize = ftell( fp );
//     // rewind( fp );
//     //
//     // cbuffer = (char*)calloc( 1, lSize+1 ); // allocate memory for entire content
//     // if( !cbuffer ) fclose(fp),fputs("Memory alloc fails.",stderr),exit(1);
//     //
//     // if( 1!=fread( cbuffer , lSize, 1 , fp) ) // copy the file into the buffer
//     //   fclose(fp),free(cbuffer),fputs("Entire fread function fails. Something wrong with the file.",stderr),exit(1);
//     //
//     // fclose(fp);
//     //
//     // // 'conversion' to ASCII/UTF happens here, also eliminate negative numbers
//     // std::vector<int>tmp(lSize);
//     // int k=0;
//     // for (int i = 0; i < lSize; i++)
//     // {
//     //     if (cbuffer[i]>0)
//     //     {
//     //         tmp[k] = cbuffer[i];
//     //         k++;
//     //     }
//     // }
//     //
//     // std::vector<int>data(k); //k is the correct size of the input vector
//     // data = tmp; //tmp is longer than data, so tmp is cut off to the size of data
//     // return data;
// }
