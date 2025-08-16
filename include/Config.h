#pragma once

namespace CORE {
namespace CRYPTO {

const char *const PATH_CONFIG = "config.xml";
const char *const PATH_LOGGINGPROPS = "logging.properties";

const std::string TAG_SESSION_CONFIG = "SessionConfig";
const std::string TAG_SESSION = "Session";

const std::string ATTR_SESSION_NAME = "name";
const std::string ATTR_ACTIVE = "active";
const std::string ATTR_NUMID = "num_id";
const std::string ATTR_HOST = "host";
const std::string ATTR_PORT = "port";
const std::string ATTR_APIKEY = "api_key";
const std::string ATTR_SECRETKEY = "secret_key";
const std::string ATTR_RECVWINDOW = "recv_window";
const std::string ATTR_INSTRUMENTS = "instruments";
const std::string ATTR_SESSIONS = "sessions";
const std::string ATTR_CHANNELS = "channels";
const std::string ATTR_ORDERS_HTTP = "orders_http";
const std::string ATTR_EXCHANGE_INFO_HTTP = "exchange_info_http";
const std::string ATTR_SNAPSHOT_HTTP = "snapshot_http";
const std::string ATTR_ORDER_MONITORING_INTERVAL = "order_monitoring_interval";
const std::string ATTR_PARAMETER = "Parameter";
const std::string ATTR_PARAMETER_NAME = "name";
const std::string ATTR_PARAMETER_VALUE = "value";
const std::string ATTR_DEPTH = "depth";
const std::string ATTR_PROTOCOL = "protocol";
const std::string ATTR_PASSPHRASE = "passphrase";
const std::string ATTR_SCHEMA = "schema";


const int ATTR_RECVWINDOW_DEFAULT = 5000;
const int ATTR_PORT_DEFAULT = 443;

// Cryptocurrencies specific connection settings..
struct Settings
{
	int64_t m_numId { };
	std::string m_name;
	bool m_active;
	std::string m_host;
	int m_port { };
	std::string m_orders_http;
	std::string m_snapshot_http;
	std::string m_apikey;
	std::string m_secretkey;
	std::string m_passphrase;
	// With recvWindow, you can specify that the request must be processed within
	// a certain number of milliseconds or be rejected by the server
	// (this is possibly exchange-specific (binance))
	int m_recvWindow { };
	
	std::string m_channels;
	std::string m_instruments;
	unsigned int m_depth { 0 };
	std::string m_protocol;

	std::string m_schema;
	
	// Specific parameters for each configuration <name:value>
	std::map<std::string, std::string> m_parameters;

	// Returns a parameter value by name (defaultValue if not found)
	std::string GetParameter(const std::string &name, const std::string &defaultValue = "") const
	{
		const auto iter = m_parameters.find(name);
		return iter != m_parameters.cend() ? iter->second : defaultValue;
	}
};

}
}
