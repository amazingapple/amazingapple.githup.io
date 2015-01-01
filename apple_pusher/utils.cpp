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
#include <fcntl.h>
#include <sys/types.h>
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

/* 
 *  * make a true rand 
 *   */  
static unsigned int new_rand ()  
{  
	int fd;  
	unsigned int n = 0;  

	fd = open ("/dev/urandom", O_RDONLY);  
	if (fd > 0){  
		read (fd, &n, sizeof (n));  
	}  
	close (fd);  

	return n;  
}  
/* 
 *  * change rand number to char 
 *   */  
static unsigned char randomchar(void)  
{  
	unsigned int rand;  
	unsigned char a;  

	a =(unsigned char)new_rand();  
	if (a < 'A')  
		a = a % 10 + 48;  
	else if (a < 'F')  
		a = a % 6 + 65;  
	else if (a < 'a' || a > 'f')  
		a = a % 6 + 97;  

	return a;  
}  
/* 
 *  * product one mac addr 
 *   */  
static void product_one_mac(char *str)  
{  
	char mac[18]={'0', '0'};  
	int i;  
	/*set mac addr */  
	for (i=3; i<17; i++){  
		usleep(10);  
		mac[i] = randomchar();  
	}  
	mac[2] = mac[5] = mac[8] = mac[11] = mac[14] = '-';  
	mac[17] = '\0';  

	sprintf(str, "%s", mac);
}

void get_random_str(char *str, int len)
{
	int i;

	if (len <=0)
		len = 1;

	char *tmp = (char *)malloc(len+1);
	for (i=0; i< len; i++) {
		tmp[i] =  randomchar();
	}

	tmp[i] = '\0';
	snprintf(str, len+1, "%s", tmp);

	free(tmp);
} 

void get_format_time_str(char *str)
{
	time_t timep;
	struct tm *p;

	time(&timep);
	p = localtime(&timep);
	sprintf(str, "%4d-%02d-%02d %02d:%02d:%02d", 1900+p->tm_year,
			1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

inline int get_random_number(unsigned int max)
{
	unsigned int r;

	r = new_rand();

	return (r % max + 1);
}

void get_random_ip(char *str)
{
	sprintf(str, "%d.%d.%d.%d", get_random_number(254), 
			get_random_number(254),get_random_number(254),
			get_random_number(254));
}

unsigned global_index = 0;

void fill_table_agent(tableAgent &ta)
{
	char str[32] = {0};

	get_random_ip(str);
	string rip(str);
	//cout << "ip is " << rip << endl;
	ta.agent_ip = rip;

	memset(str, 0, 32);
	get_format_time_str(str);
	string tmp_time(str);
	//cout << "now is " << tmp_time << endl;
	ta.first_run_time = tmp_time;
	ta.last_run_time = tmp_time;
	ta.run_date = tmp_time.substr(0, 10);

	memset(str, 0, 32);
	product_one_mac(str);
	string mac(str);	
	//cout << "mac is " << mac << endl;
	ta.agent_mac = mac;

	if ( global_index % 2 == 0)
		ta.os_name = "Microsoft Windows 7";
	else
		ta.os_name = "Microsoft Windows xp";
	global_index++;

	memset(str, 0, 32);
	unsigned int serial = new_rand() % 999;
	sprintf(str, "pc%u", serial);
	ta.pc_name = string(str);
	//cout << "pc " << ta.pc_name << endl;
}

int server_main()
{  
	//unsigned tt = 1; 

	for (;;)
	{
		//	tt = new_rand() % 4 + 1;
		//cout << "sleep " << tt << endl;
		sleep(253);

		tableAgent ta;
		ta.guest_id = "2014999";
		fill_table_agent(ta);
		if (!ta.agent_mac.empty())
			SaveToMysql(ta);
	}

	return 0;  
}

static void gen_rand_str ()
{
	int fd;
	char str[32];

	fd = open ("/dev/urandom", O_RDONLY);
	if (fd > 0){
		read (fd, str, 31);
	}
	close (fd);
	str[31] = '\0';

	printf("%s\n", str);
}


int main(int argc, char* argv[]) 
{
	return 0;
}

