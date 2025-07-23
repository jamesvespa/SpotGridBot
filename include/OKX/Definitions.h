//
// Created by james on 30/05/23.
//
#include <string>

#pragma once

namespace CORE {
namespace OKX {

const std::string SCHEMA = "OKX";
// Message types
const std::string MSGTYPE_Unknown      = "!UNKNOWN!";
const std::string MSGTYPE_Snapshot     = "snapshot";
const std::string MSGTYPE_Update       = "update";
const std::string MSGTYPE_Subscribe    = "subscribe";
const std::string MSGTYPE_Unsubscribe    = "unsubscribe";

// Parameters attributes
const std::string PARAM_ATTR_Passphrase        = "passphrase";
const std::string PARAM_ATTR_SimulatedTrading  = "x-simulated-trading";
const std::string PARAM_ATTR_CancelOrderHttp   = "cancel_order_http";
const std::string PARAM_ATTR_OrdersLimitPeriod = "orders_limit_period_in_seconds";
const std::string PARAM_ATTR_OrdersLimitMax    = "orders_limit_max";
const std::string PARAM_ATTR_SystemTimeHttp    = "system_time_http";
const std::string PARAM_ATTR_TDMode            = "tdMode";

// Default limits for orders and cancel orders
const int DefaultOrdersLimitPeriodInSec = 2;
const int DefaultOrdersLimitMax = 60;

// Request fields
const std::string REQUEST_FLD_CONTENT_TYPE      = "CONTENT-TYPE";
const std::string REQUEST_FLD_ACCESS_KEY        = "OK-ACCESS-KEY";
const std::string REQUEST_FLD_ACCESS_SIGN       = "OK-ACCESS-SIGN";
const std::string REQUEST_FLD_ACCESS_TIMESTAMP  = "OK-ACCESS-TIMESTAMP";
const std::string REQUEST_FLD_ACCESS_PASSPHRASE = "OK-ACCESS-PASSPHRASE";
const std::string REQUEST_FLD_SIMULATED_TRADING = "x-simulated-trading";
// this field is used for automated testing only (plays no role in actual request)
const std::string REQUEST_FLD_TEST_BODY         = "_test_body_";

// Content type used in REQUEST_FLD_CONTENT_TYPE
const std::string APP_JSON_CONTENT_TYPE = "application/json";

// Order status names
const std::string ORD_STATUS_LIVE             = "live";
const std::string ORD_STATUS_PARTIALLY_FILLED = "partially_filled";
const std::string ORD_STATUS_FILLED           = "filled";
const std::string ORD_STATUS_CANCELED         = "canceled";

}
};