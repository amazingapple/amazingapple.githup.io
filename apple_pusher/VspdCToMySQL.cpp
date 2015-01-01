#include "VspdCToMySQL.h"
#include <iostream>
using namespace std;

VspdCToMySQL::VspdCToMySQL()
{
}
 
VspdCToMySQL::~VspdCToMySQL()
{
	CloseMySQLConn();
}
 
//初始化数据
int VspdCToMySQL::ConnMySQL(char *host,char * port ,char * Db,
							char * user,char* passwd,char * charset,char * Msg)
{
       if( mysql_init(&mysql) == NULL )
       {
	  fprintf(stderr, "Failed to init mysql: Error: %s\n", mysql_error(&mysql));
          return 1;
       }    
 
       if (mysql_real_connect(&mysql,host,user,passwd,Db,0,NULL,0) == NULL)
       {
	  fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
              return 1;
       }    
	   
	char value = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &value);

       if(mysql_set_character_set(&mysql,"GBK") != 0)
       {
		fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
		return 1;
       }
	Msg = NULL;

       return 0;
}
 
//查询数据
std::vector<tableAgent> VspdCToMySQL::SelectData(const std::string &SQL, std::string &Msg)
{
    MYSQL_ROW m_row;
    MYSQL_RES *m_res;

	std::vector<tableAgent> result;
 
    if(mysql_query(&mysql,SQL.c_str()) != 0)
    {
         Msg = "select ps_info Error";
		 cout << "select Data Error, error is : " << mysql_error(&mysql) << endl;
	fprintf(stderr, "Query data failed: Error: %s\n", mysql_error(&mysql));
         return result;
    }
    m_res = mysql_store_result(&mysql);

    if(m_res==NULL)
    {
         Msg = "select username Error";
	fprintf(stderr, "Mysql store failed: Error: %s\n", mysql_error(&mysql));
         return result;
    }

    while(m_row = mysql_fetch_row(m_res))
    {
		tableAgent tmp;
		tmp.guest_id = m_row[0];
		tmp.agent_ip = m_row[1];
		tmp.first_run_time = m_row[2];
		tmp.last_run_time = m_row[3];
		tmp.agent_mac = m_row[4];
		tmp.run_date = m_row[5];
		tmp.pc_name = m_row[6];
		tmp.os_name = m_row[7];
		//cout << tmp.id << " " << tmp.first_run_time << " " << tmp.agent_mac << endl;
		(void)result.push_back(tmp);
    }

    mysql_free_result(m_res);
//	cout << "result.size=" << result.size() << endl;

    return result;
}
 
//插入数据
int VspdCToMySQL::InsertData(const std::string &SQL,std::string &Msg)
{
       if(mysql_query(&mysql,SQL.c_str()) != 0)
       {
              Msg = "Insert Data Error";
			  cout << "Insert Data Error, error is : " << mysql_error(&mysql) << endl;
		fprintf(stderr, "Mysql store failed: Error: %s\n", mysql_error(&mysql));
              return 1;
       }
       return 0;
}
 
//更新数据
int VspdCToMySQL::UpdateData(const std::string &SQL, std::string &Msg)
{
	if(mysql_query(&mysql,SQL.c_str()) != 0)
    {
        Msg = "Update Data Error";
		cout << "Update Data Error, error is : " << mysql_error(&mysql) << endl;
        return 1;
    }
    return 0;
}
 
//删除数据
int VspdCToMySQL::DeleteData(const std::string &SQL, std::string &Msg)
{
       if(mysql_query(&mysql,SQL.c_str()) != 0)
       {
              Msg = "Delete Data error";
			  cout << "DeleteData Error, error is : " << mysql_error(&mysql) << endl;
		fprintf(stderr, "Mysql query failed: Error: %s\n", mysql_error(&mysql));
              return 1;
       }
       return 0;
}
 
//关闭数据库连接
void VspdCToMySQL::CloseMySQLConn()
{
       mysql_close(&mysql);
}
 
