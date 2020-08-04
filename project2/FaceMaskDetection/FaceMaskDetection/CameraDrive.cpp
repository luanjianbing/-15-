#include "CameraDrive.h"
#include <mutex>

std::mutex cameraGet;
Camera::Camera() {
	stop = false;
}

void Camera::camStart() {
	//if (capture1.open(source1, cv::CAP_FFMPEG)) 
	//if (capture1.open(0, cv::CAP_DSHOW))	// ����cv::CAP_DSHOW�˳�ʱ�ᱨwarning
	//	startthread1();
	//else
	//	throw "Camera Open Failed";
	if (this->Init())
	{
		if (this->Login("58.199.166.90", "admin", "tongxint704", 8000))//�û����Լ����룬���ݴ�ϵ�в���һ�еķ����鿴������
		{
			std::cout << "login successfully" << std::endl;
			this->show();
			//startthread1();
		}
		else
			throw "camera login error";
	}
	else
		throw "camera init error";
}

LONG g_nPort;
cv::Mat g_BGRImage;
cv::Mat lastImage;
bool isOnConvertingImage = false;
bool newImage = true;
//std::mutex mtxImg;
//���ݽ���ص�������
//���ܣ���YV_12��ʽ����Ƶ������ת��Ϊ�ɹ�opencv�����BGR���͵�ͼƬ���ݣ���ʵʱ��ʾ��
Solution solution;
void CALLBACK DecCBFun(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{

	if (pFrameInfo->nType == T_YV12)
	{
		//if (!newImage)
		//	return;
		//newImage = false;
		isOnConvertingImage = true;
		cameraGet.lock();
		if (g_BGRImage.empty())
		{
			g_BGRImage.create(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
		}
		cv::Mat YUVImage(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

		cvtColor(YUVImage, g_BGRImage, cv::COLOR_YUV2BGR_YV12);

		//cv::flip(g_BGRImage, g_BGRImage, 1);
		cameraGet.unlock();
		//cv::imshow("", g_BGRImage);
		//cv::waitKey(1);
		isOnConvertingImage = false;
		lastImage = g_BGRImage;
		YUVImage.~Mat();
	}
}

//ʵʱ��Ƶ�������ݻ�ȡ �ص�����
void CALLBACK g_RealDataCallBack_V30(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser)
{
	if (dwDataType == NET_DVR_STREAMDATA)//��������
	{
		if (dwBufSize > 0 && g_nPort != -1)
		{
			if (!PlayM4_InputData(g_nPort, pBuffer, dwBufSize))
			{
				//std::cout << "fail input data" << std::endl;
			}
			else
			{
				//std::cout << "success input data" << std::endl;
			}

		}
	}
}

bool Camera::Login(char* sDeviceAddress, char* sUserName, char* sPassword, WORD wPort)            //��½
{
	NET_DVR_USER_LOGIN_INFO pLoginInfo = { 0 };
	NET_DVR_DEVICEINFO_V40 lpDeviceInfo = { 0 };

	pLoginInfo.bUseAsynLogin = 0;     //ͬ����¼��ʽ
	strcpy_s(pLoginInfo.sDeviceAddress, sDeviceAddress);
	strcpy_s(pLoginInfo.sUserName, sUserName);
	strcpy_s(pLoginInfo.sPassword, sPassword);
	pLoginInfo.wPort = wPort;

	lUserID = NET_DVR_Login_V40(&pLoginInfo, &lpDeviceInfo);

	if (lUserID < 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Camera::Init()
{
	if (NET_DVR_Init())
	{
		return true;
	}
	else
	{
		return false;
	}
}

//��Ƶ����ʾ����
void Camera::show()
{
	if (PlayM4_GetPort(&g_nPort))            //��ȡ���ſ�ͨ����
	{
		if (PlayM4_SetStreamOpenMode(g_nPort, STREAME_REALTIME))      //������ģʽ
		{
			if (PlayM4_OpenStream(g_nPort, NULL, 0, 3840 * 2160))         //����
			{
				if (PlayM4_SetDecCallBackExMend(g_nPort, DecCBFun, NULL, 0, NULL))
				{
					if (PlayM4_Play(g_nPort, NULL))
					{
						std::cout << "success to set play mode" << std::endl;
					}
					else
					{
						std::cout << "fail to set play mode" << std::endl;
					}
				}
				else
				{
					std::cout << "fail to set dec callback " << std::endl;
				}
			}
			else
			{
				std::cout << "fail to open stream" << std::endl;
			}
		}
		else
		{
			std::cout << "fail to set stream open mode" << std::endl;
		}
	}
	else
	{
		std::cout << "fail to get port" << std::endl;
	}
	//����Ԥ�������ûص�������
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = NULL; //����Ϊ�գ��豸SDK������ֻȡ��
	struPlayInfo.lChannel = 1; //Channel number �豸ͨ��
	struPlayInfo.dwStreamType = 1;// �������ͣ�0-��������1-��������2-����3��3-����4, 4-����5,5-����6,7-����7,8-����8,9-����9,10-����10
	struPlayInfo.dwLinkMode = 0;// 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-������ȡ��, 1-����ȡ��, �������SDK�ڲ�connectʧ�ܽ�����5s�ĳ�ʱ���ܹ�����,���ʺ�����ѯȡ������.

	if (NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL))
	{
		cv::namedWindow("RGBImage2");
	}
}

void Camera::startthread1() {

	std::thread task(&video_capture_thread1, (void*)this);

	task.detach();
}

void *Camera::video_capture_thread1(void *arg) {
	Camera *came = (Camera *)arg;

	while (!came->stop)
	{
		came->getImage(1);
		
	}

	delete came;
	return 0;
}

void Camera::getImage(int threadId) {
	switch (threadId)
	{
	case 1:
	{
		capture1 >> frame;
		//cv::resize(frame, frame, cv::Size(1280, 720), CV_INTER_AREA);
		break;
	}
	default:
		break;
	}
}

void Camera::getImage(cv::Mat &getImg)
{
	//if (isOnConvertingImage)
	//	getImg = lastImage;
	//else
	cameraGet.lock();
		getImg = g_BGRImage;
		//newImage = true;
		cameraGet.unlock();
	
}

void Camera::camStop() {
	stop = true;
	capture1.release();
	//std::cout << capture1.isOpened() << std::endl;
}

Camera::~Camera() {
	camStop();
}