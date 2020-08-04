#pragma once
#ifndef MYSQLCONNECTION_H
#define MYSQLCONNECTION_H 

#include <iostream>
#include <vector>

#include <winsock.h>	// ���Ҫ�ӵ�mysqlͷ�ļ�ǰ��
#include <mysql.h>

#define HOST_ "localhost"
#define USER_ "root"
#define PASSWORD_ "111111"
#define DATABASE_ "facemaskdatabase"

struct conn_info {
	char *host;
	char *user;
	char *password;
	char *database;
};

class MySQLConnection
{
public:
	MySQLConnection();
	~MySQLConnection();

	// ���ת����vector
	std::vector<std::vector<std::string>> tab2Vector(const char * table, const int colNum);

	// warning�����
	void insertTableWarning(std::vector<std::string> &vect);

	// log�����
	void insertTableLog(const std::vector<std::vector<std::string>> &vect);

private:
	MYSQL *mysql_conn;
	conn_info info;

	// �������ݿ�
	MYSQL *mysqlConnect(conn_info con);

	// ���ݿ����
	void mysqlOperation(char *sql_op);

	// ���ݿ�Ͽ�
	void mysqlDisconnect();
};

#endif // MYSQLCONNECTION_H
