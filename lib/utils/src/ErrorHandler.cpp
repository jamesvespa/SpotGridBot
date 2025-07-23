//
// Created by jameson 19.01.18.
//

#include <Poco/Logger.h>

#include "Utils/Utils.h"
#include "Utils/ErrorHandler.h"

namespace UTILS {

std::string ErrorHandler::GetLastError() const
{
	std::shared_lock lock { m_lastError.Mutex() };
	return *m_lastError;
}

bool ErrorHandler::HasError() const
{
	std::shared_lock lock { m_lastError.Mutex() };
	return !m_lastError->empty();
}

/*! \brief Clears the error message
 *
 * @return Always \a true (no error)
 */
bool ErrorHandler::ClearError() const
{
	return SetError("");
}

/*! \brief Sets an error message
 *
 * Replaces the previously set error message with a new one.
 *
 * @param msg   Error message to be set
 * @return \a false if an error message was set, \a true if the empty string (= no error) was set.
 */
bool ErrorHandler::SetError(std::string msg) const
{
	std::unique_lock lock { m_lastError.Mutex() };
	*m_lastError = std::move(msg);
	
	if (m_logger && !m_lastError->empty())
	{
		poco_error(*m_logger, *m_lastError);
	}
	return m_lastError->empty();
}

/*! \brief Creates a new error message by inserting the last error message into
 * a given string
 *
 * Creates a new error message by replacing all occurrences of '_' in the given
 * string with the last error message.
 *
 * @param str   Template for the new error message that contains '_' where the
 * last error message is to be inserted
 * @return new error message
 */
std::string ErrorHandler::insertLastError(std::string str) const
{
	std::string newStr { std::move(str) };
	std::shared_lock lock { m_lastError.Mutex() };
	if (UTILS::replaceAll(newStr, "_", *m_lastError) == 0 && !m_lastError->empty())
	{
		newStr += "\n\n" + *m_lastError;
	}
	return newStr;
}

} // namespace CORE
