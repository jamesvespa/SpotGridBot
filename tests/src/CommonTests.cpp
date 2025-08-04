//
// Created by alex on 12/09/22.
//
#include <gtest/gtest.h>

#include "Tools.h"
#include "Crypto.h"
#include "JSONDocument.h"

namespace MERCURY::TEST {
//----------------------------------------------------------------------------
TEST(COMMON, Test_CreateEmptyExecutionReportData)
{
	// Arrange/Act
	const auto data = CORE::CRYPTO::TOOLS::CreateEmptyExecutionReportData();
	
	// Check
	ASSERT_EQ("", data.m_orderId);
	ASSERT_EQ("", data.m_clOrdID);
	ASSERT_EQ(ORDTYPE_MARKET, data.m_ordType);
	ASSERT_EQ(UTILS::Currency(), data.m_currency);
	ASSERT_EQ("", data.m_execID);
	ASSERT_EQ("", data.m_settlDate);
	ASSERT_EQ(EXECTYPE_NONE, data.m_execType);
	ASSERT_EQ(ORDSTATUS_NOTSENT, data.m_ordStatus);
	ASSERT_EQ(data.m_instrument.QtyToDouble(0), data.m_orderQty);
	ASSERT_EQ(0, data.m_orderPx);
	ASSERT_EQ(data.m_instrument.QtyToDouble(0), data.m_lastQty);
	ASSERT_EQ(0, data.m_lastPx);
	ASSERT_EQ(data.m_instrument.QtyToDouble(0), data.m_leavesQty);
	ASSERT_EQ(data.m_instrument.QtyToDouble(0), data.m_cumQty);
	ASSERT_EQ(0, data.m_avgPx);
	ASSERT_EQ("", data.m_text);
	ASSERT_EQ("", data.m_account);
	ASSERT_EQ("", data.m_orderText);
	ASSERT_EQ("", data.m_username);
	ASSERT_EQ("", data.m_quoteId);
}

//----------------------------------------------------------------------------
TEST(COMMON, Test_CreateJSONMessageWithCode)
{
	using namespace CORE::CRYPTO;
	
	ASSERT_EQ("{\"code\":1,\"msg\":\"hello there\"}", CreateJSONMessageWithCode("hello there"));
	ASSERT_EQ("{\"code\":0,\"msg\":\"\"}", CreateJSONMessageWithCode("", 0));
	ASSERT_EQ("{\"code\":-12345,\"msg\":\"error!!!\"}", CreateJSONMessageWithCode("error!!!", -12345));
}


//----------------------------------------------------------------------------
TEST(COMMON, Test_ParseJSONMessageWithCode_NotZeroCode)
{
	// Arrange
	using namespace CORE::CRYPTO;
	const std::string json = "{\"code\":123,\"msg\":\"hello there\"}";
	
	// Act
	const auto state = ParseJSONMessageWithCode(std::make_shared<JSONDocument>(json));
	
	// Check
	ASSERT_EQ(123, state.code);
	ASSERT_EQ(std::string("hello there"), state.msg);
}


//----------------------------------------------------------------------------
TEST(COMMON, Test_ParseJSONMessageWithCode_ZeroCode)
{
	// Arrange
	using namespace CORE::CRYPTO;
	const std::string json = "{\"code\":0,\"msg\":\"yay\"}";
	
	// Act
	const auto state = ParseJSONMessageWithCode(std::make_shared<JSONDocument>(json));
	
	// Check
	ASSERT_EQ(0, state.code);
	ASSERT_EQ(std::string("yay"), state.msg);
}


//----------------------------------------------------------------------------
TEST(COMMON, Test_ParseJSONMessageWithCode_BadJson_MsgMissing)
{
	// Arrange
	using namespace CORE::CRYPTO;
	const std::string badJson = "{\"code\":123}";
	
	// Act
	const auto state = ParseJSONMessageWithCode(std::make_shared<JSONDocument>(badJson));
	
	// Check
	ASSERT_EQ(123, state.code);
	ASSERT_EQ(std::string(""), state.msg);
}


//----------------------------------------------------------------------------
TEST(COMMON, Test_ParseJSONMessageWithCode_BadJson_CodeMissing)
{
	// Arrange
	using namespace CORE::CRYPTO;
	const std::string badJson = "{\"msg\":\"yay\"}";
	
	// Act
	const auto state = ParseJSONMessageWithCode(std::make_shared<JSONDocument>(badJson));
	
	// Check
	ASSERT_EQ(0, state.code);
	ASSERT_EQ(std::string("yay"), state.msg);
}
} // ns