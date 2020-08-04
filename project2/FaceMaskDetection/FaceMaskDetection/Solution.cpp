#include "Solution.h"

Solution::Solution()
{
	// Initialize face detection model
	detector = new seeta::FaceDetection("./model/seeta_fd_frontal_v1.0.bin");
	detector->SetMinFaceSize(40);
	detector->SetScoreThresh(2.f);
	detector->SetImagePyramidScaleFactor(0.8f);
	detector->SetWindowStep(4, 4);
	// Initialize face alignment model 
	//seeta::FaceAlignment point_detector("./models/seeta_fa_v1.1.bin");
	point_detector = new seeta::FaceAlignment("./models/seeta_fa_v1.1.bin");
	// Initialize face recognize model 
	//seeta::FaceIdentification face_recognizer("./models/seeta_fr_v1.0.bin");
	face_recognizer = new seeta::FaceIdentification("./models/seeta_fr_v1.0.bin");
	readFeatureCSV();	
}

void Solution::readFeatureCSV()
{
	std::ifstream pf("./feature.csv", std::ifstream::in);
	std::string lineStr;
	while (std::getline(pf, lineStr))
	{
		std::stringstream ss(lineStr);
		std::string str;
		std::vector<std::string> lineArray;
		while (std::getline(ss, str, ','))
			lineArray.push_back(str);
		csvData.push_back(lineArray);
	}
	pf.close();

	// ����csvData��[...][0]Ϊ���ƣ�֮��Ϊ����
	// ����һ��
	allFeature = new float[2048 * csvData.size()];
	for (int i = 0; i < csvData.size(); ++i)
	{
		int curSize = 2048 * i;
		for (int j = 1; j < csvData[i].size(); ++j)
		{
			*(allFeature + curSize + j - 1) = string_to_float(csvData[i][j]);
		}
	}
}

inline float Solution::string_to_float(std::string str) {
	int i = 0, len = str.length();
	float sum = 0;
	while (i<len) {
		if (str[i] == '.') break;
		sum = sum * 10 + str[i] - '0';
		++i;
	}
	++i;
	float t = 1, d = 1;
	while (i<len) {
		d *= 0.1;
		t = str[i] - '0';
		sum += t*d;
		++i;
	}
	return sum;
}

// ����Ȩ���ļ���ģ���ļ�
bool Solution::initialParam()
{
	std::string modelTxt = "./models/ssd30000.prototxt";
	std::string modelBin = "./models/ssd30000.caffemodel";
	try {
		m_mapNet = cv::dnn::readNetFromCaffe(modelTxt, modelBin);
	}
	catch (cv::Exception &ee) {
		return false;
	}
	return true;
}

const int Solution::ssdFaceMaskDetect(cv::Mat& inMat, std::vector<ObjInfo>& vReList)
{
	//cv::Mat inMat = cv::imread("./models/6.jpg");
	//std::cout << inMat.rows << " " << inMat.cols << " " << inMat.channels() << std::endl;
	//// inMatԭ��ΪCV8UC3

	// Ԥ����(prototxt����û��mean����mean��Ϊ0��)
	// https://blog.csdn.net/u011028771/article/details/84901361
	cv::Mat inputBlob = cv::dnn::blobFromImage(inMat, 1.0, cv::Size(300, 300), cv::Scalar(123.0, 117.0, 104.0), true, false);
	// ��һ��֮��inputBlob�ͱ��CV32FC1
	m_mapNet.setInput(inputBlob, "blob1");	// dataΪinputLayer�����֣���prototxt�����һ��

	std::vector<cv::Mat> outputBlobs;
	std::vector<cv::String> outBlobNames;
	outBlobNames.push_back("permute1"); //loc1
	outBlobNames.push_back("permute2"); //conf1
	outBlobNames.push_back("permute3"); //loc2
	outBlobNames.push_back("permute4"); //conf2
	outBlobNames.push_back("permute5"); //loc3
	outBlobNames.push_back("permute6"); //conf3
	outBlobNames.push_back("permute7"); //loc4
	outBlobNames.push_back("permute8"); //conf4
	outBlobNames.push_back("permute9");
	outBlobNames.push_back("permute10");
	m_mapNet.forward(outputBlobs, outBlobNames);	// ����������֣�outputBlobs���Ƕ�Ӧ������

	cv::Mat locBlobs;
	cv::Mat clsBlobs;
	cv::Mat locBlob[5];
	cv::Mat confBlob[5];
	for (int i = 0; i < 5; i++) {
		locBlob[i] = outputBlobs[i * 2].reshape(0, priorNum[i]);
		confBlob[i] = outputBlobs[i * 2 + 1].reshape(0, priorNum[i]);
	}
	cv::vconcat(locBlob[0], locBlob[1], locBlobs);
	cv::vconcat(locBlobs, locBlob[2], locBlobs);
	cv::vconcat(locBlobs, locBlob[3], locBlobs);
	cv::vconcat(locBlobs, locBlob[4], locBlobs);
	cv::vconcat(confBlob[0], confBlob[1], clsBlobs);
	cv::vconcat(clsBlobs, confBlob[2], clsBlobs);
	cv::vconcat(clsBlobs, confBlob[3], clsBlobs);
	cv::vconcat(clsBlobs, confBlob[4], clsBlobs);

	//cv::Mat locBlobs = outputBlobs.front().reshape(0, 4);  // ��ȡ�����ڶ��ν����ͨ�������䣬��Ϊ4��(����?)
	//cv::Mat clsBlobs = outputBlobs.back().reshape(0, 2);  // ��ȡ���һ��������Ϊ2��(���?)

	// �����С
	//int locSize = locBlobs.channels()*locBlobs.rows*locBlobs.cols;	// 1 x 4 x 5972
	//int clsSize = clsBlobs.channels()*clsBlobs.rows*clsBlobs.cols;	// 1 x 2 x 5972
	//																// ת��
	//cv::Mat concatLoc(locSize / 4, 4, CV_32F, outputBlobs.front().ptr(0, 0));	// 5972 x 4 
	//cv::Mat concatCls(clsSize / 2, 2, CV_32F, outputBlobs.back().ptr(0, 0));	// 5972 x 2
	softmax(clsBlobs, totalDefaultBox, 3);
	std::vector<ObjInfo> vResultList = decode_bbox(locBlobs, clsBlobs);
	//std::vector<ObjInfo> vReList = vResultList;
	vReList = vResultList;
	int width = inMat.cols;
	int height = inMat.rows;
	for (int i = 0; i < vResultList.size(); ++i)
	{
		vReList[i]._faceLoc.x = std::max(0, int(vReList[i]._faceLoc.x * width));
		vReList[i]._faceLoc.y = std::max(0, int(vReList[i]._faceLoc.y * height));
		vReList[i]._faceLoc.width = std::max(0, int(vReList[i]._faceLoc.width * width));
		vReList[i]._faceLoc.height = std::max(0, int(vReList[i]._faceLoc.height * height));
		vReList[i]._faceLoc.width = std::min(width - (int)vReList[i]._faceLoc.x, int(vReList[i]._faceLoc.width));
		vReList[i]._faceLoc.height = std::min(height - (int)vReList[i]._faceLoc.y, int(vReList[i]._faceLoc.height));
	}
	//vReList.clear();
	//if (inMat.empty()) {
	//	return -1;
	//}

	//cv::Mat img;
	//cv::resize(inMat, img, cv::Size(260, 260));	// ģ������ߴ��С��260x260
	//// imgԭ��ΪCV8UC3

	//// Ԥ����(prototxt����û��mean����mean��Ϊ0��)
	//// https://blog.csdn.net/u011028771/article/details/84901361
	//cv::Mat inputBlob = cv::dnn::blobFromImage(img, 1.0 / 255.0, cv::Size(), cv::Scalar(0, 0, 0), false, false);
	//// ��һ��֮��inputBlob�ͱ��CV32FC1
	//cv::Mat pred;
	//m_mapNet.setInput(inputBlob, "data");	// dataΪinputLayer�����֣���prototxt�����һ��

	//std::vector<cv::Mat> outputBlobs;
	//std::vector<cv::String> outBlobNames;
	//outBlobNames.push_back("loc_branch_concat");
	//outBlobNames.push_back("cls_branch_concat");
	//m_mapNet.forward(outputBlobs, outBlobNames);	// ����������֣�outputBlobs���Ƕ�Ӧ������

	//cv::Mat locBlobs = outputBlobs.front().reshape(0, 4);  // ��ȡ�����ڶ��ν����ͨ�������䣬��Ϊ4��(����?)
	//cv::Mat clsBlobs = outputBlobs.back().reshape(0, 2);  // ��ȡ���һ��������Ϊ2��(���?)

	//// �����С
	//int locSize = locBlobs.channels()*locBlobs.rows*locBlobs.cols;	// 1 x 4 x 5972
	//int clsSize = clsBlobs.channels()*clsBlobs.rows*clsBlobs.cols;	// 1 x 2 x 5972
	//// ת��
	//cv::Mat concatLoc(locSize / 4, 4, CV_32F, outputBlobs.front().ptr(0, 0));	// 5972 x 4 
	//cv::Mat concatCls(clsSize / 2, 2, CV_32F, outputBlobs.back().ptr(0, 0));	// 5972 x 2

	//std::vector<ObjInfo> vResultList = decode_bbox(concatLoc, concatCls);
	//vReList = vResultList;
	//int width = inMat.cols;
	//int height = inMat.rows;
	//for (int i = 0; i < vResultList.size(); ++i)
	//{
	//	vReList[i]._faceLoc.x = std::max(0, int(vReList[i]._faceLoc.x * width));
	//	vReList[i]._faceLoc.y = std::max(0, int(vReList[i]._faceLoc.y * height));
	//	vReList[i]._faceLoc.width = std::max(0, int(vReList[i]._faceLoc.width * width));
	//	vReList[i]._faceLoc.height = std::max(0, int(vReList[i]._faceLoc.height * height));
	//	vReList[i]._faceLoc.width = std::min(width - (int)vReList[i]._faceLoc.x, int(vReList[i]._faceLoc.width));
	//	vReList[i]._faceLoc.height = std::min(height - (int)vReList[i]._faceLoc.y, int(vReList[i]._faceLoc.height));
	//}

	return 1;
}

//��ÿһ�н���softmax
void Solution::softmax(cv::Mat &x, int row, int column)
{
	for (int j = 0; j < row; ++j)
	{
		float max = 0.0;
		float sum = 0.0;
		for (int k = 0; k < column; ++k)
			if (max < x.at<float>(j, k))
				max = x.at<float>(j, k);
		for (int k = 0; k < column; ++k)
		{
			x.at<float>(j, k) = exp(x.at<float>(j, k) - max);    // prevent data overflow
			sum += x.at<float>(j, k);
		}
		for (int k = 0; k < column; ++k) x.at<float>(j, k) /= sum;
	}
}

//// ��������������Ҫ��Ϣ
//std::vector<ObjInfo> Solution::decode_bbox(cv::Mat& rawBox, cv::Mat& rawCls)
//{
//	std::vector<ObjInfo> vR; vR.clear();
//	if (rawBox.empty() || rawCls.empty())
//		return vR;
//	if (rawBox.rows != rawCls.rows)
//		return vR;
//
//	float ecsplion = 2.718282;	// 
//	float variances[4] = { 0.1, 0.1, 0.2, 0.2 };	//
//	float anchor_centers_x[5972];
//	float anchor_centers_y[5972];
//	float anchors_w[5972];
//	float anchors_h[5972];
//	float raw_outputs_rescale[5972][4];
//	float predict_center_x[5972];
//	float predict_center_y[5972];
//	float predict_w[5972];
//	float predict_h[5972];
//	std::vector<int> vValidateIndexList; vValidateIndexList.clear();
//	std::vector<int> vValidateClassList; vValidateClassList.clear();
//	std::vector<float> vValidateProbabilityList; vValidateProbabilityList.clear();
//	std::vector<cv::Rect2f> vValidateRectList; vValidateRectList.clear();
//
//	for (int i = 0; i < 5972; ++i)
//	{
//		float cls_0 = rawCls.at<float>(i, 0);
//		float cls_1 = rawCls.at<float>(i, 1);
//		// ÿ�бȽϣ�ȡ���
//		int maxClass = 0;	// ��
//		float maxCls = cls_0;	// ֵ
//		if (cls_1 > cls_0) {
//			maxClass = 1;
//			maxCls = cls_1;
//		}
//		if (maxCls > 0.5) {	// ����0.5��Ϊ��Ч
//			vValidateProbabilityList.push_back(maxCls);
//			vValidateIndexList.push_back(i);
//			vValidateClassList.push_back(maxClass);
//		}
//		else {
//			//throw "maxCls < 0.5";
//			continue;
//		}
//
//		// ����ÿ�е����ĵ�Ϳ��
//		anchor_centers_x[i] = (anchor_f[i][0] + anchor_f[i][2]) / 2;
//		anchor_centers_y[i] = (anchor_f[i][1] + anchor_f[i][3]) / 2;
//		anchors_w[i] = anchor_f[i][2] - anchor_f[i][0];
//		anchors_h[i] = anchor_f[i][3] - anchor_f[i][1];
//		// 
//		raw_outputs_rescale[i][0] = rawBox.at<float>(i, 0) * variances[0];
//		raw_outputs_rescale[i][1] = rawBox.at<float>(i, 1) * variances[1];
//		raw_outputs_rescale[i][2] = rawBox.at<float>(i, 2) * variances[2];
//		raw_outputs_rescale[i][3] = rawBox.at<float>(i, 3) * variances[3];
//		//
//		predict_center_x[i] = raw_outputs_rescale[i][0] * anchors_w[i] + anchor_centers_x[i];
//		predict_center_y[i] = raw_outputs_rescale[i][1] * anchors_h[i] + anchor_centers_y[i];
//		predict_w[i] = std::pow(ecsplion, raw_outputs_rescale[i][2]) * anchors_w[i];
//		predict_h[i] = std::pow(ecsplion, raw_outputs_rescale[i][3]) * anchors_h[i];
//		// ������
//		cv::Rect2f tempRt;
//		tempRt.x = predict_center_x[i] - predict_w[i] / 2;
//		tempRt.y = predict_center_y[i] - predict_h[i] / 2;
//		tempRt.width = predict_center_x[i] + predict_w[i] / 2 - tempRt.x;
//		tempRt.height = predict_center_y[i] + predict_h[i] / 2 - tempRt.y;
//		vValidateRectList.push_back(tempRt);
//	}
//	//���ŶȴӴ�С����
//	std::vector<int> vMaxSortList; vMaxSortList.clear();
//	std::vector<float> vTempValidateProbabilityList = vValidateProbabilityList;
//	std::sort(vTempValidateProbabilityList.begin(), \
//		vTempValidateProbabilityList.end(), \
//		[](float a1, float a2) {return a1 > a2 ? 1 : 0; });	// ��������
//	// ��һ����ʵҲ�����ҵ���Ӧ������(����ӳ������)
//	for (int i = 0; i < vTempValidateProbabilityList.size(); ++i)
//	{
//		float iConf = vTempValidateProbabilityList[i];
//		int maxInd = i;
//		for (int j = 0; j < vValidateProbabilityList.size(); ++j)
//		{
//			float jconf = vValidateProbabilityList[j];
//			if (jconf == iConf) {
//				maxInd = j;
//				break;
//			}
//		}
//		vMaxSortList.push_back(maxInd);
//	}
//	//�ϲ�rect
//	for (int i = 0; i < vMaxSortList.size(); ++i) {
//		int tempInd = vMaxSortList[i];
//		cv::Rect2f tempRect = vValidateRectList[tempInd];
//		for (int j = i + 1; j < vMaxSortList.size(); ++j) {
//			int probInd = vMaxSortList[j];
//			cv::Rect2f probRect = vValidateRectList[probInd];
//			float lapRate = rectOverlap(probRect, tempRect);
//			if (lapRate > 0.4) {
//				vMaxSortList.erase(vMaxSortList.begin() + j);
//				--j;
//			}
//		}
//	}
//	//������
//	vR.resize(vMaxSortList.size());
//	for (int i = 0; i < vMaxSortList.size(); ++i)
//	{
//		int objInd = vMaxSortList[i];
//		vR[i]._faceLoc = vValidateRectList[objInd];
//		vR[i]._faceCla = vValidateClassList[objInd];
//		vR[i]._conf = vValidateProbabilityList[objInd];
//	}
//
//	return vR;
//}
// ��������������Ҫ��Ϣ
std::vector<ObjInfo> Solution::decode_bbox(cv::Mat& rawBox, cv::Mat& rawCls)
{
	std::vector<ObjInfo> vR; vR.clear();
	if (rawBox.empty() || rawCls.empty())
		return vR;
	if (rawBox.rows != rawCls.rows)
		return vR;

	float ecsplion = 2.718282;	// 
	float variances[4] = { 0.1, 0.1, 0.2, 0.2 };	//
	float anchor_centers_x[totalDefaultBox];
	float anchor_centers_y[totalDefaultBox];
	float anchors_w[totalDefaultBox];
	float anchors_h[totalDefaultBox];
	float raw_outputs_rescale[totalDefaultBox][4];
	float predict_center_x[totalDefaultBox];
	float predict_center_y[totalDefaultBox];
	float predict_w[totalDefaultBox];
	float predict_h[totalDefaultBox];
	std::vector<int> vValidateIndexList; vValidateIndexList.clear();
	std::vector<int> vValidateClassList; vValidateClassList.clear();
	std::vector<float> vValidateProbabilityList; vValidateProbabilityList.clear();
	std::vector<cv::Rect2f> vValidateRectList; vValidateRectList.clear();

	for (int i = 0; i < totalDefaultBox; ++i)
	{
		float cls_0 = rawCls.at<float>(i, 1);
		float cls_1 = rawCls.at<float>(i, 2);
		// ÿ�бȽϣ�ȡ���
		int maxClass = 0;	// ��
		float maxCls = cls_0;	// ֵ
		if (cls_1 > cls_0) {
			maxClass = 1;
			maxCls = cls_1;
		}
		if (maxCls > 0.4) {	// ����0.5��Ϊ��Ч
			vValidateProbabilityList.push_back(maxCls);
			vValidateIndexList.push_back(i);
			vValidateClassList.push_back(maxClass);
		}
		else {
			//throw "maxCls < 0.5";
			continue;
		}

		// ����ÿ�е����ĵ�Ϳ��
		anchor_centers_x[i] = anchor_f[i][0];
		anchor_centers_y[i] = anchor_f[i][1];
		anchors_w[i] = anchor_f[i][2];
		anchors_h[i] = anchor_f[i][3];
		// 
		raw_outputs_rescale[i][0] = rawBox.at<float>(i, 0) * variances[0];
		raw_outputs_rescale[i][1] = rawBox.at<float>(i, 1) * variances[1];
		raw_outputs_rescale[i][2] = rawBox.at<float>(i, 2) * variances[2];
		raw_outputs_rescale[i][3] = rawBox.at<float>(i, 3) * variances[3];
		//
		predict_center_x[i] = raw_outputs_rescale[i][0] * anchors_w[i] + anchor_centers_x[i];
		predict_center_y[i] = raw_outputs_rescale[i][1] * anchors_h[i] + anchor_centers_y[i];
		predict_w[i] = std::exp(raw_outputs_rescale[i][2]) * anchors_w[i];
		predict_h[i] = std::exp(raw_outputs_rescale[i][3]) * anchors_h[i];
		// ������
		cv::Rect2f tempRt;
		tempRt.x = predict_center_x[i] - predict_w[i] / 2;
		tempRt.y = predict_center_y[i] - predict_h[i] / 2;
		tempRt.width = predict_center_x[i] + predict_w[i] / 2 - tempRt.x;
		tempRt.height = predict_center_y[i] + predict_h[i] / 2 - tempRt.y;
		vValidateRectList.push_back(tempRt);
	}
	//���ŶȴӴ�С����
	std::vector<int> vMaxSortList; vMaxSortList.clear();
	std::vector<float> vTempValidateProbabilityList = vValidateProbabilityList;
	std::sort(vTempValidateProbabilityList.begin(), \
		vTempValidateProbabilityList.end(), \
		[](float a1, float a2) {return a1 > a2 ? 1 : 0; });	// ��������
															// ��һ����ʵҲ�����ҵ���Ӧ������(����ӳ������)
	for (int i = 0; i < vTempValidateProbabilityList.size(); ++i)
	{
		float iConf = vTempValidateProbabilityList[i];
		int maxInd = i;
		for (int j = 0; j < vValidateProbabilityList.size(); ++j)
		{
			float jconf = vValidateProbabilityList[j];
			if (jconf == iConf) {
				maxInd = j;
				break;
			}
		}
		vMaxSortList.push_back(maxInd);
	}
	//�ϲ�rect
	for (int i = 0; i < vMaxSortList.size(); ++i) {
		int tempInd = vMaxSortList[i];
		cv::Rect2f tempRect = vValidateRectList[tempInd];
		for (int j = i + 1; j < vMaxSortList.size(); ++j) {
			int probInd = vMaxSortList[j];
			cv::Rect2f probRect = vValidateRectList[probInd];
			float lapRate = rectOverlap(probRect, tempRect);
			if (lapRate > 0.4) {
				vMaxSortList.erase(vMaxSortList.begin() + j);
				--j;
			}
		}
	}
	//������
	vR.resize(vMaxSortList.size());
	for (int i = 0; i < vMaxSortList.size(); ++i)
	{
		int objInd = vMaxSortList[i];
		vR[i]._faceLoc = vValidateRectList[objInd];
		vR[i]._faceCla = vValidateClassList[objInd];
		vR[i]._conf = vValidateProbabilityList[objInd];
	}

	return vR;
}

float Solution::rectOverlap(cv::Rect2f rect_1, cv::Rect2f rect_2)
{
	bool bIsOverlapReturn = false;

	float maxleftx = std::max(rect_1.x, rect_2.x);
	float maxlefty = std::max(rect_1.y, rect_2.y);
	float minrightx = std::min(rect_1.x + rect_1.width, rect_2.x + rect_2.width);
	float minrigty = std::min(rect_1.y + rect_1.height, rect_2.y + rect_2.height);
	if (maxleftx >= minrightx || maxlefty >= minrigty)
	{
		bIsOverlapReturn = false;
		return 0;
	}
	else
	{
		float areaa = (minrightx - maxleftx)*(minrigty - maxlefty);
		float  fRatio = areaa / (rect_2.width*rect_2.height);

		return fRatio;
	}

}


bool Solution::extractFaceFeature(cv::Mat dstMat, std::vector<ObjInfo> listInfo, std::vector<std::string> &name)
{
	//nameGet.resize(listInfo.size(), -1);
	//cv::Mat dstMatCopy = dstMat;
	//std::vector<ObjInfo> listInfoCopy = listInfo;
	//for (int j = 0; j < nameGet.size(); ++j)
	//{
	//	cv::Mat vectFace = dstMatCopy(cv::Rect(listInfoCopy[j]._faceLoc));
	//	cv::Mat grayFace;
	//	cv::cvtColor(vectFace, grayFace, CV_RGB2GRAY);
	//	IplImage *img_color, *img_grayscale;
	//	img_color = &IplImage(vectFace);
	//	img_grayscale = &IplImage(grayFace);
	//	int im_width = img_grayscale->width;
	//	int im_height = img_grayscale->height;
	//	unsigned char* data = new unsigned char[im_width * im_height];
	//	unsigned char* data_ptr = data;
	//	unsigned char* image_data_ptr = (unsigned char*)img_grayscale->imageData;
	//	for (int h = 0; h < im_height; h++) {
	//		memcpy(data_ptr, image_data_ptr, im_width);
	//		data_ptr += im_width;
	//		image_data_ptr += img_grayscale->widthStep;
	//	}
	//	seeta::ImageData image_data;
	//	image_data.data = data;
	//	image_data.width = im_width;
	//	image_data.height = im_height;
	//	image_data.num_channels = 1;
	//	seeta::Rect faceRect;
	//	faceRect.x = faceRect.y = 0;
	//	faceRect.width = im_width, faceRect.height = im_height;
	//	seeta::FaceInfo faceInfo;
	//	faceInfo.bbox = faceRect;
	//	faceInfo.roll = -9.25596e+61;
	//	faceInfo.pitch = -9.25596e+61;
	//	faceInfo.yaw = -9.25596e+61;
	//	faceInfo.score = 34.9409;
	//	// Detect 5 facial landmarks
	//	seeta::FacialLandmark points[5];
	//	point_detector->PointDetectLandmarks(image_data, faceInfo, points);

	//	// ImageData store data of an image without memory alignment.
	//	seeta::ImageData src_img_data(vectFace.cols, vectFace.rows, vectFace.channels());
	//	src_img_data.data = vectFace.data;
	//	// 5 located landmark points (left eye, right eye, nose, left and right 
	//	// corner of mouse).
	//	float* feats = new float[face_recognizer->feature_size()];
	//	std::wcout << "1.........." << std::endl;
	//	/* Extract feature: ExtractFeatureWithCrop */
	//	face_recognizer->ExtractFeatureWithCrop(src_img_data, points, feats);
	//	std::wcout << "2.........." << std::endl;
	//	/* Caculate similarity*/
	//	int curNum = 0, minNum = 0;
	//	float minSim = 1;
	//	for (; curNum < csvData.size(); curNum++)
	//	{
	//		float sim = face_recognizer->CalcSimilarity(allFeature + curNum * 2048, feats);
	//		std::cout << "Similarity : " << 1 - sim << std::endl;
	//		if (sim < minSim && (1 - sim) > 0.4) {
	//			minSim = sim;
	//			minNum = curNum;
	//		}
	//		//if (sim < 0.5)
	//		//	break;
	//	}
	//	std::wcout << "3.........." << std::endl;
	//	delete[] feats;
	//	//if (curNum == csvData.size())
	//	//	nameGet[j] = -1;
	//	//else
	//	//	nameGet[j] = curNum;
	//	mtx.lock();
	//	if (minSim == 1)
	//		nameGet[j] = -1;
	//	else
	//		nameGet[j] = minNum;
	//	mtx.unlock();
	//}
	//for (int i = 0; i < name.size(); ++i)
	//	if (nameGet[i] == -1)
	//		name[i] = "Unknown";
	//	else
	//		name[i] = csvData[nameGet[i]][0];
	//hasFirstRes = true;
	if (threadFlags == false) {
		threadFlags = true;
		//mtx.lock();
		nameGet.resize(listInfo.size(), -1);
		cv::Mat dstMatCopy = dstMat;
		std::vector<ObjInfo> listInfoCopy = listInfo;
		//mtx.unlock();
		std::thread run([=](){
			std::wcout << "in.........." << std::endl;
			
			for (int j = 0; j < nameGet.size(); ++j)
			{
				cv::Mat vectFace = dstMatCopy(cv::Rect(listInfoCopy[j]._faceLoc));
				cv::Mat grayFace;
				cv::cvtColor(vectFace, grayFace, CV_RGB2GRAY);
				IplImage *img_color, *img_grayscale;
				img_color = &IplImage(vectFace);
				img_grayscale = &IplImage(grayFace);
				int im_width = img_grayscale->width;
				int im_height = img_grayscale->height;
				unsigned char* data = new unsigned char[im_width * im_height];
				unsigned char* data_ptr = data;
				unsigned char* image_data_ptr = (unsigned char*)img_grayscale->imageData;
				for (int h = 0; h < im_height; h++) {
					memcpy(data_ptr, image_data_ptr, im_width);
					data_ptr += im_width;
					image_data_ptr += img_grayscale->widthStep;
				}
				seeta::ImageData image_data;
				image_data.data = data;
				image_data.width = im_width;
				image_data.height = im_height;
				image_data.num_channels = 1;
				seeta::Rect faceRect;
				faceRect.x = faceRect.y = 0;
				faceRect.width = im_width, faceRect.height = im_height;
				seeta::FaceInfo faceInfo;
				faceInfo.bbox = faceRect;
				faceInfo.roll = -9.25596e+61;
				faceInfo.pitch = -9.25596e+61;
				faceInfo.yaw = -9.25596e+61;
				faceInfo.score = 34.9409;
				// Detect 5 facial landmarks
				seeta::FacialLandmark points[5];
				point_detector->PointDetectLandmarks(image_data, faceInfo, points);

				// ImageData store data of an image without memory alignment.
				seeta::ImageData src_img_data(vectFace.cols, vectFace.rows, vectFace.channels());
				src_img_data.data = vectFace.data;
				// 5 located landmark points (left eye, right eye, nose, left and right 
				// corner of mouse).
				float* feats = new float[face_recognizer->feature_size()];
				std::wcout << "1.........." << std::endl;
				/* Extract feature: ExtractFeatureWithCrop */
				face_recognizer->ExtractFeatureWithCrop(src_img_data, points, feats);
				std::wcout << "2.........." << std::endl;
				/* Caculate similarity*/
				int curNum = 0, minNum = 0;
				float minSim = 1;
				for (; curNum < csvData.size(); curNum++)
				{
					float sim = face_recognizer->CalcSimilarity(allFeature + curNum * 2048, feats);
					std::cout << "Similarity : " << 1-sim << std::endl;
					if (sim < minSim && (1-sim) > 0.55) {
						minSim = sim;
						minNum = curNum;
					}
					//if (sim < 0.5)
					//	break;
				}
				std::wcout << "3.........." << std::endl;
				delete[] feats;
				//if (curNum == csvData.size())
				//	nameGet[j] = -1;
				//else
				//	nameGet[j] = curNum;
				mtx.lock();
				if (minSim == 1)
					nameGet[j] = -1;
				else
					nameGet[j] = minNum;
				mtx.unlock();
			}
			mtx.lock();
			threadFlags = false;
			lastNameGet = nameGet;
			hasLastRes = true;
			// ��ʹ��һ�α�־λ��õ�һ�����
			hasFirstRes = true;
			mtx.unlock();
		});
		run.detach();
	}
	try
	{

	
		if (hasLastRes)
		{
			if (threadFlags == false) {
				for (int i = 0; i < name.size(); ++i)
					if (nameGet[i] == -1)
						name[i] = "Unknown";
					else
						name[i] = csvData[nameGet[i]][0];
			}
			else {
				if (name.size() == lastNameGet.size()){
					for (int i = 0; i < name.size(); ++i)
						if (lastNameGet[i] == -1) 
							name[i] = "Unknown";
						else
							name[i] = csvData[lastNameGet[i]][0];
				}
				else {
					for (int i = 0; i < name.size(); ++i)
						name[i] = "Unknown";
				}
			}
		}
		else
		{
			for (int i = 0; i < name.size(); ++i)
				name[i] = "Unknown";
		}
	}
	catch (const std::exception&)
	{

	}
	return hasFirstRes;
	
	////seeta::FaceIdentification face_recognizer("./models/seeta_fr_v1.0.bin");
	////for (int i = 0; i < vectFace.size(); i++)
	////{
	////	cv::Mat thisMat = vectFace[i];
	//cv::Mat thisMat = vectFace;
	//	// ImageData store data of an image without memory alignment.
	//	seeta::ImageData thisImg(thisMat.cols, thisMat.rows, thisMat.channels());
	//	thisImg.data = thisMat.data;
	//	/* Extract feature */
	//	float* feats = new float[face_recognizer->feature_size()];
	//	seeta::FacialLandmark pt5[5];
	//	face_recognizer->ExtractFeatureWithCrop(thisImg, pt5, feats);
	//	//std::cout << "feats:";
	//	//for (int i = 0; i < 2048; i++)
	//	//{
	//	//	std::cout << *(feats + i) << " ";
	//	//}
	//	//std::cout << std::endl;
	//	//float sim = face_recognizer->CalcSimilarity(feats, feats);
	//	int curNum = 0;
	//	for (; curNum < csvData.size(); curNum++)
	//	{
	//		float sim = face_recognizer->CalcSimilarity(allFeature + curNum *2048, feats);
	//		if (sim > 0.8)
	//			break;
	//	}
	//	delete[] feats;
	////}
	////	return"";
	////QString tmp = "İ����";
	////res = tmp.toStdString();
	////resUtf = "İ����";
	////std::cout << strlen(res.c_str()) << ":" << strlen(resUtf.c_str()) << std::endl;
	//res = "İ����";	// ������GBK����ֻ��6λ
	//res = csvData[curNum][0];
	////std::cout << strlen(res.c_str()) << std::endl;
}

void Solution::extractFaceFeatureAndSave(cv::Mat inputFace, std::string &name)
{
	initialParam();
	std::vector<ObjInfo> vReListOne;	// ��������������ض��������Ϣ
	ssdFaceMaskDetect(inputFace, vReListOne);
	if (vReListOne.size() == 0) {
		std::cout << "detect no face" << std::endl;
	}
	else if (vReListOne.size() > 1) {
		std::cout << "detect more than One face" << std::endl;
	}
	else if (vReListOne.size() == 1)
	{
		cv::Mat grayFace;
		cv::cvtColor(inputFace, grayFace, CV_RGB2GRAY);
		IplImage *img_color, *img_grayscale;
		img_color = &IplImage(inputFace);
		img_grayscale = &IplImage(grayFace);
		int im_width = img_grayscale->width;
		int im_height = img_grayscale->height;
		unsigned char* data = new unsigned char[im_width * im_height];
		unsigned char* data_ptr = data;
		unsigned char* image_data_ptr = (unsigned char*)img_grayscale->imageData;
		for (int h = 0; h < im_height; h++) {
			memcpy(data_ptr, image_data_ptr, im_width);
			data_ptr += im_width;
			image_data_ptr += img_grayscale->widthStep;
		}
		seeta::ImageData image_data;
		image_data.data = data;
		image_data.width = im_width;
		image_data.height = im_height;
		image_data.num_channels = 1;
		seeta::Rect faceRect;
		faceRect.x = faceRect.y = 0;
		faceRect.width = im_width, faceRect.height = im_height;
		seeta::FaceInfo faceInfo;
		faceInfo.bbox = faceRect;
		faceInfo.roll = -9.25596e+61;
		faceInfo.pitch = -9.25596e+61;
		faceInfo.yaw = -9.25596e+61;
		faceInfo.score = 34.9409;

		// Detect 5 facial landmarks
		seeta::FacialLandmark points[5];
		point_detector->PointDetectLandmarks(image_data, faceInfo, points);

		// ImageData store data of an image without memory alignment.
		seeta::ImageData src_img_data(inputFace.cols, inputFace.rows, inputFace.channels());
		src_img_data.data = inputFace.data;
		// 5 located landmark points (left eye, right eye, nose, left and right 
		// corner of mouse).
		float* feats = new float[face_recognizer->feature_size()];

		/* Extract feature: ExtractFeatureWithCrop */
		face_recognizer->ExtractFeatureWithCrop(src_img_data, points, feats);

		/* ��������2048D*/
		std::ofstream pf;
		pf.open("feature.csv", std::ios::out);

		for (int i = 0; i < csvData.size(); ++i)
		{
			for (int j = 0; j < csvData[i].size() - 1; ++j)
			{
				pf << csvData[i][j] << ',';
			}
			pf << csvData[i].back() << std::endl;
		}

		pf << name << ','; // ����������
		for (int i = 0; i < 2047; ++i)
		{
			pf << *(feats + i) << ',';
		}
		pf << *(feats + 2047) << std::endl;
		std::string path = "./featureFace/";
		path.append(name).append(".png");
		cv::imwrite(path, inputFace);
		pf.close();
		delete[] feats;
	}
	//cv::Mat thisMat = inputFace;
	//// ImageData store data of an image without memory alignment.
	//seeta::ImageData thisImg(thisMat.cols, thisMat.rows, thisMat.channels());
	//thisImg.data = thisMat.data;
	///* Extract feature */
	//float* feats = new float[face_recognizer->feature_size()];
	//seeta::FacialLandmark pt5[5];
	//face_recognizer->ExtractFeatureWithCrop(thisImg, pt5, feats);
	//// ��������2048D
	//std::ofstream pf;
	//pf.open("feature.csv", std::ios::out);

	////pf << "name" << ',' << "age" << ',' << "hobby" << std::endl;
	////pf << "Mike" << ',' << 18 << ',' << "paiting" << std::endl;
	////pf << "Tom" << ',' << 25 << ',' << "football" << std::endl;
	////pf << "Jack" << ',' << 21 << ',' << "music" << std::endl;
	//for (int i = 0; i < csvData.size(); ++i)
	//{
	//	for (int j = 0; j < csvData[i].size() - 1; ++j)
	//	{
	//		pf << csvData[i][j] << ',';
	//	}
	//	pf << csvData[i].back() << std::endl;
	//}

	//pf << name << ','; // ����������
	//for (int i = 0; i < 2047; ++i)
	//{
	//	pf << *(feats + i) << ',';
	//}
	//pf << *(feats + 2047) << std::endl;
	//std::string path = "./featureFace/";
	//path.append(name).append(".png");
	//cv::imwrite(path, inputFace);
	//pf.close();
	//delete[] feats;
}

Solution::~Solution()
{
	delete face_recognizer;
	delete point_detector;
	delete allFeature;
}
