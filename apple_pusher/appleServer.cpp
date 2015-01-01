#include <stdio.h>  
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>  

#include <event2/event.h>
#include <event2/bufferevent.h>

#include "tableAgent.h"
#include "VspdCToMySQL.h"

using namespace std;

#define LISTEN_PORT 6100
#define LISTEN_BACKLOG 128

#define BUF_LEN 1024
#define FD_SIZE 100
#define MAX_BACK 100

#define MAX_BUFLEN 4096
#define MAX_FILE_NUMBER 128
  
#define MSGSIZE  1024   
#define MANGO_MESSAGE_SIZE_MIN (38)
#define MANGO_MESSAGE_SIZE_MAX (53)
//00-1F-3A-1F-08-4F;2014-03-01 23:26:59;
//17+20
//192.192.168.222;

const static char *MANGO_PATH = "/opt/";
char MAGIC_PATH[64];

struct files_array {
    char *f_path[MAX_FILE_NUMBER];
    char *f_name[MAX_FILE_NUMBER];
    unsigned int f_len[MAX_FILE_NUMBER];
    unsigned int f_n;
    unsigned int total_name_len; /* total length of files including '\0' */
};

int g_iTotalConn = 0;   
int g_CliSocketArr[FD_SETSIZE];   
VspdCToMySQL *g_MySQLHelper = NULL;

void * WorkerThread(void *params);
tableAgent ResolveMessage(const string &guest_id, char *msg);
void SaveToMysql(const tableAgent &ta);

void do_accept(evutil_socket_t listener, short event, void *arg);
void read_cb(struct bufferevent *bev, void *arg);
void error_cb(struct bufferevent *bev, short event, void *arg);
void write_cb(struct bufferevent *bev, void *arg);

VspdCToMySQL *InitMySQLConnect()
{
	VspdCToMySQL *mysqlHelper = new VspdCToMySQL();

    char* host="localhost";
    char* user="root";
    char* port ="3306";
    char* passwd="2014magicApplemysql";
    char* dbname="mango"; 
    char* charset = "GBK";//支持中文
    char* Msg = "";//消息变量
    //初始化
    if(mysqlHelper->ConnMySQL(host,port,dbname,user,passwd,charset,Msg) == 0)
	{
		printf("MySQL is successfully connected!\n");
		return mysqlHelper;
	}   
    else
	{
		printf("Error message is %s\n", Msg);
		return NULL;
	}         
}

#define MSG_BUF_LEN 64
tableAgent ResolveMessage(const string &guest_id, char *msg)
{
	tableAgent ta;
	if (NULL == msg)
		return ta;
	size_t len = strlen(msg);
	if (MANGO_MESSAGE_SIZE_MIN > len)
	{
		cout << "message size is not correct " << len << endl;
		return ta;
	}		

	ta.guest_id = guest_id;
	char *ch = msg;
	char buf[MSG_BUF_LEN] = {0};
	for (int i=0; ((*ch) != ';') && i < len; ++ch, ++i)
	{
		buf[i] = (*ch);
	}
	ta.agent_mac = string(buf);
	memset(buf, 0, MSG_BUF_LEN);

	++ch;
	for (int i=0; ((*ch) != ';') && i < len; ++ch, ++i)
	{
		buf[i] = (*ch);
	}
	ta.first_run_time = string(buf);
	ta.last_run_time = ta.first_run_time;
	ta.run_date = ta.first_run_time.substr(0, 10);

        memset(buf, 0, MSG_BUF_LEN);
	++ch;
        for (int i=0; ((*ch) != ';') && i < len; ++ch, ++i)
        {
                buf[i] = (*ch);
        }
        ta.pc_name = string(buf);

        memset(buf, 0, MSG_BUF_LEN);
        ++ch;
        for (int i=0; ((*ch) != ';') && i < len; ++ch, ++i)
        {
                buf[i] = (*ch);
        }
        ta.os_name = string(buf);
	
#if 0
	cout << "id= " << ta.guest_id << " , ";
	cout << "first time = " << ta.first_run_time << " , ";
	cout << "last time = " << ta.last_run_time << " , ";
	cout << "mac = " << ta.agent_mac << endl;
#endif

	return ta;
}

void SaveToMysql(const tableAgent &ta)
{
	if (NULL == g_MySQLHelper)
	{
		cout << "Mysql connection is invalid" << endl;
		return;
	}

	string selectSql = "select * from agent_run where agent_mac='" + ta.agent_mac + "' and run_date='"+ ta.run_date +"';";
	//cout << "****" << selectSql << endl;
	string msg;
	vector<tableAgent> searchResult = g_MySQLHelper->SelectData(selectSql, msg);
	//cout << "**size=**" << searchResult.size() << "empty=" << searchResult.empty() << endl;
	//插入新数据
	if (searchResult.empty())
	{
		ostringstream oss;
		oss << "insert into agent_run(guest_id, agent_ip,first_run_time,last_run_time,agent_mac,run_date,pc_name,os_name) "
			<< "values('" << ta.guest_id << "','" << ta.agent_ip << "','" << ta.first_run_time << "','" 
			<< ta.last_run_time << "','" << ta.agent_mac << "','" << ta.run_date << "','" 
			<< ta.pc_name << "','" << ta.os_name <<"');";
		string insertSql = oss.str();
		//cout << insertSql << endl;
		int ret = g_MySQLHelper->InsertData(insertSql, msg);
		if (ret)
			cout << "insert failed: " << msg << endl;
	}
	else
	{
		ostringstream oss1;
		oss1 << "update agent_run set last_run_time='" << ta.last_run_time 
			<< "' where agent_mac='" << ta.agent_mac << "' and run_date='" << ta.run_date << "';";
		string tmp = oss1.str();
		//cout << tmp << endl;
		int ret1 = g_MySQLHelper->UpdateData(tmp, msg);
		if (ret1)
			cout << "update failed: " << msg << endl;
	}
}

void FillClientIp(int sockfd, tableAgent &ta)
{
    socklen_t rsa_len = sizeof(struct sockaddr_in);
    struct sockaddr_in rsa;
    char *ip = NULL;

    if(getpeername(sockfd, (struct sockaddr *)&rsa, &rsa_len) == 0)
    {
        ip = inet_ntoa(rsa.sin_addr);
		ta.agent_ip = string(ip);
		//cout << "Accepted client is " << ta.agent_ip << endl;
	}
}

void free_files(struct files_array *files)
{
    int i;
    for (i = 0; i < files->f_n; i++)
    {
    if (files->f_path[i])
        free(files->f_path[i]);
    if (files->f_name[i])
        free(files->f_name[i]);
    }
}

ssize_t get_file_len(const char *file_path)
{
    ssize_t filesize = 0;      
    struct stat statbuff;  

    if(stat(file_path, &statbuff) < 0){  
    return filesize;  
    }else{  
    filesize = statbuff.st_size;  
    }

    return filesize;  
}

void get_files(const char * dir, struct files_array *files)  
{  
    DIR *dp;  
    struct dirent *entry;  
    struct stat statbuf;  
    ssize_t buf_len = 0;
    ssize_t file_name_len = 0;

    if((dp = opendir(dir)) == NULL) {  
    fprintf(stderr,"cannot open directory: %s\n", dir);  
    return;  
    }  

    chdir(dir);  

    while((entry = readdir(dp)) != NULL) {  
    if (MAX_FILE_NUMBER == files->f_n)
    {
        break;
    }

    lstat(entry->d_name,&statbuf);  
    if(S_ISDIR(statbuf.st_mode)) {  

        if(strcmp(".",entry->d_name) == 0 ||  
            strcmp("..",entry->d_name) == 0)  
        continue;  
    }  
    else{
        buf_len = strlen(dir)+strlen(entry->d_name)+2;
        file_name_len = strlen(entry->d_name) + 1;
        files->f_path[files->f_n] = (char *)malloc(buf_len);
        files->f_name[files->f_n] = (char *)malloc(file_name_len);

        files->total_name_len += file_name_len;

        snprintf(files->f_name[files->f_n], file_name_len, "%s", entry->d_name);
        snprintf(files->f_path[files->f_n], buf_len, "%s/%s",dir,entry->d_name);  

        files->f_len[files->f_n] = (unsigned int )get_file_len(files->f_path[files->f_n]);

        files->f_n++;
    }  
    }  
    chdir("..");  
    closedir(dp);  
} 

void prepare_mango_hdr(struct files_array *files)
{
    files->f_n = 0;
    files->total_name_len = 0;
    get_files(MAGIC_PATH, files);
}

int read_exact(struct bufferevent *bev, void *data, size_t size)
{
    size_t offset = 0;
    ssize_t len;

    while ( offset < size )
    {
	len = bufferevent_read(bev, (char *)data + offset, size - offset);
	if ( (len == -1) && (errno == EINTR) )
	    continue;
	if ( len == 0 )
	    errno = 0;
	if ( len < 0 )
	    return -1;
	offset += len;
    }

    return 0;
}

void send_mango_hdr(struct bufferevent *bev, struct files_array *files)
{
    ssize_t sent_len = 0;
    int i;

    /* send number of file */
    if (bufferevent_write(bev, &files->f_n, sizeof(files->f_n)) < 0){
        printf("[%s:%d]Failed to bufferevent_write number of file\n", __FUNCTION__, __LINE__);
        return;
    }

    /* send every file len */
    for(i = 0; i < files->f_n; i++){
        if (bufferevent_write(bev, &files->f_len[i], sizeof(files->f_len[i])) < 0){
            printf("[%s:%d]Failed to bufferevent_write file[%d]'s' len\n", 
                __FUNCTION__, __LINE__, i);
            return;
        }
    }

    /* send total name length */
    if (bufferevent_write(bev, &files->total_name_len, sizeof(files->total_name_len)) < 0){
        printf("[%s:%d]Failed to bufferevent_write total file name len\n", __FUNCTION__, __LINE__);
        return;
    }    

    /* send file name list */
    for (i = 0; i < files->f_n; i++)
    {
        if (bufferevent_write(bev, files->f_name[i], strlen(files->f_name[i])+1) < 0){
            printf("[%s:%d]Failed to bufferevent_write file[%d]'s' name\n", 
                __FUNCTION__, __LINE__, i);
            return;
        }
    }
}

void send_file_to_client(struct bufferevent *bev, struct files_array *files)
{
    char tmp_buf[MAX_BUFLEN];
    int file_blk_len = 0;
    unsigned long total_len = 0;
    int i;
    FILE *fp = NULL;

    for(i = 0; i < files->f_n; i++)
    {
        fp = fopen(files->f_path[i], "r+b");
        if (NULL == fp) {
            printf("open file  failed \n");
            return;
        }
        
        //bzero(tmp_buf, MAX_BUFLEN);
        while ((file_blk_len = fread(tmp_buf, sizeof(char), MAX_BUFLEN, fp)) > 0)
        {

            total_len += file_blk_len;
            if (bufferevent_write(bev, tmp_buf, file_blk_len) < 0){
                printf("[%s:%d]Failed to bufferevent_write\n", __FUNCTION__, __LINE__);
                fclose(fp);
                return;
            }
            //bzero(tmp_buf, MAX_BUFLEN);
        }

        fclose(fp);
    }

}

void do_work(struct bufferevent *bev)
{
    struct files_array files;

    prepare_mango_hdr(&files);

    send_mango_hdr(bev, &files);

    send_file_to_client(bev, &files);

    free_files(&files);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    evutil_socket_t fd;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);;
    fd = accept(listener, (struct sockaddr *)&sin, &slen);
    if (fd < 0) {
        perror("accept");
        return;
    }
    if (fd > FD_SETSIZE) {
        perror("fd > FD_SETSIZE\n");
        return;
    }

    //printf("ACCEPT: fd = %u\n", fd);

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, error_cb, arg);
    bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

char *magic_apple="magic_apple";

static int find_first_slip(char *str)
{
	int  i, len;

	if (NULL == str)
		return -1;
	len = strlen(str);
	if (len==0 | len == 1)
		return -1;

	for(i=0; i < len; i++) {
		if (';' == str[i])
			return i;			
	}
}

void read_cb(struct bufferevent *bev, void *arg)
{
#define MAX_LINE    256
    char line[MAX_LINE+1];
    char *message;
    int n;
    unsigned int msg_size;
    evutil_socket_t fd = bufferevent_getfd(bev);
    char guest_id[32] = {0};

    n = bufferevent_read(bev, line, MAX_LINE);
    if (n < 0)
    {
//      printf("[%s:%d] bufferevent_read failed\n");
        return;
    }
	line[n]='\0';
	//printf("buf=%s\n", line);

	//magic_apple000
    if( strncmp( line, magic_apple, strlen(magic_apple) ) == 0 )
    {
	memset(MAGIC_PATH, 0, 64);
	strncpy(MAGIC_PATH, MANGO_PATH, strlen(MANGO_PATH));
	strncat(MAGIC_PATH, line, 14);
//	printf("MAGIC_PATH=%s\n", MAGIC_PATH);

	n = find_first_slip(line+14);
	strncpy(guest_id, line+14, n);
	//printf("Guest id=%s\n", guest_id);
	
	message = line + 14 + n + 1;
	//printf("message =%s\n", message);

        do_work(bev);

        tableAgent ta = ResolveMessage(string(guest_id), message);
	FillClientIp(fd, ta);
	if (!ta.agent_mac.empty())
   		SaveToMysql(ta);
    }

}

void write_cb(struct bufferevent *bev, void *arg) {}

void error_cb(struct bufferevent *bev, short event, void *arg)
{
    evutil_socket_t fd = bufferevent_getfd(bev);
//    printf("fd = %u, ", fd);
    if (event & BEV_EVENT_TIMEOUT) {
        printf("[%s:%d]Timed out\n", __FUNCTION__, __LINE__); //if bufferevent_set_timeouts() called
    }
    else if (event & BEV_EVENT_EOF) {
        //printf("connection closed\n");
    }
    else if (event & BEV_EVENT_ERROR) {
        printf("some other error\n");
    }
    bufferevent_free(bev);
}

int server_main()
{   
    int ret;
    evutil_socket_t listener;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    assert(listener > 0);
    evutil_make_listen_socket_reuseable(listener);

    struct sockaddr_in sin;

    bzero( &sin, sizeof( sin ) );
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl( INADDR_ANY );
    sin.sin_port = htons(LISTEN_PORT);

    if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind"  );
        return 1;
    }

    if (listen(listener, LISTEN_BACKLOG) < 0) {
        perror("listen" );
        return 1;
    }

    printf ("Listening...\n");

    evutil_make_socket_nonblocking(listener);

    struct event_base *base = event_base_new();
    assert(base != NULL);
    struct event *listen_event;
    listen_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    event_add(listen_event, NULL);
    event_base_dispatch(base);

    printf("The End.");
    return 0;  
}   

int main(int argc, char* argv[])   
{
	g_MySQLHelper = InitMySQLConnect();
	
	server_main();

	if (NULL != g_MySQLHelper)
		delete g_MySQLHelper;
	return 0;
}
