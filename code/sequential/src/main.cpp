#include "../includes/encode.hpp"
#include "../includes/decode.hpp"
#include "../includes/optlist.hpp"
#include "../includes/utils.hpp"
#include <iostream>

using namespace std;

option_t *optList, *thisOpt;
FILE *inFile, *outFile;
bool toEncode;
model_t model;

vector<unsigned int> nData;
vector<unsigned int> nLimited;
vector<unsigned int> nCoded;
vector<unsigned int> nDecoded;
vector<unsigned int> K(3);

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

void encode() {
	cout << "Encoding file...";

	nData = open_raw_file(inFile);
	nLimited = generate_limited_data(nData);
	K = generate_random_key(nData);
	nCoded = generate_coded_vector(nData, K);

	vector<unsigned int> coded_vector = K;

	coded_vector.push_back(nLimited.size());
	coded_vector.push_back(nCoded.size());
	coded_vector.insert(coded_vector.end(), nLimited.begin(), nLimited.end());
	coded_vector.insert(coded_vector.end(), nCoded.begin(), nCoded.end());

	string sMsg = vec2string(coded_vector);
	ArEncodeString(sMsg, outFile, model);

	fclose(outFile);

	cout << "DONE!" << endl;
}

void decode() {
	cout << "Decoding file...";

	string sMsg = ArDecodeFile(inFile, model);
	fclose(inFile);

	vector<unsigned int> data = string2vec(sMsg);

	K[0] = data[0]; K[1] = data[1]; K[2] = data[2];
	unsigned int nL = data[3]; unsigned int nC = data[4];
	unsigned int iFirst = 5; unsigned int iLast = 5 + nL; unsigned int iLen = iLast - iFirst;

	nLimited.resize(iLen);
	memcpy(&nLimited[0], &data[iFirst], iLen*sizeof(unsigned int));

	iFirst = iLast; iLast = iFirst + nC; iLen = iLast - iFirst;

	nCoded.resize(iLen);
	memcpy(&nCoded[0], &data[iFirst], iLen*sizeof(unsigned int));

	nDecoded = decode_vector(nCoded, nLimited, K);

	for (unsigned int i = 0; i < nDecoded.size(); i++) {
		fprintf(outFile, "%c", nDecoded[i]);
	}

	fclose(outFile);

	cout << "DONE!" << endl;
}

int main(int argc, char *argv[]) {
	initializeData();

	getOpt(argc, argv);
	validateCommandLine(argv);

	if (toEncode)
		encode();
	else
		decode();

	return 0;
}
