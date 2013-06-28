/*
 * Main.cpp
 *
 *  Created on: 27/giu/2013
 *      Author: mattia
 */

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include <iostream>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace cv;

Mat src; Mat src_gray;
IplImage* frame;
int thresh = 100;
int max_thresh = 255;
IplImage* srci;
RNG rng(12345);

IplImage* GetThresholdedImge(IplImage* img);


int main() {
	CvCapture* capture = cvCaptureFromCAM( CV_CAP_ANY );
	if ( !capture )
	{
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return -1;
	}

	while ( 1 )
	{
		frame=cvQueryFrame(capture);
		if ( !frame )
		{
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}
		//calc(frame);
		srci=frame;
		src = cvarrToMat(frame);
		cvtColor( src, src_gray, CV_BGR2GRAY );
		blur( src_gray, src_gray, Size(3,3) );
		//thresh_callback(0, 0);
		IplImage* thresholded = GetThresholdedImge(frame);

		cvShowImage( "mywindow", frame );
		cvShowImage("Thresholded", thresholded);

		if ( (cvWaitKey(10) & 255) == 27 ) break;
	}
	cvReleaseCapture( &capture );
	cvDestroyWindow( "mywindow" );
	return 0;
 }

IplImage* GetThresholdedImge(IplImage* img)
{
    // Convert the image into an HSV image
    IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
    cvCvtColor(img, imgHSV, CV_BGR2HSV);
    IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);
    cvInRangeS(imgHSV, cvScalar(0, 0, 160, 0), cvScalar(255, 400, 300, 255), imgThreshed);
    cvReleaseImage(&imgHSV);
    return imgThreshed;
}


