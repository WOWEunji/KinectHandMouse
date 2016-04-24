#include "Kinect.h"
/*
	20115-03-31  depth, color, skeleton
		Color+Skeleton + circle
*/


RGBQUAD Nui_ShortToQuad_Depth(USHORT s)
{
	USHORT realDepth = (s&0xfff8) >> 3;
	//      USHORT testRealDepth = NuiDepthPixelToDepth(s);

	// Convert depth info into an intensity for display
	BYTE b = 255 - static_cast<BYTE>(256 * realDepth / 0x0fff);

	//      cout<<"Real Depth : "<<realDepth<<endl;
	//      cout<<"test Depth : "<<testRealDepth<<endl;
	b = 255;
	RGBQUAD q;
	q.rgbRed = q.rgbBlue = q.rgbGreen = b;
	return q;
}

RGBQUAD Depth_Color(USHORT s)
{
	USHORT realDepth = (s&0xfff8) >> 3;
	//      USHORT testRealDepth = NuiDepthPixelToDepth(s);

	// Convert depth info into an intensity for display
	BYTE b = 255 - static_cast<BYTE>(256 * realDepth / 0x0fff);

	//      cout<<"Real Depth : "<<realDepth<<endl;
	//      cout<<"test Depth : "<<testRealDepth<<endl;
	RGBQUAD q;
	q.rgbRed = ~b;
	return q;
}

int main(void)
{

	HANDLE colorStreamHandle;
	HANDLE DepthStreamHandle = NULL;
	HANDLE SkeletonStreamHandle;

	HANDLE nextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE nextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE nextSkeletonFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	HRESULT hr;

	InitializeKinect();

	IplImage *Color = cvCreateImage(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 4);
	IplImage *Depth = cvCreateImage(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), IPL_DEPTH_8U, 4);

	cvNamedWindow("Color Image", CV_WINDOW_AUTOSIZE);

	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, nextColorFrameEvent, &colorStreamHandle);
	NuiSkeletonTrackingEnable(nextColorFrameEvent, NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT);
	
	if(FAILED(hr))
	{
		cout<<"Could not open Image Stream"<<endl;
		return hr;
	}

	
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, 0, 2, nextDepthFrameEvent, &DepthStreamHandle);
//	NuiImageStreamSetImageFrameFlags(DepthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE);
	if(FAILED(hr))
	{
		cout<<"Could not open Depth Image Stream"<<endl;
		return hr;
	}


	while(1)
	{
		
		cvSmooth(Depth, Depth, CV_GAUSSIAN, 3, 0, 0, 0);

		WaitForSingleObject(nextColorFrameEvent, 1000);
		createRGBImage(colorStreamHandle, Color);
		WaitForSingleObject(nextDepthFrameEvent, 0);
		createDepthImage(DepthStreamHandle, Depth);

//		binary_depth(Depth, 200);

		createSkeleton(Color);

		binaray_1(Depth);

//		MouseEvent(hand1_depth, hand2_depth);

		
		if(cvWaitKey(10) == 0x001b)
		{
			break;
		}
	}

//	cvReleaseImage(&g_gray);
//	cvreleaseimage(&g_binary);

	cvReleaseImageHeader(&Depth);
	cvReleaseImageHeader(&Color);
	cvReleaseImage(&gray);
	cvReleaseImage(&output);
	cvReleaseImageHeader(&gray);
	cvReleaseImageHeader(&output);
	cvReleaseMemStorage(&storage);

//	cvReleaseMemStorage(&storage_convex);
	cvClearSeq(contours);
	cvClearSeq(hull);
	cvClearSeq(ptseq);

	NuiShutdown();
	cvDestroyAllWindows();



	return 0;
}

void InitializeKinect()
{
	bool FailToConnect;

	do
	{
		HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX );

		if (FAILED(hr))
		{
			system("cls");
			cout<<"Failed to connect!"<<endl<<endl;
			FailToConnect = true;
			system("PAUSE");
		}
		else
		{
			cout<<"Connection Established!"<<endl<<endl;
			FailToConnect = false;
		}

	}
	while (FailToConnect);
}

int createRGBImage(HANDLE h, IplImage* Color)
{
	const NUI_IMAGE_FRAME *pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 1000, &pImageFrame);

	if(FAILED(hr))
	{
		cout<<"Create RGB Image Failed"<<endl;
		return -1;
	}


	INuiFrameTexture *pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	if(LockedRect.Pitch != 0)
	{
		BYTE *pBuffer = (BYTE*)LockedRect.pBits;
		cvSetData(Color, pBuffer, LockedRect.Pitch);
		cvShowImage("Color Image", Color);

	}

	NuiImageStreamReleaseFrame(h, pImageFrame);

	return 0;
}

int createDepthImage(HANDLE h, IplImage* Depth)
{
	const NUI_IMAGE_FRAME *pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 1000, &pImageFrame);

	if(FAILED(hr))
	{
		cout<<"Create Depth Image Failed"<<endl;
		return -1;
	}

	INuiFrameTexture *pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	

	if(LockedRect.Pitch != 0)
	{
		BYTE *pBuffer = (BYTE*)LockedRect.pBits;
		RGBQUAD *rgbrun = rgb;
		USHORT *pBufferRun = (USHORT*) pBuffer;

		for(int y = 0; y < DEPTH_HEIGHT; y++)
		{
			for(int x = 0; x < DEPTH_WIDTH; x++)
			{
				RGBQUAD quad = Nui_ShortToQuad_Depth(*pBufferRun);
				
				
				float Realdepth = static_cast<float>(*pBufferRun);

				if((hand1_depth-700<Realdepth)&&(hand1_depth+700>Realdepth))
				{
					quad = Depth_Color(*pBufferRun);
				}
				pBufferRun++;
				*rgbrun = quad;
				rgbrun++;
				

			}
		}

		left_hand = hand1_depth/1000;
		right_hand = hand2_depth/1000;

//		printf("left : %d\t\t\n", left_hand);
//		printf("right : %d", right_hand);

		cvSetData(Depth, (BYTE*)rgb, Depth->widthStep);
		cvShowImage("Depth Image", Depth);
	}

	NuiImageStreamReleaseFrame(h, pImageFrame);

	return 0;
}


void createSkeleton(IplImage* Skeleton)
{
	NUI_SKELETON_FRAME skeletonFrame = {0};

	IplImage *Skeleton_clear = cvCreateImage(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 4);
	cvCopy(Skeleton_clear, Skeleton);

	HRESULT hr = NuiSkeletonGetNextFrame(0, &skeletonFrame);
	if(FAILED(hr))
	{
		return;
	}

	NuiTransformSmooth(&skeletonFrame, NULL);

	for(int i= 0; i<NUI_SKELETON_COUNT; i++)
	{
		NUI_SKELETON_TRACKING_STATE state = skeletonFrame.SkeletonData[i].eTrackingState;

		if(NUI_SKELETON_TRACKED == state)
		{
			drawSkeleton(skeletonFrame.SkeletonData[i], Skeleton);
		}
//		cvShowImage("Skeleton Image", Skeleton);
	}

	cvReleaseImage(&Skeleton_clear);
}

void drawBone(const NUI_SKELETON_DATA &position, NUI_SKELETON_POSITION_INDEX j1, NUI_SKELETON_POSITION_INDEX j2, IplImage *Skeleton)
{
	NUI_SKELETON_POSITION_TRACKING_STATE j1state = position.eSkeletonPositionTrackingState[j1];
	NUI_SKELETON_POSITION_TRACKING_STATE j2state = position.eSkeletonPositionTrackingState[j2];

	if(j1state == NUI_SKELETON_POSITION_TRACKED && j2state == NUI_SKELETON_POSITION_TRACKED)
	{
		cvLine(Skeleton, points[j1], points[j2], CV_RGB(0, 255, 0), 3, 8, 0);
	}
}

CvPoint SkeletonToScreen(Vector4 skeletonPoint)
{
	LONG x, y;
	USHORT depth;
	
	NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depth, NUI_IMAGE_RESOLUTION_640x480);

	float screenPointX = static_cast<float>(x);
	float screenPointY = static_cast<float>(y);
	return cvPoint(screenPointX, screenPointY);
}

void drawSkeleton(const NUI_SKELETON_DATA &position, IplImage *Skeleton)
{

	for(int i=0; i<NUI_SKELETON_POSITION_COUNT; i++)
	{
		points[i] = SkeletonToScreen(position.SkeletonPositions[i]);
	}
		
		c1 = points[7];
		c2 = points[11];

		w1 = points[6];
		w2 = points[10];

		hand1_depth = HandDepth(position.SkeletonPositions[7]);
		hand2_depth = HandDepth(position.SkeletonPositions[11]);


		
	drawBone(position, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT, Skeleton);

	drawBone(position, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT, Skeleton);
}

float HandDepth(Vector4 Hand){
	LONG x, y;
	USHORT realDepth;

	NuiTransformSkeletonToDepthImage(Hand, &x, &y, &realDepth, NUI_IMAGE_RESOLUTION_640x480);

	float HandDepth = static_cast<float>(realDepth);
	return HandDepth;
}

void binaray_1(IplImage* Image)
{


	if(!output){

		gray = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);
		output = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);
	}
	else
	{
		cvClearMemStorage(storage);
	}

	cvCvtColor(Image, gray, CV_RGB2GRAY);	
	cvThreshold(gray, output, 5, 255, CV_THRESH_BINARY);	

	cvCanny(gray, gray, 100, 100);

	cvFindContours(gray, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	
	cvErode(gray, gray, element, 4);
	cvDilate(output, gray, element);
	cvErode(gray, gray, element, 4);
	cvDilate(output, gray, element);
	cvDilate(gray, gray, element);
	cvDilate(gray, gray, element);
	cvDilate(gray, gray, element);
	
	if(contours)
	{
	if(storage->bottom != NULL)
	{
		contours = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 3, 1);

		

		cvZero(gray);

		if(contours)
		{
			for(int x = 1; x<contours->total; x++)
			{
				st = (CvPoint*)cvGetSeqElem(contours, 0);
				cvCircle(gray, st[x], 3, CV_RGB(255, 255, 255), 2);
			}

			cvDrawContours(gray, contours, cvScalarAll(255), cvScalarAll(128), 5);
		}
	
		hull = cvConvexHull2(contours, 0, CV_COUNTER_CLOCKWISE, 0);
		defect = cvConvexityDefects(contours, hull, NULL);
		for(;defect; defect = defect->h_next)
		{
			int total = defect->total;
			if(!total)
				continue;

			CvConvexityDefect* defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*total);
			cvCvtSeqToArray(defect, defectArray, CV_WHOLE_SEQ);

			for(int i = 0; i<total; i++)
			{
				cvCircle(gray,*(defectArray[i].start), 5,CV_RGB(255, 255, 255), -1, 8, 0); 
			}
			pt0.x = 0, pt0.y = 0;

			for(int x = 0; x<hull->total; x++)
			{
				hull_pt = **CV_GET_SEQ_ELEM(CvPoint*, hull, x);

				if(pt0.x == 0 &&pt0.y == 0)
				{
					pt0 = hull_pt;
					end_pt = pt0;
					start_pt = hull_pt;
				}

				double dx = hull_pt.x - pt0.x;
				double dy = hull_pt.y - pt0.y;
				double dx2 = pow(dx, 2);
				double dy2 = pow(dy, 2);
				double fDist = sqrt(dx2+dy2);

				if(fDist>15)
				{
					cvCircle(gray, hull_pt, 3, CV_RGB(255, 255, 255), 3, 8, 0);
					cvLine(gray, pt0, hull_pt, CV_RGB(255, 255, 255), 2, 8);
					pt0 = hull_pt;
				}

				if(x==hull->total-1)
					cvLine(gray, hull_pt, end_pt, CV_RGB(255, 255, 255), 2, 8);
					cvLine(gray, hull_pt, c1, CV_RGB(255, 255, 255), 2, 8);
			}

			int n = contours->total-total;
			printf("defect : %d\n", n);
			if((n>5)&&(n<10))
			{
				mode = 1;				//주먹
				printf("주먹");
			}

			else if((n>24)&&(n<50))
			{
				mode = 3;				//손가락 5개
				printf("손가락 5개");
			}
		}
	}
	}
	cvShowImage("Contours", gray);
}

void MouseClick(int Lbtn, int Rbtn)
{
	switch(Lbtn)
	{
	case 1:
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, NULL);	
		break;
	
	case 2:
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, NULL);
		//MouseMove(c2.x*3, c2.y*3);
		break;
	default:
		mouse_event(MOUSEEVENTF_ABSOLUTE, 0, 0, 0, NULL);
		break;

	}

	switch(Rbtn)
	{
	case 1:
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, NULL);
		break;
	case 2:
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, NULL);
		//MouseMove(c2.x*3, c2.y*3);
		break;
	default:
		mouse_event(MOUSEEVENTF_ABSOLUTE, 0, 0, 0, NULL);
		break;

	}
}

void MouseMove(DWORD nX, DWORD nY)
{
	nX = (DWORD)(65535.0*nX)/(double)GetSystemMetrics(SM_CXSCREEN);
	nY = (DWORD)(64435.0*nY)/(double)GetSystemMetrics(SM_CXSCREEN);
	mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE, nX, nY, 0, NULL);
	
}

void MouseEvent(float h1_depth, float h2_depth)
{
	if(hand1_depth<8000)				//click
		left_1 = 1;
	else
		left_1 = 0;

	if(hand2_depth<8000)			// mouse
		right_1 = 1;
	else
		right_1 = 0;

	//MouseMove(c2.x*3, c2.y*3);
	

	if(left_1==0)
	{
		if(right_1==0)
			MouseMove(c2.x*3, c2.y*3);
		else if(right_1 ==1)
		{
			MouseClick(1,0);
			MouseClick(2,0);
		}
	}

	else if(left_1==1)
	{
		MouseClick(1, 0);

		if(left_1==0)
			MouseClick(2, 0);
	}
}
