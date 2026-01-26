#include "Client.h"

Client::Client(sockaddr_in addr) : Id(NextId()), Online(true), LastEcho(std::chrono::steady_clock::now())
{
	Addr = addr;
}

Client::Client(const Client& other)
    : Addr(other.Addr), Id(other.Id), Name(other.Name), Online(other.Online), LastEcho(other.LastEcho), GameRoomId(other.GameRoomId)
{ }

uint32_t Client::NextId()
{
	return ++_maxId;
}
