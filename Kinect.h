
#include <Windows.h>
#include <NuiApi.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cxcore.h>

using namespace std;
using namespace cv;

#define COLOR_WIDTH                   640
#define COLOR_HEIGHT                  480

#define DEPTH_WIDTH                    640
#define DEPTH_HEIGHT                  480


CvMemStorage* storage = cvCreateMemStorage(0);
CvMemStorage* storage_convex = cvCreateMemStorage(0);

IplImage* gray = 0;
IplImage* output = 0;
IplConvKernel*	element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE, NULL);

int thresh;
Mat src;
Mat src_gray;
RNG rng(12345);

RGBQUAD rgb[640*480];
CvPoint points[NUI_SKELETON_POSITION_COUNT];
CvPoint pt0;
CvPoint c1, c2 ;
CvPoint w1, w2;
float Mouse_x = 3*c2.x;
float Mouse_y = 3*c2.y;

int right_1;
int left_1;

CvSeq* ptseq = cvCreateSeq(CV_SEQ_KIND_GENERIC | CV_32SC2, sizeof(CvContour), sizeof(CvPoint), storage_convex);
CvSeq* hull;
CvSeq* defect;
CvSeq* contours = 0;

CvPoint start_pt;
CvPoint end_pt;
CvPoint hull_pt;


int hullcount = 0;
int mouse_flag = 0;
int count_ = 0;

float hand1_depth, hand2_depth;

int left_hand;
int right_hand;

void InitializeKinect();
void drawSkeleton(const NUI_SKELETON_DATA &position, IplImage *Skeleton);
void drawBone(const NUI_SKELETON_DATA &position, NUI_SKELETON_POSITION_INDEX j1, NUI_SKELETON_POSITION_INDEX j2, IplImage *Skeleton);
void createSkeleton(IplImage* Skeleton);


void binaray_1(IplImage* Image);

void MouseClick(int Lbtn, int Rbtn);
void MouseMove(DWORD nX, DWORD nY);
void MouseEvent(float h1_depth, float h2_depth);


float HandDepth(Vector4 Hand);

int createRGBImage(HANDLE h, IplImage* Color);
int createDepthImage(HANDLE h, IplImage* Depth);

RGBQUAD Nui_ShortToQuad_Depth(USHORT s);

CvPoint SkeletonToScreen(Vector4 skeletonPoint);
CvPoint handPos;
CvPoint* st;

double GetDistance2d(CvPoint p1, CvPoint p2)
{
	return sqrt(pow((float)p1.x-p2.x, 2) + pow((float)p1.y - p2.y, 2));
}
void GetMidpoint(CvPoint p1, CvPoint p2, CvPoint *p3)
{
	p3->x = (p1.x + p2.x)/2.0;
	p3->y = (p1.y + p2.y)/2.0;
}

int hand_Dleft = GetDistance2d(c1, w1);
int hand_Dright = GetDistance2d(c2, w2);
int mode;
int wheel;


