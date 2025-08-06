#pragma once

#include "RestConnectionBase.h"
#include "OKX/Definitions.h"

namespace CORE {
	class ConnectionManager;
}

namespace CORE {
namespace OKX {

std::string Sign(const std::string &timeStampStr, const std::string &requestType, const std::string &requestPath, const std::string &requestBody,
				 const std::string &secretKey);

////////////////////////////////////////////////////////////////////////////
/*! \brief OKX connection class */
////////////////////////////////////////////////////////////////////////////
class ConnectionORD : public CORE::RESTAPI::RestConnectionBase
{
public:
	ConnectionORD(const CRYPTO::Settings &settings, const std::string& loggingPropsPath, const ConnectionManager& connectionManager);
	
	std::string SendOrder(const UTILS::CurrencyPair &instrument, const UTILS::Side side, const RESTAPI::EOrderType orderType,
						  const UTILS::TimeInForce timeInForce, const double price, const double quantity,
						  const std::string &clientOrderId = "") override;
	
	std::string QueryOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
						   const std::optional<std::string> &origClientOrderId = std::nullopt) override;
	
	std::string CancelOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
							const std::optional<std::string> &origClientOrderId = std::nullopt) override;
	
	/*! \brief Translates order result
	* @return: ExecutionReportData
	* */
	TExecutionReports TranslateOrderResult(const std::shared_ptr<CRYPTO::JSONDocument> jd) const override;
	
	TExecutionReports TranslateOrder(const std::shared_ptr<CRYPTO::JSONDocument> jd, Poco::Logger *logger = nullptr) const override;
	
	virtual std::string GenerateTimeStamp() const;
	
	struct OrderActionResult
	{
		CRYPTO::JSONError State;
		Poco::JSON::Array::Ptr Data; // data array (can be nullptr)
	}; // OrderActionResult
	
	OrderActionResult GetOrderActionResult(const std::string &json) const;
	OrderActionResult GetOrderActionResult(const std::shared_ptr<CRYPTO::JSONDocument>(jd)) const;

protected:
	// Web request wrapper
	std::string DoWebRequest(const std::string &url, const std::string &requestType,
							 std::function<void(std::string &path)> customizeRequestPathFunc = nullptr,
							 std::function<void(Poco::Net::HTTPRequest &request)> customizeRequestFunc = nullptr,
							 std::function<void(std::ostream &)> handleRequestStreamFunc = nullptr);

private:
	//We need to sync with the OKX exchange time before sending orders.
	int64_t GetSystemTime() const;
};
	
} // ns OKX
}
