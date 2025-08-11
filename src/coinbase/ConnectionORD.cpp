#include <sstream>
#include "Definitions.h"
#include "RestConnectionBase.h"

#include "Tools.h"
#include "Utils/Result.h"
#include "Poco/URI.h"
#include "coinbase/ConnectionORD.h"

namespace CORE {
	class ConnectionManager;
}

using namespace UTILS;

using namespace CORE::CRYPTO;

namespace CORE {
namespace COINBASE {
////////////////////////////////////////////////////////////////////////////////
// ConnectionTT
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
ConnectionORD::ConnectionORD(const CRYPTO::Settings &settings, const std::string &loggingPropsPath, const ConnectionManager& connectionManager)
		: RESTAPI::RestConnectionBase(settings, loggingPropsPath, settings.m_name)
{
	GetMessageProcessor().Register([] (std::shared_ptr<CRYPTO::JSONDocument> jd)
								   {
									   // Try checking type field 'e'
									   auto msgType = jd->GetValue<std::string>("e");
									   if (!msgType.empty())
									   {
										   return msgType;
									   } // it was simple - type was in the field

									   // Try error message
									   // {"error":{"code":3,"msg":"Invalid JSON: expected `,` or `]` at line 4 column 6"}}
									   if (jd->Has(MSGTYPE_Error))
									   {
										   return MSGTYPE_Error;
									   }

									   // Try result message
									   // {"result":null,"id":1}
									   if (jd->Has(MSGTYPE_Result) && jd->Has("id"))
									   {
										   return MSGTYPE_Result;
									   }

									   return MSGTYPE_Unknown;
								   } );


	GetMessageProcessor().Register(MSGTYPE_Result, [this](const std::shared_ptr<CRYPTO::JSONDocument> jd)
	{
		OnMsgResult(jd->GetValue<std::string>("result"), jd->GetValue<int>("id"), true);
	});

	GetMessageProcessor().Register(MSGTYPE_Error, [this](const std::shared_ptr<CRYPTO::JSONDocument> jd)
	{
		try
		{
			auto errDesc = jd->GetSubObject("error");
			if (errDesc)
			{
				OnMsgError(errDesc->get("code").convert<int>(), errDesc->get("msg").toString(), true);
			}
			else
			{
				OnMsgError(0, "", UTILS::BoolResult(false, "Invalid error message descriptor"));
			}
		}
		catch (std::exception &e)
		{
			OnMsgError(0, "", UTILS::BoolResult(false, "Invalid error message descriptor: %s", std::string(e.what())));
		}
	});
}
//------------------------------------------------------------------------------
/*! \brief called when Result message received
* @param result: extracted result
* @param id: extracted id
* @param res: parsing result
* */
void ConnectionORD::OnMsgResult(const std::string &result, const int id, const UTILS::BoolResult &res)
{
	poco_information_f2(logger(), "received result='%s' for id='%s'", result, std::to_string(id));
}


//------------------------------------------------------------------------------
/*! \brief called when Error message received
* @param errCode: extracted error code
* @param errMsg: extracted error message
* @param res: parsing result
* */
void ConnectionORD::OnMsgError(const int errCode, const std::string &errMsg, const UTILS::BoolResult &res)
{
	poco_error_f2(logger(), "received 'error': code='%s', msg='%s'", std::to_string(errCode), errMsg);
}

//------------------------------------------------------------------------------
std::string ConnectionORD::SendOrder(const UTILS::CurrencyPair &instrument, const UTILS::Side side, const RESTAPI::EOrderType orderType,
								  const UTILS::TimeInForce timeInForce, const double price, const double quantity, const std::string &clientOrderId)
{
	const auto isMarketOrder = orderType == RESTAPI::EOrderType::Market || !price;
	std::string queryString = "symbol=" + instrument.BaseCCY().ToString() + instrument.QuoteCCY().ToString();
	queryString += "&side=" + std::string(side.Buy() ? "BUY" : "SELL");
	queryString += "&type=" + std::string(isMarketOrder ? "MARKET" : "LIMIT");
	if (!isMarketOrder)
	{
		queryString += "&timeInForce=" + timeInForce.ToString();
		queryString += "&price=" + UTILS::to_string_with_precision<double>(price, instrument.Precision());
	}
	queryString += "&quantity=" + std::to_string(quantity);
	if (!clientOrderId.empty())
	{
		queryString += "&newClientOrderId=" + clientOrderId;
	}
	queryString += "&recvWindow=" + std::to_string(m_settings.m_recvWindow);
	queryString += "&timestamp=" + std::to_string(UTILS::CurrentTimestamp() / 1000000); // timestamp must be in ms
	const auto signature = CORE::CRYPTO::TOOLS::EncryptWithHMAC(m_settings.m_secretkey, queryString);
	
	return DoWebRequest(m_settings.m_orders_http, Poco::Net::HTTPRequest::HTTP_POST, [&queryString, &signature](std::string &path)
	{
		path += "?" + queryString + "&signature=" + signature;
	}, [this](Poco::Net::HTTPRequest &request)
						{
							request.add("X-MBX-APIKEY", m_settings.m_apikey);
						});
}


//------------------------------------------------------------------------------
std::string ConnectionORD::QueryOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
								   const std::optional<std::string> &origClientOrderId)
{
	return QueryOrderOrCancel(ERequestType::QueryOrder, instrument, orderId, origClientOrderId);
}


//------------------------------------------------------------------------------
std::string ConnectionORD::CancelOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
									const std::optional<std::string> &origClientOrderId)
{
	return QueryOrderOrCancel(ERequestType::CancelOrder, instrument, orderId, origClientOrderId);
}


//------------------------------------------------------------------------------
/*! \brief implements common parts of two functions */
std::string ConnectionORD::QueryOrderOrCancel(const ERequestType requestType, const UTILS::CurrencyPair &instrument, const std::string &orderId,
										   const std::optional<std::string> &origClientOrderId)
{
	std::string queryString = "symbol=" + instrument.BaseCCY().ToString() + instrument.QuoteCCY().ToString();
	if (!orderId.empty())
	{
		queryString += "&orderId=" + orderId;
	}
	if (origClientOrderId.has_value())
	{
		queryString += "&origClientOrderId=" + origClientOrderId.value();
	}
	queryString += "&recvWindow=" + std::to_string(m_settings.m_recvWindow);
	queryString += "&timestamp=" + std::to_string(UTILS::CurrentTimestamp() / 1000000); // timestamp must be in ms
	const auto signature = CORE::CRYPTO::TOOLS::EncryptWithHMAC(m_settings.m_secretkey, queryString);
	return DoWebRequest(m_settings.m_orders_http,
						requestType == ERequestType::QueryOrder ? Poco::Net::HTTPRequest::HTTP_GET : Poco::Net::HTTPRequest::HTTP_DELETE,
						[&queryString, &signature](std::string &path)
						{
							path += "?" + queryString + "&signature=" + signature;
						}, [this](Poco::Net::HTTPRequest &request)
						{
							request.add("X-MBX-APIKEY", m_settings.m_apikey);
						});
}


//==============================================================================
// Binance web request wrapper
std::string ConnectionORD::DoWebRequest(const std::string &url, const std::string &requestType,
									 std::function<void(std::string &path)> customizeRequestPathFunc,
									 std::function<void(Poco::Net::HTTPRequest &request)> customizeRequestFunc)
{
	return ExecuteWebRequest(url, requestType, customizeRequestPathFunc, customizeRequestFunc,
							 [](const Poco::Net::HTTPResponse &aRespons){});
}

//------------------------------------------------------------------------------
// static
std::tuple<char, char> ConnectionORD::TranslateOrderStatus(const std::string &status)
{
	if (status == ORD_STATUS_NAME_NEW)
	{
		return std::make_tuple(ORDSTATUS_NEW, EXECTYPE_NEW);
	}
	else if (status == ORD_STATUS_NAME_PARTIALLY_FILLED)
	{
		return std::make_tuple(ORDSTATUS_PARTIALLY_FILLED, EXECTYPE_PARTIAL_FILL);
	}
	else if (status == ORD_STATUS_NAME_FILLED)
	{
		return std::make_tuple(ORDSTATUS_FILLED, EXECTYPE_FILL);
	}
	else if (status == ORD_STATUS_NAME_CANCELED)
	{
		return std::make_tuple(ORDSTATUS_CANCELED, EXECTYPE_CANCELED);
	}
	else if (status == ORD_STATUS_NAME_EXPIRED)
	{
		return std::make_tuple(ORDSTATUS_EXPIRED, EXECTYPE_EXPIRED);
	}
	
	// for any unknown state we consider it "REJECTED" (including ORD_STATUS_NAME_REJECTED)
	return std::make_tuple(ORDSTATUS_REJECTED, EXECTYPE_REJECTED);
}


//------------------------------------------------------------------------------
/*! \brief Translates order result
* @return: ExecutionReportData
* */
CORE::RESTAPI::RestConnectionBase::TExecutionReports ConnectionORD::TranslateOrderResult(const std::shared_ptr<CRYPTO::JSONDocument> jd) const
{
	return TranslateOrder(jd, &logger());
}

// static
CORE::RESTAPI::RestConnectionBase::TExecutionReports ConnectionORD::TranslateOrder(const std::shared_ptr<CRYPTO::JSONDocument> jd, Poco::Logger *logger) const
{
	// Check possible error
	CORE::RESTAPI::RestConnectionBase::TExecutionReports execs;
	const auto errCode = jd->GetValue<std::string>("code");
	if (!errCode.empty())
	{
		// It's an error
		// (expected format is like {"code":-1013,"msg":"Price * QTY is zero or less."})
		auto singleExecution { CORE::CRYPTO::TOOLS::CreateEmptyExecutionReportData() };
		singleExecution.m_ordStatus = ORDSTATUS_REJECTED;
		singleExecution.m_execType = EXECTYPE_REJECTED;
		singleExecution.m_text = UTILS::Format("The order has failed: Error code='%s', message='%s'", errCode, jd->GetValue<std::string>("msg"));
		if (logger)
		{
			poco_error_f1(*logger, "Connection::TranslateOrderResult error %s: ", singleExecution.m_text);
		}
		execs.push_back(singleExecution);
		return execs;
	}
	std::string arrayName { "fills" };
	
	CurrencyPair instrument { UTILS::CurrencyPair(jd->GetValue<std::string>("symbol")) };
	
	if (!instrument.Valid() && logger)
	{
		poco_error_f1(*logger, "Invalid instrument in exec report %s", instrument.ToString());
	}
	
	const auto orderQty = jd->GetValue<double>("origQty");
	const auto [ordStatus, ordExecType] = ConnectionORD::TranslateOrderStatus(jd->GetValue<std::string>("status"));
	
	double executedQty { 0. };
	
	// Returns pre-filled execution report
	const auto prepareExecReportFunc = [&]
	{
		auto singleExecution { CORE::CRYPTO::TOOLS::CreateEmptyExecutionReportData() };
		singleExecution.m_orderId = jd->GetValue<std::string>("orderId");
		singleExecution.m_clOrdID = jd->GetValue<std::string>("clientOrderId");
		singleExecution.m_ordType = jd->GetValue<std::string>("type") == "MARKET" ? ORDTYPE_MARKET : ORDTYPE_LIMIT;
		singleExecution.m_instrument = instrument;
		singleExecution.m_currency = instrument.BaseCCY();
		const auto side = jd->GetValue<std::string>("side");
		singleExecution.m_side = side == "SELL" ? UTILS::Side::SELL : (side == "BUY" ? UTILS::Side::BUY : UTILS::Side::INVALID);
		singleExecution.m_tif = UTILS::TimeInForce(jd->GetValue<std::string>("timeInForce"));
		singleExecution.m_orderPx = jd->GetValue<double>("price");
		singleExecution.m_orderQty = orderQty;
		singleExecution.m_ordStatus = ordStatus;
		singleExecution.m_execType = ordExecType;
		return singleExecution;
	}; // prepareExecReportFunc
	
	// Returns single execution array
	const auto generateSingleExecutionFunc = [&]
	{
		// we have no fills - just return the exec report
		auto singleExecution = prepareExecReportFunc();
		singleExecution.m_cumQty = jd->GetValue<double>("cummulativeQuoteQty");
		singleExecution.m_leavesQty = orderQty;
		execs.push_back(singleExecution);
	};
	
	if (auto array = jd->GetArray(arrayName))
	{
		Poco::Dynamic::Array fills = *array;
		auto numExecs { array->size() };
		
		if (numExecs == 0)
		{
			// Empty fills - just a single execution
			generateSingleExecutionFunc();
		}
		
		for (size_t i { 0 }; i < numExecs; ++i)
		{
			auto singleExecution = prepareExecReportFunc();
			
			singleExecution.m_lastPx = fills[i]["price"];
			singleExecution.m_lastQty = fills[i]["qty"];
			executedQty += singleExecution.m_lastQty;
			singleExecution.m_cumQty = executedQty;
			singleExecution.m_leavesQty = orderQty - executedQty;
			
			if (i < numExecs - 1)
			{
				singleExecution.m_ordStatus = ORDSTATUS_PARTIALLY_FILLED;
				singleExecution.m_execType = EXECTYPE_PARTIAL_FILL;
			}
			execs.push_back(singleExecution);
		}
	}
	else
	{
		// No fills - just a single execution
		generateSingleExecutionFunc();
	}
	return execs;
}

}
}
