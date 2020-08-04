#pragma once

#include <QtCharts>
QT_CHARTS_USE_NAMESPACE
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QtWidgets/QWidget>
#include <QPixmap>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QImage>
#include <QTimer>
#include <QDateTime>
#include <QImage>


#include "ui_FaceMaskDetection.h"

#include "RegisterFace.h"
#include "InfoQuery.h"
#include "Solution.h"
#include "MySQLConnection.h"


#include "putText.h"

#include <thread>
#include <time.h>

class FaceMaskDetection : public QWidget
{
    Q_OBJECT

public:
    FaceMaskDetection(QWidget *parent = Q_NULLPTR);
	~FaceMaskDetection();

private:
    Ui::FaceMaskDetectionClass ui;

	// log
	int logItemNum = 12;

	// Camera
	Camera myCamera;	
	void clonePic(cv::Mat &dst);
	QTimer timerForCamLive;

	// �˵���ʼ��
	QMenu *setUpMenu;
	void menuInit();

	// data time
	void dateTimeInit();
	QDateTime dateTime;
	QString dataTimeStr;
	QTimer timerForDataTime;

	// solution
	Solution *solve;
	std::vector<ObjInfo> lastMaskDetectList;
	int maskDetectCount = 0;
	int updateCount = 20;

	// mysql
	//bool isOnSavingImg = false;
	MySQLConnection sql_conn;	// 
	void updateDataInDataBase(std::vector<QImage> &headImg, std::vector<std::string> &name, std::vector<bool> &status);

	// ������ʾ
	std::vector<bool> lastStatusVect;
	std::vector<std::string> lastNameVect;
	std::vector<std::string> lastNameVectGBK;
	void updateDisplay(std::vector<QImage> &headImg, std::vector<std::string> &name, std::vector<bool> &status, bool startUpdate);
	int curKnownRecordSize = 0;
	struct KnownRecord
	{
		QImage curImg;
		QString curName;
		QString curStatus;
		KnownRecord() {}
		KnownRecord(QImage Img, std::string Name, bool Status){
			this->curImg = Img;
			this->curName = QString::fromStdString(Name);
			this->curStatus = Status == 0 ? "��" : "��";
		}
	};
	std::vector<KnownRecord> vectKnownRecord;
	void updateKnownRecord(QImage &curImg, const std::string curName, const bool curStatus);

	// �޸�����
	cv::Size CameraSize = cv::Size(640, 480);	// ��������ͷ����
	//cv::Size CameraSize = cv::Size(3840, 2160); // ��������
	//cv::Size CameraSize = cv::Size(1280, 720); // ��������
	double reScale = 2.5;
	void rectCenterScale(cv::Rect &rect, cv::Rect &resizeRect, const double scale);

	// chart
	int notWornMaskNum = 0;
	QChart *m_chart;
	QLineSeries  *m_series;	// ����
	int maxChartAxisXSize = 5;	// ��ʾ��ȥ5���ͳ������
	int maxNotWornMaskNum = 5000;
	std::vector<int> lastData;
	void countChartInit();	// ��ʼ�������ʾ
	//void updateCountChart();	// ���¼���

private slots:
	void onMenuActionCloseTriggered();
	void onMenuActionRunTriggered();

	void cameraShot();

	void onUpdateDateTime();

	void onMenuActionRegisterTriggered();

	void onMenuActionInfoQueryTriggered();
};
