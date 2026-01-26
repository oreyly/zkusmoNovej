#include "Client.h"

Client::Client(sockaddr_in addr) : Id(NextId())
{
	Addr = addr;
}

Client::Client(const Client& other)
    : Addr(other.Addr), Id(other.Id), Name(other.Name)
{ }

uint32_t Client::NextId()
{
	return ++_maxId;
}
