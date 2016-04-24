
/*
	20115-03-31  depth, color, skeleton
		Color+Skeleton + circle
*/

#include "Kinect.h"



int main(void)
{


	HRESULT hr;

	InitializeKinect();

	Color = cvCreateImage(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 4);
	Depth = cvCreateImage(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), IPL_DEPTH_8U, 4);

	cvNamedWindow("Color Image", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Depth Image", CV_WINDOW_AUTOSIZE);

	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, nextColorFrameEvent, &colorStreamHandle);
	if(FAILED(hr))
	{
		cout<<"Could not open Image Stream"<<endl;
	//	return hr;
	}

	hr = NuiSkeletonTrackingEnable(nextColorFrameEvent, NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT);
	
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_320x240, 0, 2, nextDepthFrameEvent, &DepthStreamHandle);

	if(FAILED(hr))
	{
		cout<<"*Could not open Depth Image Stream"<<endl;
	//	return hr;
	}

	

	while(1)
	{
		WaitForSingleObject(nextColorFrameEvent, 0);
		createRGBImage(colorStreamHandle, Color);
		WaitForSingleObject(nextDepthFrameEvent, 0);
		Update();

		//createSkeleton(Color);

		if(cvWaitKey(10) == 0x001b)
		{
			break;
		}
	}
	cvReleaseImageHeader(&Depth);
	cvReleaseImageHeader(&Color);
	NuiShutdown();
	cvDestroyAllWindows();

	return 0;
}


void Update()
{
	if(NULL == m_pNuiSensor)
	{
		return;
	}

	if(WAIT_OBJECT_0 == WaitForSingleObject(nextDepthFrameEvent, 0))
	{
		ProcessDepth();
	}
	
}

HRESULT CreateFirstConeected()
{
	INuiSensor *pNuisensor;
	HRESULT hr;

	int iSensorcount = 0;
	hr = NuiGetSensorCount(&iSensorcount);
	if(FAILED(hr))
	{
		return hr;
	}

	for(int i = 0; i<iSensorcount; i++)
	{
		hr = NuiCreateSensorByIndex(i, &pNuisensor);
		if(FAILED(hr))
		{
			continue;
		}

		hr = pNuisensor->NuiStatus();
		if(S_OK == hr)
		{
			m_pNuiSensor = pNuisensor;
			break;
		}

		pNuisensor->Release();
	}

	if(NULL != m_pNuiSensor)
	{
		return E_FAIL;
	}

	hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX|NUI_INITIALIZE_FLAG_USES_SKELETON|NUI_INITIALIZE_FLAG_USES_COLOR);
	
	if(FAILED(hr))
	{
		return hr;
	}

	nextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(NULL==m_pNuiSensor||FAILED(hr))
	{
		cout<<"No Ready Kinect found";
		return E_FAIL;
	}

	nextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	hr = m_pNuiSensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH,
		NUI_IMAGE_RESOLUTION_640x480,
		0,
		2,
		nextDepthFrameEvent,
		&DepthStreamHandle);
	
	if(FAILED(hr))
	{
		"Not Open the DepthStream";
		return hr;
	}

	return hr;
}

HRESULT ToggleNearMode()
{
	HRESULT hr = E_FAIL;
	if(m_pNuiSensor)
	{
		hr = m_pNuiSensor->NuiImageStreamSetImageFrameFlags(DepthStreamHandle, m_NearMode ? 0:NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE);
		
		if(SUCCEEDED(hr))
		{
			m_NearMode =! m_NearMode;
		}
	}

	return hr;
}

void ProcessDepth()
{

	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(DepthStreamHandle, 0, &imageFrame);;
	if(FAILED(hr))
	{
		return;
	}
	INuiFrameTexture* pTexture = NULL;

	hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(DepthStreamHandle, &imageFrame, &m_NearMode, &pTexture);
	if(FAILED(hr))
	{
		goto ReleaseFrame;
	}

	NUI_LOCKED_RECT LockedRect;

	pTexture->LockRect(0, &LockedRect, NULL, 0);

	if(LockedRect.Pitch != 0)
	{
		int minDepth  = (m_NearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
		int maxDepth = (m_NearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

		BYTE *rgbrun = m_depthRGBX;

		const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

		const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (DEPTH_WIDTH * DEPTH_HEIGHT);

		while(pBufferRun < pBufferEnd)
		{
			USHORT depth = pBufferRun->depth;
			BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? depth % 256 :0);
			*(rgbrun++) = intensity;
			*(rgbrun++) = intensity;
			*(rgbrun++) = intensity;
			++rgbrun;
			++pBufferRun;
		}
		cvSetData(Depth, (BYTE*)m_depthRGBX, Depth->widthStep);
		cvShowImage("depth Image", Depth);
	}

	pTexture->UnlockRect(0);

	pTexture->Release();

ReleaseFrame:

	m_pNuiSensor->NuiImageStreamReleaseFrame(DepthStreamHandle, &imageFrame);
}

void InitializeKinect()
{
	bool FailToConnect;

	do
	{
		HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON);

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
	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);

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
		cvCircle(Color, c1, 30,CV_RGB(255, 0, 0),3);
		cvCircle(Color, c2, 30,CV_RGB(255, 0, 0),3);
		cvShowImage("Color Image", Color);

	}

	NuiImageStreamReleaseFrame(h, pImageFrame);

	return 0;
}

int createDepthImage(HANDLE h, IplImage* Depth)
{
	const NUI_IMAGE_FRAME *pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);

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
				pBufferRun++;
				*rgbrun = quad;
				rgbrun++;

			}
		}

		cvSetData(Depth, (BYTE*)rgb, Depth->widthStep);
		cvShowImage("Depth Image", Depth);
	}

	NuiImageStreamReleaseFrame(h, pImageFrame);

	return 0;
}

RGBQUAD Nui_ShortToQuad_Depth(USHORT s)
{
	USHORT realDepth = (s&0xfff8) >> 3;
//	      USHORT testRealDepth = NuiDepthPixelToDepth(s);

//	 Convert depth info into an intensity for display
	BYTE b = 255 - static_cast<BYTE>(256 * realDepth / 0x0fff);

	      
//	      cout<<"test Depth : "<<testRealDepth<<endl;

	RGBQUAD q;
	q.rgbRed = q.rgbBlue = q.rgbGreen = ~b;
	return q;
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
		
	drawBone(position, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT, Skeleton);

	drawBone(position, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT, Skeleton);
	drawBone(position, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT, Skeleton);
}

/////////////////////////////////////////////////////////////////////////////
CvPoint distTF(IplImage* img)			//손의 무게중심을 찾기 위한
{
	float mask[3];
	IplImage *dist = 0;
	IplImage *dist8u = 0;
	IplImage *dist32s = 0;
	int max;
	CvPoint p;

	dist = cvCreateImage(cvGetSize(img), IPL_DEPTH_32F, 1);		//IPL_DEPTH_32F 32bit 실수로 표현하겠다는 뜻
	dist8u = cvCloneImage(img);									//복사하고자 하는 영역의 이미지
	dist32s = cvCreateImage(cvGetSize(img), IPL_DEPTH_32S, 1);


	//거리변환 행렬 생성
	mask[0] = 1.0f;
	mask[1] = 1.5f;

	//거리 변환 함수 사용
	cvDistTransform(img, dist, CV_DIST_USER, 3, mask, NULL);	//intput 이미지의 픽셀마다 가장 가까운 0픽셀과의 거리를 측정해서  output 배열에 넣는 함수.

	//눈에 보이게 변환
	cvConvertScale(dist, dist, 1000, 0);						//배열을 저장 후 scaling 한다.
	cvPow(dist, dist, 0.5);

	cvConvertScale(dist, dist32s, 1.0, 0.5);
	cvAndS(dist32s, cvScalarAll(255), dist32s, 0);				//cvAnds는 스칼라 값을 대상으로 cvand는 이미지를 대상으로 
	cvConvertScale(dist32s, dist8u, 1, 0);


	//가장 큰 좌표값을 찾는다.
	for(int i = max; i<dist8u->height; i++)
	{
		int index=i*dist8u->widthStep;
		for(int j = 0; j<dist8u->width; j++)
		{
			if((unsigned char)dist8u->imageData[index+j]>max)
			{
				max = (unsigned char)dist8u->imageData[index + j];
				p.x = j;
				p.y = i;
			}
		}
	}

	cvReleaseImage(&dist);
	cvReleaseImage(&dist8u);
	cvReleaseImage(&dist32s);

	return p;
}