#pragma once

#include "stdafx.h"
#include "ui_InfoQuery.h"
#include "MySQLConnection.h"

#include <QLabel>
#include <QSizePolicy>

class InfoQuery : public QWidget
{
	Q_OBJECT

public:
	InfoQuery(QWidget *parent = Q_NULLPTR);
	~InfoQuery();

private:
	Ui::InfoQueryClass ui;

	// ��ѯ��ע���û�������
	int perColumnDis = 3;
	int perDisW = 180, perDisH = 160;
	std::vector<cv::Mat> hasRegisteredImage;
	std::vector<std::string> hasRegisteredName;
	void queryHasRegisteredUsers();
	void hasRegisteredUsersDisplay();

	void hasRecordListDisplay();

	// ��ѯ���ݿ������м�¼
	int perColumnRecordDis = 2;
	int perDisRecordW = 700, perDisRecordH = 240;
	MySQLConnection sql_conn;
	std::vector<std::vector<std::string>> recordList;

	private slots:
	
	void onButtonCloseClicked();
	//void onRecordListDisplay(int idx);
};