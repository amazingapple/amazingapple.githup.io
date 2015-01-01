#pragma once

#include <stdio.h>
#include <string>
#include "mysql.h"

#include <vector>
#include "tableAgent.h"

class VspdCToMySQL 
{
public: 
       /*
       ���캯����ϡ������
       */
       VspdCToMySQL();
       virtual ~VspdCToMySQL();
 
       /*
       ��Ҫ�Ĺ��ܣ�
       ��ʼ�����ݿ�
       �������ݿ�
       �����ַ���
 
       ��ڲ�����
       host ��MYSQL������IP
       port:���ݿ�˿�
       Db�����ݿ�����
       user�����ݿ��û�
       passwd�����ݿ��û�������
       charset��ϣ��ʹ�õ��ַ���
       Msg:���ص���Ϣ������������Ϣ
 
       ���ڲ�����
       int ��0��ʾ�ɹ���1��ʾʧ��
       */
       int ConnMySQL(char *host,char * port,char * Db,
		   char * user,char* passwd,char * charset,char * Msg);
 
       /*
       ��Ҫ�Ĺ��ܣ�
       ��ѯ����
 
       ��ڲ�����
       SQL����ѯ��SQL���
       Cnum:��ѯ������
       Msg:���ص���Ϣ������������Ϣ
 
       ���ڲ�����
       string ׼�����÷��ص����ݣ�������¼����0x06����,�����λ��0x05����
       ��� ���صĳ��ȣ� 0�����ʾ����
       */
	   std::vector<tableAgent> SelectData(const std::string &SQL, std::string &Msg);
      
       /*
       ��Ҫ���ܣ�
       ��������
      
       ��ڲ���
       SQL����ѯ��SQL���
       Msg:���ص���Ϣ������������Ϣ
 
       ���ڲ�����
       int ��0��ʾ�ɹ���1��ʾʧ��
       */
	   int InsertData(const std::string &SQL,std::string &Msg);
 
       /*
       ��Ҫ���ܣ�
       �޸�����
      
       ��ڲ���
       SQL����ѯ��SQL���
       Msg:���ص���Ϣ������������Ϣ
 
       ���ڲ�����
       int ��0��ʾ�ɹ���1��ʾʧ��
       */
       int UpdateData(const std::string &SQL, std::string &Msg);
 
 
       /*
       ��Ҫ���ܣ�
       ɾ������
      
       ��ڲ���
       SQL����ѯ��SQL���
       Msg:���ص���Ϣ������������Ϣ
 
       ���ڲ�����
       int ��0��ʾ�ɹ���1��ʾʧ��
       */
       int DeleteData(const std::string &SQL, std::string &Msg);
private:
       //����
       MYSQL mysql;
       /*
       ��Ҫ���ܣ�
       �ر����ݿ�����
       */
       void CloseMySQLConn();
 
};

