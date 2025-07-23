//
// Created by jameson 28.11.17.
//
#pragma once

#include <getopt.h>
#include <string>
#include <map>

#include <Utils/Logging.h>
#include <Utils/EventManager.h>

namespace CORE {

constexpr auto  USE_COLOR_CODES             = true;

constexpr auto  OPT_PATH_CONFIG             = 'C'; // requires argument
constexpr auto  OPT_PATH_LOGGINGPROPS       = 'L'; // requires argument
constexpr auto  OPT_CONFIG_XML              = 'M';
constexpr auto  OPT_PATH_FIX8_GLOBAL_LOG    = 'G'; // requires argument
constexpr auto  OPT_TRANSACTION_TEMPLATES   = 'T';
constexpr auto  OPT_MODIFY                  = 'm'; // requires argument
constexpr auto  OPT_MOD_DATACONNECTION      = 'd'; // optional argument
constexpr auto  OPT_MOD_FIX8CONFIG          = 'f'; // optional argument
constexpr auto  OPT_MOD_REQUESTCONFIG       = 'r'; // optional argument
constexpr auto  OPT_VIEWS                   = 'v'; // optional argument
constexpr auto  OPT_KILL                    = 'k'; // requires argument
constexpr auto  OPT_HELP                    = 'h';
constexpr auto  OPT_TRANSACTIONS            = 't'; // optional argument
constexpr auto  OPT_PRINT                   = 'p'; // requires argument
constexpr auto  OPT_STARTUPMODE             = 's'; // requires argument
constexpr auto  OPT_SOFTKILL                = 'q'; // optional argument
constexpr auto  OPT_LOG                     = 'l'; // requires argument
constexpr auto  OPT_EXIT                    = 'x'; // optional argument
constexpr auto  OPT_REJECT_COUNT      		= 'j'; // optional argument
constexpr auto  OPT_BREAK_COUNT				= 'b'; // optional argument
constexpr auto  OPT_NOTIFICATION			= 'n'; // requires argument
constexpr auto  OPT_ENSURE_CONNECTION		= 'e'; // optional argument
constexpr auto  OPT_COMMAND					= 'a'; // requires argument
constexpr auto  OPT_RELOAD_TRANSACTIONS		= 'z'; // reloading transactions

class SessionConfig;
namespace RESTAPI {
	enum class EOrderType;
}

/*! \brief Process command line parameters
 *
 * This class stores settings provided to the program via command line
 * parameters.
 */
class Options : public UTILS::Logging
{
public:
	explicit Options();
	
	Options(const std::string &configPath, const std::string &loggingPropsPath);
	
	Options(int argc, char **argv);
	
	/*! \brief Reset settings
	 *
	 * Resets all settings previously initialized using command line parameters.
	 */
	void Clear() { m_optMap.clear(); }
	
	bool Read(int argc, char **argv);
	
	/*! \brief Processes options
	 *
	 * Processes currently stored options, using stdout for output.
	 *
	 */

	
	std::string GetValue(char opt, std::string defVal = std::string()) const;
	
	int GetIntValue(char opt, int defVal = 0) const;
	
	bool GetFlag(char opt) const;
	
	
	/*! \brief Get the general config file.
	 *
	 * Returns the path to the general config file.
	 *
	 * @return Path of the general config file.
	 */
	 const std::string &ConfigPath() const { return m_configPath; }
	
	/*! \brief Get the POCO logging config file.
	 *
	 * Returns the path to the POCO logging config file.
	 *
	 * @return Path of the POCO logging config file.
	 */
	 const std::string &LoggingPropsPath() const { return m_loggingPropsPath; }
	
	bool ReadFromStream(std::istream &istr);
	
	static void SaveOptions(std::ostream &ostr, int argc, char **argv);
	
	static void PrintHelp();
	


protected:
	
	/*! \brief Reads an option list from file and creates argv-style options
	 *
	 * This private class reads options from a specified file and
	 * creates an argv array from it, to be used by getopt().
	 */
	class ParseArgs
	{
	public:
		explicit ParseArgs(std::istream &istr);
		
		virtual ~ParseArgs();
		
		/*! \brief Default copy constructor. */
		ParseArgs(const ParseArgs & /* other */) = delete;
		
		/*! \brief Default copy assignment operator. */
		ParseArgs &operator=(const ParseArgs & /* other */) = delete;
		
		/*! \brief Default move constructor. */
		ParseArgs(ParseArgs && /* other */) = delete;
		
		/*! \brief Default move assignment operator. */
		ParseArgs &operator=(ParseArgs && /* other */) = delete;
		
		/*! \brief Access argc
		 *
		 * @param argc    Number of arguments
		 */
		int argc() const { return m_argc; }
		
		/*! \brief Access argv
		 *
		 * @param argv   Array of arguments
		 */
		char **argv() { return m_argv; }
	
	private:
		/*! character used to delimit arguments */
		const char delimiter { '\x01' };
		/*! buffer for all arguments stored in argv */
		char *m_buffer;
		/*! argument count */
		int m_argc;
		/*! argument array */
		char **m_argv;
	
	protected:
		int setargs(char *args, char **argv, int argn) const;
		
		char **parsedargs(char *args, int *argc);
	};

private:
	/*! Option array passed to getopt_long */
	static const option long_opts[];
	/*! Usage information */
	static const char *m_usage;
	/*! Help information */
	static const char *m_help;
	/*! String representation of last error occurred */
	std::string m_lastErrorMessage;
	/*! Map of option values */
	std::map<char, std::string> m_optMap;
	/*! config file path */
	std::string m_configPath;
	/*! logging properties file path */
	std::string m_loggingPropsPath;
protected:
	static std::string getDefaultOptArg(char opt) ;
	
	static void printUsage() ;
	
	char guessModificationKind(const std::string &path, const std::string &prop);
	
	void executeModification(std::ostream &ostr, char kind, const std::string &path, const std::string &prop, const std::string &value);
	
	void executeModifications(std::ostream &ostr, const std::string& mods, char kind);
	
	void PrintSessionInfo(std::ostream &ostr, const CORE::SessionConfig *sessionConfig) const;
	
	void print(std::ostream &ostr, const std::string &arg);
	
	void SendNotification(std::ostream &ostr, const std::string &arg);

	//std::string getAttrVal(const XmlElement *elem, const std::string& tag, const XmlElement *fallbackElem, const std::string& defaultValue = "") const;
	//std::string getAttrVal(const XmlElement *elem, const std::string& tag, const std::string& defaultValue = "") const;
	
	void ExecuteCommand(std::ostream &ostr, const std::string &cmd, const std::map<std::string, std::string> &args);
};

} // namespace CORE
