/**
 * @file
 * @author      thomas <thomas.grim-schlink@bbf-it.at>
 * @date        2020-12-14
 * @version     2020.12.14		initial version
 * @copyright   (c) 2020 BBF-IT
 * @brief		cmd argument parser
*/


#ifndef MERCURYUTILS_CMDARGPARSER_H
#define MERCURYUTILS_CMDARGPARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "Utils/Logging.h"
#include "Utils/Result.h"

namespace UTILS
{

/**
 * @brief 	struct for working with arguments \n
 * 			hold all necessary information about possible arguments
 */

struct Argument
{
	Argument(std::string lArgShort, std::string lArgLong, std::string lDesc, bool lHasVal)
		: argShort(std::move(lArgShort)), argLong(std::move(lArgLong)), desc(std::move(lDesc)), hasVal(lHasVal), parsed(false)
	{ }
	std::string argShort;	//!< short name of argument
	std::string argLong;	//!< long name of argument
	std::string desc;		//<! description, used for print help
	std::string value;		//!< value of argument if hasVal
	bool 		hasVal;		//!< indicate argument has a value, default false
	bool 		parsed;		//!< is argument already parsed?, default false
};

/**
 * @brief	CmdArgParser is a helper for given arguments to main() function \n
 * 			parse given arguments, print usage help and provides the value of arguments
 */
class CmdArgParser : public Logging
{
public:
	explicit CmdArgParser(const std::string &logName) : Logging(logName)
	{ }
	~CmdArgParser() = default;
	
	CmdArgParser(CmdArgParser const &) = delete;
	CmdArgParser(CmdArgParser &&) = delete;
	CmdArgParser &operator=(CmdArgParser const &) = delete;
	CmdArgParser &operator=(CmdArgParser &&) = delete;
	
	/**
	 * @brief 			parse all passed parameters \n
	 * 					assign the values to the defined options, evaluate set flags and save passed text arguments.
	 * @param[in] argc 	argc from main()
	 * @param[in] argv 	argv from main()
	 * @return 			true if parsing successful otherwise false with set error msg inside result
	 */
	BoolResult ParseArguments(int argc, char** argv);

	/**
	 * @brief				set the arguments which are possible
	 * @param[in] argShort 	short name e.g. h typical for help
	 * @param[in] argLong 	long name e.g. help
	 * @param[in] desc 		description of argument, e.g. print this help
	 * @param[in] hasVal 	if value required set it to true
	 */
	void SetArgument(const std::string &argShort, const std::string &argLong, const std::string &desc, bool hasVal = false);

	/**
	 * @brief			set usage description, part of PrintHelp()
	 * @param[in] usage description
	 */
	void SetUsage(std::string usage) { m_usage = std::move(usage); }
	
	/**
	 * @brief					get the value of an option \n
	 * 							option must be defined before with SetArgument()
	 * @param[in] 	argFlag 	flag name, can used shot and long name
	 * @param 		argValue	filled with value
	 * @return					true if value found, otherwise false with set error message
	 */
	BoolResult GetValue(const std::string& argFlag, std::string &argValue);

	/**
	 * @brief				get text argument from given index
	 * @param[in]	index 	index, starts with 0
	 * @param 		value 	filled with searched text argument
	 * @return				true if text argument exist, otherwise false with set error message
	 */
	BoolResult GetTextArgument(unsigned short index, std::string &value);
	
	/**
	 * @brief			get whole text arguments
	 * @param	value 	filled with all text arguments, separate by " "
	 * @return			true if text argument exist, otherwise false with set error message
	 */
	BoolResult GetTextArgument(std::string &value);
	
	/**
	 * @brief				check if a flag exit in the argument list
	 * @param{in]	argFlag flag name, can used shot and long name
	 * @return 				returns true if exist otherwise false with set error message
	 */
	BoolResult Exists(std::string argFlag);

	/**
	 * @brief	return flag counter of parsed flags
	 * @return 	amount of parsed flags
	 */
	unsigned int FlagCount() const 	{ return m_flagCounter; }
	
	/**
	 * @brief	return text argument counter of parsed arguments
	 * @return 	amount of separate text arguments
	 */
	size_t TextArgCount()			{ return m_textArgs.size(); }

	/**
	 * @brief	returns the name of the executable (argv[0])
	 * @return 	executable name
	 */
	std::string ExecName() 			{ return m_execName; }

	/**
	 * @brief	print help, help text set with SetUsage()
	 */
	void PrintHelp();

protected:
	
	/**
	 * @brief				write log message from BoolResult message
	 * @param[in]	result
	 */
	void Log(const BoolResult &result);
	
private:
	bool 			m_parsed { false };		//!< flag present parsed state
	unsigned int 	m_flagCounter { 0 };	//!< counter of successful parsed flag
	std::string 	m_execName;				//!< executable name store
	std::string 	m_usage;				//!< usage help description store
	std::vector<std::shared_ptr<Argument>>				m_args;		//!< hold all given arguments
	std::map<std::string, std::shared_ptr<Argument>>	m_argNames; //!< helper map for short / long arg name to arguments
	std::vector<std::string> 							m_textArgs;	//!< old all text arguments
};

}



#endif //MERCURYUTILS_CMDARGPARSER_H
