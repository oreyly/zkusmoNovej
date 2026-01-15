#pragma once
#include <string>
#include <unordered_map>

enum class ERROR_CODES
{
	CANNOT_CREATE_SOCKET,
	CANNOT_CREATE_LOG_FILE,
	TEST_ERROR,
	CANNOT_BIND_SOCKET,
	BAD_SOCKET,
	FATAL_SOCKET,
	CANNOT_SEND_VIA_SOCKET,
	UNKNOWN_SOCKET_RETURN,
	MISSING_ACKNOWLEDGEMENT,
	ALREAD_REGISTERED,
	ALREADY_CONNECTED_MANAGER,
	UNKNOWN_ORIGIN,
	RESPONSE_TO_UNKNOWN_REQUEST
};

class Errors
{
public:
	Errors() = delete;

	static std::string GetErrorMessage(ERROR_CODES ec);
private:
	inline static const std::unordered_map<ERROR_CODES, std::string> error_messages = {
		{ ERROR_CODES::CANNOT_CREATE_SOCKET, "Nelze vytvořit socket." },
		{ ERROR_CODES::CANNOT_CREATE_LOG_FILE, "Nelze vytvořit logovací soubor." },
		{ ERROR_CODES::TEST_ERROR, "Error pro testování funkcionalit spojených s errory." },
		{ ERROR_CODES::CANNOT_BIND_SOCKET, "Nelze přiřadit socket na port." },
		{ ERROR_CODES::BAD_SOCKET, "Přijat neplatný socket." },
		{ ERROR_CODES::FATAL_SOCKET, "Socket je rozbitý, je nutné ukončit aplikaci." },
		{ ERROR_CODES::CANNOT_SEND_VIA_SOCKET, "Přes socket nelze posílat zprávy, je nutné ukončit aplikaci." },
		{ ERROR_CODES::UNKNOWN_SOCKET_RETURN, "Socket je v nedefinovaném stavu, je nutné ukončit aplikaci." },
		{ ERROR_CODES::MISSING_ACKNOWLEDGEMENT, "Nebyla přijata odpověď na zprávu." },
		{ ERROR_CODES::ALREAD_REGISTERED, "Objekt již byl nelze zaregistrovat, jelikož už byl jednou registrován." },
		{ ERROR_CODES::ALREADY_CONNECTED_MANAGER, "Připojený manažer se snaží opět připojit ke zdroji." },
		{ ERROR_CODES::UNKNOWN_ORIGIN, "Přišel packet s neznámým původem." },
		{ ERROR_CODES::RESPONSE_TO_UNKNOWN_REQUEST, "Aplikace se snaží odpovědět na neexistující požadavek." },
	};
};
