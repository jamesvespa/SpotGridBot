
#pragma once


#include <Poco/Logger.h>

namespace CORE {

namespace RESTAPI {class RestConnectionBase;}

namespace ADAPTERS {

class AdapterExecutor
{
public:
	explicit AdapterExecutor(CORE::FullContext &ctx, Poco::Logger &logger);
	
	~AdapterExecutor();
	
	void TriggerShutdown(std::ostream &ostr, int type);
	
	void StartupTakerRateSessions(std::ostream &ostr, bool);

	UTILS::BoolResult SendExecutionReport(CORE::RESTAPI::RestConnectionBase &connection, const std::string &senderCompId, const UTILS::Currency::Value currency,
										  const UTILS::MESSAGE::ExecutionReportData &execReport) const;

protected:

	virtual CORE::RESTAPI::RestConnectionBase* FindConnection(const std::string &sessionName, UTILS::BoolResult &result) const;

private:

	FullContext &m_ctx;
	Poco::Logger &m_logger;
	std::string m_clientComponentName;
};
} // ns
} // ns
