//
// Created by alex on 20/07/22.
//

#include <gtest/gtest.h>
#include "coinbase/ConnectionMD.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Document.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/String.h>

#include "unittest.h"
#include "TestHelpers.h"
#include "SchemaDefs.h"


namespace MERCURY::TEST {
	using namespace CORE;

	//--------------------------------------------------------------------------
	TEST(ConfigTests, Test_Settings_ctor)
	{
		CRYPTO::Settings settings;
		ASSERT_EQ(0, settings.m_numId);
		ASSERT_EQ("", settings.m_name);
		ASSERT_EQ("", settings.m_host);
		ASSERT_EQ(0, settings.m_port);
		ASSERT_EQ("", settings.m_orders_http);
		ASSERT_EQ("", settings.m_apikey);
		ASSERT_EQ("", settings.m_secretkey);
		ASSERT_EQ(0, settings.m_recvWindow);
		ASSERT_EQ("", settings.m_channel);
		ASSERT_EQ("", settings.m_instruments);
		ASSERT_EQ(0, settings.m_parameters.size());
	}


	//--------------------------------------------------------------------------
	TEST(ConfigTests, Test_Settings_GetParameter)
	{
		// Arrange
		CRYPTO::Settings settings;
		const std::string name = "param name";
		const std::string value = "VALUE";
		const std::string defaultVal = "some default value";
		settings.m_parameters.emplace(name, value);

		// Check
		// Found, no default
		ASSERT_EQ(value, settings.GetParameter(name));
		// Found, with default
		ASSERT_EQ(value, settings.GetParameter(name, defaultVal));
		// Not found, with default
		ASSERT_EQ(defaultVal, settings.GetParameter("unknown", defaultVal));
		// Not found, no default
		ASSERT_EQ("", settings.GetParameter("unknown"));
	}
}