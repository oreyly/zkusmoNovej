#include "Errors.h"

std::string Errors::GetErrorMessage(ERROR_CODES ec)
{
	return Errors::error_messages.at(ec);
}
