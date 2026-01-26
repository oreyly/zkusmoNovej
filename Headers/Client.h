#pragma once
#include <netinet/in.h>
#include <string>

class Client
{
public:
	Client(sockaddr_in addr);

	// Kopírovací konstruktor
	Client(const Client& other);

	// Operátor přiřazení
	Client& operator=(const Client& other) = delete;

	sockaddr_in Addr;
	const uint32_t Id;
	std::string Name;
	uint32_t GameRoomId;

	void AssignRoom(const uint32_t gameRoomId)
	{
		GameRoomId = gameRoomId;
	}

private:
	static inline uint32_t _maxId = 0;
	static uint32_t NextId();
};
