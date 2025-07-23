//
// Created by james on 24/05/23.
//

#include "Utils/Utils.h"
#include "binance/ConnectionSS.h"

namespace CORE {
namespace BINANCE {

/*! \brief returns snapshot string message obtained from specified url
* @param url: url where to request a snapshot from (like https://api.binance.com/api/v3/depth?symbol=<INSTRUMENT>&limit=1000)
* @return: snapshot payload as a string
* */
std::string ConnectionSS::GetSnapshot(const std::string &url)
{
	return ExecuteWebRequest(url, Poco::Net::HTTPRequest::HTTP_GET, nullptr, nullptr, nullptr);
}

}
}