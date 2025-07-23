//
// Created by jameson 21.11.17.
//

#include <Poco/Logger.h>
#include "Utils/Logging.h"

namespace UTILS {

/*! \brief Constructor.
 *
 * Associates the created object with the specified logger.
 *
 * @param loggerName       Name of the POCO logger to be associated with the object.
 */
Logging::Logging(const std::string &loggerName)
		: m_logger(&Poco::Logger::get(loggerName))
{
}

/*! \brief Returns a reference to the POCO logger.
 *
 * Returns a reference to the POCO logger associated with the object.
 *
 * @return  Reference to the POCO logger.
 */
Poco::Logger &Logging::logger() const
{
	return *m_logger;
}

/*! \brief Returns a pointer to the POCO logger.
 *
 * Returns a pointer to the POCO logger associated with the object.
 *
 * @return  Pointer to the POCO logger.
 */
Poco::Logger *Logging::pLogger() const
{
	return m_logger;
}


}
