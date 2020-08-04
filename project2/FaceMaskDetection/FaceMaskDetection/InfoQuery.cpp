#include "InfoQuery.h"

InfoQuery::InfoQuery(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setWindowModality(Qt::ApplicationModal);

	// ����ʾ������
	this->setWindowFlags(Qt::FramelessWindowHint);

	// button close
	connect(ui.buttonClose, SIGNAL(clicked()), this, SLOT(onButtonCloseClicked()));

	// ��ȡע���û���Ϣ
	queryHasRegisteredUsers();
	// ����perColumnDis��
	for (int i = 0; i < perColumnDis; ++i)
	{
		ui.gridLayoutRg->setColumnStretch(i, 1);
	}
	// ��ʾע���û���Ϣ
	hasRegisteredUsersDisplay();

	// ��ȡ������
	recordList = sql_conn.tab2Vector("log", 5);	// ���ȶ�ȡ���м�¼

	// ��ʾ���м�¼
	hasRecordListDisplay();
	// connect tab changed
	//connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onRecordListDisplay(int)));
}

void InfoQuery::queryHasRegisteredUsers()
{
	std::string pattern_jpg;
	std::vector<cv::String> image_files;
	pattern_jpg = "./featureFace/*.png";
	cv::glob(pattern_jpg, image_files);
	// ��ʼ����С
	hasRegisteredImage.resize(image_files.size());
	hasRegisteredName.resize(image_files.size());
	// ��ȡͼ���Լ�����
	for (int ii = 0; ii<image_files.size(); ii++)
	{
		// 14Ϊǰ���ļ����ַ������ȣ���������Ϣ�����￪ʼ
		int nameLength = image_files[ii].find(".png") - 14;
		//std::cout << image_files[ii].substr(14, nameLength) << std::endl;
		hasRegisteredName[ii] = image_files[ii].substr(14, nameLength);
		hasRegisteredImage[ii] = imread(image_files[ii]);
	}
}

void InfoQuery::hasRegisteredUsersDisplay()
{
	int sizeAll = hasRegisteredName.size() / perColumnDis + 1;
	ui.scrollAreaWidgetContentsRg->setMinimumHeight(sizeAll * perDisH + 100);
	for (int i = 0; i < hasRegisteredName.size(); ++i)
	{
		QLabel *labelWidget = new QLabel();
		labelWidget->setFixedSize(perDisW + 60, perDisH + 10);
		labelWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		labelWidget->setFrameShape(QFrame::WinPanel);
		QGridLayout *gridLayoutLabelWidget = new QGridLayout(labelWidget);
		QLabel *labelImg = new QLabel();
		QImage thisImg(hasRegisteredImage[i].data, hasRegisteredImage[i].cols, hasRegisteredImage[i].rows, hasRegisteredImage[i].step, QImage::Format_RGB888);
		labelImg->setPixmap(QPixmap::fromImage(thisImg).scaled(perDisW, perDisH, Qt::KeepAspectRatio));
		QLabel *labelName = new QLabel();
		labelName->setText(QString::fromStdString(hasRegisteredName[i]));
		gridLayoutLabelWidget->addWidget(labelImg, 0, 0);
		gridLayoutLabelWidget->addWidget(labelName, 0, 1);
		ui.gridLayoutRg->addWidget(labelWidget);
	}
}

void InfoQuery::hasRecordListDisplay()
{
	int sizeAll = recordList.size() + 1;
	ui.scrollAreaWidgetContentsRd->setMinimumHeight(sizeAll * perDisRecordH + 100);
	for (int i = 0; i < recordList.size(); ++i)
	{
		QLabel *labelWidget = new QLabel();
		labelWidget->setFixedSize(perDisRecordW + 60, perDisRecordH + 10);
		labelWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		labelWidget->setFrameShape(QFrame::WinPanel);
		QGridLayout *gridLayoutLabelWidget = new QGridLayout(labelWidget);
		QLabel *labelImg = new QLabel();
		labelImg->setFixedSize((perDisRecordW + 60) / 4, perDisRecordH);
		labelImg->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		cv::Mat tmpImg = cv::imread(recordList[i][4]);
		cv::cvtColor(tmpImg, tmpImg, CV_RGB2BGR);
		QImage thisImg(tmpImg.data, tmpImg.cols, tmpImg.rows, tmpImg.step, QImage::Format_RGB888);
		labelImg->setPixmap(QPixmap::fromImage(thisImg).scaled(labelImg->width(), labelImg->height(), Qt::KeepAspectRatio));

		QLabel *labelNameText = new QLabel();
		labelNameText->setText("���� : ");
		QLabel *labelName = new QLabel();
		labelName->setText(QString::fromStdString(recordList[i][2]));
		QLabel *labelTimeText = new QLabel();
		labelTimeText->setText("ʱ�� : ");
		QLabel *labelTime = new QLabel();
		labelTime->setText(QString::fromStdString(recordList[i][1]));
		QLabel *labelStatusText = new QLabel();
		labelStatusText->setText("״̬ : ");
		QLabel *labelStatus = new QLabel();
		QString tmpStatus = recordList[i][3] == "0" ? "δ�������" : "���������";
		labelStatus->setText(tmpStatus);
		
		gridLayoutLabelWidget->addWidget(labelImg, 0, 0);
		gridLayoutLabelWidget->addWidget(labelNameText, 0, 1);
		gridLayoutLabelWidget->addWidget(labelName, 0, 2);
		gridLayoutLabelWidget->addWidget(labelTimeText, 0, 3);
		gridLayoutLabelWidget->addWidget(labelTime, 0, 4);
		gridLayoutLabelWidget->addWidget(labelStatusText, 1, 1);
		gridLayoutLabelWidget->addWidget(labelStatus, 1, 2);
		ui.gridLayoutRd->addWidget(labelWidget);
	}
}

//void InfoQuery::onRecordListDisplay(int idx)
//{
//	std::cout << idx << std::endl;
//	
//
//}

void InfoQuery::onButtonCloseClicked()
{
	this->close();
}

InfoQuery::~InfoQuery()
{
	
}