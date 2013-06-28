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

#define NUM_DEFECTS 8
#define NUM_FINGERS 5



Mat src; Mat src_gray;

int thresh = 100;
int max_thresh = 255;
int fps, width, height;
RNG rng(12345);

//
IplImage *frame;
IplImage *thr_image;
IplImage *temp_image1;
IplImage *temp_image3;

CvCapture* capture;

CvSeq *contour;
CvSeq *hull;

CvMemStorage *hull_st;
CvMemStorage *defects_st;
CvMemStorage *temp_st;
CvMemStorage *contour_st;

CvPoint hand_center;
CvPoint *fingers;
CvPoint *defects;

int hand_radius;
int num_defects;
int num_fingers;

void InitRecording();
void InitFrames();
void InitVariables();
IplImage* GetThresholdedImge(IplImage* img);
void GetContours(IplImage* thr);
void GetConvHull();
void GetFingers();
void Show(bool mouse);
void Move(CvPoint p);

int main() {
	capture = cvCaptureFromCAM( CV_CAP_ANY );
	if ( !capture )
	{
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return -1;
	}
	frame = cvQueryFrame(capture);
	InitRecording();
	InitFrames();
	InitVariables();

	while ( 1 )
	{
		frame=cvQueryFrame(capture);
		if ( !frame )
		{
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}


		//thresh_callback(0, 0);
		IplImage* thresholded = GetThresholdedImge(frame);
		GetContours(thresholded);
		GetConvHull();
		GetFingers();
		//cvDrawContours(frame, contour, CV_RGB(0, 0, 255), CV_RGB(0, 255, 0), 0, 1, CV_AA, cvPoint(0,0));
		Show(false);
		if(num_fingers == 1)
			Move(fingers[0]);
		cvShowImage( "Output", frame );
		cvShowImage( "Threshold", thresholded );
		thr_image = thresholded;

		if ( (cvWaitKey(50) & 255) == 27 ) break;
	}
	cvReleaseCapture( &capture );
	cvDestroyWindow( "mywindow" );
	return 0;
 }

void InitRecording()
{


	fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	width = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	height = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

	if(fps<10)
		fps = 10;
}

void InitFrames()
{
	cvNamedWindow("Output", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Threshold", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("Output", 50, 50);
	cvMoveWindow("Threshold", 700, 50);
}

void InitVariables()
{
	thr_image = cvCreateImage(cvGetSize(frame), 8, 1);
	temp_image1 = cvCreateImage(cvGetSize(frame), 8, 1);
	temp_image3 = cvCreateImage(cvGetSize(frame), 8, 1);

	contour_st = cvCreateMemStorage(0);
	hull_st = cvCreateMemStorage(0);
	temp_st = cvCreateMemStorage(0);
	defects_st = cvCreateMemStorage(0);

	fingers = (CvPoint*) calloc(NUM_FINGERS+1, sizeof(CvPoint));
	defects = (CvPoint*) calloc(NUM_DEFECTS, sizeof(CvPoint));
}
IplImage* GetThresholdedImge(IplImage* img)
{
    // Convert the image into an HSV image
    IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
    cvCvtColor(img, imgHSV, CV_BGR2HSV);
    IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);
    cvSmooth(img, imgHSV, CV_GAUSSIAN, 3);
    cvSmooth(imgHSV, imgHSV, CV_MEDIAN, 11, 11, 0, 0);
    cvInRangeS(imgHSV, cvScalar(0, 0, 160, 0), cvScalar(255, 400, 300, 255), imgThreshed);
    cvReleaseImage(&imgHSV);
    return imgThreshed;
}

void GetContours(IplImage* thr)
{
	thr_image = cvCloneImage(thr);
	float area, max_area = 0.0f;
	CvSeq *contours, *tmp = NULL;

	cvFindContours(thr_image, temp_st, &contours,
			sizeof(CvContour), CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	tmp = contours;
	while(tmp != NULL)
	{
		area = fabs(cvContourArea(tmp, CV_WHOLE_SEQ, 0));
		if(area > max_area)
		{
			max_area = area;
			contour = tmp;
		}
		tmp = tmp->h_next;
	}

	// poly line approx
	if(contour)
	{
		contour = cvApproxPoly(contour, sizeof(CvContour),
				contour_st, CV_POLY_APPROX_DP, 2, 1);
	}



}

void GetConvHull()
{
	CvSeq *defect;
	CvConvexityDefect *defect_array;
	int i;
	int x = 0, y = 0;
	int dist = 0;

	if(!contour)
		return;

	hull =cvConvexHull2(contour, hull_st, CV_CLOCKWISE, 0);

	if(hull)
	{
		printf("hull c'è");
		defect= cvConvexityDefects(contour, hull, defects_st);
		if(defect && defect->total)
		{
			printf("         defect!\n");
			defect_array = (CvConvexityDefect*) calloc(defect->total, sizeof(CvConvexityDefect));
			cvCvtSeqToArray(defect, defect_array, CV_WHOLE_SEQ);

			// Trovo il centro della mano con la media tra le coordinate dei defects_point
			for(i = 0; i < defect->total && i<NUM_DEFECTS; i++)
			{
				x += defect_array[i].depth_point->x;
				y += defect_array[i].depth_point->y;

				defects[i] = cvPoint(defect_array[i].depth_point->x, defect_array[i].depth_point->y);
			}

			x /= defect->total;	// coordinata x del centro della mano
			y /= defect->total;	// coordinata y del centro della mano

			num_defects = defect->total;

			hand_center = cvPoint(x, y);

			// calcolo del raggio della mano inteso come la distanza media dei defects point dal centro
			for(i=0; i<defect->total; i++)
			{
				int d = (x-defect_array[i].depth_point->x)*(x-defect_array[i].depth_point->x) +
						(y-defect_array[i].depth_point->y)*(y-defect_array[i].depth_point->y);
				dist += sqrt(d);
			}

			hand_radius = dist/defect->total;
			free(defect_array);
		}
	}
}

void GetFingers()
{
	int n;
		int i;
		CvPoint *points;
		CvPoint max_point;

		int dist1 = 0, dist2 = 0;
		int finger_distance[NUM_FINGERS+1];
		num_fingers =0;

		if(!contour || !hull)
			return;

		n = contour->total;
		points = (CvPoint*) calloc(n, sizeof(CvPoint));
		cvCvtSeqToArray(contour, points, CV_WHOLE_SEQ);
		for(i = 0; i < n; i++)
		{
			int dist;
			int cx = hand_center.x;
			int cy = hand_center.y;

			dist = (cx - points[i].x) * (cx - points[i].x) +
					(cy - points[i].y) * (cy - points[i].y);
				if(dist<dist1 && dist1>dist2 && max_point.x != 0 && max_point.y < cvGetSize(frame).height-10)
				{
					if(sqrt(dist)>=hand_radius+10)
					{
						finger_distance[num_fingers] = dist;
						fingers[num_fingers++] = max_point;
						if(num_fingers >= NUM_FINGERS + 1)
							break;
					}

				}
			dist2 = dist1;
			dist1=dist;
			max_point=points[i];
		}
		free(points);
}

void Show(bool mouse)
{
	if(!mouse)
	{
		int i;

		cvDrawContours(frame, contour, CV_RGB(0, 0, 255), CV_RGB(0, 255, 0), 0, 1, CV_AA, cvPoint(0,0));
		cvDrawContours(frame, hull, CV_RGB(255, 0, 255), CV_RGB(0, 255, 0), 0, 1, CV_AA, cvPoint(0,0));

		cvCircle(frame, hand_center, 5, CV_RGB(255, 0, 255), 1, CV_AA, 0);
		cvCircle(frame, hand_center, hand_radius, CV_RGB(255, 0, 255), 1, CV_AA, 0);

		for(i=0; i<num_fingers; i++)
		{
			cvCircle(frame, fingers[i], 5, CV_RGB(255, 0, 255), 3, CV_AA, 0);
			cvLine(frame, hand_center, fingers[i], CV_RGB(255, 0, 0), 1, CV_AA, 0);
		}

		for (i = 0; i < num_defects; i++)
		{
			cvCircle(frame, defects[i], 2, CV_RGB(200, 200, 200), 2, CV_AA, 0);
		}
		cvShowImage("Output", frame);
	}
	else
	{
		int nxl, nyl, nxh, nyh;
		nxl = width/2 + width/5;
		nyl = height/2 + height/5;
		nxh = width/2 - width/5;
		nyh = height/2 - height/5;
		cvLine(frame, cvPoint(0, height/2), cvPoint(width, height/2), CV_RGB(255, 0, 0), 1, CV_AA, 0); //linea orizziontale per mouse
		cvLine(frame, cvPoint(width/2, 0), cvPoint(width/2, height), CV_RGB(255, 0, 0), 1, CV_AA, 0); //linea verticale per mouse
		cvLine(frame, cvPoint(nxh, nyh), cvPoint(nxl, nyh), CV_RGB(255, 0, 0), 1, CV_AA, 0); //definizione linee zona neutra
		cvLine(frame, cvPoint(nxl, nyl), cvPoint(nxh, nyl), CV_RGB(255, 0, 0), 1, CV_AA, 0);
		cvLine(frame, cvPoint(nxh, nyh), cvPoint(nxh, nyl), CV_RGB(255, 0, 0), 1, CV_AA, 0);
		cvLine(frame, cvPoint(nxl, nyl), cvPoint(nxl, nyh), CV_RGB(255, 0, 0), 1, CV_AA, 0);
	}
}

void Move(CvPoint p)
{
	Show(true);
	int nxl, nyl, nxh, nyh;
	nxl = width/2 + width/5;
	nyl = height/2 + height/5;
	nxh = width/2 - width/5;
	nyh = height/2 - height/5;
	if(p.x < nxl && p.x >nxh && p.y <nyl && p.y > nyh)
		return;


	Display *displayMain = XOpenDisplay(NULL);
	int offsetx, offsety = 0;
	if(displayMain == NULL)
	{
		fprintf(stderr, "Errore nell'apertura del Display !!!\n");
		exit(EXIT_FAILURE);
	}
	if(p.x>width/2)
	{
		offsetx = p.x-width/2;
		if(p.y>height/2)
			offsety = (p.y-height/2);
		else
			offsety = -(p.y+height/2);
	}
	else
	{
		offsetx = p.x-width/2;
			if(p.y>height/2)
				offsety = (p.y-height/2);
			else
				offsety = -(p.y+height/2);
	}
	XWarpPointer(displayMain, None, None, 0, 0, 0, 0, offsetx, offsety);
	printf("%d",height/2);
	XCloseDisplay(displayMain);
}











