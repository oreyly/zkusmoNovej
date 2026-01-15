#pragma once
#include <netinet/in.h>
#include <string>

class Client
{
public:
	Client(sockaddr_in addr);

	sockaddr_in Addr;
	const uint32_t Id;
private:
	static inline uint32_t _maxId = 0;
	static uint32_t NextId();

	std::string _mess;
};
