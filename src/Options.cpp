//
//
// Created by james on 28.11.17.
//

#include <iostream>
#include <iomanip>
#include <zconf.h>

#include <Poco/String.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Logger.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/Util/LoggingConfigurator.h>

#include "Options.h"
#include "RestConnectionBase.h"
#include "Utils/ContextBase.h"

using namespace UTILS;

namespace CORE {

Options::Options()
		: Options(CORE::CRYPTO::PATH_CONFIG, CORE::CRYPTO::PATH_LOGGINGPROPS)
{
}


Options::Options(const std::string &configPath, const std::string &loggingPropsPath)
		: Logging("Options"), m_configPath(std::move(configPath)), m_loggingPropsPath(std::move(loggingPropsPath))
{
	m_optMap[OPT_PATH_CONFIG] = m_configPath;
	
	try
	{
		Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pLoggingConfig { new Poco::Util::PropertyFileConfiguration(m_loggingPropsPath) };
		Poco::Util::LoggingConfigurator configurator;
		configurator.configure(pLoggingConfig);
	}
	catch (const std::exception &e)
	{
		std::cout << "Failed to initialize Poco logging:" << std::endl;
		std::cout << e.what() << std::endl;
	}
}

std::string Options::getDefaultOptArg(char opt)
{
	std::string arg;
	switch (opt)
	{
		case OPT_PATH_CONFIG:
			arg = CRYPTO::PATH_CONFIG;
			break;
		case OPT_PATH_LOGGINGPROPS:
			arg = CRYPTO::PATH_LOGGINGPROPS;
			break;
		case OPT_KILL:
			arg = "15"; // default: SIGTERM
			break;
		case OPT_HELP:
		default:
			break;
	}
	return arg;
}

Options::Options(int argc, char **argv)
		: Options()
{
	Read(argc, argv);
}

bool Options::Read(int argc, char **argv)
{
	const std::string options { // options with mandatory arguments
			Format((const std::string &) "%c:%c:%c:%c:%c:%c:%c:%c:", OPT_PATH_CONFIG, OPT_PATH_LOGGINGPROPS, OPT_MODIFY,
				   OPT_KILL, OPT_PRINT, OPT_STARTUPMODE, OPT_PATH_FIX8_GLOBAL_LOG, OPT_LOG) + Format((const std::string &) "%c:%c:", OPT_NOTIFICATION,
																									 OPT_COMMAND)
			// options with optional arguments
			+ Format((const std::string &) "%c::%c::%c::%c::%c::%c::%c::%c::%c::", OPT_MOD_DATACONNECTION, OPT_MOD_FIX8CONFIG, OPT_MOD_REQUESTCONFIG,
					 OPT_TRANSACTIONS, OPT_VIEWS, OPT_SOFTKILL, OPT_REJECT_COUNT, OPT_BREAK_COUNT, OPT_EXIT) + Format((const std::string &) "%c::",
																													  OPT_ENSURE_CONNECTION)
			// options without arguments
			+ Format((const std::string &) "%c%c%c%c", OPT_HELP, OPT_TRANSACTION_TEMPLATES, OPT_CONFIG_XML, OPT_RELOAD_TRANSACTIONS) };
	const std::string &programName { (const std::string &) argv[0] };
	int currentOpt;

	m_lastErrorMessage = "";
	opterr = 0;
	optind = 0;

	while ((currentOpt = getopt_long(argc, argv, options.c_str(), long_opts, &optind)) != -1)
	{
		if (currentOpt != '?')
		{
			std::string value { optarg ? (const std::string &) optarg : getDefaultOptArg((char) currentOpt) };
			switch (currentOpt)
			{
				case OPT_PATH_CONFIG:
					m_configPath = value;
					break;
				case OPT_PATH_LOGGINGPROPS:
					m_loggingPropsPath = value;
					break;
				default:
					break;
			}
			m_optMap[currentOpt] = value;
		}
		else // c == '?'
		{
			currentOpt = optopt;
			if (options.find((char) currentOpt) != std::string::npos)
			{
				m_lastErrorMessage += Format((const std::string &) "%s: Option '%c' requires an argument\n", programName, char(currentOpt));
			}
			else if (isprint(currentOpt))
			{
				m_lastErrorMessage += Format((const std::string &) "%s: Unknown option -%c\n", programName, char(currentOpt));
			}
			else
			{
				m_lastErrorMessage += Format((const std::string &) "%s: Unknown option character %d\n", programName, optopt);
			}
		}
	}
	return m_lastErrorMessage.empty();
}

/*! \brief Constructor
 *
 * Reads options from a specified stream and creates an argv array.
 *
 * @param istr    Input stream
 */
Options::ParseArgs::ParseArgs(std::istream &istr)
{
	std::string str;
	std::string line;
	while (getline(istr, line))
	{
		str += line + delimiter;
	}
	m_buffer = strdup(str.c_str());
	m_argv = parsedargs(m_buffer, &m_argc);
}

/*! \brief Destructor
 *
 * Releases the memory used by the object
 *
 */
Options::ParseArgs::~ParseArgs()
{
	if (m_argv)
	{
		free(m_argv);
	}
	free(m_buffer);
}

/*! \brief Move arguments from single string to argv array
 *
 * Scans the passed argument string and saves the pointers to the
 * arguments in argv
 *
 * @param args  argument string (arguments are delimited by \a delimiter)
 * @param argv  argv array to receive pointers to single arguments
 * If nullptr -> only return argument count
 *
 * @return number of arguments
 */
int Options::ParseArgs::setargs(char *args, char **argv, int argn) const
{
	int count;
	
	for (count = 0; *args != '\0' && (argn < 0 || count < argn); ++count)
	{
		if (argv)
		{
			argv[count] = args;
		}                       // set current argv to current position in args
		while (*args != '\0' && *args != delimiter)
		{
			++args;
		} // move to next delimiter
		if (argv && *args != '\0')
		{
			*(args++) = '\0';
		}        // change cell from delimiter to '\0'
		while (*args == delimiter)
		{
			++args;
		}                  // skip further delimiters
	}
	
	return count;
}

/*! \brief Parse arguments in string and return argv array
 *
 * Scans the passed argument string and returns a pointer
 * to the created argv array
 *
 * @param args  argument string (arguments are delimited by \a delimiter)
 * @param argc  Pointer to integer to receive number of arguments
 *
 * @return argv array
 */
char **Options::ParseArgs::parsedargs(char *args, int *argc)
{
	char **argv { nullptr };
	int argn { 0 };
	
	if (args && *args && ((argn = setargs(args, nullptr, -1)) != 0) && (argv = (char **) malloc((argn) * sizeof(char *))) != nullptr)
	{
		argn = setargs(args, argv, argn);
	}
	
	*argc = argn;
	return argv;
}

const option Options::long_opts[] = {{ "cpath",                1, nullptr, OPT_PATH_CONFIG },
										   { "lpath",                1, nullptr, OPT_PATH_LOGGINGPROPS },
										   { "glpath",               1, nullptr, OPT_PATH_FIX8_GLOBAL_LOG },
										   { "modify",               1, nullptr, OPT_MODIFY },
										   { "data",                 2, nullptr, OPT_MOD_DATACONNECTION },
										   { "session",              2, nullptr, OPT_MOD_FIX8CONFIG },
										   { "request",              2, nullptr, OPT_MOD_REQUESTCONFIG },
										   { "kill",                 1, nullptr, OPT_KILL },
										   { "transaction",          2, nullptr, OPT_TRANSACTIONS },
										   { "book",                 2, nullptr, OPT_VIEWS },
										   { "template",             0, nullptr, OPT_TRANSACTION_TEMPLATES },
										   { "print",                1, nullptr, OPT_PRINT },
										   { "startup",              1, nullptr, OPT_STARTUPMODE },
										   { "log",                  1, nullptr, OPT_LOG },
										   { "reject-count",         1, nullptr, OPT_REJECT_COUNT },
										   { "break-count",          1, nullptr, OPT_BREAK_COUNT },
										   { "notification",         1, nullptr, OPT_NOTIFICATION },
										   { "subscribe-marketdata", 1, nullptr, OPT_COMMAND },
										   { "help",                 0, nullptr, OPT_HELP },
										   { "exit",                 2, nullptr, OPT_EXIT },
										   { nullptr,                0, nullptr, 0 }};

const char *Options::m_usage = "Usage: FPEGridBot [OPTION]...\n"
									  "\"FPEGridBot -h\" prints further information.";


const char *Options::m_help = "A FIX client for storing market data snapshots.\n"
									 "\n"
									 "Without any options: FPEGridBot using the configuration defined in standard config files.\n"
									 "\n"
									 "If an instance of FPEGridBot is already running, it is not started a second time. Instead,\n"
									 "the command line parameters are passed to the running instance. Output is passed back\n"
									 "to the calling process.\n"
									 "\n"
									 "COMMAND LINE OPTIONS:\n"
									 "\n"
									 "-F[FILE]                configures FIX sessions using [FILE] (default: fix8config.xml)\n"
									 "-L[FILE]                configures logging using [FILE] (default: logging.properties)\n"
									 "-h                      print this help information\n"
									 "-k[SIGNUM]              send signal [SIGNUM] to running process\n"
									 "-d[MODIFICATIONS]       lists or modifies data connections\n"
									 "                        Syntax of MODIFICATIONS (see also 'examples' section):\n"
									 "                        MODIFICATION[;MODIFICATION]...\n"
									 "                        MODIFICATION: [CONNECTION NAME]:[PROPERTY NAME]=[VALUE]\n"
									 "                                  or: [+-][CONNECTION NAME]\n"
									 "-f[MODIFICATIONS]       lists or modifies FIX sessions (see 'examples' section)\n"
                                     "-r[MODIFICATIONS]       modifies request properties (see 'examples' section)\n"
									 "-s[MODE]                startup mode:\n"
									 "                        0 ... start with SecurityListRequest\n"
									 "                        1 ... skip SecurityListRequest, start with MarketDataRequests\n"
									 "                        ParseArgs2 ... start with ProtocolRequest\n"
                                     "-T                      lists transaction templates\n"
									 "-t[MODIFICATIONS]       lists or modifies transactions (see 'examples' section)\n"
									 "-v[MODIFICATIONS]       lists or modifies views\n"
                                     "-l[MODIFICATIONS]       modifies current logging flags\n"
                                     "-p[MODE]                prints out requested information:\n"
                                     "                        * C,c,D,d prints active and inactive connections\n"
                                     "                        * S,s,F,f prints active and inactive sessions\n"
                                     "                        * T prints active and inactive templates\n"
                                     "                        * t prints active and inactive transactions\n"
                                     "                        * A,a sets restrictions to active\n"
                                     "                        * I,i sets restrictions to inactive\n"
                                     "-n[subject/text]        sends notification with optional subject\n"
                                     "-q                      halts all sessions\n"
                                     "-e[SESSION]             checks connection state of a session (all if not specified)\n"
                                     "-j[SESSION]             resets rejects count for specified session or\n"
                                     "                        prints out rejects counts for all sessions if session is not specified\n"
                                     "-b[SESSION]             resets trade breakouts count for specified session or\n"
                                     "                        prints out trade breakouts counts for all sessions if session is not specified\n"
									 "-z                      Reloads all transactions\n"
									 "-x                      shutdown and exit FPEGridBot\n"
									 "-a                      executes a command passed in quotes\n"
									 "\n"
									 "EXAMPLES:\n"
									 "\n"
									 "FPEGridBot -d               Lists all data connections\n"
									 "FPEGridBot -dconn1:batchsize=100\n"
									 "                        Changes batch size of connection 'conn1' to 100\n"
									 "FPEGridBot -dconn1:db=newdb\n"
									 "                        Changes database of connection 'conn1' to 'newdb'\n"
									 "FPEGridBot -\"d +conn2;-conn1\"\n"
									 "                        Activates connection 'conn2' and deactivates connection 'conn1'\n"
									 "FPEGridBot -d-conn2         Deactivates connection 'conn2'\n"
									 "FPEGridBot -f               Lists all FIX sessions\n"
									 "FPEGridBot -f+sess2         Starts session 'sess2'\n"
									 "FPEGridBot -f-sess2         Stops session 'sess2'\n"
									 "FPEGridBot -k2              Sends a SIGINT signal to the running instance of FPEGridBot\n"
									 "FPEGridBot -t               Lists all transactions, both open and completed/aborted\n"
                                     "FPEGridBot -r+EUR/USD       Sends a Market Data Request for the currency pair EUR/USD\n"
                                     "FPEGridBot -r-EUR/USD       Cancels a Market Data Request for the currency pair EUR/USD\n"
                                     "FPEGridBot -p v             prints all views\n"
                                     "FPEGridBot -p t             prints all transactions\n"
                                     "FPEGridBot -l+256           switches logging flag 256 ON\n"
                                     "FPEGridBot -l-256           switches logging flag 256 OFF\n"
                                     "FPEGridBot -n hello         sends notification 'hello' with subject 'Direct Message'\n"
                                     "FPEGridBot -n Subj/hello    sends notification 'hello' with subject 'Subj'\n"
                                     "FPEGridBot -q               halts all sessions\n"
                                     "FPEGridBot -e               checks connection state of all sessions\n"
                                     "FPEGridBot -eTY_FSS_JPMC_SWP_SkewSafe_TR_6\n"
                                     "                                 checks connection state of TY_FSS_JPMC_SWP_SkewSafe_TR_6 session\n"
                                     "FPEGridBot -j               prints out reject counts for all sessions\n"
                                     "FPEGridBot -jTEST_MT_01     resets reject count for session TEST_MT_01\n"
                                     "FPEGridBot -b               prints out trade breakouts counts for all sessions\n"
                                     "FPEGridBot -bTEST_MT_01     resets trade breakouts count for session TEST_MT_01\n"
                                     "FPEGridBot -a\"sub EURUSD,TY_FSS_JPMC_SWP_SkewSafe_TR_6\"\n"
                                     "                                 Subscribes instrument 'EURUSD' to session 'TY_FSS_JPMC_SWP_SkewSafe_TR_6\n"
									 "";
	
} // namespace CORE
