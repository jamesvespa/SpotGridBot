#pragma once

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include "Utils/Logging.h"
#include "Utils/ErrorHandler.h"

namespace CORE {
namespace RESTAPI {

class RestBase : public UTILS::Logging, public UTILS::ErrorHandler
{
public:
	RestBase(const std::string &loggerName)
			: UTILS::Logging(loggerName)
	{
	}
	
	virtual ~RestBase()
	{
	}

protected:
	std::string ExecuteWebRequest(const std::string &url, const std::string &requestType,
								  std::function<void(std::string &path)> customizeRequestPathFunc = nullptr,
								  std::function<void(Poco::Net::HTTPRequest &request)> customizeRequestFunc = nullptr,
								  std::function<void(const Poco::Net::HTTPResponse &response)> processResponseFunc = nullptr,
								  std::function<void(std::ostream &)> handleRequestStreamFunc = nullptr) const;
	
	virtual std::string SendWebRequest(const std::string &url, Poco::Net::HTTPSClientSession &session, Poco::Net::HTTPRequest &request,
							   std::function<void(const Poco::Net::HTTPResponse &response)> processResponseFunc,
							   std::function<void(std::ostream &)> handleRequestStreamFunc) const;
};
}
}