#include "Client.h"

Client::Client(sockaddr_in addr) : Id(NextId())
{
	Addr = addr;
}

uint32_t Client::NextId()
{
	return ++_maxId;
}