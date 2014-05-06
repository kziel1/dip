//============================================================================
// Name        : Dip1.cpp
// Author      : Ronny Haensch
// Version     : 2.0
// Copyright   : -
// Description : implementation of all processing functions
//============================================================================

#include "Dip1.h"

using namespace std;

// function that performs some kind of (simple) image processing
/*
img input image
return  output image
*/
Mat Dip1::doSomethingThatMyTutorIsGonnaLike(Mat& img){
	
	Mat retImg;
	
	//converting to grayscale image
	cvtColor(img, retImg, CV_RGB2GRAY);

	// densing the brightness distribution to the extremas
	// "bigger contrast"
	auto biggerContrast = [](Mat img) -> Mat {
		
		for (int x = 0; x < img.cols; x++) {
			for (int y = 0; y < img.rows; y++) {
				if (img.ptr<uchar>(x)[y] < 128) {
					img.ptr<uchar>(x)[y] = img.ptr<uchar>(x)[y]/2;
				} else {
					img.ptr<uchar>(x)[y] = img.ptr<uchar>(x)[y]/2 + 128;
				}
			}
		}

		return img;
	};

	// pixelation
	auto pixelate = [](Mat img) -> Mat {
		
		for (int x = 0; x < img.cols; x += 2) {
			for (int y = 0; y < img.rows; y += 2) {
				img.ptr<uchar>(x+1)[y] = img.ptr<uchar>(x)[y];
				img.ptr<uchar>(x)[y+1] = img.ptr<uchar>(x)[y];
				img.ptr<uchar>(x+1)[y+1] = img.ptr<uchar>(x)[y];
			}
		}

		return img;
	};

	// salt and pepper noise
	auto saltAndPepper = [](Mat img) -> Mat {
		for (int x = 0; x < img.cols; x += 2) {
			for (int y = 0; y < img.rows; y += 2) {
				if (rand() % 50 < 1) {
					if (rand() % 2) {
						img.ptr<uchar>(x)[y] = 0;
						img.ptr<uchar>(x+1)[y] = 0;
						img.ptr<uchar>(x)[y+1] = 0;
						img.ptr<uchar>(x+1)[y+1] = 0;
					} else {
						img.ptr<uchar>(x)[y] = 255;
						img.ptr<uchar>(x+1)[y] = 255;
						img.ptr<uchar>(x)[y+1] = 255;
						img.ptr<uchar>(x+1)[y+1] = 255;
					}
				}
			}
		}

		return img;
	};

	return saltAndPepper(pixelate(biggerContrast(retImg)));
}

/* *****************************
	GIVEN FUNCTIONS
***************************** */

// function loads input image, calls processing function, and saves result
/*
fname   path to input image
*/
void Dip1::run(string fname){

	// window names
	string win1 = string ("Original image");
	string win2 = string ("Result");
  
	// some images
	Mat inputImage, outputImage;
  
	// load image
	cout << "load image" << endl;
	inputImage = imread( fname );
	cout << "done" << endl;
	
	// check if image can be loaded
	if (!inputImage.data){
		cerr << "ERROR: Cannot read file " << fname << endl;
		exit(-1);
	}

	// show input image
	//namedWindow( win1.c_str() );
	//imshow( win1.c_str(), inputImage );
	
	// do something (reasonable!)
	outputImage = doSomethingThatMyTutorIsGonnaLike( inputImage );

	// show result
	namedWindow( win2.c_str() );
	imshow( win2.c_str(), outputImage );
	
	// save result
	imwrite("result.jpg", outputImage);
	
	// wait a bit
	waitKey(0);

}

// function loads input image and calls processing function
// output is tested on "correctness" 
/*
fname   path to input image
*/
void Dip1::test(string fname){

	// some image variables
	Mat inputImage, outputImage;
  
	// load image
	inputImage = imread( fname );

	// check if image can be loaded
	if (!inputImage.data){
		cerr << "ERROR: Cannot read file " << fname << endl;
		exit(-1);
	}

	// create output
	outputImage = doSomethingThatMyTutorIsGonnaLike( inputImage );
	// test output
	test_doSomethingThatMyTutorIsGonnaLike(inputImage, outputImage);
	
}

// function loads input image and calls processing function
// output is tested on "correctness" 
/*
inputImage  input image as used by doSomethingThatMyTutorIsGonnaLike()
outputImage output image as created by doSomethingThatMyTutorIsGonnaLike()
*/
void Dip1::test_doSomethingThatMyTutorIsGonnaLike(Mat& inputImage, Mat& outputImage){

	// ensure that input and output have equal number of channels
	if ( (inputImage.channels() == 3) and (outputImage.channels() == 1) )
		cvtColor(inputImage, inputImage, CV_RGB2GRAY);

	// split (multi-channel) image into planes
	vector<Mat> inputPlanes, outputPlanes;
	split( inputImage, inputPlanes );
	split( outputImage, outputPlanes );

	// number of planes (1=grayscale, 3=color)
	int numOfPlanes = inputPlanes.size();

	// calculate and compare image histograms for each plane
	Mat inputHist, outputHist;
	// number of bins
	int histSize = 100;
	float range[] = { 0, 256 } ;
	const float* histRange = { range };
	bool uniform = true; bool accumulate = false;
	double sim = 0;
	for(int p = 0; p < numOfPlanes; p++){
		// calculate histogram
		calcHist( &inputPlanes[p], 1, 0, Mat(), inputHist, 1, &histSize, &histRange, uniform, accumulate );
		calcHist( &outputPlanes[p], 1, 0, Mat(), outputHist, 1, &histSize, &histRange, uniform, accumulate );
		// normalize
		inputHist = inputHist / sum(inputHist).val[0];
		outputHist = outputHist / sum(outputHist).val[0];
		// similarity as histogram intersection
		sim += compareHist(inputHist, outputHist, CV_COMP_INTERSECT);
	}
	sim /= numOfPlanes;

	// check whether images are to similar after transformation
	if (sim >= 0.8)
		cout << "The input and output image seem to be quite similar (similarity = " << sim << " ). Are you sure your tutor is gonna like your work?" << endl;

}
