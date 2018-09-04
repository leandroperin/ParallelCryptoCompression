#include "../includes/encode.hpp"
#include "../includes/decode.hpp"
#include "../includes/utils.hpp"

#include <dirent.h>
#include <sys/time.h>
#include "opencv2/opencv.hpp"

FILE *inFile;
model_t model;

struct timeval tic, toc;

char* filePathOriginals;
char* filePathCompressed;
char* filePathDecompressed;

std::vector<int> nDecoded;

std::string read_coded_file(char* szFilePath, char* szFileName) {
  char cFileName[256];
  memset(cFileName, 0, sizeof cFileName);
  strcpy(cFileName,szFilePath);
  strcat(cFileName,"/");
  strcat(cFileName,szFileName);

  inFile = fopen(cFileName, "rb");

  if (inFile == NULL) {
    std::cout << "Cannot Open File to Decode!" << std::endl;
    return NULL;
  }

  std::string szSerialized = ArDecodeFile(inFile, model);

  fclose(inFile);

  return szSerialized;
}

void write_img_file(char* szFilePath, char* szFileName, cv::Mat img, char* szExtension = "bmp") {
  std::string fileout = szFilePath;
  fileout.append("/");
  fileout.append(szFileName);
  fileout.append(".");
  fileout.append(szExtension);

  DIR *dir;

  if ((dir = opendir(szFilePath)) == NULL) {
    std::cout << "Destination directory does not exist: " << szFilePath << std::endl;
    std::cout << "File not saved: EXIT_FAILURE" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (szExtension == "jpg") {
    std::vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);
    cv::imwrite(fileout, img, compression_params);
  } else {
    cv::imwrite(fileout, img);
  }
}

void initializeData() {
  inFile = NULL;
  model = MODEL_STATIC;

  filePathOriginals  = "/home/lperin/imgtest/originals";
  filePathCompressed = "/home/lperin/imgtest/compressed";
  filePathDecompressed = "/home/lperin/imgtest/decompressed";
}

cv::Mat generate_quantization_matrix(int Blocksize, int R) {
  cv::Mat Q(Blocksize, Blocksize, CV_32F, cv::Scalar::all(0));

  for (int r = 0; r < Blocksize; r++) {
    for (int c = 0; c < Blocksize; c++) {
      Q.at<float>(r,c) = (float)(r+c)*R;

      if (r == 0 & c == 0) Q.at<float>(r,c) = 1;
    }
  }

  return Q;
}

cv::Mat decode(char* filePathIn, char* fileName) {
  std::string szSerialized = read_coded_file(filePathIn, fileName);
  std::string szTmp, szCompact, szNumbers;

  for (auto it=szSerialized.begin(); it!=szSerialized.end(); ++it) {
    std::string szSpace { *it };
    szTmp += *it;
    szCompact += *it;

    if (szSpace == " ")
      if (szTmp.size() < 10) {
	szNumbers += szTmp;
	szTmp = szCompact = "";
      }
  }

  std::vector<int> data = string2vec(szNumbers);

  int COLS = data[0]; int ROWS = data[1]; int planes = data[2];
  int R = data[3]; int Blocksize = data[4]; int bQ = data[5];
  int DCheight = data[6]; int ACwidth = data[7]; int ACheight = data[8];

  int ZeroLocSize[] = {data[9],  data[10], data[11]}; int TwoLocSize[] = {data[12], data[13], data[14]};
  int ThreeLocSize[] = {data[15], data[16], data[17]}; int NegLocSize[] = {data[18], data[19], data[20]};
  int CodedNegLocSize[] = {data[21], data[22], data[23]}; int UniqueNegSize[] = {data[24], data[25], data[26]};
  int CompactOneSize[] = {data[27], data[28], data[29]}; int CompactTwoSize[] = {data[30], data[31], data[32]};
  int CompactThreeSize[] = {data[33], data[34], data[35]};

  std::vector<std::vector<int>> KNeg(planes);

  int nStart = 36;
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = first + 3;
    KNeg[i] = std::vector<int> (first, last); nStart += 3;
  }

  std::vector<std::vector<int>> vnDiffDC(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + DCheight;
    vnDiffDC[i] = std::vector<int> (first, last); nStart += DCheight;
  }

  std::vector<std::vector<int>> vnDiffZeroLoc(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + ZeroLocSize[i];
    vnDiffZeroLoc[i] = std::vector<int> (first, last); nStart += ZeroLocSize[i];
  }

  std::vector<std::vector<int>> vnDiffTwoLoc(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + TwoLocSize[i];
    vnDiffTwoLoc[i] = std::vector<int> (first, last); nStart += TwoLocSize[i];
  }

  std::vector<std::vector<int>> vnDiffThreeLoc(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + ThreeLocSize[i];
    vnDiffThreeLoc[i] = std::vector<int> (first, last); nStart += ThreeLocSize[i];
  }

  std::vector<std::vector<int>> vnCodedDiffNegLoc(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + CodedNegLocSize[i];
    vnCodedDiffNegLoc[i] = std::vector<int> (first, last); nStart += CodedNegLocSize[i];
  }

  std::vector<std::vector<int>> vnUniqueNeg(planes);
  for (int i = 0; i < planes; i++) {
    auto first = data.begin() + nStart; auto last  = data.begin() + nStart + UniqueNegSize[i];
    vnUniqueNeg[i] = std::vector<int> (first, last); nStart += UniqueNegSize[i];
  }

  std::vector<std::string> szCompactOneData(planes); std::vector<std::string> szCompactTwoData(planes); std::vector<std::string> szCompactThreeData(planes);
  std::vector<std::vector<int>> vnOneData(planes); std::vector<std::vector<int>> vnTwoData(planes); std::vector<std::vector<int>> vnThreeData(planes);

  nStart = 0;
  for (int i = 0; i < planes; i++) {
    szCompactOneData[i] = szCompact.substr(nStart, CompactOneSize[i]);
    vnOneData[i] = compactstring2vec(szCompactOneData[i], 1);
    nStart += CompactOneSize[i];
  }
  for (int i = 0; i < planes; i++) {
    szCompactTwoData[i] = szCompact.substr (nStart, CompactTwoSize[i]);
    vnTwoData[i] = compactstring2vec(szCompactTwoData[i], 2);
    nStart += CompactTwoSize[i];
  }
  for (int i = 0; i < planes; i++) {
    szCompactThreeData[i]= szCompact.substr (nStart, CompactThreeSize[i]);
    vnThreeData[i] = compactstring2vec(szCompactThreeData[i], 3);
    nStart += CompactThreeSize[i];
  }

  std::vector<std::vector<int>> vnNonZeroData(planes); std::vector<std::vector<int>> vnTwoLoc(planes);
  std::vector<std::vector<int>> vnThreeLoc(planes); std::vector<std::vector<int>> vSize(planes);
  std::vector<std::vector<int>> vnDC(planes);

  for (int i = 0; i < planes; i++) {
    int index2 = vnDiffTwoLoc[i][0];
    int index3 = vnDiffThreeLoc[i][0];

    vnTwoLoc[i].push_back(index2);
    vnThreeLoc[i].push_back(index3);

    for (int j = 1; j < vnDiffTwoLoc[i].size(); j++) {
      index2 += vnDiffTwoLoc[i][j];
      vnTwoLoc[i].push_back(index2);
    }
    for (int j = 1; j < vnDiffThreeLoc[i].size(); j++) {
      index3 += vnDiffThreeLoc[i][j];
      vnThreeLoc[i].push_back(index3);
    }

    vnNonZeroData[i] = std::vector<int> (vnOneData[i].size() + vnTwoData[i].size() + vnThreeData[i].size(), -1);

    for (int j = 0; j < vnTwoLoc[i].size(); j++) {
      vnNonZeroData[i][vnTwoLoc[i][j]] = vnTwoData[i][j];
    }
    for (int j = 0; j<vnThreeLoc[i].size(); j++) {
      vnNonZeroData[i][vnThreeLoc[i][j]] = vnThreeData[i][j];
    }

    int idx = 0;
    for (int j = 0; j < vnNonZeroData[i].size(); j++) {
      if (vnNonZeroData[i][j] == -1) {
	vnNonZeroData[i][j] = vnOneData[i][idx];
	idx++;
      }
    }

    int diff = vnDiffDC[i][0];
    vnDC[i].push_back(diff);
    for (int j = 1; j < vnDiffDC[i].size(); j++) {
      diff += vnDiffDC[i][j];
      vnDC[i].push_back(diff);
    }
  }

  std::vector<std::vector<int>> vnDiffNegLoc(planes);
  for (int i = 0; i < planes; i++) {
    std::vector<int> newVec = vnCodedDiffNegLoc[i];
    nDecoded = decode_vector_binary(newVec, vnUniqueNeg[i], KNeg[i]);

    if (nDecoded.size() > NegLocSize[i]) {
      int diff = nDecoded.size() - NegLocSize[i];
      auto first = nDecoded.begin();
      auto last  = first + nDecoded.size() - diff;
      nDecoded = std::vector<int> (first, last);
    }

    vnDiffNegLoc[i] = nDecoded;

    std::vector<int> vNegIndices;
    int index = vnDiffNegLoc[i][0];
    vNegIndices.push_back(index);

    for (int j = 1; j < vnDiffNegLoc[i].size(); j++) {
      index += vnDiffNegLoc[i][j];
      vNegIndices.push_back(index);
    }

    for (int j = 0; j < vNegIndices.size(); j++)
      vnNonZeroData[i][vNegIndices[j]] *= -1;
  }

  std::vector<std::vector<int>> vnDctACcol(planes);
  std::vector<std::vector<int>> vnZeroLoc(planes);

  for (int i = 0; i < planes; i++) {
    std::vector<int> vec(vnNonZeroData[i].size() + vnDiffZeroLoc[i].size(), -1);

    int index0 = 0;
    for (int j = 0; j < vnDiffZeroLoc[i].size(); j++) {
      index0 += vnDiffZeroLoc[i][j];
      vec[index0] = 0;
    }

    int index = 0;
    for (int j = 0; j < vec.size(); j++) {
      if ( vec[j] == -1) {
	vec[j] = vnNonZeroData[i][index];
	index++;
      }
    }

    vnDctACcol[i] = vec;
  }


  std::vector<cv::Mat> vmatPlanes(planes);
  std::vector<cv::Mat> vmatOutPlanes(planes);

  for (int i = 0; i < planes; i++) {
    cv::Mat img2(ROWS, COLS, CV_32F, cv::Scalar(0));
    int start = 0;
    int step = (Blocksize/2)*(Blocksize/2)-1;
    int dcIndex = 0; int acIndex = 0;

    for (int r = 0; r < ROWS-Blocksize; r += Blocksize) {
      for (int c = 0; c < COLS-Blocksize; c += Blocksize) {
	auto first = vnDctACcol[i].begin() + start;
	auto last  = first + step;
	std::vector<int> vec(first, last);

	cv::Mat roi (Blocksize/2, Blocksize/2, CV_32F);
	int idx = 0;

	for (int j = 0; j < Blocksize/2; j++)
	  for (int k = 0; k < Blocksize/2; k++)
	    if (j == 0 & k == 0) {
	      roi.at<float>(j, k) = float(vnDC[i][dcIndex]);
	      dcIndex++;
	    } else {
	      roi.at<float>(j,k) = float(vec[idx]);
	      idx++;
	    }

	if (bQ) {
	  cv::Mat Q = generate_quantization_matrix(Blocksize, R);
	  cv::multiply(roi, Q, roi);
	}

	roi.copyTo(img2(cv::Rect(c, r, roi.cols, roi.rows)));

	cv::Mat roi2 = img2(cv::Rect(c,r,Blocksize,Blocksize));
	cv::Mat dst;

	cv::idct(roi2, dst);

	dst.convertTo(dst, CV_8UC1);
	dst.copyTo(img2(cv::Rect(c, r, dst.cols, dst.rows)));

	start += step;
      }
    }

    vmatPlanes[i] = img2.clone();
    vmatPlanes[i].convertTo(vmatPlanes[i], CV_8UC1);
  }

  cv::Mat mergedImg;
  cv::merge(vmatPlanes, mergedImg);

  return mergedImg;
}

std::string encode(cv::Mat matImg) {
  int COLS = matImg.size().width;
  int ROWS = matImg.size().height;

  std::vector<cv::Mat> planes;
  split(matImg, planes);

  int planesSize = planes.size();

  int R = 1;
  int Blocksize = 8;
  int bQ = 0;
  cv::Mat Q = generate_quantization_matrix(Blocksize, R);

  std::vector<cv::Mat> matDctData(planesSize);

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

  std::vector<cv::Mat> matNonZeroLocXY(planesSize);
  std::vector<cv::Mat> matZeroLocXY(planesSize);
  std::vector<cv::Mat> matOneLocXY(planesSize);
  std::vector<cv::Mat> matTwoLocXY(planesSize);
  std::vector<cv::Mat> matThreeLocXY(planesSize);
  std::vector<cv::Mat> matNegLocXY(planesSize);

  std::vector<cv::Mat> matDctDC(planesSize);
  std::vector<cv::Mat> matDctAC(planesSize);
  std::vector<cv::Mat> matDctACcol(planesSize);
  std::vector<cv::Mat> matNonZeroData(planesSize);

  std::vector<std::vector<int>> vnDctACcol(planesSize);
  std::vector<std::vector<int>> vnNonZeroData(planesSize);
  std::vector<std::vector<int>> vnDC(planesSize);
  std::vector<std::vector<int>> vnZeroLoc(planesSize);
  std::vector<std::vector<int>> vnTwoLoc(planesSize);
  std::vector<std::vector<int>> vnThreeLoc(planesSize);
  std::vector<std::vector<int>> vnNegLoc(planesSize);

  std::vector<std::vector<int>> vnCodedDC(planesSize);
  std::vector<std::vector<int>> vnCodedNegLoc(planesSize);
  std::vector<std::vector<int>> vnUniqueNegLoc(planesSize);

  std::vector<std::vector<int>> vnOneData(planesSize);
  std::vector<std::vector<int>> vnTwoData(planesSize);
  std::vector<std::vector<int>> vnThreeData(planesSize);

  std::vector<std::string> szCompactOneData(planesSize);
  std::vector<std::string> szCompactTwoData(planesSize);
  std::vector<std::string> szCompactThreeData(planesSize);

  std::vector<std::vector<int>> KNegLoc(planesSize);

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

    cv::Point pt = matZeroLocXY[i].at<cv::Point>(0);
    int index = pt.y;
    vnZeroLoc[i].push_back(index);

    for (int j = 1; j < matZeroLocXY[i].size().height; j++) {
      cv::Point pnt = matZeroLocXY[i].at<cv::Point>(j);
      vnZeroLoc[i].push_back(pnt.y - index);
      index = pnt.y;
    }

    for (int j = 0; j < matNonZeroLocXY[i].size().height; j++) {
      cv::Point pnt = matNonZeroLocXY[i].at<cv::Point>(j);
      int nValue = matDctACcol[i].at<int>(pnt.y, pnt.x);
      vnNonZeroData[i].push_back(nValue);

      cv::Mat row(1, 1, CV_32S);
      row.at<int>(0, 0) = nValue;
      matNonZeroData[i].push_back(row);
    }

    matBoolean.release();
    matBoolean = matNonZeroData[i] < 0;
    cv::findNonZero(matBoolean, matNegLocXY[i]);

    pt = matNegLocXY[i].at<cv::Point>(0);
    index = pt.y;
    vnNegLoc[i].push_back(index);

    for (int j = 1; j < matNegLocXY[i].size().height; j++) {
      cv::Point pnt = matNegLocXY[i].at<cv::Point>(j);
      vnNegLoc[i].push_back(pnt.y - index);
      index = pnt.y;
    }

    cv::Mat matAbsNonZeroAC = abs(matNonZeroData[i].clone());

    matBoolean.release();
    matBoolean = (matAbsNonZeroAC < 10);

    cv::Mat matOneLocXY;
    cv::findNonZero(matBoolean, matOneLocXY);

    for (int j = 0; j < matOneLocXY.size().height; j++) {
      cv::Point pnt = matOneLocXY.at<cv::Point>(j);
      int nValue = matAbsNonZeroAC.at<int>(pnt.y, pnt.x);
      vnOneData[i].push_back(nValue);
    }

    matBoolean.release();
    matBoolean = (matAbsNonZeroAC > 9 & matAbsNonZeroAC < 100);
    cv::findNonZero(matBoolean, matTwoLocXY[i]);

    if (matTwoLocXY[i].size().height > 0) {
      pt = matTwoLocXY[i].at<cv::Point>(0);
      index = pt.y;
      vnTwoLoc[i].push_back(index);

      int value = matAbsNonZeroAC.at<int>(index, pt.x);
      vnTwoData[i].push_back(value);

      for (int j = 1; j < matTwoLocXY[i].size().height; j++) {
	cv::Point pnt = matTwoLocXY[i].at<cv::Point>(j);
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
      pt = matThreeLocXY[i].at<cv::Point>(0);
      index = pt.y;
      vnThreeLoc[i].push_back(index);

      int value = matAbsNonZeroAC.at<int>(index, pt.x);
      vnThreeData[i].push_back(value);

      for (int j = 1; j < matThreeLocXY[i].size().height; j++) {
	cv::Point pnt = matThreeLocXY[i].at<cv::Point>(j);
	vnThreeLoc[i].push_back(pnt.y - index);
	index = pnt.y;

	int nValue = matAbsNonZeroAC.at<int>(index, pnt.x);
	vnThreeData[i].push_back(nValue);
      }
    }

    szCompactOneData[i] = vec2compactstring(vnOneData[i]);
    szCompactTwoData[i] = vec2compactstring(vnTwoData[i]);
    szCompactThreeData[i] = vec2compactstring(vnThreeData[i]);

    std::vector<std::vector<int>> vcodednloc = code_vector(vnNegLoc[i]);

    KNegLoc[i] = vcodednloc[0];
    vnUniqueNegLoc[i] = vcodednloc[1];
    vnCodedNegLoc[i] = vcodednloc[2];
  }

  std::vector<int> vnSerialized;
  std::string szCompact;

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
  for (int i = 0; i < planesSize; i++) vnSerialized.push_back(vnUniqueNegLoc[i].size());
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

  for (int i = 0; i < planesSize; i++) szCompact += szCompactOneData[i];
  for (int i = 0; i < planesSize; i++) szCompact += szCompactTwoData[i];
  for (int i = 0; i < planesSize; i++) szCompact += szCompactThreeData[i];

  std::string szSerialized = vec2string(vnSerialized);
  szSerialized += " " + szCompact;

  return szSerialized;
}

void write_coded_file(char* szFilePath, char* szFileName, std::string szSerialized) {
  char fileout[256];
  memset(fileout, 0, sizeof fileout);
  strcpy(fileout, szFilePath);
  strcat(fileout, "/");
  strcat(fileout, szFileName);
  strcat(fileout, ".cry");

  FILE *fpout = fopen(fileout, "wb");

  if (fpout != NULL) {
    myArEncodeString(szSerialized, fpout, model);
    fclose(fpout);
  }
}

cv::Mat read_img_file(char* szFilePath, char* szFileName) {
  std::string cFileName = szFilePath;
  cFileName.append("/");
  cFileName.append(szFileName);

  return cv::imread(cFileName);
}

void batch_img_decode() {
  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir(filePathCompressed)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      char* name = ent->d_name;

      if (name[0]!=46) {
	cv::Mat imgout = decode(filePathCompressed, name);
	write_img_file(filePathDecompressed, name, imgout);
      }
    }

    closedir(dir);
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
	std::string szSerialized = encode(img);
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
