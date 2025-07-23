/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2017-11-21
 * @copyright   (c) 2017 BBF-IT
*/

#ifndef MERCURYUTILS_LOGGING_H
#define MERCURYUTILS_LOGGING_H

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

#endif //MERCURYUTILS_LOGGING_H
