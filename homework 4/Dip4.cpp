//============================================================================
// Name        : Dip4.cpp
// Author      : Ronny Haensch
// Version     : 2.0
// Copyright   : -
// Description : 
//============================================================================

#include "Dip4.h"

// Performes a circular shift in (dx,dy) direction
/*
in       :  input matrix
dx       :  shift in x-direction
dy       :  shift in y-direction
return   :  circular shifted matrix
*/
Mat Dip4::circShift(Mat& in, int dx, int dy){

   // sanitze input
   
   dx = dx % in.cols;
   dy = dy % in.rows;

   Mat out = Mat::zeros(in.rows, in.cols, CV_32FC1);

   for (int x = 0; x < out.rows; x++) for (int y = 0; y < out.cols; y++) {
      
      int newX = (x + dx) % out.cols;
      int newY = (y + dy) % out.rows;
      
      newX = newX < 0 ? out.cols + newX : newX;
      newY = newY < 0 ? out.rows + newY : newY;
      out.at<float>(newX, newY) = in.at<float>(x, y);

   };

   return out;

}

// Function applies inverse filter to restorate a degraded image
/*
degraded :  degraded input image
filter   :  filter which caused degradation
return   :  restorated output image
*/
Mat Dip4::inverseFilter(Mat& degraded, Mat& filter){

  Mat tempA = Mat::zeros(degraded.size(), CV_32FC1);
  
  // O = FFT(s) / Q
  // Q = ...

  // convert to frequency spectrum
  Mat degradedFreq = degraded.clone();
  Mat filterFreq = Mat::zeros(degraded.size(), CV_32F);

  // add Border
  for (int x = 0; x < filter.rows; x++) for (int y = 0; y < filter.cols; y++) {
    filterFreq.at<float>(x, y) = filter.at<float>(x, y);
  }
   
  // filterFreq = circShift(filterFreq, -1, -1);

  // transform to complex - no idea what i'm doing here
  Mat planes[] = {degradedFreq, Mat::zeros(degraded.size(), CV_32F)};
  Mat planesFilter[] = {filterFreq, Mat::zeros(filterFreq.size(), CV_32F)};
  
  merge(planes, 2, degradedFreq);
  merge(planesFilter, 2, filterFreq);

  dft(degradedFreq, degradedFreq, DFT_COMPLEX_OUTPUT); // degradedFreq == S
  dft(filterFreq, filterFreq, DFT_COMPLEX_OUTPUT); // filterFreq == P


  // create Q

  split(filterFreq, planes);

  Mat Re = planes[0];
  Mat Im = planes[1];
  
  // calculate Threshold
  double thresholdFactor = 0.2, threshold;
  double max = 0;

  Re.copyTo(tempA);
  abs(tempA);
  minMaxIdx(tempA, 0, &max, 0, 0, Mat());

  cout << "Max: " << max << endl;
  
  threshold = thresholdFactor * max;

  for (int x = 0; x < filterFreq.rows; x++) for (int y = 0; y < filterFreq.cols; y++) {

    if (Re.at<float>(x, y) >= threshold) {

      float realsq = Re.at<float>(x, y) * Re.at<float>(x, y);
      float imsq = Im.at<float>(x, y) * Im.at<float>(x, y);

      // complex numbers need special attention

      Re.at<float>(x, y) = Re.at<float>(x, y) / (realsq + imsq);
      Im.at<float>(x, y) = Im.at<float>(x, y) / (realsq + imsq);

    } else {
      Re.at<float>(x, y) = 1/threshold;
      Im.at<float>(x, y) = 1/threshold;
    }

  }
  
  Mat Q = Mat::zeros(filterFreq.size(), CV_32F);
  
  merge(planes, 2, Q);

  Mat original;

  mulSpectrums(degradedFreq, Q, original, 1);
  dft(original, original, DFT_INVERSE + DFT_SCALE);
  split(original, planes);
 
  return planes[0];
}

// Function applies wiener filter to restorate a degraded image
/*
degraded :  degraded input image
filter   :  filter which caused degradation
snr      :  signal to noise ratio of the input image
return   :   restorated output image
*/
Mat Dip4::wienerFilter(Mat& degraded, Mat& filter, double snr){
   
  // Q_k = conjugate_transpose(P_k) / | P_k | ^2  + 1/SNR^2

  Mat filterFreq = Mat::zeros(degraded.size(), CV_32F);
  Mat degradedFreq = degraded.clone();

  // add Border
  for (int x = 0; x < filter.rows; x++) for (int y = 0; y < filter.cols; y++) {
    filterFreq.at<float>(x, y) = filter.at<float>(x, y);
  }

  // filterFreq = circShift(filterFreq, -1, -1);

  Mat planes[] = {degradedFreq, Mat::zeros(degraded.size(), CV_32F)};
  Mat planesFilter[] = {filterFreq, Mat::zeros(filterFreq.size(), CV_32F)};
  
  merge(planes, 2, degradedFreq);
  merge(planesFilter, 2, filterFreq);

  dft(degradedFreq, degradedFreq, DFT_COMPLEX_OUTPUT); // degradedFreq == S
  dft(filterFreq, filterFreq, DFT_COMPLEX_OUTPUT); // filterFreq == P

  // create Q

  split(filterFreq, planes);

  Mat Re = planes[0];
  Mat Im = planes[1];

  Mat QRe = Re.clone();
  Mat QIm = Im.clone();

  for (int x = 0; x < filterFreq.rows; x++) for (int y = 0; y < filterFreq.cols; y++) {

    // A*_ij = Ã_ji
    float ReConjugateTranspose = filterFreq.at<float>(y, x);
    float ImConjugateTranspose = -filterFreq.at<float>(y, x);
    float realsq = Re.at<float>(x, y) * Re.at<float>(x, y);
    float imsq = Im.at<float>(x, y) * Im.at<float>(x, y);
    float ReAbs = Re.at<float>(x, y) / (realsq + imsq);
    float ImAbs = Im.at<float>(x, y) / (realsq + imsq);

    QRe.at<float>(x, y) = ReConjugateTranspose / (ReAbs * ReAbs + 1/(snr * snr));
    QIm.at<float>(x, y) = ImConjugateTranspose / (ImAbs * ImAbs + 1/(snr * snr));

  }
  
  Mat Q = Mat::zeros(filterFreq.size(), CV_32F);

  Mat qplanes[] = {QRe, QIm};
  
  merge(qplanes, 2, Q);

  Mat original;

  mulSpectrums(degradedFreq, Q, original, 0);
  dft(original, original, DFT_INVERSE + DFT_SCALE);
  split(original, planes);
 
  return planes[0];

}

/* *****************************
  GIVEN FUNCTIONS
***************************** */

// function calls processing function
/*
in                   :  input image
restorationType     :  integer defining which restoration function is used
kernel               :  kernel used during restoration
snr                  :  signal-to-noise ratio (only used by wieder filter)
return               :  restorated image
*/
Mat Dip4::run(Mat& in, string restorationType, Mat& kernel, double snr){

   if (restorationType.compare("wiener")==0){
      return wienerFilter(in, kernel, snr);
   }else{
      return inverseFilter(in, kernel);
   }

}

// Function degrades a given image with gaussian blur and additive gaussian noise
/*
img         :  input image
degradedImg :  degraded output image
filterDev   :  standard deviation of kernel for gaussian blur
snr         :  signal to noise ratio for additive gaussian noise
return      :  the used gaussian kernel
*/
Mat Dip4::degradeImage(Mat& img, Mat& degradedImg, double filterDev, double snr){

    int kSize = round(filterDev*3)*2 - 1;
   
    Mat gaussKernel = getGaussianKernel(kSize, filterDev, CV_32FC1);
    gaussKernel = gaussKernel * gaussKernel.t();
    filter2D(img, degradedImg, -1, gaussKernel);

    Mat mean, stddev;
    meanStdDev(img, mean, stddev);

    Mat noise = Mat::zeros(img.rows, img.cols, CV_32FC1);
    randn(noise, 0, stddev.at<double>(0)/snr);
    degradedImg = degradedImg + noise;
    threshold(degradedImg, degradedImg, 255, 255, CV_THRESH_TRUNC);
    threshold(degradedImg, degradedImg, 0, 0, CV_THRESH_TOZERO);

    return gaussKernel;
}

// Function displays image (after proper normalization)
/*
win   :  Window name
img   :  Image that shall be displayed
cut   :  whether to cut or scale values outside of [0,255] range
*/
void Dip4::showImage(const char* win, Mat img, bool cut){

   Mat tmp = img.clone();

   if (tmp.channels() == 1){
      if (cut){
         threshold(tmp, tmp, 255, 255, CV_THRESH_TRUNC);
         threshold(tmp, tmp, 0, 0, CV_THRESH_TOZERO);
      }else
         normalize(tmp, tmp, 0, 255, CV_MINMAX);
         
      tmp.convertTo(tmp, CV_8UC1);
   }else{
      tmp.convertTo(tmp, CV_8UC3);
   }
   imshow(win, tmp);
}

// function calls some basic testing routines to test individual functions for correctness
void Dip4::test(void){

   test_circShift();
   cout << "Press enter to continue"  << endl;
   cin.get();

}

void Dip4::test_circShift(void){
   
   Mat in = Mat::zeros(3,3,CV_32FC1);
   in.at<float>(0,0) = 1;
   in.at<float>(0,1) = 2;
   in.at<float>(1,0) = 3;
   in.at<float>(1,1) = 4;
   Mat ref = Mat::zeros(3,3,CV_32FC1);
   ref.at<float>(0,0) = 4;
   ref.at<float>(0,2) = 3;
   ref.at<float>(2,0) = 2;
   ref.at<float>(2,2) = 1;
   
   if (sum((circShift(in, -1, -1) == ref)).val[0]/255 != 9){
      cout << "ERROR: Dip4::circShift(): Result of circshift seems to be wrong!" << endl;
      return;
   }
   cout << "Message: Dip4::circShift() seems to be correct" << endl;
}
