//
// Created by ludwig on 8/1/17.
//

#pragma once

#include <iomanip>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <atomic>
#include <optional>

#include <Poco/Timestamp.h>

#include "MercuryData/NotificationManager.h"
#include "MercuryProtobuf/quote.pb.h"
#include "MercuryProtobuf/session_mapping.pb.h"
#include "MercuryUtils/EventManager.h"
#include "MercuryUtils/Logging.h"

#include "ConnectionBase.h"
#include "SessionConfig.h"

namespace CORE {
class FullContext;

/*! Type alias for a map from string to vector of unique pointers to session config objects */
using SessionConfigMap = std::map<int64_t, SessionConfig::Ptr>;

/*! Type alias for vector of raw pointers to session config objects */
using SessionConfigVector = std::vector<SessionConfig *>;

/*! \brief Pool of session configuration objectspbs
 *
 * This class represents the pool of all session configurations defined by the
 * configuration. Its elements are session configuration objects which hold all the configuration
 * and runtime information associated with each session.
 */
class SessionConfigController final : UTILS::Logging, public UTILS::ErrorHandler
{
public:
	explicit SessionConfigController(FullContext &ctx);
	
	/*! \brief Default copy constructor. */
	SessionConfigController(const SessionConfigController & /* other */) = delete;
	
	/*! \brief Default copy assignment operator. */
	SessionConfigController &operator=(const SessionConfigController & /* other */) = delete;
	
	/*! \brief Default move constructor. */
	SessionConfigController(SessionConfigController && /* other */) = delete;
	
	/*! \brief Creates all sessionConfig objects defined in the FIX8 config file
     *
     * Creates all sessionConfig objects using the global config file, located in
     * \a configPath. If no sessionConfigs are defined in the global config file,
     * throw an exception.
     *
     * @param configPath    Path to the configuration file
     */
	void CreateSessions(std::set<int64_t> excluded = { });
	
	/*! \brief Default move assignment operator. */
	SessionConfigController &operator=(SessionConfigController && /* other */) = delete;
	
	/*! \brief Creates a session
     * @param numId: session id in the SQL database
     */
	void CreateSession(int64_t numId);
	
	UTILS::BoolResult Connect(const std::string &name);
	
	bool Connects();
	
	/*! \brief Disconnects from a sessionConfig object
	 *
	 * Disconnects from a sessionConfig object defined by the sessionConfig name.
	 *
	 * @param name   Name of the sessionConfig object or the session group to be disconnected
	 * @return true if successful
	 */
	UTILS::BoolResult DisconnectSession(const std::string &name, bool logout);
	
	/*! \brief Disconnects from a sessionConfig object
     *
     * Disconnects from an sessionConfig object defined by the sessionConfig name.
     *
     * @param sessionConfig Pointer to a session config object
     * @return true if successful
     */
	UTILS::BoolResult DisconnectSession(SessionConfig *sessionConfig, bool logout);
	
	/*! \brief Disconnect all sessions. */
	void ShutdownAllSessions();
	
	void ConnectTakerRateStartupList();
	
	/*! \brief Executes an action for all sessionConfigs
	 *
	 * Executes a given action for all sessionConfig objects.
	 *
	 * @param action	Action to be executed
	 */
	template <typename A>
	void ForEachSessionConfig(A action) const
	{
		for (const auto &it: m_sessionConfigs)
		{
			const SessionConfig::Ptr &session { it.second };
			if (session)
			{
				action(static_cast<SessionConfig &>(reinterpret_cast<SessionConfig &>(*session)));
			}
		}
	}
	
	/*! \brief Executes an action for all *active* sessionConfigs
 	*
 	* Executes a given action for all sessionConfig objects.
 	*
 	* @param action	Action to be executed
 	*/
	template <typename A>
	void ForEachActiveSessionConfig(A action) const
	{
		ForEachSessionConfig([&action](const SessionConfig &sc)
							 {
								 if (sc.GetConnection()->IsActive())
								 {
									 action(sc);
								 }
							 });
	}
	
	/*! \brief Executes an action for all *active* sessionConfigs
	 *
	 * Executes a given action for all sessionConfig objects.
	 *
	 * @param action	Action to be executed
	 */
	template <typename A>
	void ForEachActiveSessionConfig(A action)
	{
		ForEachSessionConfig([&action](SessionConfig &sc)
							 {
								 if (sc.GetConnection()->IsActive())
								 {
									 action(sc);
								 }
							 });
	}
	
	/*! \brief Finds the first sessionConfig object fulfilling a given condition
	 *
	 * Finds the first sessionConfig object fulfilling a given condition.
	 *
	 * @param predicate	Condition to be fulfilled
	 * @return Pointer to the first sessionConfigObject fulfilling the given condition,
	 * null pointer if no sessionConfig object was found
	 */
	template <typename P>
	SessionConfig *FindFirst(P predicate) const
	{
		for (const auto &it: m_sessionConfigs)
		{
			const SessionConfig::Ptr &session { it.second };
			if (session && predicate(*session))
			{
				return session.get();
			}
		}
		return nullptr;
	}
	
	SessionConfig *FindByName(const std::string &name) const;
	
	SessionConfig *FindById(int64_t id) const;
	
	SessionConfig::Ptr FindPtrById(int64_t id) const;
	
	const DATA::NotificationManager &NotificationManager() const;
	
	/*! Reference to the global context */
	FullContext &Ctx() { return m_ctx; }
	
	/*! Const reference to the global context */
	const FullContext &Ctx() const { return m_ctx; }
	
	void PublishSessionStatus(const std::string &sessionName, BBF::SessionStatus_Status status);
	
	/*! \brief Finds a connection creator for specified schema
	 * @param schema: schema name, like 'coinbase'
	 * @return: function creating connection,
	 */
	using TConnectionCreator = std::function<std::shared_ptr<CRYPTO::IConnection>(const CRYPTO::Settings &, FullContext &)>;
	
	TConnectionCreator CreateConnection(const std::string &schema) const
	{
		auto factoryIter = m_connectionsFactory.find(schema);
		return factoryIter != m_connectionsFactory.cend() ? factoryIter->second : (TConnectionCreator) nullptr;
	}
	
	// Sessions settings <session id : settings>
	using TSettingsCollection = std::unordered_map<int64_t, CRYPTO::Settings>;
	
	const TSettingsCollection &GetSettingsCollection() const
	{
		return m_settingsCollection;
	}
	
	static Poco::DateTime EpochToPocoDateTime(int64_t epochTs)
	{
		FIX8::Tickval tv { epochTs };
		struct tm tm { };
		tv.as_tm(tm);
		return { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, int(epochTs / 1'000'000 % 1'000), int(epochTs / 1'000 % 1'000) };
	}
	
	// Load sessions settings..
	bool LoadConfig();
	
	bool LoadConfig(const UTILS::XmlDocPtr &pDoc);

protected:
	/*! \brief Helper to register a connection creator for specified schema */
	template <typename TConnection>
	void RegisterConnection(const std::string &schema)
	{
		m_connectionsFactory.emplace(schema, [](const CRYPTO::Settings &settings, FullContext &ctx)
		{
			return std::make_shared<TConnection>(settings, ctx);
		});
	}

private:
	bool AddSessionConfigObject(std::shared_ptr<SessionConfig> sessionConfig, std::string &service, bool addToStartupList = false);
	
	std::string GetServiceName();
	
	void ClearActiveSessionConfig();
	
	/*! Process model used (currently always FIX8::pm_coro) */
	/*! Global context */
	FullContext &m_ctx;

	/*! Map of all session configs */
	SessionConfigMap m_sessionConfigs;
	
	std::string m_service; // associated adapter service where this session runs
	
	// Sessions settings <session id : settings>
	TSettingsCollection m_settingsCollection;
	
	std::vector<SessionConfig *> m_startupSessions; //!< list of startup sessions
	
	// Factory of connections
	std::map<std::string, TConnectionCreator> m_connectionsFactory;
	
	/*! \brief Container of instruments per session
	* Example of the container
	* {{"BINANCE_TR_01", "BNB/USDT, BNB/ETH, ETH/BTC, BTC/USDT"}, {"COINBASE_TR_01", "BTC/USD, BNB/USD"}}
	* */
	using TSessionsInstruments = std::map<std::string, std::string>; // <session name: instruments separated by comma>
	TSessionsInstruments m_sessionsInstruments;
}; // SessionConfigController

} // namespace CORE
