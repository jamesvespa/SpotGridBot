#pragma once

#include <string>

namespace Poco
{
	class Logger;
}

namespace UTILS
{

/*! \brief WorkElem class for all classes using POCO logging.
 *
 * This class simply retrieves and stores the POCO logger of the name
 * specified in the constructor.
 */
class Logging
{
public:
	explicit Logging(const std::string &loggerName);
	
	Poco::Logger &logger() const;
	
	Poco::Logger *pLogger() const;

private:
	/*! Pointer to the POCO logger associated with the class */
	Poco::Logger *m_logger;
};

}
