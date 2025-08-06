#include "RestConnectionBase.h"

#include "Tools.h"

#include "Utils/Result.h"

#include "Poco/URI.h"

#include "OKX/ConnectionORD.h"

#include "Crypto.h"
#include "libbase64.hpp"

using namespace UTILS;
using namespace CORE;

namespace CORE {
namespace OKX {

//------------------------------------------------------------------------------
std::string Sign(const std::string &timeStampStr, const std::string &requestType, const std::string &requestPath, const std::string &requestBody,
				 const std::string &secretkeyOrd)
{
	const auto str = timeStampStr + requestType + requestPath + requestBody;
	const auto digest = HMAC(EVP_sha256(), secretkeyOrd.c_str(), secretkeyOrd.size(), (unsigned char *) str.c_str(), str.size(), nullptr, nullptr);
	return libbase64::encode<std::string, char, unsigned char, true>(digest, SHA256_DIGEST_LENGTH);
}


//------------------------------------------------------------------------------
std::string ToString(const UTILS::CurrencyPair &instrument)
{
	return CRYPTO::TranslateSymbolToExchangeSpecific(instrument);
}

//------------------------------------------------------------------------------
ConnectionORD::ConnectionORD(const CRYPTO::Settings &settings, const std::string& loggingPropsPath, const ConnectionManager& connectionManager)
		: RESTAPI::RestConnectionBase(settings, loggingPropsPath, settings.m_name)
{
	GetMessageProcessor().Register([](const std::shared_ptr<CRYPTO::JSONDocument> jd)
								   {
									   // Try checking type field 'action'
									   auto msgType = jd->GetValue<std::string>("action");
									   if (!msgType.empty())
									   {
										   return msgType;
									   } // it was simple - type was in the field
		
									   // Try checking type field 'event'
									   msgType = jd->GetValue<std::string>("event");
									   if (!msgType.empty())
									   {
										   return msgType;
									   }
									   return MSGTYPE_Unknown;
								   });
	
}

ConnectionORD::OrderActionResult ConnectionORD::GetOrderActionResult(const std::string &json) const
{
	OrderActionResult res;
	try
	{
		return GetOrderActionResult(std::make_shared<CRYPTO::JSONDocument>(json));
		
	}
	catch (const std::exception &e)
	{
		res.State = CRYPTO::JSONError(std::string(e.what()), 1);
	}
	
	return res;
}


ConnectionORD::OrderActionResult ConnectionORD::GetOrderActionResult(const std::shared_ptr<CRYPTO::JSONDocument>(jd)) const
{
	ConnectionORD::OrderActionResult res;

	res.State = CRYPTO::ParseJSONMessageWithCode(jd);
	res.Data = jd->GetArray("data");
	return res;
}

//------------------------------------------------------------------------------
std::string ConnectionORD::SendOrder(const UTILS::CurrencyPair &instrument, const UTILS::Side side, const RESTAPI::EOrderType orderType,
									const UTILS::TimeInForce timeInForce, const double price, const double quantity, const std::string &clientOrderId)
{
	const auto isMarketOrder = ((RESTAPI::EOrderType) orderType == RESTAPI::EOrderType::Market || !price);
	std::string body = "{";
	body += UTILS::Format("\"instId\":\"%s\",", CRYPTO::TranslateSymbolToExchangeSpecific(instrument));
	if (!clientOrderId.empty())
	{
		body += UTILS::Format("\"clOrdId\":\"%s\",", clientOrderId);
	}
	body += UTILS::Format("\"tdMode\":\"%s\",", GetSettings().GetParameter(PARAM_ATTR_TDMode));
	body += UTILS::Format("\"side\":\"%s\",", std::string(side.Buy() ? "buy" : "sell"));
	
	std::string tif;
	switch (timeInForce.Index())
	{
		case 3: // '3': Immediate or Cancel
			tif = "ioc";
			break;
		case 4: // '4': Fill or Kill
			tif = "fok";
			break;
		default:
			tif = isMarketOrder ? "market" : "limit";
	}
	
	body += UTILS::Format("\"ordType\":\"%s\",", tif);
	if (!isMarketOrder)
	{
		body += UTILS::Format("\"px\":\"%s\",", UTILS::to_string_with_precision<double>(price, instrument.Precision()));
	}
	body += UTILS::Format("\"sz\":\"%s\"", std::to_string(quantity));
	body += "}";
	
	const auto timeStampStr = GenerateTimeStamp();
	std::string signature;
	
	poco_information_f2(logger(), "OK-ACCESS-TIMESTAMP: %s SendOrder: %s", timeStampStr, body);
	const auto js =
			DoWebRequest(GetSettings().m_orders_http, Poco::Net::HTTPRequest::HTTP_POST, [this, &signature, &timeStampStr, &body](std::string &path)
			{
				// Make up signature here using path
				signature = Sign(timeStampStr, Poco::Net::HTTPRequest::HTTP_POST, path, body, GetSettings().m_secretkey);
			}, [this, &signature, &timeStampStr, &body](Poco::Net::HTTPRequest &request)
						 {
							 request.add(REQUEST_FLD_CONTENT_TYPE, APP_JSON_CONTENT_TYPE);
							 request.add(REQUEST_FLD_ACCESS_KEY, GetSettings().m_apikey);
							 request.add(REQUEST_FLD_ACCESS_SIGN, signature);
							 request.add(REQUEST_FLD_ACCESS_PASSPHRASE, GetSettings().GetParameter(PARAM_ATTR_Passphrase));
							 request.add(REQUEST_FLD_ACCESS_TIMESTAMP, timeStampStr);
							 request.add(REQUEST_FLD_SIMULATED_TRADING, GetSettings().GetParameter(PARAM_ATTR_SimulatedTrading));
							 request.add(REQUEST_FLD_TEST_BODY, body);
							 request.setContentLength(body.size());
						 }, [&body](std::ostream &ostr)
						 {
							 // The body must be passed directly to request stream
							 ostr << body;
						 });
	
	try
	{
		const auto res = GetOrderActionResult(js);
		if (res.State.code)
		{
			return CRYPTO::CreateJSONMessageWithCode(res.State.msg, res.State.code);
		}
		if (res.Data)
		{
			if (res.Data->size() > 0)
			{
				Poco::Dynamic::Array dynArray = *res.Data;
				// Only one element expected
				const auto &s = dynArray[0];
				const auto code = s["sCode"].convert<long>();
				const auto msg = s["sMsg"].toString();
				if (code != 0)
				{
					// Server error
					return CRYPTO::CreateJSONMessageWithCode(msg, code);
				}
				// We have no error - query order details
				poco_information_f1(logger(), "Success: %s. Now querying the order details...", msg);
				return QueryOrder(instrument, s["ordId"].toString());
			} // if
			throw std::runtime_error("data[] is empty");
		} // if
		throw std::runtime_error("data[] not found");
	} // try
	catch (const std::exception &e)
	{
		poco_error_f1(logger(), "Error: %s", std::string(e.what()));
		return CRYPTO::CreateJSONMessageWithCode(e.what());
	}
}

//------------------------------------------------------------------------------
//We need to get the OKX exchange time
int64_t ConnectionORD::GetSystemTime() const
{
	CRYPTO::JSONDocument
			jd(ExecuteWebRequest(m_settings.GetParameter(PARAM_ATTR_SystemTimeHttp), Poco::Net::HTTPRequest::HTTP_GET, nullptr, nullptr));
	auto data = jd.GetArray("data");
	Poco::Dynamic::Array dynArray = *data;
	const auto &ts = dynArray[0]; //Contains one entry only
	return ts["ts"].convert<long>();
}

//------------------------------------------------------------------------------
std::string ConnectionORD::QueryOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
									 const std::optional<std::string> &origClientOrderId)
{
	const auto timeStampStr = GenerateTimeStamp();
	std::string signature;
	return DoWebRequest(GetSettings().m_orders_http, Poco::Net::HTTPRequest::HTTP_GET,
						[this, &instrument, &orderId, &origClientOrderId, &signature, &timeStampStr](std::string &path)
						{
							path += "?ordId=" + orderId + (origClientOrderId.has_value() ? ("&clOrdId=" + origClientOrderId.value())
																						 : "") + "&instId=" + ToString(instrument);
							signature = Sign(timeStampStr, Poco::Net::HTTPRequest::HTTP_GET, path, "", GetSettings().m_secretkey);
							poco_information_f1(logger(), "QueryOrder: %s", path);
						}, [this, &signature, &timeStampStr](Poco::Net::HTTPRequest &request)
						{
							request.add(REQUEST_FLD_CONTENT_TYPE, APP_JSON_CONTENT_TYPE);
							request.add(REQUEST_FLD_ACCESS_KEY, GetSettings().m_apikey);
							request.add(REQUEST_FLD_ACCESS_SIGN, signature);
							request.add(REQUEST_FLD_ACCESS_TIMESTAMP, timeStampStr);
							request.add(REQUEST_FLD_ACCESS_PASSPHRASE, GetSettings().GetParameter(PARAM_ATTR_Passphrase));
							request.add(REQUEST_FLD_SIMULATED_TRADING, GetSettings().GetParameter(PARAM_ATTR_SimulatedTrading));
						});
}


//------------------------------------------------------------------------------
std::string ConnectionORD::CancelOrder(const UTILS::CurrencyPair &instrument, const std::string &orderId,
									  const std::optional<std::string> &origClientOrderId)
{
	std::string body = "{";
	body += "\"ordId\":" + orderId + ",";
	if (origClientOrderId.has_value())
	{
		body += UTILS::Format("\"clOrdId\":\"%s\",", origClientOrderId.value());
	}
	body += UTILS::Format("\"instId\":\"%s\"", ToString(instrument));
	body += "}";
	poco_information_f1(logger(), "<<< CancelOrder: %s", body);
	
	const auto timeStampStr = GenerateTimeStamp();
	std::string signature;
	
	return DoWebRequest(GetSettings().GetParameter(PARAM_ATTR_CancelOrderHttp), Poco::Net::HTTPRequest::HTTP_POST,
						[this, &signature, &timeStampStr, &body](std::string &path)
						{
							// Make up signature here using path
							signature = Sign(timeStampStr, Poco::Net::HTTPRequest::HTTP_POST, path, body, GetSettings().m_secretkey);
						}, [this, &signature, &timeStampStr, &body](Poco::Net::HTTPRequest &request)
						{
							request.add(REQUEST_FLD_CONTENT_TYPE, APP_JSON_CONTENT_TYPE);
							request.add(REQUEST_FLD_ACCESS_KEY, GetSettings().m_apikey);
							request.add(REQUEST_FLD_ACCESS_SIGN, signature);
							request.add(REQUEST_FLD_ACCESS_TIMESTAMP, timeStampStr);
							request.add(REQUEST_FLD_ACCESS_PASSPHRASE, GetSettings().GetParameter(PARAM_ATTR_Passphrase));
							request.add(REQUEST_FLD_SIMULATED_TRADING, GetSettings().GetParameter(PARAM_ATTR_SimulatedTrading));
							request.add(REQUEST_FLD_TEST_BODY, body);
							request.setContentLength(body.size());
						}, [&body](std::ostream &ostr)
						{
							// The body must be passed directly to request stream
							ostr << body;
						});
}


//==============================================================================
// Binance web request wrapper
std::string ConnectionORD::DoWebRequest(const std::string &url, const std::string &requestType,
									   std::function<void(std::string &path)> customizeRequestPathFunc,
									   std::function<void(Poco::Net::HTTPRequest &request)> customizeRequestFunc,
									   std::function<void(std::ostream &)> handleRequestStreamFunc)
{
	return ExecuteWebRequest(url, requestType, customizeRequestPathFunc, customizeRequestFunc, nullptr, handleRequestStreamFunc);
}


//------------------------------------------------------------------------------
/*! \brief generates specifically formatted time stamp string sourcing
 * time from OKX and converting to UTC.
* @return: UTC time stamp from OKX string in format "2020-12-08T09:08:57.000Z"
* */
std::string ConnectionORD::GenerateTimeStamp() const
{
	return CRYPTO::TOOLS::to_iso_string(GetSystemTime()); //Convert OKX to UTC
}

//------------------------------------------------------------------------------
/*! \brief Translates order result
* @return: ExecutionReportData
* */
RESTAPI::RestConnectionBase::TExecutionReports ConnectionORD::TranslateOrderResult(const std::shared_ptr<CRYPTO::JSONDocument> jd) const
{
	return TranslateOrder(jd, &logger());
}

// static
RESTAPI::RestConnectionBase::TExecutionReports ConnectionORD::TranslateOrder(const std::shared_ptr<CRYPTO::JSONDocument> jd, Poco::Logger *logger) const
{
	TExecutionReports fullData;
	
	auto res = CRYPTO::TOOLS::CreateEmptyExecutionReportData();
	const auto oar = GetOrderActionResult(jd);
	if (oar.State.code)
	{
		// It's an error
		res.m_text = UTILS::Format("The order has failed: Error code='%d', message='%s'", int(oar.State.code), oar.State.msg);
		fullData.push_back(res);
		return fullData;
	}
	
	if (!oar.Data)
	{
		res.m_text = "'data' is missing or not an array";
		fullData.push_back(res);
		return fullData;
	}
	
	Poco::Dynamic::Array dynArray = *oar.Data;
	if (dynArray.empty())
	{
		res.m_text = "'data' is empty";
		fullData.push_back(res);
		return fullData;
	}
	
	// Only one element expected
	const auto &elem = dynArray[0];
	
	// Ok, we have got a response result
	// Expected result looks so
	/*
		{
			"code":"0",
			"data":
			[{
				"accFillSz":"0",
				"avgPx":"",
				"cTime":"1670493761203",
				"cancelSource":"",
				"cancelSourceReason":"",
				"category":"normal",
				"ccy":"",
				"clOrdId":"",
				"fee":"0",
				"feeCcy":"BTC",
				"fillPx":"",
				"fillSz":"0",
				"fillTime":"",
				"instId":"BTC-USDT",
				"instType":"SPOT",
				"lever":"",
				"ordId":"521023829901148160",
				"ordType":"limit",
				"pnl":"0",
				"posSide":"net",
				"px":"10000",
				"quickMgnType":"",
				"rebate":"0",
				"rebateCcy":"USDT",
				"reduceOnly":"false",
				"side":"buy",
				"slOrdPx":"",
				"slTriggerPx":"",
				"slTriggerPxType":"",
				"source":"",
				"state":"live",
				"sz":"0.001",
				"tag":"",
				"tdMode":"cash",
				"tgtCcy":"",
				"tpOrdPx":"",
				"tpTriggerPx":"",
				"tpTriggerPxType":"",
				"tradeId":"",
				"uTime":"1670493761203"
			}],
			"msg":""
		}
	*/
	
	res.m_orderId = elem["ordId"].convert<std::string>();
	res.m_clOrdID = elem["clOrdId"].convert<std::string>();
	
	const auto ordStatus = elem["state"].convert<std::string>();
	if (ordStatus == ORD_STATUS_LIVE)
	{
		res.m_ordStatus = ORDSTATUS_NEW;
		res.m_execType = EXECTYPE_NEW;
	}
	else if (ordStatus == ORD_STATUS_PARTIALLY_FILLED)
	{
		res.m_ordStatus = ORDSTATUS_PARTIALLY_FILLED;
		res.m_execType = EXECTYPE_PARTIAL_FILL;
	}
	else if (ordStatus == ORD_STATUS_FILLED)
	{
		res.m_ordStatus = ORDSTATUS_FILLED;
		res.m_execType = EXECTYPE_FILL;
	}
	else if (ordStatus == ORD_STATUS_CANCELED)
	{
		res.m_ordStatus = ORDSTATUS_CANCELED;
		res.m_execType = EXECTYPE_CANCELED;
	}
	else // for any unknown state we consider it "REJECTED"
	{
		res.m_ordStatus = ORDSTATUS_REJECTED;
		res.m_execType = EXECTYPE_REJECTED;
	}
	
	const auto ordType = elem["ordType"].convert<std::string>();
	res.m_ordType = ordType == "market" ? ORDTYPE_MARKET : ORDTYPE_LIMIT;
	res.m_instrument = UTILS::CurrencyPair(CRYPTO::TranslateSymbol(elem["instId"].convert<std::string>()));
	res.m_currency = res.m_instrument.QuoteCCY();

	res.m_settlDate = UTILS::FormatTimestamp(TSF_SETTLDATE, elem["uTime"].convert<int64_t>() * 1000000);
	
	const auto side = elem["side"].convert<std::string>();
	res.m_side = side == "sell" ? UTILS::Side::SELL : side == "buy" ? UTILS::Side::BUY : UTILS::Side::INVALID;
	
	// Helper: return double value or 0 from field
	const auto getValFunc = [&elem](const std::string &name)
	{
		try
		{
			return elem[name].convert<double>();
		}
		catch (...)
		{
			std::cout << "ERROR unable to convert << " << name << " setting to 0" << std::endl;
		}
		
		return 0.0;
	};
	res.m_orderQty = getValFunc("sz");
	res.m_lastQty = getValFunc("fillSz");
	res.m_cumQty = res.m_lastQty; // OKX doesn't have cumulative qty
	res.m_orderPx = getValFunc("px");
	res.m_lastPx = getValFunc("fillPx");
	
	// order types "market", "limit", "post_only" are treated as GTC
	res.m_tif = ordType == "fok" ? UTILS::TimeInForce::FOK : ordType == "ioc" || ordType == "optimal_limit_ioc" ? UTILS::TimeInForce::IOC
																												: UTILS::TimeInForce::GTC;
	
	fullData.push_back(res);
	return fullData;
}

}
}