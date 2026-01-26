#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

enum class ERROR_CODES : uint32_t
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
	RESPONSE_TO_UNKNOWN_REQUEST,
	COMMUNICATOR_NOT_INITIALIZED,
	INVALID_COMMAND,
	SOCEK_IFNO_UNAVAILABLE,
	UNKNOWN_ROOM
};

class Errors
{
public:
	Errors() = delete;

	static std::string GetErrorMessage(ERROR_CODES ec);
private:
	inline static const std::unordered_map<ERROR_CODES, std::string> error_messages = {
		{ ERROR_CODES::CANNOT_CREATE_SOCKET, "Nelze vytvorit socket." },
		{ ERROR_CODES::CANNOT_CREATE_LOG_FILE, "Nelze vytvorit logovaci soubor." },
		{ ERROR_CODES::TEST_ERROR, "Error pro testovani funkcionalit spojenych s errory." },
		{ ERROR_CODES::CANNOT_BIND_SOCKET, "Nelze priradit socket na port." },
		{ ERROR_CODES::BAD_SOCKET, "Prijat neplatny socket." },
		{ ERROR_CODES::FATAL_SOCKET, "Socket je rozbity, je nutne ukoncit aplikaci." },
		{ ERROR_CODES::CANNOT_SEND_VIA_SOCKET, "Pres socket nelze posilat zpravy, je nutne ukoncit aplikaci." },
		{ ERROR_CODES::UNKNOWN_SOCKET_RETURN, "Socket je v nedefinovanem stavu, je nutne ukoncit aplikaci." },
		{ ERROR_CODES::MISSING_ACKNOWLEDGEMENT, "Nebyla prijata odpoved na zpravu." },
		{ ERROR_CODES::ALREAD_REGISTERED, "Objekt nelze zaregistrovat, jelikoz uz byl jednou registrovan." },
		{ ERROR_CODES::ALREADY_CONNECTED_MANAGER, "Pripojeny manazer se snazi opet pripojit ke zdroji." },
		{ ERROR_CODES::UNKNOWN_ORIGIN, "Pøišel packet s neznamym puvodem." },
		{ ERROR_CODES::RESPONSE_TO_UNKNOWN_REQUEST, "Aplikace se snazi odpovedet na neexistujici pozadavek." },
		{ ERROR_CODES::COMMUNICATOR_NOT_INITIALIZED, "Pokus o pouziti komunikatoru, ktery jeste nebyl inicializovan." },
		{ ERROR_CODES::INVALID_COMMAND, "Byl zavolan neexistujici prikaz." },
		{ ERROR_CODES::SOCEK_IFNO_UNAVAILABLE, "Nelze ziskat informace o socketu." },
		{ ERROR_CODES::UNKNOWN_ROOM, "Pokus o pristup do neexistujici roomky" },
	};
};
