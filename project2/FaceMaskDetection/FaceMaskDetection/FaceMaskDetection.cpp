#include "FaceMaskDetection.h"

FaceMaskDetection::FaceMaskDetection(QWidget *parent)
	: QWidget(parent)
{
	// ����ʾ������
	this->setWindowFlags(Qt::FramelessWindowHint);

	ui.setupUi(this);

	// �������� ui.xxx->setxxx �ŵ� ui.setupUi(this) ǰ�ᱨ��
	// �˵���ʼ��
	menuInit();

	// ��timerForDataTime
	dateTimeInit();

	// ssd��ʼ��
	solve = new Solution();
	solve->initialParam();

	// ����Camera
	myCamera.camStart();

	//// �·���¼����4��
	//vectKnownRecord.resize(4);

	// chart
	//lastData.resize(maxChartAxisXSize, maxNotWornMaskNum / 2);	// ��ʼ���������
	lastData.resize(maxChartAxisXSize);
	for (int i = 0; i < maxChartAxisXSize; ++i)
	{
		lastData[i] = rand() % maxNotWornMaskNum;	// �����������ʼ��
	}
	countChartInit();
}

void FaceMaskDetection::menuInit()
{
	setUpMenu = new QMenu();

	// ��������¼�
	QAction *startAction = new QAction(setUpMenu);
	startAction->setIcon(QIcon("./icon/run.png"));
	startAction->setText(tr("  ����"));
	setUpMenu->addAction(startAction);
	// ��ӹر��¼�
	QAction *closeAction = new QAction(setUpMenu);
	closeAction->setIcon(QIcon("./icon/close.png"));
	closeAction->setText(tr("  �ر�"));
	setUpMenu->addAction(closeAction);
	// ���ע���¼�
	QAction *registerAction = new QAction(setUpMenu);
	registerAction->setIcon(QIcon("./icon/register.png"));
	registerAction->setText(tr("  ע��"));
	setUpMenu->addAction(registerAction);
	// �����Ϣ��ѯ�¼�
	QAction *infoQueryAction = new QAction(setUpMenu);
	infoQueryAction->setIcon(QIcon("./icon/query.png"));
	infoQueryAction->setText(tr("  ��Ϣ��ѯ"));
	setUpMenu->addAction(infoQueryAction);

	// ���ò˵�
	setUpMenu->setWindowFlags(setUpMenu->windowFlags() | Qt::FramelessWindowHint);
	setUpMenu->setAttribute(Qt::WA_TranslucentBackground);
	setUpMenu->setStyleSheet(" QMenu {border-radius:5px;font-family:'Arial';font-size:16px;}"
		" QMenu::item {height:28px; width:120px;padding-left:25px;border: 1px solid none;}"
		"QMenu::item:selected {background-color:rgb(0,120,215);\
							padding-left:25px;border: 1px solid rgb(65,173,255);}");
	ui.buttonMenu->setMenu(setUpMenu);

	// ��Ӧ����Action����
	connect(startAction, SIGNAL(triggered()), this, SLOT(onMenuActionRunTriggered()));
	connect(closeAction, SIGNAL(triggered()), this, SLOT(onMenuActionCloseTriggered()));
	connect(registerAction, SIGNAL(triggered()), this, SLOT(onMenuActionRegisterTriggered()));
	connect(infoQueryAction, SIGNAL(triggered()), this, SLOT(onMenuActionInfoQueryTriggered()));
}

void FaceMaskDetection::cameraShot()
{
	//clock_t t = clock();

	cv::Mat curImg;
	clonePic(curImg);

	QImage res(curImg.data, curImg.cols, curImg.rows, curImg.step, QImage::Format_RGB888);
	ui.labelVideoStream->setPixmap(QPixmap::fromImage(res).scaled(ui.labelVideoStream->width(), ui.labelVideoStream->height(), Qt::KeepAspectRatio));

	//std::cout << ((double)clock() - t) / CLOCKS_PER_SEC << std::endl;
}

void FaceMaskDetection::clonePic(cv::Mat &dst)
{
	try
	{

		cv::Mat imgGet;
		//myCamera.frame.copyTo(dst);
		myCamera.getImage(imgGet);
		//cv::flip(dst, dst, 1);	// ��ת(����Camera��Ҫ)
		dst = imgGet.clone();

		
		//cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);	// ��ɫת��(����Camera��Ҫ)
		maskDetectCount = 0;
		if (maskDetectCount > 0)	// maskDetectCount * 10ms����һ��mask detect
		{
			maskDetectCount--;

			for (int i = 0; i < lastMaskDetectList.size(); ++i)
			{
				cv::Size sizeName = cv::getTextSize(lastNameVect[i], cv::FONT_HERSHEY_SIMPLEX, 1, 1, 0);
				cv::Rect rectName = cv::Rect(lastMaskDetectList[i]._faceLoc);
				rectName.y -= sizeName.height;
				rectName.height = sizeName.height;
				//rectName.width = sizeName.width;
				
				

				if (lastMaskDetectList[i]._faceCla == 0) {
					rectangle(dst, rectName, cv::Scalar(0, 0, 255), cv::FILLED);
					rectangle(dst, cv::Rect(lastMaskDetectList[i]._faceLoc), cv::Scalar(0, 0, 255), 2);
				}
				else {
					rectangle(dst, rectName, cv::Scalar(255, 0, 0), cv::FILLED);
					rectangle(dst, cv::Rect(lastMaskDetectList[i]._faceLoc), cv::Scalar(255, 0, 0), 2);
				}
				putTextZH(dst, lastNameVectGBK[i].c_str(), cv::Point(rectName.x, rectName.y), cv::Scalar(255, 255, 255), 20, "Arial");
			}
		}
		else
		{
			

			std::vector<ObjInfo> vReList;	// ��������������ض��������Ϣ
			//std::thread t_detect(&Solution::ssdFaceMaskDetect, std::bind(std::ref(dst), std::ref(vReList)), &solve);
			//std::thread t_detect(&Solution::ssdFaceMaskDetect, &dst, &vReList);
			//t_detect.join();
			solve->ssdFaceMaskDetect(dst, vReList);	// 1�������Ļ�vReList.size() == 1;
			cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);	// ��ɫת��(����Camera��Ҫ)
			cv::Mat dstCopy = dst.clone();
			lastMaskDetectList = vReList;

			int curSize = vReList.size();

			std::vector<QImage> headImg(curSize);
			std::vector<std::string> name(curSize, "");
			std::vector<std::string> nameGBK(curSize, "");
			std::vector<bool> status(curSize, 0);

			bool firstRes = solve->extractFaceFeature(dst, vReList, nameGBK);

			for (int i = 0; i < curSize; ++i)
			{
				//// �����ȶԲ���(TODO)
				////solve->readFeatureCSV();	// ��ȡ����������Ϣ
				//cv::Mat face = dst(cv::Rect(vReList[i]._faceLoc));
				//solve->extractFaceFeature(face, nameGBK[i]);
				//// ����ͨ��QStringת��һ�±�������
				QString str = QString::fromLocal8Bit(nameGBK[i].c_str());	
				//if (nameGBK.size() == 2)
				//{
				//	nameGBK[0] = "zgh";
				//	nameGBK[1] = "ljb";
				//}
				name[i]= str.toStdString();
				

				cv::Rect rect, resizeRect;
				rect = cv::Rect(vReList[i]._faceLoc);
				rectCenterScale(rect, resizeRect, reScale);	// �������귶Χ

				cv::Mat cut = dstCopy(resizeRect);	
				headImg[i] = QImage(cut.data, cut.cols, cut.rows, cut.step, QImage::Format_RGB888);

				cv::Size sizeName = cv::getTextSize(name[i], cv::FONT_HERSHEY_SIMPLEX, 1, 1, 0);
				cv::Rect rectName = cv::Rect(vReList[i]._faceLoc);
				rectName.y -= sizeName.height;
				rectName.height = sizeName.height;
				//rectName.width = sizeName.width;
				
				

				if (vReList[i]._faceCla == 0) {
					rectangle(dst, rectName, cv::Scalar(0, 0, 255), cv::FILLED);
					rectangle(dst, cv::Rect(vReList[i]._faceLoc), cv::Scalar(0, 0, 255), 2);
					status[i] = 1;
				}
				else {
					rectangle(dst, rectName, cv::Scalar(255, 0, 0), cv::FILLED);
					rectangle(dst, cv::Rect(vReList[i]._faceLoc), cv::Scalar(255, 0, 0), 2);
					status[i] = 0;
				}
				putTextZH(dst, nameGBK[i].c_str(), cv::Point(rectName.x, rectName.y), cv::Scalar(255, 255, 255), 20, "Arial");
			
			}
			lastNameVectGBK.clear();
			nameGBK.swap(lastNameVectGBK);

			//if (firstRes)
			updateCount--;
			if (updateCount == 0) {
				updateDisplay(headImg, name, status, firstRes);	// ������ʾ
				updateCount = 20;
			}
		

			maskDetectCount = 5;	// ����ԭ����
		}
	}
	catch (const std::exception&e)
	{
		std::cout << e.what() << std::endl;
	}
}

void FaceMaskDetection::updateDisplay(std::vector<QImage> &headImg, std::vector<std::string> &name, std::vector<bool> &status, bool startUpdate)
{

	QDateTime dt = QDateTime::currentDateTime();
	QString dtStr = dt.toString("MM-dd hh:mm:ss");

	bool isNeedUpdate = false;
	// ����ͨ�������ж��Ƿ�Ҫˢ��
	if (startUpdate) {
		if (name.size() != lastNameVect.size()) isNeedUpdate = true;
		else
		{
			int i = 0;
			for (; i < name.size(); ++i) {
				if (name[i] != lastNameVect[i])
					break;
			}
			if (i == name.size()) { 
				isNeedUpdate = false; 
			}
			else isNeedUpdate = true;
			
		
		}
	//else isNeedUpdate = false;
	}
	else
	{
		lastNameVect.clear();
		lastStatusVect.clear();
		name.swap(lastNameVect);
		status.swap(lastStatusVect);
	}
	// Ȼ��ͨ��״̬�ж��Ƿ�Ҫˢ��
	if (isNeedUpdate == false)
	{
		int i = 0;
		for (; i < status.size(); ++i) {
			if (status[i] != lastStatusVect[i])
				break;
		}
		if (i == status.size()) {
			isNeedUpdate = false;
		}
		else isNeedUpdate = true;
	}
	//isNeedUpdate = false;
	std::cout << "startUpdate : "<< startUpdate <<"isNeedUpdate : " << isNeedUpdate << std::endl;
	if (isNeedUpdate == false)
	{
		// 
		//lastNameVect.clear();
		//lastStatusVect.clear();
		//name.swap(lastNameVect);
		//status.swap(lastStatusVect);
	}
	else
	{
		
		try
		{

		
		for (int i = 0; i < status.size(); i++)
		{
			if (name[i] == "Unknown")
			{
				if (status[i] == 0)	// δ�������
				{
					// Ԥ��
					//QString isOnMask = "δ�������";
					QListWidgetItem *list = new QListWidgetItem();
					QString perItem = QString("%1      %2      %3").arg(dtStr).arg("Unknown").arg("δ�������");
					// (TODO)������Ϣ
					list->setText(perItem);
					ui.listWidgetLog->addItem(list);

					// δ�����ֱ���
					ui.labelUnknownImage->setPixmap(QPixmap::fromImage(headImg[i]).scaled(ui.labelUnknownImage->width(), ui.labelUnknownImage->height(), Qt::KeepAspectRatio));
					QString dtStrUnknown = dt.toString("yyyy-MM-dd hh:mm:ss");
					ui.labelUnknownTimeDis->setText(dtStrUnknown);
					ui.labelUnknownMaskDis->setText("��");

					notWornMaskNum++;
				}
				else  // �������
				{

				}
			}
			else	// ��İ����
			{
				// �·���¼��ʾ
				//updateKnownRecord(headImg[i], name[i], status[i]);	// �����·���¼��ʾ
				if (status[i] == 0)	// δ�������
				{
					updateKnownRecord(headImg[i], name[i], status[i]);
					// Ԥ��
					//QString isOnMask = "δ�������";
					QListWidgetItem *list = new QListWidgetItem();
					QString perItem = QString("%1      %2      %3").arg(dtStr).arg(QString::fromStdString(name[i])).arg("δ�������");
					// (TODO)������Ϣ
					list->setText(perItem);
					ui.listWidgetLog->addItem(list);

					notWornMaskNum++;

				}
				else  // �������
				{

				}
			}

		}
		// ����Ԥ����ϢItem������ʾ����
		if (ui.listWidgetLog->count() > logItemNum)
		{
			if (ui.listWidgetLog->count() == logItemNum + 1) {
				QListWidgetItem *tmp = ui.listWidgetLog->takeItem(0);
				delete tmp;
			}
			else {
				for (int i = ui.listWidgetLog->count() - logItemNum; i > 0; --i)
				{
					QListWidgetItem *tmp = ui.listWidgetLog->takeItem(i);
					delete tmp;
				}
			}
		}

		updateDataInDataBase(headImg, name, status);	// �������ݿ���Ϣ

		lastNameVect.clear();
		lastStatusVect.clear();
		name.swap(lastNameVect);
		status.swap(lastStatusVect);
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

}

void FaceMaskDetection::updateKnownRecord(QImage &curImg, const std::string curName, const bool curStatus)
{
	KnownRecord kr(curImg, curName, curStatus);

	if (curKnownRecordSize > 4)	// ��4��
	{
		if (vectKnownRecord[3].curName == kr.curName)
			return;
		for (int i = 0; i < 3; ++i)
		{
			vectKnownRecord[i] = vectKnownRecord[i + 1];
		}
		vectKnownRecord[3] = kr;

		ui.labelImage_1->setPixmap(QPixmap::fromImage(vectKnownRecord[0].curImg).scaled(ui.labelImage_1->width(), ui.labelImage_1->height(), Qt::KeepAspectRatio));
		ui.labelNameDis_1->setText(vectKnownRecord[0].curName);
		ui.labelMaskDis_1->setText(vectKnownRecord[0].curStatus);

		ui.labelImage_2->setPixmap(QPixmap::fromImage(vectKnownRecord[1].curImg).scaled(ui.labelImage_2->width(), ui.labelImage_2->height(), Qt::KeepAspectRatio));
		ui.labelNameDis_2->setText(vectKnownRecord[1].curName);
		ui.labelMaskDis_2->setText(vectKnownRecord[1].curStatus);

		ui.labelImage_3->setPixmap(QPixmap::fromImage(vectKnownRecord[2].curImg).scaled(ui.labelImage_3->width(), ui.labelImage_3->height(), Qt::KeepAspectRatio));
		ui.labelNameDis_3->setText(vectKnownRecord[2].curName);
		ui.labelMaskDis_3->setText(vectKnownRecord[2].curStatus);

		ui.labelImage_4->setPixmap(QPixmap::fromImage(vectKnownRecord[3].curImg).scaled(ui.labelImage_4->width(), ui.labelImage_4->height(), Qt::KeepAspectRatio));
		ui.labelNameDis_4->setText(vectKnownRecord[3].curName);
		ui.labelMaskDis_4->setText(vectKnownRecord[3].curStatus);
	}
	else	// С��4��
	{
		if (vectKnownRecord.size() > 0)
		{
			if (vectKnownRecord.back().curName == kr.curName)
				return;
		}
		switch (vectKnownRecord.size())
		{
		case 0:
		{
			ui.labelImage_1->setPixmap(QPixmap::fromImage(kr.curImg).scaled(ui.labelImage_1->width(), ui.labelImage_1->height(), Qt::KeepAspectRatio));
			ui.labelNameDis_1->setText(kr.curName);
			ui.labelMaskDis_1->setText(kr.curStatus);
			break;
		}
		case 1:
		{
			ui.labelImage_2->setPixmap(QPixmap::fromImage(kr.curImg).scaled(ui.labelImage_2->width(), ui.labelImage_2->height(), Qt::KeepAspectRatio));
			ui.labelNameDis_2->setText(kr.curName);
			ui.labelMaskDis_2->setText(kr.curStatus);
			break;
		}
		case 2:
		{
			ui.labelImage_3->setPixmap(QPixmap::fromImage(kr.curImg).scaled(ui.labelImage_3->width(), ui.labelImage_3->height(), Qt::KeepAspectRatio));
			ui.labelNameDis_3->setText(kr.curName);
			ui.labelMaskDis_3->setText(kr.curStatus);
			break;
		}
		case 3:
		{
			ui.labelImage_4->setPixmap(QPixmap::fromImage(kr.curImg).scaled(ui.labelImage_4->width(), ui.labelImage_4->height(), Qt::KeepAspectRatio));
			ui.labelNameDis_4->setText(kr.curName);
			ui.labelMaskDis_4->setText(kr.curStatus);
			break;
		}
		default:
			break;
		}
		vectKnownRecord.push_back(kr);
		curKnownRecordSize++;
	}
}

void FaceMaskDetection::updateDataInDataBase(std::vector<QImage> &headImg, std::vector<std::string> &name, std::vector<bool> &status)
{
	std::vector<std::vector<std::string>> toSaveVect;
	toSaveVect.resize(name.size());
	for (int i = 0; i < toSaveVect.size(); ++i)
	{
		toSaveVect[i].resize(4);
	}
	std::string timeStr = dateTime.toString("yyyy-MM-dd hh:mm:ss").toStdString();
	std::string pathImg = "./image/";
	pathImg.append(dateTime.toString("yyyyMMdd_hhmmss").toStdString()).append("_");
	for (int i = 0; i < toSaveVect.size(); ++i)
	{
		toSaveVect[i][0] = timeStr;
		toSaveVect[i][1] = name[i];
		toSaveVect[i][2] = status[i] == true ? "1" : "0";
		toSaveVect[i][3] = pathImg.append(std::to_string(i));
	}
	// �����̴߳洢ͼƬ(����������治�ϸ��µĻ�Ӧ����������ģ�����ô�����ˣ���������ô���߳�)
	//auto t_writeImage = [=] {
		//isOnSavingImg = true;
		for (int i = 0; i < toSaveVect.size(); ++i)
		{
			headImg[i].save(QString::fromStdString(toSaveVect[i][3]), "JPG");	// ���ﱣ���ͼ���û�ͼ���Դ�
		}
		// �������ݿ��
		sql_conn.insertTableLog(toSaveVect);
	//};
	//std::thread cthread(t_writeImage);
	////cthread.detach();
	//cthread.join();	// detach���е����⣬��ʱ������
}

// �˵���������
void FaceMaskDetection::onMenuActionRunTriggered()
{

	if (timerForCamLive.isActive() == false) 
	{
		timerForCamLive.start(20);
		connect(&timerForCamLive, SIGNAL(timeout()), this, SLOT(cameraShot()));
	}
}

// �˵��رն���
void FaceMaskDetection::onMenuActionCloseTriggered()
{

	if (timerForCamLive.isActive() == true)
	{
		timerForCamLive.stop();
		disconnect(&timerForCamLive, SIGNAL(timeout()), this, SLOT(cameraShot()));
		ui.labelVideoStream->clear();
		//myCamera.camStop();
	}
	else 
	{
		this->close();
	}
}

// �˵�ע�ᶯ��
void FaceMaskDetection::onMenuActionRegisterTriggered()
{
	// ע��ʱ�ȰѶ�ʱ���ص�
	if (timerForCamLive.isActive() == true)
	{
		timerForCamLive.stop();
		disconnect(&timerForCamLive, SIGNAL(timeout()), this, SLOT(cameraShot()));
		ui.labelVideoStream->clear();
	}
	//// �ر�cam
	//myCamera.camStop();
	
	RegisterFace *registerFace = new RegisterFace(&myCamera);
	// �����Ӵ������ԣ����Ӵ��ڹر�֮���ͷ��Ӵ��ڵ���Դ(�ͷ�ָ��)
	registerFace->setAttribute(Qt::WA_DeleteOnClose);
	registerFace->show();
}

// �˵���Ϣ��ѯ����
void FaceMaskDetection::onMenuActionInfoQueryTriggered()
{
	InfoQuery *infoQuery = new InfoQuery();
	// �����Ӵ������ԣ����Ӵ��ڹر�֮���ͷ��Ӵ��ڵ���Դ(�ͷ�ָ��)
	infoQuery->setAttribute(Qt::WA_DeleteOnClose);
	infoQuery->show();
}

// chart init
void FaceMaskDetection::countChartInit()
{	
	m_chart = new QChart();
	m_series = new QLineSeries(m_chart);

	qint64 curTimeTotalMSecond = QDateTime::currentMSecsSinceEpoch();	// ��ȡ����Ϊֹ��ms��
	// ���㵱���Ѿ���ms��
	qint64 todayTimeTotalMSecond = QTime::currentTime().hour() * 3600 * 1000 + QTime::currentTime().minute() * 60 * 1000
		+ QTime::currentTime().second() * 1000 + QTime::currentTime().msec();
	// ���maxChartAxisXSize���㵽������
	for (int i = maxChartAxisXSize; i > 0; --i)
	{
		m_series->append(curTimeTotalMSecond - 86400000 * i - todayTimeTotalMSecond, lastData[maxChartAxisXSize - i]);
	}
	//m_series->append(curTimeTotalMSecond - 86400000 * 5 - todayTimeTotalMSecond
	//	, 0);
	//m_series->append(curTimeTotalMSecond - 86400000 * 4 - todayTimeTotalMSecond
	//	, maxNotWornMaskNum / 2);
	//m_series->append(curTimeTotalMSecond - 86400000 * 3 - todayTimeTotalMSecond
	//	, 0);
	//m_series->append(curTimeTotalMSecond - 86400000 * 2 - todayTimeTotalMSecond
	//	, maxNotWornMaskNum / 2);
	//m_series->append(curTimeTotalMSecond - 86400000 - todayTimeTotalMSecond
	//	, 0);

	//m_series->setName("test");
	//m_series->setVisible(true);
	//m_series->setPointLabelsFormat("(@xPoint,@yPoint)");
	//m_series->setPointLabelsVisible(true);
	m_chart->addSeries(m_series);
	m_series->setUseOpenGL(true);	// ����

	m_chart->setTheme(QChart::ChartThemeDark);	// ��������

	// ʱ��X��
	QDateTimeAxis *axisX = new QDateTimeAxis();
	axisX->setFormat("MM-dd");	// ���ø�ʽ
	QDateTime curDateInit = QDateTime::currentDateTime();	// ��ȡ��ǰʱ��
	axisX->setMin(curDateInit.addDays(-maxChartAxisXSize).addMSecs(-todayTimeTotalMSecond));	// ����ʱ��X����Сֵ
	axisX->setMax(curDateInit.addDays(-1).addMSecs(-todayTimeTotalMSecond));	// ����ʱ��X�����ֵ
	axisX->setTickCount(maxChartAxisXSize);	// ���ü���
	// ֵY��
	QValueAxis *axisY = new QValueAxis();
	axisY->setLabelFormat("%d");
	axisY->setRange(0, maxNotWornMaskNum);	// ���÷�Χ
	axisY->setTickCount(maxChartAxisXSize);	// ���ü���
	axisY->setGridLineVisible(false);	// ȡ��y��������

	m_chart->setAxisX(axisX, m_series);
	m_chart->setAxisY(axisY, m_series);
	m_chart->legend()->hide();	// ����ͼ��

	ui.widgetChart->setChart(m_chart);
}

//// ���ݸ���
//void FaceMaskDetection::updateCountChart()
//{
//
//}

// ����ʱ���ʼ��
void FaceMaskDetection::dateTimeInit()
{
	if (timerForDataTime.isActive() == false) {
		timerForDataTime.start(1000);	// 1s
		connect(&timerForDataTime, SIGNAL(timeout()), this, SLOT(onUpdateDateTime()));
	}
}

// ����ʱ����ʾ����
void FaceMaskDetection::onUpdateDateTime()
{
	dateTime = QDateTime::currentDateTime();
	dataTimeStr = dateTime.toString("yyyy-MM-dd hh:mm:ss ddd");
	ui.labelTime->setText(dataTimeStr);

	if (dateTime.time().toString("hh:mm:ss") == "00:00:01")
	{
		qint64 curTimeTotalMSecond = QDateTime::currentMSecsSinceEpoch();
		//// forTest
		//qint64 todatTimeTotalMSecond = QTime::currentTime().hour() * 3600 * 1000 + QTime::currentTime().minute() * 60 * 1000
		//	+ QTime::currentTime().second() * 1000 + QTime::currentTime().msec();
		qint64 todatTimeTotalMSecond = 1 * 1000;
		// ����X��
		//// forTest
		//m_chart->axisX()->setMin(QDateTime::currentDateTime().addDays(-maxChartAxisXSize + 1).addMSecs(-todatTimeTotalMSecond));
		//m_chart->axisX()->setMax(QDateTime::currentDateTime().addDays(0).addMSecs(-todatTimeTotalMSecond));
		m_chart->axisX()->setMin(QDateTime::currentDateTime().addDays(-maxChartAxisXSize).addMSecs(-todatTimeTotalMSecond));
		m_chart->axisX()->setMax(QDateTime::currentDateTime().addDays(-1).addMSecs(-todatTimeTotalMSecond));
		// ��������
		//// forTest
		//m_series->removePoints(0, 1);
		//m_series->append(curTimeTotalMSecond - todatTimeTotalMSecond, notWornMaskNum);
		m_series->removePoints(0, 1);
		m_series->append(curTimeTotalMSecond - todatTimeTotalMSecond - 86400000, notWornMaskNum);
		// (TODO)
		// ����Ҫ��Ҫ�����ݸ��µ�lastData�У�
	}
}

void FaceMaskDetection::rectCenterScale(cv::Rect &rect, cv::Rect &resizeRect, const double scale)
{
	if (scale < 1) return;	// ��ǰֻ����Ŵ�
	double tmp = (scale - 1) / 2.0;
	resizeRect = rect;

	resizeRect.x -= resizeRect.width * tmp;
	resizeRect.y -= resizeRect.height * tmp;
	resizeRect.width *= scale;
	resizeRect.height *= scale;

	if (resizeRect.x < 0) resizeRect.x = 0;
	if (resizeRect.y < 0) resizeRect.y = 0;
	if (resizeRect.x + resizeRect.width > CameraSize.width) resizeRect.width = CameraSize.width - resizeRect.x;
	if (resizeRect.y + resizeRect.height > CameraSize.height) resizeRect.height = CameraSize.height - resizeRect.y;
}

FaceMaskDetection::~FaceMaskDetection()
{
	// ɾ���˵�
	delete setUpMenu;
	//// ɾ��myCamera
	//delete myCamera;
	// �ر�timerForCamLive
	if (timerForCamLive.isActive() == true)
		timerForCamLive.stop();
	// �ر�Camera
	myCamera.camStop();
	// �ر�timerForDataTime
	if (timerForDataTime.isActive() == true)
		timerForDataTime.stop();
	// ɾ��solve
	delete solve;
	// ɾ��chart
	delete m_chart;
	// ɾ��series
	delete m_series;
}
