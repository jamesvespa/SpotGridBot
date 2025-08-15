#include "ConnectionBase.h"
#include "CryptoCommon.h"

#include "coinbase/Messages.h"
#include "coinbase/ConnectionMD.h"
#include "Poco/URI.h"

#include "libbase64.hpp"

using namespace UTILS;

namespace CORE {
    namespace COINBASE {
        //----------------------------------------------------------------------
        ConnectionMD::ConnectionMD(const CRYPTO::Settings &settings, const std::string &loggingPropsPath, const ConnectionManager& connectionManager)
            : ConnectionBase(settings, loggingPropsPath, settings.m_name, connectionManager) {

            GetMessageProcessor().Register([](const std::shared_ptr<CRYPTO::JSONDocument> message)
                                            {
                                                return message->GetValue<std::string>("type");
                                            });

            // Register messages
            GetMessageProcessor().Register(MSG_TYPE_SNAPSHOT, [this](const std::shared_ptr<CRYPTO::JSONDocument> jd) {
                auto cp = GetCurrency(jd);
                if (!cp.Valid()) {
                    poco_error(logger(), "Invalid (or not supported) instrument ignored");
                    return;
                }
                const auto update = ParseMessage(jd, "bids", "asks");
                PublishQuotes(ParseQuote(update->Bids, QuoteType::BID, cp));
                PublishQuotes(ParseQuote(update->Asks, QuoteType::OFFER, cp));

                poco_information_f2(logger(), "QT_SNAPSHOT %s bid Levels: %d ", cp.ToString(), int(update->Bids.size()));
                poco_information_f2(logger(), "QT_SNAPSHOT %s ask Levels: %d ", cp.ToString(), int(update->Asks.size()));
            });

            GetMessageProcessor().Register(MSG_TYPE_L2UPDATE, [this](const std::shared_ptr<CRYPTO::JSONDocument> jd) {
                auto cp = GetCurrency(jd);
                if (!cp.Valid()) {
                    poco_error(logger(), "Invalid (or not supported) instrument - ignored");
                    return;
                }
                const auto publishFunc = [&cp, this](const std::vector<change::Ptr> changes) {
                    for (const auto &iter: changes) {
                        std::vector level{std::make_shared<CORE::CRYPTO::Level>(iter->price, iter->size)};
                        PublishQuotes(ParseQuote(level, (iter->side == "buy" ? QuoteType::BID : QuoteType::OFFER), cp));
                    }
                };

                publishFunc(L2Update(jd).GetChanges());
            });

            GetMessageProcessor().Register(MSG_TYPE_HEARTBEAT, [this](const std::shared_ptr<CRYPTO::JSONDocument> jd) {
                static int cnt = 0;
                if (cnt++ < 10) //We show a few msgs only on startup, to verify we are heart beating.
                {
                    poco_information_f1(logger(), "Received Heartbeat: %s", GetCurrency(jd).ToString());
                }
            });

            GetMessageProcessor().Register(MSG_TYPE_SUBSCRIPTIONS,
                                           [this](const std::shared_ptr<CRYPTO::JSONDocument> jd) {
                                               poco_information(logger(), "Received Subscription response..");
                                           });
        }

        //----------------------------------------------------------------------
        UTILS::CurrencyPair ConnectionMD::GetCurrency(const std::shared_ptr<CRYPTO::JSONDocument> msg) const {
            return GetCurrencyPair(TranslateSymbol(msg->GetValue<std::string>("product_id")));
        }

        //Create a Market Data authentication signature
        const CRYPTO::AuthHeader ConnectionMD::GetAuthHeader() {
            auto cb_access_timestamp = std::to_string(UTILS::CurrentTimestamp() / 1000000000); //as seconds
            auto cb_access_method{"GET"};
            auto cb_access_request_path{"/users/self/verify"};

            //Now build the signature...
            auto msg = cb_access_timestamp + cb_access_method + cb_access_request_path;

            const auto decodedKey = libbase64::decode<std::string, char, unsigned char, true>(m_settings.m_secretkey);
            const auto digest = HMAC(EVP_sha256(), decodedKey.c_str(), decodedKey.size(), (unsigned char *) msg.c_str(), msg.size(), NULL, NULL);
            const auto cb_access_sign = libbase64::encode<std::string, char, unsigned char, true>(digest, SHA256_DIGEST_LENGTH);

            return CRYPTO::AuthHeader(cb_access_sign, m_settings.m_apikey, m_settings.m_passphrase, cb_access_timestamp);
        }

        //----------------------------------------------------------------------
        void ConnectionMD::Subscribe(const CRYPTO::ConnectionBase::TInstruments &instruments, const std::string &method,
                                     const std::string &channels) {
            std::string prods;
            for (const auto &inst: instruments) {
                prods += (prods.empty() ? "" : ",") + std::string("\"") + inst + "\"";
            }

            std::string payload;

            if (m_settings.m_apikey.empty() && m_settings.m_secretkey.empty() && m_settings.m_passphrase.empty()) {
                //use the public feed, no authentication required..
                payload = "{ \"type\": \"" + method + "\", \"product_ids\": [" + prods + "], \"channels\": [\"" +
                          channels + "\"] }";
            } else {
                // Use direct feed, authentication attributes required..
                CRYPTO::AuthHeader header = GetAuthHeader();
                payload =
                        "{ \"type\": \"" + method + "\", \"product_ids\": [" + prods + "], \"channel\":  \"" + channels
                        + "\" , \"signature\": \"" + std::get<0>(
                            header) + "\",\"key\":\"" + std::get<1>(header) + "\",\"passphrase\":\"" + std::get<2>(
                            header) + "\",\"timestamp\":\"" + std::get<3>(header) + "\"}";
            }

            Send(payload);
        }
    }
}
