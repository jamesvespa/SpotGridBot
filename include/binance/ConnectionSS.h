//
// Created by james - Snapshot Connector used by TR Connector on 24/05/23.
//

#pragma once

#include "binance/Definitions.h"
#include "RestBase.h"
#include "JSONDocument.h"

namespace CORE {
namespace BINANCE {

//Dedicated connection for Binance Snapshots
class ConnectionSS final : public CORE::RESTAPI::RestBase
{
public:
	ConnectionSS(const std::string &loggerName)
			: RestBase(loggerName) { }

/*! \brief returns snapshot string message obtained from specified url
* @param url: url where to request a snapshot from (like https://api.binance.com/api/v3/depth?symbol=<INSTRUMENT>&limit=1000)
* @return: snapshot payload as a string
* */
	std::string GetSnapshot(const std::string &url);
};

} // ns BINANCE
}