#pragma once

//Shared definitions across connections
namespace CORE {
namespace CRYPTO {
const std::string MSGTYPE_Unknown = "!UNKNOWN!";
const std::string MSGTYPE_Error = "error";
const std::string MSGTYPE_Result = "result";

// Order status names
const std::string ORD_STATUS_NAME_NEW = "NEW";
const std::string ORD_STATUS_NAME_PARTIALLY_FILLED = "PARTIALLY_FILLED";
const std::string ORD_STATUS_NAME_FILLED = "FILLED";
const std::string ORD_STATUS_NAME_CANCELED = "CANCELED";
const std::string ORD_STATUS_NAME_REJECTED = "REJECTED";
const std::string ORD_STATUS_NAME_EXPIRED = "EXPIRED";
}
}