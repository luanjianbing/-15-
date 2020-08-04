#pragma once
#ifndef COMMON_H
#define COMMON_H

#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

typedef struct _ObjInfo
{
	cv::Rect2f _faceLoc;	// ����
	float _conf;	// ���Ŷ�
	int _faceCla;	// �жϽ��
} ObjInfo;

#endif // COMMON_H
