#include <gtest/gtest.h>
#include <Utils/FixTypes.h>
#include "coinbase/ConnectionMD.h"

#include "TestHelpers.h"
#include "Options.h"

namespace TEST::COINBASE {

using TestConnectionMD = TestConnectionMD<CORE::COINBASE::ConnectionMD>;

//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_TranslateSymbol)
{
	// Arrange
	RegisterTestCurrencies();

	CORE::CRYPTO::Settings settings;
	TestConnectionMD conn(settings);
	
	// Check
	// (removes dash)
	ASSERT_EQ("", conn.TranslateSymbol(""));
	ASSERT_EQ("BTCUSD", conn.TranslateSymbol("BTC-USD"));
	ASSERT_EQ("BTCUSD", conn.TranslateSymbol("--BTC--USD--"));
	ASSERT_EQ("BTCUSD", conn.TranslateSymbol("BTCUSD"));
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_TranslateSymbolToExchangeSpecific)
{
	// Arrange
	RegisterTestCurrencies();
	CORE::CRYPTO::Settings settings;
	TestConnectionMD conn(settings);
	
	// Check
	// (adds dash)
	ASSERT_EQ("-", conn.TranslateSymbolToExchangeSpecific(""));
	ASSERT_EQ("BTC-USD", conn.TranslateSymbolToExchangeSpecific("BTC/USD"));
	ASSERT_EQ("BTC-USD", conn.TranslateSymbolToExchangeSpecific("BTCUSD"));
	ASSERT_EQ("-" + std::string(UTILS::INVALID_CURRENCY), conn.TranslateSymbolToExchangeSpecific("--BTC--USD--"));
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_GetInstruments)
{
	// Arrange
	RegisterTestCurrencies();
	CORE::CRYPTO::Settings settings;
	// (note: we test that empty substrings and spaces are processed correctly)
	settings.m_instruments = " , ,   bnbbtc ,bnbeth,,  ,  ethbtc,  ";
	TestConnectionMD conn(settings);
	
	// Act
	const auto instruments = conn.GetInstruments();
	
	// Check
	ASSERT_EQ(3, instruments.size());
	ASSERT_TRUE(instruments.find("BNB-BTC") != instruments.cend()); // note upper case
	ASSERT_TRUE(instruments.find("BNB-ETH") != instruments.cend());
	ASSERT_TRUE(instruments.find("ETH-BTC") != instruments.cend());
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_QuoteFeed_Processed_Success)
{
	// Arrange
	//--------
	RegisterTestCurrencies();

	CORE::CRYPTO::Settings settings;
	settings.m_numId = 4711; // should be used as session id
	TestConnectionMD conn(settings);
	conn.Connect();
	
	const std::string buyUpdate = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
								  "\"changes\":[[\"buy\",\"21954.52\",\"0.00994868\"]],\"time\":\"2022-07-18T14:26:56.696623Z\"}";
	const std::string sellUpdate = "{\"type\":\"l2update\",\"product_id\":\"BTC-GBP\","
								   "\"changes\":[[\"sell\",\"34703.87\",\"20.45500000\"]],\"time\":\"2022-07-18T14:26:57.63887Z\"}";
	const std::string buysellUpdate = "{\"type\":\"l2update\",\"product_id\":\"BTC-EUR\","
									  "\"changes\":[[\"buy\",\"17583.48\",\"0.001089\"],"
									  "[\"sell\",\"18520.55\",\"11.35400000\"]],\"time\":\"2022-07-18T14:27:58.34887Z\"}";
	
	// Act
	//----
	
	// Simulate receiving messages
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(buyUpdate)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(sellUpdate)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(buysellUpdate)));
	
	// Check
	//------
	
	const size_t expectedSize = 4;
	UTILS::WaitUntil([&conn]()
					 {
						 return conn.m_numberOfOnPublishQuoteBufferCalls >= expectedSize;
					 }, 1000);

}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_QuoteFeed_DifferentQuoteTypes_SameSide)
{
	// Arrange
	//--------
	RegisterTestCurrencies();

	CORE::CRYPTO::Settings settings;
	settings.m_numId = 4711; // should be used as session id
	TestConnectionMD conn(settings);
	conn.Connect();
	
	const std::string msgNew = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
							   "\"changes\":[[\"buy\",\"21954.52\",\"0.00994868\"]],\"time\":\"2022-07-18T14:26:56.696623Z\"}";
	const std::string msgUpdate = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
								  "\"changes\":[[\"buy\",\"21954.52\",\"0.00277333\"]],\"time\":\"2022-07-18T14:26:57.696623Z\"}";
	const std::string msgDelete = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
								  "\"changes\":[[\"buy\",\"21954.52\",\"0.0000\"]],\"time\":\"2022-07-18T14:26:58.696623Z\"}";
	
	// Act
	//----
	
	// Simulate receiving messages
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgNew)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgUpdate)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgDelete)));
	
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_QuoteFeed_DifferentQuoteTypes_DifferentSides)
{
	// Arrange
	//--------
	RegisterTestCurrencies();

	CORE::CRYPTO::Settings settings;
	settings.m_numId = 4711; // should be used as session id
	TestConnectionMD conn(settings);
	conn.Connect();
	
	const std::string msgNew = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
							   "\"changes\":[[\"buy\",\"21954.52\",\"0.00994868\"]],\"time\":\"2022-07-18T14:26:56.696623Z\"}";
	const std::string msgUpdate = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
								  "\"changes\":[[\"sell\",\"21954.52\",\"0.00277333\"]],\"time\":\"2022-07-18T14:26:57.696623Z\"}";
	const std::string msgDelete = "{\"type\":\"l2update\",\"product_id\":\"BTC-USD\","
								  "\"changes\":[[\"buy\",\"21954.52\",\"0.0000\"]],\"time\":\"2022-07-18T14:26:58.696623Z\"}";
	
	// Act
	//----
	
	// Simulate receiving messages
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgNew)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgUpdate)));
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgDelete)));
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_QuoteFeed_NotSupportedMessageType)
{
	// Arrange
	//--------
	RegisterTestCurrencies();
	CORE::CRYPTO::Settings settings;

	TestConnectionMD conn(settings);

	// Act
	//----
	
	// Simulate receiving not subscribed message
	const std::string msg = "{\"type\":\"unknown\",\"product_id\":\"BTC-USD\","
							"\"changes\":[[\"buy\",\"21954.52\",\"0.00994868\"]],\"time\":\"2022-07-18T14:26:56.696623Z\"}";

	const auto res = conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msg));
	
	// Check
	//------
	
	ASSERT_FALSE(res);
	ASSERT_EQ("Not supported message: 'unknown'", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_QuoteFeed_MessageTypeCannotBeDetected_BecauseTagIsMissed)
{
	RegisterTestCurrencies();
	CORE::CRYPTO::Settings settings;

	TestConnectionMD conn(settings);
	conn.Connect();
	
	// Simulate message with missing type tag
	const std::string msg = "{\"product_id\":\"BTC-USD\","
							"\"changes\":[[\"buy\",\"21954.52\",\"0.00994868\"]],\"time\":\"2022-07-18T14:26:56.696623Z\"}";
	const auto res = conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msg));
	
	ASSERT_FALSE(res);
	ASSERT_EQ("Not supported message: ''", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(CoinbaseConnection, Test_Snapshot)
{
	// Arrange
	//--------
	RegisterTestCurrencies();
	CORE::CRYPTO::Settings settings;
	settings.m_numId = 4711; // should be used as session id
	TestConnectionMD conn(settings);
	conn.Connect();
	
	const std::string msgSnapshot = "{\"type\":\"snapshot\",\"product_id\":\"BTC-GBP\","
									"\"asks\":[[\"19800.64\",\"0.10541151\"],[\"19801.91\",\"0.10541072\"]],"
									"\"bids\":[[\"19789.94\",\"0.08800000\"]]}";
	
	// Simulate receiving messages
	ASSERT_TRUE(conn.GetMessageProcessor().ProcessMessage(std::make_shared<CORE::CRYPTO::JSONDocument>(msgSnapshot)));
	
	const size_t expectedSize = 3; // 2 for asks and 1 for bids
	UTILS::WaitUntil([&conn]()
					 {
						 return conn.m_numberOfOnPublishQuoteBufferCalls >= expectedSize;
					 }, 1000);
}
	
} // ns