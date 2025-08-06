//
// Created by james on 8/1/17.
//

#include <google/protobuf/util/json_util.h>

#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Logger.h>
#include "Poco/Data/RecordSet.h"
#include <Poco/Thread.h>
#include <Poco/Condition.h>

#include "Utils/Utils.h"
#include "Utils/XmlConfigReader.h"

#include "Config.h"
#include "ConnectionManager.h"


#include "binance/ConnectionMD.h"
#include "binance/ConnectionORD.h"
#include "coinbase/ConnectionMD.h"
#include "OKX/ConnectionMD.h"
#include "OKX/ConnectionORD.h"
#include "SchemaDefs.h"

using namespace UTILS;

namespace CORE {

ConnectionManager::ConnectionManager(const std::string& configPath, const std::string& loggingPropsPath, BOOK::OrderBook& orderBook)
	: Logging("ConnectionManager"), ErrorHandler(pLogger()), m_configPath(configPath), m_loggingPropsPath(loggingPropsPath), m_orderBook(orderBook) {

	// Register supported connection types
	RegisterConnectionCreator<BINANCE::ConnectionMD>(BINANCE::SCHEMAMD);
	RegisterConnectionCreator<BINANCE::ConnectionORD>(BINANCE::SCHEMAORD);
	RegisterConnectionCreator<COINBASE::ConnectionMD>(COINBASE::SCHEMAMD);
	RegisterConnectionCreator<OKX::ConnectionMD>(OKX::SCHEMAMD);
	RegisterConnectionCreator<OKX::ConnectionORD>(OKX::SCHEMAORD);

	LoadConfig();

	for (auto &setting: m_settingsCollection)
	{
		CreateSession(setting.second.m_numId);
	}
}

void ConnectionManager::CreateSession(int64_t numId)
{
	auto creator = FindConnectionCreator(m_settingsCollection[numId].m_schema);
	if (!creator)
	{
		SetError(UTILS::Format("Not supported schema '%s'", m_settingsCollection[numId].m_schema));
		return;
	}

	m_connections.emplace( m_settingsCollection[numId].m_name, creator(m_settingsCollection[numId]) );
}

void ConnectionManager::Connect()
{
	for ( auto iter=m_connections.begin(); iter!=m_connections.end(); ++iter) {
		auto conn = *iter;
		conn.second->Connect();
	};
}

void ConnectionManager::Disconnect()
{
 	for ( auto iter=m_connections.begin(); iter!=m_connections.end(); ++iter) {
		auto conn = *iter;
		conn.second->Disconnect();
	};
}
	

bool ConnectionManager::LoadConfig()
{
	poco_information_f1(logger(), "Loading definitions using: %s", m_configPath);
	return LoadConfig(GetConfigDoc(m_configPath));
}

bool ConnectionManager::LoadConfig(const UTILS::XmlDocPtr &pDoc)
{
	try
	{
		std::string errMsg;
		
		// Read market data transactions
		if (auto *baseNode = GetConfigNode(pDoc, CORE::CRYPTO::TAG_TRANSACTION_CONFIG, &errMsg))
		{
			if (baseNode->hasChildNodes())
			{
				Poco::XML::NodeList *children { baseNode->childNodes() };
				if (children)
				{
					for (long unsigned int i = 0; i < children->length(); ++i)
					{
						Poco::XML::Node *childNode = children->item(i);
						if (childNode->nodeType() == Poco::XML::Node::ELEMENT_NODE)
						{
							const std::string session = UTILS::GetXmlAttribute(childNode, CORE::CRYPTO::ATTR_SESSIONS, "");
							const std::string instruments = UTILS::GetXmlAttribute(childNode, CORE::CRYPTO::ATTR_INSTRUMENTS, "");

							auto sessIter = m_sessionsInstruments.find(session);
							if (sessIter != m_sessionsInstruments.cend())
							{
								poco_error_f1(logger(), "Session '%s' has already been configured in "
														"another market data transaction. Ignored.", session);
								continue;
							}
							m_sessionsInstruments[session] = instruments;
							poco_information_f2(logger(), "Added instruments configuration for session: '%s' -> '%s'", session, instruments);
						}
					}
				}
			}
		}
		
		// Read sessions
		//--------------
		if (auto *baseNode = GetConfigNode(pDoc, CORE::CRYPTO::TAG_SESSION_CONFIG, &errMsg))
		{
			if (baseNode->hasChildNodes())
			{
				poco_information_f1(logger(), "Loading %s definitions from XML", CORE::CRYPTO::TAG_SESSION);
				Poco::XML::NodeList *children { baseNode->childNodes() };
				if (children)
				{
					for (long unsigned int i = 0; i < children->length(); ++i)
					{
						Poco::XML::Node *childNode = children->item(i);
						if (childNode->nodeType() == Poco::XML::Node::ELEMENT_NODE)
						{
							poco_information_f1(logger(), "Reading %s attributes from XML", CORE::CRYPTO::TAG_SESSION);
							CRYPTO::Settings settings;
							settings.m_numId = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_NUMID, 0);
							
							std::string active { UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_ACTIVE, "false") };
							settings.m_active = active == "true";
							settings.m_name = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_SESSION_NAME, "");
							settings.m_host = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_HOST, "");
							settings.m_port = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_PORT, CRYPTO::ATTR_PORT_DEFAULT);
							settings.m_orders_http = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_ORDERS_HTTP, "");
							settings.m_snapshot_http = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_SNAPSHOT_HTTP,
																			  "https://api.binance.com/api/v3/depth?symbol=INSTRUMENT&limit=5000");
							settings.m_apikey = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_APIKEY, "");
							settings.m_secretkey = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_SECRETKEY, "");
							settings.m_recvWindow = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_RECVWINDOW, CRYPTO::ATTR_RECVWINDOW_DEFAULT);
							settings.m_channels = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_CHANNELS, "");
							
							auto sessIter = m_sessionsInstruments.find(settings.m_name);
							if (sessIter == m_sessionsInstruments.cend())
							{
								poco_error_f2(logger(), "Session '%s' has no configured instruments in %s. Ignored.",
                                                settings.m_name, CORE::CRYPTO::TAG_TRANSACTION_CONFIG);
                                continue;

                            }
                            settings.m_instruments = sessIter->second;
							settings.m_depth = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_DEPTH, 0);
							settings.m_protocol = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_PROTOCOL, "ws");

							settings.m_username = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_USERNAME, "");
							settings.m_password = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_PASSWORD, "");
							settings.m_schema = UTILS::GetXmlAttribute(childNode, CRYPTO::ATTR_SCHEMA, "");
							
							// Load parameters
							if (auto *paramNodes = childNode->childNodes())
							{
								for (unsigned long i = 0; i < paramNodes->length(); ++i)
								{
									auto *paramNode = paramNodes->item(i);
									if (paramNode->nodeType() == Poco::XML::Node::ELEMENT_NODE && paramNode->localName() == CRYPTO::ATTR_PARAMETER)
									{
										std::string name = Poco::trim(std::string(UTILS::GetXmlAttribute(paramNode, CRYPTO::ATTR_PARAMETER_NAME, "")));
										if (!name.empty())
										{
											settings.m_parameters.emplace(name, Poco::trim(std::string(
													UTILS::GetXmlAttribute(paramNode, CRYPTO::ATTR_PARAMETER_VALUE, ""))));
										}
									}
								}
							}
							m_settingsCollection[settings.m_numId] = settings;
						}
					}
					children->release();
				}
				else
				{
					poco_error_f1(logger(), "Internal error reading child nodes of %s", baseNode->localName());
				}
			}
			else
			{
				poco_error_f1(logger(), "Error loading config: Base node '%s' has no child nodes", baseNode->localName());
			}
		}
		else
		{
			poco_error(logger(), "Error loading config: Invalid base node");
			return false;
		}
	}
	catch (std::exception &e)
	{
		poco_error_f1(logger(), "Error loading config: %s", std::string(e.what()));
		return false;
	}
	return true;
}
}