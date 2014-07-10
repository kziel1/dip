//============================================================================
// Name        : Dip5.cpp
// Author      : Ronny Haensch
// Version     : 2.0
// Copyright   : -
// Description : 
//============================================================================

#include "Dip5.h"

// uses structure tensor to define interest points (foerstner)
void Dip5::getInterestPoints(Mat& img, double sigma, vector<KeyPoint>& points){
	int kernelSize = (int) ceil(3*sigma) + 1-kernelSize%2;
	Mat fstdevKernelX =  createFstDevKernel(0);
	//cout << "fstKernel = "<< endl << " "  << fstdevKernelX << endl << endl;
	Mat fstdevKernelY =  fstdevKernelX.t();
	//cout << "fstKernel = "<< endl << " "  << fstdevKernelY << endl << endl;
	Mat gradientsX;
	filter2D(img, gradientsX, CV_32FC1, fstdevKernelX) ;
	// showImage(gradientsX, "asd", 1, true, false);
	Mat gradientsY;
	filter2D(img, gradientsY, CV_32FC1, fstdevKernelY) ;
	// showImage(gradientsY, "qwe", 0, true, false);
	Mat structureTensor = Mat::zeros(2,2,CV_32FC1);
	int i,j;
	Mat plesseyHarrisDetector = Mat::zeros(img.rows,img.cols,CV_32FC1);
	for(int x=kernelSize;x<img.rows-kernelSize;x++){
 		for(int y=kernelSize;y<img.cols-kernelSize;y++){
structureTensor = Mat::zeros(2,2,CV_32FC1);
 			for(int xw=0;xw<kernelSize/2;xw++){
 				for(int yw=0;yw<kernelSize;yw++){
 					i=x+xw-kernelSize/2;
 					j=y+yw-kernelSize/2;
					structureTensor.at<float>(0, 0)+=gradientsX.at<float>(i,j)*gradientsX.at<float>(i,j);
 					structureTensor.at<float>(1, 1)+=gradientsY.at<float>(i,j)*gradientsY.at<float>(i,j);
 					structureTensor.at<float>(1, 0)+=gradientsX.at<float>(i,j)*gradientsY.at<float>(i,j);
 				}
 			}
 			structureTensor.at<float>(0, 1)=structureTensor.at<float>(1, 0);
			float structureTensorTrace = sum(trace(structureTensor))[0];
 			plesseyHarrisDetector.at<float>(x, y)=determinant(structureTensor)-0.04*structureTensorTrace*structureTensorTrace;
 		}
 	}
 	plesseyHarrisDetector = nonMaxSuppression(plesseyHarrisDetector);
 	for(int x=kernelSize;x<img.rows-kernelSize;x++){
 		for(int y=kernelSize;y<img.cols-kernelSize;y++){
				if(abs(plesseyHarrisDetector.at<float>(x, y))>100000){
 				points.push_back(KeyPoint(y,x,5));
 			}
 		}
 	}
}

// creates kernel representing fst derivative of a Gaussian kernel in x-direction
/*
sigma	standard deviation of the Gaussian kernel
return	the calculated kernel
*/
Mat Dip5::createFstDevKernel(double sigma){
	sigma=this->sigma;
	int kernelSize = (int) ceil(3*sigma) + 1-kernelSize%2;
	Mat gaussianKernelX = getGaussianKernel(kernelSize,sigma, CV_32FC1);
	Mat gaussianKernelY = getGaussianKernel(kernelSize,sigma, CV_32FC1);
	Mat gaussianKernel = gaussianKernelX*gaussianKernelY.t();
	Mat fstKernel = Mat::ones(kernelSize, kernelSize, CV_32FC1);
 	for(int x=0;x<kernelSize;x++){
 		for(int y=0;y<kernelSize;y++){
 			int rx=x-kernelSize/2;
 			fstKernel.at<float>(x, y)=-rx*gaussianKernel.at<float>(x,y)/sigma/sigma;
 		}
 	}
	//cout << "fstKernel = "<< endl << " "  << fstKernel << endl << endl;
	return fstKernel;
}

/* *****************************
  GIVEN FUNCTIONS
***************************** */

// function calls processing function
/*
in		:  input image
points	:	detected keypoints
*/
void Dip5::run(Mat& in, vector<KeyPoint>& points){
   this->getInterestPoints(in, this->sigma, points);
}

// non-maxima suppression
// if any of the pixel at the 4-neighborhood is greater than current pixel, set it to zero
Mat Dip5::nonMaxSuppression(Mat& img){

	Mat out = img.clone();
	
	for(int x=1; x<out.cols-1; x++){
		for(int y=1; y<out.rows-1; y++){
			if ( img.at<float>(y-1, x) >= img.at<float>(y, x) ){
				out.at<float>(y, x) = 0;
				continue;
			}
			if ( img.at<float>(y, x-1) >= img.at<float>(y, x) ){
				out.at<float>(y, x) = 0;
				continue;
			}
			if ( img.at<float>(y, x+1) >= img.at<float>(y, x) ){
				out.at<float>(y, x) = 0;
				continue;
			}
			if ( img.at<float>( y+1, x) >= img.at<float>(y, x) ){
				out.at<float>(y, x) = 0;
				continue;
			}
		}
	}
	return out;
}

// Function displays image (after proper normalization)
/*
win   :  Window name
img   :  Image that shall be displayed
cut   :  whether to cut or scale values outside of [0,255] range
*/
void Dip5::showImage(Mat& img, const char* win, int wait, bool show, bool save){
  
    Mat aux = img.clone();

    // scale and convert
    if (img.channels() == 1)
		normalize(aux, aux, 0, 255, CV_MINMAX);
		aux.convertTo(aux, CV_8UC1);
    // show
    if (show){
      imshow( win, aux);
      waitKey(wait);
    }
    // save
    if (save)
      imwrite( (string(win)+string(".png")).c_str(), aux);
}
