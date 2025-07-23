#pragma once

#include <Poco/Logger.h>

#include "Utils/Logging.h"
#include "MessageProcessor.h"
#include "CryptoCommon.h"
#include "IConnection.h"
#include "Logger.h"
#include "RestBase.h"
#include "Tools.h"
#include "Utils/MessageData.h"

namespace CORE {
namespace RESTAPI {

// Web requests throttle limit name
const std::string WebRequestsThrottleLimitName_MINUTES = "WebRequestsPerMinute";
// Orders throttle limit name
const std::string OrdersRequestsThrottleLimitName_SECONDS = "OrdersPerSeconds";

/*! \brief Sends order to the exchange
* @return: order response as a string (normally in json format)
* */

enum class EOrderType
{
	Market, Limit
};

////////////////////////////////////////////////////////////////////////////
/*! \brief Base class for RestApi connections */
////////////////////////////////////////////////////////////////////////////
class RestConnectionBase : public CRYPTO::IConnection, public RestBase
{
public:
	RestConnectionBase(const CRYPTO::Settings &settings, const std::string& loggingPropsPath, const std::string &loggerName);
	virtual ~RestConnectionBase();
	
	using TExecutionReports = std::vector<UTILS::MESSAGE::ExecutionReportData>;
	
	struct OrderTransaction
	{
		std::string SenderCompId;
		UTILS::Currency::Value Currency;
		UTILS::MESSAGE::ExecutionReportData ExecReport;
	};
	using TOrderTransactions = std::map<int64_t, OrderTransaction>; // <id, exec report>
	
	/*! \brief returns a copy of existing order transactions */
	TOrderTransactions GetOrderTransactions() const;
	
	/*! \brief updates existing order transactions by data from Engine
	* @param updates: actual open positions info from engine <clientOrdId:<SenderCompId,Instrument>>
	* Note: it is up to exchange implementation to support order transactions
	* */
	using TOpenPositionUpdates = std::map<std::string, std::pair<std::string, UTILS::CurrencyPair>>;
	
	TExecutionReports TranslateOrderResult(const std::string &jsonStr) const;

	UTILS::BoolResult Connect() override
	{
		m_connected=true;
		return m_connected.load();
	}
	
//------------------------------------------------------------------------------
	void Disconnect() override
	{
		m_connected = false;
		poco_information_f1(logger(), "Session '%s' has stopped", m_settings.m_name);
	}
	
	bool IsConnected() const override
	{ return m_connected; }
	
	void SetActive(bool active) override
	{ m_active = active; }
	
	bool IsActive() override
	{ return m_active; }
	
	void Start() override
	{
	}
	
	/*! \brief returns reference to settings */
	const CRYPTO::Settings &GetSettings() const override
	{
		return m_settings;
	}
	
	virtual std::string GenerateTimeStamp() const
	{
		return CORE::CRYPTO::TOOLS::to_iso_string(UTILS::CurrentTimestamp());
	}
	
	/*! \brief Translates order result
* @return: ExecutionReportData
* */
	virtual TExecutionReports TranslateOrderResult(const std::shared_ptr<CRYPTO::JSONDocument> jd) const
	{
		return { CRYPTO::TOOLS::CreateEmptyExecutionReportData() };
	}
	
	virtual std::string SendOrder(const UTILS::CurrencyPair &instrument, const UTILS::Side side, const RESTAPI::EOrderType orderType,
								  const UTILS::TimeInForce timeInForce, const double price, const double quantity,
								  const std::string &clientOrderId = "")=0;

	virtual std::string QueryOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
								   const std::optional<std::string> &origClientOrderId = std::nullopt)=0;

	virtual std::string CancelOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
									const std::optional<std::string> &origClientOrderId = std::nullopt)=0;
	
	virtual TExecutionReports TranslateOrder(const std::shared_ptr<CRYPTO::JSONDocument> jd, Poco::Logger *logger = nullptr) const=0;
	
	void StartOrderTransaction(const std::string &senderCompId, const UTILS::Currency::Value currency, const TExecutionReports &execReport);
	
	void UpdateOrderTransactions(const TOpenPositionUpdates &updates);
	
	/*! \brief Returns reference to message processor */
	CRYPTO::MessageProcessor &GetMessageProcessor()
	{
		return m_messageProcessor;
	}
	
	enum class EUpdateType
	{
		Snapshot, // for snapshots
		Feed      // for quote feed (real update type depends on volume)
	};
	
	/*! \brief processing existing order transactions each order_monitoring_interval */
	bool ProcessOrderTransactions();
	
	CRYPTO::Settings m_settings;
	CRYPTO::Logger m_logger; //Session logger..

private:
	bool m_active;
	/*! \brief Flag indicating that connection is connected or disconnected */
	std::atomic<bool> m_connected { false };

	CRYPTO::MessageProcessor m_messageProcessor;
	
	mutable TOrderTransactions m_orderTransactions;
};
}
}
