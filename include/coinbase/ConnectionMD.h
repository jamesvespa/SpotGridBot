#pragma once

#include "Config.h"
#include "ConnectionBase.h"

#include "coinbase/Messages.h"

namespace CORE {
namespace COINBASE {
const char *const SCHEMA = "Coinbase";
// Message types
const char *const MSG_TYPE_SNAPSHOT = "snapshot";
const char *const MSG_TYPE_L2UPDATE = "l2update";
const char *const MSG_TYPE_HEARTBEAT = "heartbeat";
const char *const MSG_TYPE_SUBSCRIPTIONS = "subscriptions";

typedef const std::tuple<std::string, std::string, std::string, std::string> AuthHeader;

class ConnectionMD : public CORE::CRYPTO::ConnectionBase
{
public:
	ConnectionMD(const CRYPTO::Settings &settings, const std::string &loggingPropsPath);

	UTILS::CurrencyPair GetCurrency(const std::shared_ptr<CRYPTO::JSONDocument> msg) const;

	std::string TranslateSymbol(const std::string &symbol) const override
	{
		return CORE::CRYPTO::TranslateSymbol(symbol);
	}
	
	std::string TranslateSymbolToExchangeSpecific(const std::string &symbol) const override
	{
		return CORE::CRYPTO::TranslateSymbolToExchangeSpecific(symbol);
	}

private:
	void Subscribe(const CRYPTO::ConnectionBase::TInstruments &instruments) override
	{
		Poco::StringTokenizer tok(GetSettings().m_channel, ",", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
		for (size_t i = 0; i < tok.count(); ++i)
		{
			Subscribe(instruments, "subscribe", tok[i]);
		}
	}
	
	void Unsubscribe(const CRYPTO::ConnectionBase::TInstruments &instruments) override
	{
		Poco::StringTokenizer tok(GetSettings().m_channel, ",", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
		for (size_t i = 0; i < tok.count(); ++i)
		{
			Subscribe(instruments, "unsubscribe", tok[i]);
		}
	}

	void Subscribe(const CRYPTO::ConnectionBase::TInstruments &instruments, const std::string &method, const std::string &channel);

	const AuthHeader GetAuthHeader();
};
}
}