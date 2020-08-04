#include "RegisterFace.h"

RegisterFace::RegisterFace(Camera *cam, QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setWindowModality(Qt::ApplicationModal);

	// ����ʾ������
	this->setWindowFlags(Qt::FramelessWindowHint);

	myCameraRegister = new Camera();
	// ����Camera
	//myCameraRegister->camStart();
	myCameraRegister = cam;

	// button close
	connect(ui.buttonClose, SIGNAL(clicked()), this, SLOT(onButtonCloseClicked()));

	// button start record
	connect(ui.buttonStartRecord, SIGNAL(clicked()), this, SLOT(onButtonStartRecordClicked()));

	// ����������������
	faceArea.x = CameraSize.width / 4;
	faceArea.y = CameraSize.height / 4;
	faceArea.width = CameraSize.width / 2;
	faceArea.height = CameraSize.height / 2;
}

// button start record
void RegisterFace::onButtonStartRecordClicked()
{
	// ��ʼ¼����Ϣ
	if (ui.lineEditName->text().isEmpty())	// ����ȷ���Ѿ���������
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText("<font color='gray'>Please Input Your Name First!!!</font>");
		//msgBox.setStyleSheet("background:#020d2c");
		msgBox.setStyleSheet("QPushButton { background:#162e6c;color:gray; }");
		msgBox.exec();	// ������ʾ
	}
	else
	{
		toRegisterUserName = ui.lineEditName->text().toStdString();	// ��ȡ��ע���û�����
		timerForRegister = new QTimer();	// 
		if (timerForRegister->isActive() == false)
		{
			timerForRegister->start(10);
			connect(timerForRegister, SIGNAL(timeout()), this, SLOT(camShot()));
		}
	}
}

void RegisterFace::camShot()
{
	if (faceImportCnt > 0) {
		faceImportCnt--;
		if (faceImportCnt % 100 == 1) std::cout << "Record face info will be started in " 
			<< faceImportCnt / 100 << " seconds" << std::endl;
	}
	cv::Mat curImg;
	clonePic(curImg);
	cv::rectangle(curImg, cv::Rect(faceArea), cv::Scalar(0, 255, 0), 2);
	if (!faceImportCnt) {	// ����
		faceImportCnt = -1;
		Solution *s;
		s = new Solution();
		cv::Mat cut = curImg(cv::Rect(faceArea));
		s->extractFaceFeatureAndSave(cut, toRegisterUserName);
		std::cout << "Record face info has finished!!" << std::endl;
		QMessageBox::about(this, "Tips", "<font color='gray'>Record face info has finished!!. Press OK To close.");
	}
	QImage res(curImg.data, curImg.cols, curImg.rows, curImg.step, QImage::Format_RGB888);
	ui.labelVideoStreamRegister->setPixmap(QPixmap::fromImage(res).scaled(ui.labelVideoStreamRegister->width(), ui.labelVideoStreamRegister->height(), Qt::KeepAspectRatio));
}

void RegisterFace::clonePic(cv::Mat &dst)
{
	cv::Mat dstImgGet;
	//myCameraRegister->frame.copyTo(dst);
	myCameraRegister->getImage(dstImgGet);
	//cv::flip(dst, dst, 1);	// ��ת(����Camera��Ҫ)
	dst = dstImgGet.clone();
	cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);	// ��ɫת��(����Camera��Ҫ)
}

// button close
void RegisterFace::onButtonCloseClicked()
{
	myCameraRegister->camStop();
	
	if (timerForRegister->isActive() == true)
		timerForRegister->stop();
	

	this->close();
}

RegisterFace::~RegisterFace()
{
	//delete myCameraRegister;	// ɾ���������⣿
	delete timerForRegister;
}