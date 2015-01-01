#pragma once
#include <string>

class tableAgent
{
public:
	tableAgent();
	tableAgent(const std::string &id, const std::string &agent_ip, std::string &first_run_time, 
		std::string &last_run_time, std::string &agent_mac)
		:guest_id(id), agent_ip(agent_ip), first_run_time(first_run_time), 
		last_run_time(last_run_time), agent_mac(agent_mac) {}
	virtual ~tableAgent();

	std::string guest_id;
	std::string agent_ip;
	std::string first_run_time;
	std::string last_run_time;
	std::string agent_mac;
	std::string run_date;
	std::string pc_name;
	std::string os_name;

	const static int col_num = 8;
};




