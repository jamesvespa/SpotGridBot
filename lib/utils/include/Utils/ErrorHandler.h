/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2019-01-18
 * @copyright   (c) 2019 BBF-IT
*/

#include <string>

#include <Poco/Format.h>

#include "Utils/Lockable.h"

#ifndef MERCURYUTILS_ERRORHANDLER_H
#define MERCURYUTILS_ERRORHANDLER_H

namespace Poco {
class Logger;
class Any;
}

namespace UTILS {

/*! \brief Provides virtual methods to save error messages, and public methods
 * to retrieve the saved error message.
 */
class ErrorHandler
{
public:
	
	explicit ErrorHandler(Poco::Logger *logger = nullptr)
			: m_logger(logger) { }
	
	/** @brief Delete copy constructor. */
	ErrorHandler(const ErrorHandler & /* other */) = delete;
	
	/** @brief Delete copy assignment operator. */
	ErrorHandler &operator=(const ErrorHandler & /* other */) = delete;
	
	/** @brief Delete move constructor. */
	ErrorHandler(ErrorHandler &&other) { m_logger = other.m_logger; }
	
	/** @brief Delete move assignment operator. */
	ErrorHandler &operator=(ErrorHandler &&other)
	{
		m_logger = other.m_logger;
		return *this;
	}
	
	std::string GetLastError() const;
	
	bool HasError() const;

protected:
	
	bool ClearError() const;
	
	/*! \brief Checks a condition.
	 *
	 * Checks the given condition, and calls ClearError() if it is true,
	 * or SetError() if it is false.
	 *
	 * @param condition Condition to be checked
	 * @param value Value to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	template <typename ...Args>
	bool CheckCondition(bool condition, const std::string &errorMessage, Args &&...args) const
	{
		if (condition)
		{
			ClearError();
		}
		else
		{
			if (!errorMessage.empty())
			{
				SetError(errorMessage, std::forward<Args>(args)...);
			}
			else
			{ // ensure that no empty error message is set (in that case, SetError() would return true)
				SetError("CheckCondition(): given condition is false");
			}
		}
		return condition;
	}
	
	bool SetError(std::string msg) const;
	
	/*! \brief Sets an error message
	 *
	 * Replaces the previously set error message with a new one.
	 *
	 * @param fmt   Error message format string
	 * @param value Value to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool SetError(const std::string &fmt, const Poco::Any &value) const { return SetError(Poco::format(fmt, value)); }
	
	/*! \brief Sets an error message
	 *
	 * Replaces the previously set error message with a new one.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value2 Values to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool SetError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2) const
	{
		return SetError(Poco::format(fmt, value1, value2));
	}
	
	/*! \brief Sets an error message
	 *
	 * Replaces the previously set error message with a new one.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value3 Values to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool SetError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3) const
	{
		return SetError(Poco::format(fmt, value1, value2, value3));
	}
	
	/*! \brief Sets an error message
	 *
	 * Replaces the previously set error message with a new one.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value4 Values to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool SetError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3,
				  const Poco::Any &value4) const { return SetError(Poco::format(fmt, value1, value2, value3, value4)); }
	
	/*! \brief Sets an error message
	 *
	 * Replaces the previously set error message with a new one.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value5 Values to be inserted into the format string,
	 * using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool SetError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3, const Poco::Any &value4,
				  const Poco::Any &value5) const { return SetError(Poco::format(fmt, value1, value2, value3, value4, value5)); }
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param msg   Error message to be set. If \a msg contains '_' characters,
	 * then these are replaced with the previous error message.
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &msg) const
	{
		return SetError(insertLastError(msg));
	}
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param fmt   Error message format string
	 * @param value Value to be inserted into the format string, using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &fmt, const Poco::Any &value) const { return SetError(Poco::format(insertLastError(fmt), value)); }
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value2   Value to be inserted into the format string, using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2) const
	{
		return SetError(Poco::format(insertLastError(fmt), value1, value2));
	}
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value3   Value to be inserted into the format string, using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3) const
	{
		return SetError(Poco::format(insertLastError(fmt), value1, value2, value3));
	}
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value4   Value to be inserted into the format string, using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3,
					 const Poco::Any &value4) const { return SetError(Poco::format(insertLastError(fmt), value1, value2, value3, value4)); }
	
	/*! \brief Extends an error message
	 *
	 * Replaces all occurrences of '_' in the given error message and sets the
	 * resulting string as new error message.
	 *
	 * @param fmt   Error message format string
	 * @param value1...value5   Value to be inserted into the format string, using \a Poco::format()
	 * @return \a false if an error message was set, \a true if the empty string
	 * (= no error) was set.
	 */
	bool ExtendError(const std::string &fmt, const Poco::Any &value1, const Poco::Any &value2, const Poco::Any &value3, const Poco::Any &value4,
					 const Poco::Any &value5) const { return SetError(Poco::format(insertLastError(fmt), value1, value2, value3, value4, value5)); }

private:
	/*! \brief Pointer to the logger to additionally log all error messages
	 */
	Poco::Logger *m_logger;
	
	/*! \brief Error string. Mutable, such that \a SetError() can be called from \a const functions.
	 */
	mutable SharedLockable<std::string> m_lastError;
	
	std::string insertLastError(std::string str) const;
};

} // namespace

#endif //MERCURYUTILS_ERRORHANDLER_H
