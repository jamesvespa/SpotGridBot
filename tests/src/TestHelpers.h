//
// Created by alex on 19/07/22.
//
#pragma once

#include "ConnectionBase.h"

#include "unittest.h"

namespace TEST {
const std::string ConfigPath {PATH_TEST_CONFIG};
const std::string LoggingProperties {PATH_TEST_LOGGINGPROPERTIES};

const long OrderMonitoringTimeoutMs = 3000;

const std::string NotFoundTopic = "Not found";

inline UTILS::QuoteType GetSide(int32_t side)
{
	return UTILS::QuoteType(side ? UTILS::QuoteType::BID : UTILS::QuoteType::OFFER);
}

/*! \brief Registers crypto currencies used in the tests */
void RegisterTestCurrencies();

/*! \brief Returns true if fld can be found in fields. If exact == true, full string is searched */
bool CheckRequestField(const Poco::StringTokenizer &fields, const std::string &fld, bool exact = true);


/*! \brief Returns rounded price for specified instrument */
inline double RoundPrice(const double price, const UTILS::CurrencyPair &instrument)
{
	return instrument.CpipToDbl(instrument.DblToCpip(price));
}

//--------------------------------------------------------------------------
/*! \brief Class to test Connection behaviour */
//--------------------------------------------------------------------------
template <typename TConnection>
class TestConnectionMD : public TConnection
{
public:
	TestConnectionMD(const CORE::CRYPTO::Settings &settings)
			: TConnection(settings, PATH_TEST_LOGGINGPROPERTIES) { }
	
	// Containers of received quotes and topics
	std::atomic<size_t> m_numberOfOnPublishQuoteBufferCalls { 0 };
	std::vector<std::string> m_webSocketSentPayloads;
	
	// Access to protected method for testing
	CORE::CRYPTO::MessageProcessor &GetMessageProcessor()
	{
		return TConnection::GetMessageProcessor();
	}
	
	/*! \brief Looks for a published quote in the internal buffer
	* @param symbol: currency pair like 'BTCUSDT'
	* @param qt: quote type UTILS::QuoteType::BID or UTILS::QuoteType::ASK
	* @param price: price
	* @param qty: quantity
	* @return tuple <topic:bbf quote>
	* */

protected:
//		std::string GenerateTimeStamp() const override //specific override for OKX
//		{
//			return CORE::CRYPTO::TOOLS::to_iso_string(UTILS::CurrentTimestamp());
//		}
	
	void CreateWebSocket() override
	{
		// Just do nothing
	}
	
	bool Send(const std::string &payload) override
	{
		m_webSocketSentPayloads.emplace_back(payload);
		return true;
	}
	
};

}