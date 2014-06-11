//============================================================================
// Name    : Dip3.cpp
// Author   : Ronny Haensch
// Version    : 2.0
// Copyright   : -
// Description : 
//============================================================================

#include "Dip3.h"

// Generates a gaussian filter kernel of given size
/*
kSize:     kernel size (used to calculate standard deviation)
return:    the generated filter kernel
*/
Mat Dip3::createGaussianKernel(int kSize){
   
   float sigma = 0.3 * ((kSize - 1) * 0.5 - 1) + 0.8;
   int mean = kSize/2;
   Mat kernel = Mat::zeros(kSize, kSize, CV_32FC1);
   float sum = 0;

   for (int x = 0; x < kSize; x++) for (int y = 0; y < kSize; y++) {
      
      float scale = 1/(2 * M_PI * sigma * sigma);
      float e = -0.5 * (pow((x - mean)/sigma, 2.0) + pow((y - mean)/sigma, 2.0));
      float gaussXY =  scale * exp(e);

      sum += gaussXY;

      kernel.at<float>(x, y) = gaussXY;
   
   }


   // normalize kernel

   for (int x = 0; x < kSize; x++) for (int y = 0; y < kSize; y++) {
      kernel.at<float>(x, y) = (kernel.at<float>(x, y)/sum);
   }

   return kernel;
}


// Performes a circular shift in (dx,dy) direction
/*
in       input matrix
dx       shift in x-direction
dy       shift in y-direction
return   circular shifted matrix
*/
Mat Dip3::circShift(Mat& in, int dx, int dy){

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

//Performes a convolution by multiplication in frequency domain
/*
in       input image
kernel   filter kernel
return   output image
*/
Mat Dip3::frequencyConvolution(Mat& in, Mat& kernel){

   Mat tempA = Mat::zeros(in.rows, in.cols, CV_32FC1);
   Mat tempB = Mat::zeros(in.rows, in.cols, CV_32FC1);
   
   for (int x = 0; x < kernel.rows; x++) for (int y = 0; y < kernel.cols; y++) {
      tempB.at<float>(x, y) = kernel.at<float>(x, y);
   }
   
   tempB = circShift(tempB, -1, -1);

   dft(in, tempA, 0);
   dft(tempB, tempB, 0);
   mulSpectrums(tempA, tempB, tempB, 0);
   dft(tempB, tempA, DFT_INVERSE + DFT_SCALE);

   return tempA;
}

// Performs UnSharp Masking to enhance fine image structures
/*
in       input image
type     integer defining how convolution for smoothing operation is done
         0 <==> spatial domain; 1 <==> frequency domain
size     size of used smoothing kernel
thresh   minimal intensity difference to perform operation
scale    scaling of edge enhancement
return   enhanced image
*/
Mat Dip3::usm(Mat& in, int type, int size, double thresh, double scale){

   // some temporary images 
   Mat tmp(in.rows, in.cols, CV_32FC1);
   
   // calculate edge enhancement

   // 1: smooth original image
   //    save result in tmp for subsequent usage
   switch(type){
      case 0:
         tmp = mySmooth(in, size, true);
         break;
      case 1:
         tmp = mySmooth(in, size, false);
         break;
      default:
         GaussianBlur(in, tmp, Size(floor(size/2)*2+1, floor(size/2)*2+1), size/5., size/5.);
   }

   subtract(in, tmp, tmp);

   for (int x = 0; x < in.rows; x++) for (int y = 0; y < in.cols; y++) {
      if (tmp.at<float>(x, y) > thresh) {
        in.at<float>(x, y) = in.at<float>(x, y) + tmp.at<float>(x, y) * scale;
      }
   }


   return in;

}

// convolution in spatial domain
/*
src:    input image
kernel:  filter kernel
return:  convolution result
*/
Mat Dip3::spatialConvolution(Mat& src, Mat& kernel){

   auto forEachMat = [](Mat img, int xstep, int ystep, function<Mat (Mat orig, Mat copy, int x, int y)> func) -> Mat {
      
      Mat copy = img.clone();

      for (int x = 0; x < img.cols; x+=xstep) {
         for (int y = 0; y < img.rows; y+=ystep) {
            func(img, copy, x, y);
         }
      }

      return copy;
   };
  
  auto convolution = [kernel](Mat orig, Mat copy, int x, int y) -> Mat {

    Mat defaultMat = Mat::ones(kernel.rows, kernel.cols, CV_32FC1);

    int center = kernel.rows/2;

    // border handling using default values
    for (int i = 0; i < kernel.rows; i++) for (int j = 0; j < kernel.cols; j++) {
     
     if ((x + i) >= center && (x + i) < (orig.rows + center) &&
         (y + j) >= center && (y + j) < (orig.cols + center)) {
       defaultMat.at<float>(i, j) = orig.at<float>(x + i - center, y + j - center);
     }
    
    }

    Mat flippedKernel;
    flip(kernel, flippedKernel, -1);

    float result = sum(defaultMat.dot(flippedKernel)).val[0];

    copy.at<float>(x, y) = result;

    return copy;

  };
  
  Mat copy = src.clone();

  Mat result = forEachMat(copy, 1, 1, convolution);

  return result;

}

/* *****************************
  GIVEN FUNCTIONS
***************************** */

// function calls processing function
/*
in       input image
type     integer defining how convolution for smoothing operation is done
         0 <==> spatial domain; 1 <==> frequency domain
size     size of used smoothing kernel
thresh   minimal intensity difference to perform operation
scale    scaling of edge enhancement
return   enhanced image
*/
Mat Dip3::run(Mat& in, int smoothType, int size, double thresh, double scale){

   Mat copy = in.clone();
   return usm(copy, smoothType, size, thresh, scale);

}


// Performes smoothing operation by convolution
/*
in       input image
size     size of filter kernel
spatial  true if convolution shall be performed in spatial domain, false otherwise
return   smoothed image
*/
Mat Dip3::mySmooth(Mat& in, int size, bool spatial){

   // create filter kernel
   Mat kernel = createGaussianKernel(size);
 
   // perform convoltion
   if (spatial)
      return spatialConvolution(in, kernel);
   else
      return frequencyConvolution(in, kernel);
   
}

// function calls some basic testing routines to test individual functions for correctness
void Dip3::test(void){

   test_createGaussianKernel();
   test_circShift();
   test_frequencyConvolution();
   cout << "Press enter to continue"  << endl;
   cin.get();

}

void Dip3::test_createGaussianKernel(void){

   Mat k = createGaussianKernel(11);
   
   if ( abs(sum(k).val[0] - 1) > 0.0001){
      cout << "ERROR: Dip3::createGaussianKernel(): Sum of all kernel elements is not one!" << endl;
      return;
   }
   if (sum(k >= k.at<float>(5,5)).val[0]/255 != 1){
      cout << "ERROR: Dip3::createGaussianKernel(): Seems like kernel is not centered!" << endl;
      return;
   }
   cout << "Message: Dip3::createGaussianKernel() seems to be correct" << endl;
}

void Dip3::test_circShift(void){
   
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
      cout << "ERROR: Dip3::circShift(): Result of circshift seems to be wrong!" << endl;
      return;
   }
   cout << "Message: Dip3::circShift() seems to be correct" << endl;
}

void Dip3::test_frequencyConvolution(void){
   
   Mat input = Mat::ones(9,9, CV_32FC1);
   input.at<float>(4,4) = 255;
   Mat kernel = Mat(3,3, CV_32FC1, 1./9.);

   Mat output = frequencyConvolution(input, kernel);
   
   if ( (sum(output < 0).val[0] > 0) or (sum(output > 255).val[0] > 0) ){
      cout << "ERROR: Dip3::frequencyConvolution(): Convolution result contains too large/small values!" << endl;
      return;
   }
   float ref[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 1, 1, (8+255)/9., (8+255)/9., (8+255)/9., 1, 1, 0},
                      {0, 1, 1, (8+255)/9., (8+255)/9., (8+255)/9., 1, 1, 0},
                      {0, 1, 1, (8+255)/9., (8+255)/9., (8+255)/9., 1, 1, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0}};
   for(int y=1; y<8; y++){
      for(int x=1; x<8; x++){
         if (abs(output.at<float>(y,x) - ref[y][x]) > 0.0001){
            cout << "ERROR: Dip3::frequencyConvolution(): Convolution result contains wrong values!" << endl;
            return;
         }
      }
   }
   cout << "Message: Dip3::frequencyConvolution() seems to be correct" << endl;
}
