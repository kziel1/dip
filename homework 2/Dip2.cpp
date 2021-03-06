//============================================================================
// Name        : Dip2.cpp
// Author      : Ronny Haensch
// Version     : 2.0
// Copyright   : -
// Description : 
//============================================================================

#include "Dip2.h"

Mat Dip2::forEachMat(Mat& orig, int xstep, int ystep, function<Mat (Mat orig, Mat copy, int x, int y)> func) {
    
  Mat copy = orig.clone();

  for (int x = 0; x < orig.cols; x += xstep) {
    for (int y = 0; y < orig.rows; y += ystep) {
      func(orig, copy, x, y);
    }
  }

  return copy;

};

// convolution in spatial domain
/*
src:     input image
kernel:  filter kernel
return:  convolution result
*/
Mat Dip2::spatialConvolution(Mat& src, Mat& kernel){
  
  auto convolution = [kernel](Mat orig, Mat copy, int x, int y) -> Mat {

    Mat defaultMat = Mat::ones(kernel.rows, kernel.cols, CV_32FC1);

    int center = kernel.rows/2;

    // border handling using default values
    for (int i = 0; i < kernel.rows; i++) {
      for (int j = 0; j < kernel.cols; j++) {
        if ((x + i) >= center && (x + i) < (orig.rows + center) &&
            (y + j) >= center && (y + j) < (orig.cols + center)) {
          defaultMat.at<float>(i, j) = orig.at<float>(x + i - center, y + j - center);
        }
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

// the average filter
// HINT: you might want to use Dip2::spatialConvolution(...) within this function
/*
src:     input image
kSize:   window size used by local average
return:  filtered image
*/
Mat Dip2::averageFilter(Mat& src, int kSize){

   Mat kernel = Mat(kSize, kSize, CV_32FC1, 1.0/(kSize * kSize));
   Mat copy = src.clone();
   Mat result = spatialConvolution(copy, kernel);

   return result;

}

// the adaptive filter
// HINT: you might want to use Dip2::averageFilter(...) within this function
/*
src:        input image
kSize:      window size used by local average
threshold:  threshold value on differences in order to decide which average to use
return:     filtered image
*/
Mat Dip2::adaptiveFilter(Mat& src, int kSize, double threshold){

   Mat average = averageFilter(src, kSize);
   Mat average3 = averageFilter(src, 3);

   auto adaptive = [threshold, average, average3](Mat orig, Mat copy, int x, int y) -> Mat {

    if (abs(average3.at<float>(x, y) - average.at<float>(x, y)) > threshold) {
      copy.at<float>(x, y) = average3.at<float>(x, y);
    } else {
      copy.at<float>(x, y) = average.at<float>(x, y);
    };

    return copy;
   
   };

   Mat result = forEachMat(average, 1, 1, adaptive);

   return result;

}

// the median filter
/*
src:     input image
kSize:   window size used by median operation
return:  filtered image
*/
Mat Dip2::medianFilter(Mat& src, int kSize){
  const clock_t begin_time = clock();
  Mat out(src.rows, src.cols, CV_32FC1);
  int k2=kSize*kSize;
  int array[k2];
  for(int y=kSize/2;y<src.cols-kSize/2;y++){
    for(int x=kSize/2;x<src.rows-kSize/2;x++){
      for(int k=0;k<k2;k++){
        array[k]=src.at<float>(y-1+k/kSize,x-1+k%kSize);
        std::sort(array, array+k2);
        out.at<float>(y,x)=array[k2/2];
      }
    }
  }
  std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC;
  return out;
}

// the bilateral filter
/*
src:     input image
kSize:   window size of kernel --> used to compute std-dev of spatial kernel
sigma:   standard-deviation of radiometric kernel
return:  filtered image
*/
Mat Dip2::bilateralFilter(Mat& src, int kSize, double sigma){
  
   // (Optional) TO DO !!
   return src.clone();


}

/* *****************************
  GIVEN FUNCTIONS
***************************** */

// function loads input image, calls processing function, and saves result
void Dip2::run(void){

   // load images as grayscale
	cout << "load images" << endl;
	Mat noise1 = imread("noiseType_1.jpg", 0);
   if (!noise1.data){
	   cerr << "noiseType_1.jpg not found" << endl;
      cout << "Press enter to exit"  << endl;
      cin.get();
	   exit(-3);
	}
   noise1.convertTo(noise1, CV_32FC1);
	Mat noise2 = imread("noiseType_2.jpg",0);
	if (!noise2.data){
	   cerr << "noiseType_2.jpg not found" << endl;
      cout << "Press enter to exit"  << endl;
      cin.get();
	   exit(-3);
	}
   noise2.convertTo(noise2, CV_32FC1);
	cout << "done" << endl;
	  
   // apply noise reduction
	// TO DO !!!
	// ==> Choose appropriate noise reduction technique with appropriate parameters
	// ==> "average" or "median"? Why?
	// ==> try also "adaptive" (and if implemented "bilateral")
	cout << "reduce noise" << endl;
	Mat restorated1 = noiseReduction(noise1, "median", 10, 60);
	Mat restorated2 = noiseReduction(noise2, "median", 10, 60);
	cout << "done" << endl;
	  
	// save images
	cout << "save results" << endl;
	imwrite("restorated1.jpg", restorated1);
	imwrite("restorated2.jpg", restorated2);
	cout << "done" << endl;

}

// noise reduction
/*
src:     input image
method:  name of noise reduction method that shall be performed
	      "average" ==> moving average
         "median" ==> median filter
         "adaptive" ==> edge preserving average filter
         "bilateral" ==> bilateral filter
kSize:   (spatial) kernel size
param:   if method == "adaptive" : threshold ; if method == "bilateral" standard-deviation of radiometric kernel
         can be ignored otherwise (default value = 0)
return:  output image
*/
Mat Dip2::noiseReduction(Mat& src, string method, int kSize, double param){

   // apply moving average filter
   if (method.compare("average") == 0){
      return averageFilter(src, kSize);
   }
   // apply median filter
   if (method.compare("median") == 0){
      return medianFilter(src, kSize);
   }
   // apply adaptive average filter
   if (method.compare("adaptive") == 0){
      return adaptiveFilter(src, kSize, param);
   }
   // apply bilateral filter
   if (method.compare("bilateral") == 0){
      return bilateralFilter(src, kSize, param);
   }

   // if none of above, throw warning and return copy of original
   cout << "WARNING: Unknown filtering method! Returning original" << endl;
   cout << "Press enter to continue"  << endl;
   cin.get();
   return src.clone();

}

// generates and saves different noisy versions of input image
/*
fname:   path to input image
*/
void Dip2::generateNoisyImages(string fname){
 
   // load image, force gray-scale
   cout << "load original image" << endl;
   Mat img = imread(fname, 0);
   if (!img.data){
      cerr << "ERROR: file " << fname << " not found" << endl;
      cout << "Press enter to exit"  << endl;
      cin.get();
      exit(-3);
   }

   // convert to floating point precision
   img.convertTo(img,CV_32FC1);
   cout << "done" << endl;

   // save original
   imwrite("original.jpg", img);
	  
   // generate images with different types of noise
   cout << "generate noisy images" << endl;

   // some temporary images
   Mat tmp1(img.rows, img.cols, CV_32FC1);
   Mat tmp2(img.rows, img.cols, CV_32FC1);
   // first noise operation
   float noiseLevel = 0.15;
   randu(tmp1, 0, 1);
   threshold(tmp1, tmp2, noiseLevel, 1, CV_THRESH_BINARY);
   multiply(tmp2,img,tmp2);
   threshold(tmp1, tmp1, 1-noiseLevel, 1, CV_THRESH_BINARY);
   tmp1 *= 255;
   tmp1 = tmp2 + tmp1;
   threshold(tmp1, tmp1, 255, 255, CV_THRESH_TRUNC);
   // save image
   imwrite("noiseType_1.jpg", tmp1);
    
   // second noise operation
   noiseLevel = 50;
   randn(tmp1, 0, noiseLevel);
   tmp1 = img + tmp1;
   threshold(tmp1,tmp1,255,255,CV_THRESH_TRUNC);
   threshold(tmp1,tmp1,0,0,CV_THRESH_TOZERO);
   // save image
   imwrite("noiseType_2.jpg", tmp1);

	cout << "done" << endl;
	cout << "Please run now: dip2 restorate" << endl;

}

// function calls some basic testing routines to test individual functions for correctness
void Dip2::test(void){

	test_spatialConvolution();
  test_averageFilter();
  test_medianFilter();
  test_adaptiveFilter();

  cout << "Press enter to continue"  << endl;
  cin.get();

}

// checks basic properties of the convolution result
void Dip2::test_spatialConvolution(void){

   Mat input = Mat::ones(9,9, CV_32FC1);
   input.at<float>(4,4) = 255;
   Mat kernel = Mat(3,3, CV_32FC1, 1./9.);

   Mat output = spatialConvolution(input, kernel);
   
   if ( (input.cols != output.cols) || (input.rows != output.rows) ){
      cout << "ERROR: Dip2::spatialConvolution(): input.size != output.size --> Wrong border handling?" << endl;
      return;
   }
  if ( (sum(output.row(0) < 0).val[0] > 0) or
           (sum(output.row(0) > 255).val[0] > 0) or
           (sum(output.row(8) < 0).val[0] > 0) or
           (sum(output.row(8) > 255).val[0] > 0) or
           (sum(output.col(0) < 0).val[0] > 0) or
           (sum(output.col(0) > 255).val[0] > 0) or
           (sum(output.col(8) < 0).val[0] > 0) or
           (sum(output.col(8) > 255).val[0] > 0) ){
         cout << "ERROR: Dip2::spatialConvolution(): Border of convolution result contains too large/small values --> Wrong border handling?" << endl;
         return;
   }else{
      if ( (sum(output < 0).val[0] > 0) or
         (sum(output > 255).val[0] > 0) ){
            cout << "ERROR: Dip2::spatialConvolution(): Convolution result contains too large/small values!" << endl;
            return;
      }
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
            cout << "ERROR: Dip2::spatialConvolution(): Convolution result contains wrong values!" << endl;
            return;
         }
      }
   }
   input.setTo(0);
   input.at<float>(4,4) = 255;
   kernel.setTo(0);
   kernel.at<float>(0,0) = -1;
   output = spatialConvolution(input, kernel);
   if ( abs(output.at<float>(5,5) + 255.) < 0.0001 ){
      cout << "ERROR: Dip2::spatialConvolution(): Is filter kernel \"flipped\" during convolution? (Check lecture/exercise slides)" << endl;
      return;
   }
   if ( ( abs(output.at<float>(2,2) + 255.) < 0.0001 ) || ( abs(output.at<float>(4,4) + 255.) < 0.0001 ) ){
      cout << "ERROR: Dip2::spatialConvolution(): Is anchor point of convolution the centre of the filter kernel? (Check lecture/exercise slides)" << endl;
      return;
   }
   cout << "Message: Dip2::spatialConvolution() seems to be correct" << endl;
}

// checks basic properties of the filtering result
void Dip2::test_averageFilter(void){

   Mat input = Mat::ones(9,9, CV_32FC1);
   input.at<float>(4,4) = 255;

   Mat output = averageFilter(input, 3);
   
   if ( (input.cols != output.cols) || (input.rows != output.rows) ){
      cout << "ERROR: Dip2::averageFilter(): input.size != output.size --> Wrong border handling?" << endl;
      return;
   }
  if ( (sum(output.row(0) < 0).val[0] > 0) or
           (sum(output.row(0) > 255).val[0] > 0) or
           (sum(output.row(8) < 0).val[0] > 0) or
           (sum(output.row(8) > 255).val[0] > 0) or
           (sum(output.col(0) < 0).val[0] > 0) or
           (sum(output.col(0) > 255).val[0] > 0) or
           (sum(output.col(8) < 0).val[0] > 0) or
           (sum(output.col(8) > 255).val[0] > 0) ){
         cout << "ERROR: Dip2::averageFilter(): Border of convolution result contains too large/small values --> Wrong border handling?" << endl;
         return;
   }else{
      if ( (sum(output < 0).val[0] > 0) or
         (sum(output > 255).val[0] > 0) ){
            cout << "ERROR: Dip2::averageFilter(): Convolution result contains too large/small values!" << endl;
            return;
      }
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
            cout << "ERROR: Dip2::averageFilter(): Result contains wrong values!" << endl;
            return;
         }
      }
   }
   cout << "Message: Dip2::averageFilter() seems to be correct" << endl;
}

// checks basic properties of the filtering result
void Dip2::test_medianFilter(void){

   Mat input = Mat::ones(9,9, CV_32FC1);
   input.at<float>(4,4) = 255;

   Mat output = medianFilter(input, 3);
   
   if ( (input.cols != output.cols) || (input.rows != output.rows) ){
      cout << "ERROR: Dip2::medianFilter(): input.size != output.size --> Wrong border handling?" << endl;
      return;
   }
  if ( (sum(output.row(0) < 0).val[0] > 0) or
           (sum(output.row(0) > 255).val[0] > 0) or
           (sum(output.row(8) < 0).val[0] > 0) or
           (sum(output.row(8) > 255).val[0] > 0) or
           (sum(output.col(0) < 0).val[0] > 0) or
           (sum(output.col(0) > 255).val[0] > 0) or
           (sum(output.col(8) < 0).val[0] > 0) or
           (sum(output.col(8) > 255).val[0] > 0) ){
         cout << "ERROR: Dip2::medianFilter(): Border of convolution result contains too large/small values --> Wrong border handling?" << endl;
         return;
   }else{
      if ( (sum(output < 0).val[0] > 0) or
         (sum(output > 255).val[0] > 0) ){
            cout << "ERROR: Dip2::medianFilter(): Convolution result contains too large/small values!" << endl;
            return;
      }
   }
   for(int y=1; y<8; y++){
      for(int x=1; x<8; x++){
         if (abs(output.at<float>(y,x) - 1.) > 0.0001){
            cout << "ERROR: Dip2::medianFilter(): Result contains wrong values!" << endl;
            return;
         }
      }
   }
   cout << "Message: Dip2::medianFilter() seems to be correct" << endl;

}

// checks basic properties of the filtering result
void Dip2::test_adaptiveFilter(void){

   Mat input = Mat::ones(9,9, CV_32FC1);
   input.at<float>(4,4) = 255;

   Mat output = adaptiveFilter(input, 5, 255);
   Mat output2 = adaptiveFilter(input, 5, -1);

   if ( (input.cols != output.cols) || (input.rows != output.rows) ){
      cout << "ERROR: Dip2::adaptiveFilter(): input.size != output.size --> Wrong border handling?" << endl;
      return;
   }
  if ( (sum(output.row(0) < 0).val[0] > 0) or
           (sum(output.row(0) > 255).val[0] > 0) or
           (sum(output.row(8) < 0).val[0] > 0) or
           (sum(output.row(8) > 255).val[0] > 0) or
           (sum(output.col(0) < 0).val[0] > 0) or
           (sum(output.col(0) > 255).val[0] > 0) or
           (sum(output.col(8) < 0).val[0] > 0) or
           (sum(output.col(8) > 255).val[0] > 0) ){
         cout << "ERROR: Dip2::adaptiveFilter(): Border of result contains too large/small values --> Wrong border handling?" << endl;
         return;
   }else{
      if ( (sum(output < 0).val[0] > 0) or
         (sum(output > 255).val[0] > 0) ){
            cout << "ERROR: Dip2::adaptiveFilter(): Result contains too large/small values!" << endl;
            return;
      }
   }
   float ref[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 1, (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., 1, 0},
                      {0, 1, (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., 1, 0},
                      {0, 1, (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., 1, 0},
                      {0, 1, (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., 1, 0},
                      {0, 1, (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., (24+255)/25., 1, 0},
                      {0, 1, 1, 1, 1, 1, 1, 1, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0}};
   float ref2[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
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
         if ( (abs(output.at<float>(y,x) - ref[y][x]) > 0.0001) || (abs(output2.at<float>(y,x) - ref2[y][x]) > 0.0001) ){
            cout << "ERROR: Dip2::adaptiveFilter(): Result contains wrong values!" << endl;
            return;
         }
      }
   }
   cout << "Message: Dip2::adaptiveFilter() seems to be correct" << endl;
}

