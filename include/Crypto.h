//
// Created by james on 04/10/22.
//
#pragma once

#include "Utils/Utils.h"
#include "Utils/CurrencyPair.h"

namespace CORE {
namespace CRYPTO {

const std::string WSPostFixURL = "/ws/v5/public";

////////////////////////////////////////////////////////////////////////////////
// Error Codes
////////////////////////////////////////////////////////////////////////////////

// Error codes
enum
{
	ERROR_THROTTLE_LIMIT_ALREADY_EXISTS = 0, ERROR_THROTTLE_LIMIT_NOT_FOUND, ERROR_THROTTLE_LIMIT_BREACHED,
};

////////////////////////////////////////////////////////////////////////////
// Crypto inspired Order types
////////////////////////////////////////////////////////////////////////////

// Supported order types
enum class EOrderType
{
	INVALID, LIMIT, LIMIT_MAKER, MARKET, STOP_LOSS, STOP_LOSS_LIMIT, TAKE_PROFIT, TAKE_PROFIT_LIMIT
}; // EOrderType

const std::string ORDER_TYPE_INVALID = "INVALID";
const std::string ORDER_TYPE_LIMIT = "LIMIT";
const std::string ORDER_TYPE_LIMIT_MAKER = "LIMIT_MAKER";
const std::string ORDER_TYPE_MARKET = "MARKET";
const std::string ORDER_TYPE_STOP_LOSS = "STOP_LOSS";
const std::string ORDER_TYPE_STOP_LOSS_LIMIT = "STOP_LOSS_LIMIT";
const std::string ORDER_TYPE_TAKE_PROFIT = "TAKE_PROFIT";
const std::string ORDER_TYPE_TAKE_PROFIT_LIMIT = "TAKE_PROFIT_LIMIT";

//struct OrderTypeToStrMap : public std::map<std::string, EOrderType>
//{
//	OrderTypeToStrMap()
//	{
//		emplace(ORDER_TYPE_LIMIT, EOrderType::LIMIT);
//		emplace(ORDER_TYPE_LIMIT_MAKER, EOrderType::LIMIT_MAKER);
//		emplace(ORDER_TYPE_MARKET, EOrderType::MARKET);
//		emplace(ORDER_TYPE_STOP_LOSS, EOrderType::STOP_LOSS);
//		emplace(ORDER_TYPE_STOP_LOSS_LIMIT, EOrderType::STOP_LOSS_LIMIT);
//		emplace(ORDER_TYPE_TAKE_PROFIT, EOrderType::TAKE_PROFIT);
//		emplace(ORDER_TYPE_TAKE_PROFIT_LIMIT, EOrderType::TAKE_PROFIT_LIMIT);
//	}
//}; // TOrderTypeToStrMap
//
//// Converts string to order type
//inline EOrderType StrToOrderType(const std::string &str)
//{
//	return OrderTypeToStrMap-> str, EOrderType::INVALID);
//}
//
//// Converts order type to string
//inline std::string OrderTypeToStr(const EOrderType orderType)
//{
//	return GetStaticType<COrderTypeToStrMap>().GetKey(orderType, ORDER_TYPE_INVALID);
//}


////////////////////////////////////////////////////////////////////////////////
// Crypto inspired Permissions
////////////////////////////////////////////////////////////////////////////////

enum class EPermission
{
	INVALID, SPOT, MARGIN
}; // EPermission

const std::string PERMISSION_TYPE_INVALID = "INVALID";
const std::string PERMISSION_TYPE_SPOT = "SPOT";
const std::string PERMISSION_TYPE_MARGIN = "MARGIN";

// Converts string to permission type
EPermission StrToPermission(const std::string &str);

// Converts permission type to string
std::string PermissionToStr(const EPermission perm);


////////////////////////////////////////////////////////////////////////////////
// Rate limits
////////////////////////////////////////////////////////////////////////////////


// Supported rate limits
enum class ERateLimit
{
	INVALID, SECOND, MINUTE, HOUR, DAY
}; // ERateLimit

const std::string RATE_LIMIT_INVALID = "INVALID";
const std::string RATE_LIMIT_SECOND = "SECOND";
const std::string RATE_LIMIT_MINUTE = "MINUTE";
const std::string RATE_LIMIT_HOUR = "HOUR";
const std::string RATE_LIMIT_DAY = "DAY";

// Coinbase specific translation
inline std::string TranslateSymbol(const std::string &symbol)
{
	// symbol has a currency string in a shape CCY1-CCY2, for example "BTC-USD"
	auto productId = symbol;
	// Remove separator
	productId.erase(std::remove(productId.begin(), productId.end(), '-'), productId.end());
	return productId;
}

// Translates to Coinbase specific currency
inline std::string TranslateSymbolToExchangeSpecific(const UTILS::CurrencyPair &cp)
{
	return cp.BaseCCY().ToString() + "-" + cp.QuoteCCY().ToString();
}

inline std::string TranslateSymbolToExchangeSpecific(const std::string &symbol)
{
	return TranslateSymbolToExchangeSpecific(UTILS::CurrencyPair(symbol));
}

inline std::string ToString(const UTILS::CurrencyPair &instrument)
{
	return CORE::CRYPTO::TranslateSymbolToExchangeSpecific(instrument);
}

} // ns CRYPTO
} // ns CORE