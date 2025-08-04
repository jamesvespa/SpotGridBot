#include <gtest/gtest.h>
#include "binance/ConnectionMD.h"
#include "JSONDocument.h"

namespace MERCURY::TEST {
using namespace CORE::CRYPTO;

namespace {
const std::string TestJSON = "{\"name\":\"mkyong.com\",\"messages\":[\"msg 1\",\"msg 2\",\"msg 3\"],\"age\":100}";
const auto DummyHandler = [](const std::shared_ptr<CORE::CRYPTO::JSONDocument>) { };
} // anon ns

//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_Register)
{
	// Arrange
	MessageProcessor mp;
	ASSERT_EQ(0, mp.Size()); // no handlers registered yet
	
	// Act
	const auto res1 = mp.Register("type1", DummyHandler);
	ASSERT_EQ(1, mp.Size());
	const auto res2 = mp.Register("type2", DummyHandler);
	ASSERT_EQ(2, mp.Size());
	const auto res3 = mp.Register("type1", DummyHandler); // duplicate, should fail
	ASSERT_EQ(2, mp.Size()); // another handler not added
	
	// Check
	ASSERT_TRUE(res1);
	ASSERT_TRUE(res2);
	ASSERT_FALSE(res3);
	ASSERT_EQ("Handler for message 'type1' has been already registered. Ignored", res3.ErrorMessage());
	ASSERT_TRUE(mp.FindMessageHandler("type1"));
	ASSERT_TRUE(mp.FindMessageHandler("type2"));
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_Register_WithNullHandler)
{
	// Arrange
	MessageProcessor mp;
	ASSERT_EQ(0, mp.Size()); // no handlers registered yet
	
	// Act
	const auto res = mp.Register("type1", nullptr);
	
	// Check
	ASSERT_FALSE(res);
	ASSERT_EQ("NULL message handler ignored", res.ErrorMessage());
	ASSERT_EQ(0, mp.Size()); // not added
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_FindMessageHandler)
{
	// Arrange
	MessageProcessor mp;
	ASSERT_TRUE(mp.Register("type1", DummyHandler));
	
	// Act/Check
	ASSERT_TRUE(mp.FindMessageHandler("type1"));
	ASSERT_FALSE(mp.FindMessageHandler("type2")); // this type not registered
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_Enqueue_HandlerCalledWhenEnqueued)
{
	// Arrange
	MessageProcessor mp;
	auto jd = std::make_shared<JSONDocument>(TestJSON);
	UTILS::CEvent called;
	mp.Start();
	
	// Act
	ASSERT_TRUE(mp.Enqueue(jd, [&jd, &called](const std::shared_ptr<JSONDocument> doc)
	{
		EXPECT_EQ(jd.get(), doc.get()); // must be the same buffer
		called.Set();
	}));
	
	// Check
	ASSERT_TRUE(called.Wait(1000)); // handler must be called
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_Enqueue_FailsWithNullMessage)
{
	// Arrange
	MessageProcessor mp;
	mp.Start();
	
	// Act
	const auto res = mp.Enqueue(nullptr, DummyHandler);
	
	// Check
	ASSERT_FALSE(res);
	ASSERT_EQ("NULL message ignored", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_Enqueue_FailsWithNullHandler)
{
	// Arrange
	MessageProcessor mp;
	auto jd = std::make_shared<JSONDocument>(TestJSON);
	mp.Start();
	
	// Act
	const auto res = mp.Enqueue(jd, nullptr);
	
	// Check
	ASSERT_FALSE(res);
	ASSERT_EQ("NULL message handler ignored", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_ProcessMessage_HandlerCalled_Success)
{
	// Arrange
	MessageProcessor mp;
	UTILS::CEvent called;
	mp.Register([](const std::shared_ptr<CORE::CRYPTO::JSONDocument> message)
				{
					return message->GetValue<std::string>("e");
				});
	ASSERT_TRUE(mp.Register("test", [&called](const std::shared_ptr<JSONDocument> doc)
	{
		EXPECT_EQ(123, doc->GetValue<int>("value"));
		called.Set();
	}));
	mp.Start();
	
	// Act
	const std::string json = "{\"e\":\"test\",\"value\":123}";
	auto jd = std::make_shared<JSONDocument>(json);
	ASSERT_TRUE(mp.ProcessMessage(jd));
	
	// Check
	ASSERT_TRUE(called.Wait(1000)); // handler must be called
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_ProcessMessage_Fails_ForNotRegisteredMessage)
{
	// Arrange
	MessageProcessor mp;
	ASSERT_TRUE(mp.Register("some type", DummyHandler)); // note: different message type
	mp.Start();
	
	// Act
	const std::string json = "{\"e\":\"test\",\"value\":123}"; // note: message type is "test" - not registered
	auto jd = std::make_shared<JSONDocument>(json);
	const auto res = mp.ProcessMessage(jd);
	
	// Check
	ASSERT_FALSE(res); // handler must be called
	ASSERT_EQ("Not supported message: 'Message type detector not registered.'", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_ProcessMessage_Fails_BecauseNoDetectorSpecified)
{
	// Arrange
	MessageProcessor mp;
	ASSERT_TRUE(mp.Register("test", DummyHandler));
	mp.Start();
	
	// Act
	const std::string json = "{\"e\":\"test\",\"value\":123}";
	auto jd = std::make_shared<JSONDocument>(json);
	const auto res = mp.ProcessMessage(jd);
	
	// Check
	ASSERT_FALSE(res); // handler must be called
	ASSERT_EQ("Not supported message: 'Message type detector not registered.'", res.ErrorMessage());
}


//--------------------------------------------------------------------------
TEST(MessageProcessor, Test_ProcessMessage_Fails_WithNullMessage)
{
	// Arrange
	MessageProcessor mp;
	
	// Act
	const auto res = mp.ProcessMessage(nullptr);
	
	// Check
	ASSERT_FALSE(res); // handler must be called
	ASSERT_EQ("NULL message", res.ErrorMessage());
}

} // ns MERCURY::TEST