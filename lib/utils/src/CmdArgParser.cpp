/**
 * @file
 * @author      thomas <thomas.grim-schlink@bbf-it.at>
 * @date        04.12.19
 * @version     04.12.19
 * @copyright   (c) 2019 BBF-IT
 */

#include <iostream>
#include <Poco/Logger.h>

#include "Utils/CmdArgParser.h"

#include <regex>

namespace UTILS {

BoolResult CmdArgParser::ParseArguments(int argc, char **argv)
{
	BoolResult result { true, "ParseArguments()"};
	m_execName = std::string(argv[0]);
	bool expectValue = false;
	std::map<std::string, std::shared_ptr<Argument>>::const_iterator itFlag;
	
	for (int i = 1; i < argc && static_cast<bool>(result); ++i)
	{
		std::string entry(argv[i]);
		
		if (expectValue)
		{
			if (entry.compare(0, 1, "-") == 0)
			{
				result = BoolResult { false, "ParseArguments()"
											 + (!itFlag->second->argShort.empty() ? std::string(" (" + itFlag->second->argShort + ")") : "")
											 + (!itFlag->second->argLong.empty() ? std::string(" (" + itFlag->second->argLong + ")") : "") + " value wasn't found." };
			}
			else
			{
				itFlag->second->value = entry;
				expectValue = false;
			}
		}
		else if (entry.compare(0, 1, "-") == 0)
		{
			if (!m_textArgs.empty())
			{
				result = BoolResult { false, "ParseArguments() - Flags not allowed after text arguments." };
			}
			else
			{
				if (entry.compare(0, 2, "--") == 0)
				{
					entry.erase(0, 2);
					
					itFlag = m_argNames.find(entry);
					if (itFlag == m_argNames.end())
					{
						result = BoolResult { false, "ParseArguments() - Long flag " + entry + " wasn't found." };
					}
					else
					{
						itFlag->second->parsed = true;
						++m_flagCounter;
						
						if (itFlag->second->hasVal)
						{
							expectValue = true;
						}
					}
				}
				else
				{
					entry.erase(0, 1);
					for (unsigned long i = 0; i < entry.length(); ++i)
					{
						std::string flag(&(entry[i]), 1);
						itFlag = m_argNames.find(flag);
						if (itFlag == m_argNames.end())
						{
							result = BoolResult { false, "ParseArguments() - Short flag " + flag + " wasn't found." };
						}
						else
						{
							itFlag->second->parsed = true;
							++m_flagCounter;
							
							if (itFlag->second->hasVal && i == (entry.length() - 1))
							{
								expectValue = true;
							}
						}
					}
				}
			}
		}
		else
		{
			m_textArgs.push_back(entry);
		}
	}
	
	if(expectValue && static_cast<bool>(result))
	{
		std::string fShort, fLong;
		if (nullptr != itFlag->second)
		{
			fShort = itFlag->second->argShort;
			fLong = itFlag->second->argLong;
		}
		result = BoolResult { false, "ParseArguments()" + (!fShort.empty() ? std::string(" (" + fShort + ")") : "")
						+  (!fLong.empty() ? std::string(" (" + fLong + ")") : "") + " value wasn't found." };
	}
	m_parsed = true;
	
	Log(result);
	return result;
}

void CmdArgParser::SetArgument(const std::string &argShort, const std::string &argLong, const std::string &desc, bool hasVal)
{
	auto arg = std::make_shared<Argument>(argShort, argLong, desc, hasVal);
	m_args.push_back(std::move(arg));
	
	if (!argShort.empty())
	{
		m_argNames.insert(std::pair<std::string, std::shared_ptr<Argument>>(argShort, m_args.back()));
	}
	
	if (!argLong.empty())
	{
		m_argNames.insert(std::pair<std::string, std::shared_ptr<Argument>>(argLong, m_args.back()));
	}
}

BoolResult CmdArgParser::GetValue(const std::string& argFlag, std::string &argValue)
{
	BoolResult result { false, "GetValue() - CmdArgParser::ParseArguments() not executed"};
	if (!m_parsed || !static_cast<bool>(Exists(argFlag)))
	{
		Log(result);
		return result;
	}
	
	result = BoolResult(true, "GetValue()");
	
	auto it = m_argNames.find(argFlag);
	if (it != m_argNames.end() && it->second->hasVal)
	{
		argValue = it->second->value;
	}
	
	Log(result);
	return result;
}

BoolResult CmdArgParser::GetTextArgument(unsigned short index, std::string &value)
{
	BoolResult result { false, "GetTextArgument() - CmdArgParser::ParseArguments() not executed"};
	if (!m_parsed)
	{
		Log(result);
		return result;
	}

	result = BoolResult { false, "GetTextArgument() - index(" + std::to_string(index) + ") not found" };
	if (index < m_textArgs.size())
	{
		value = m_textArgs.at(index);
		result = BoolResult(true, "GetTextArgument()");
	}
	
	Log(result);
	return result;
}

BoolResult CmdArgParser::GetTextArgument(std::string &value)
{
	BoolResult result { false, "GetTextArgument() - CmdArgParser::ParseArguments() not executed"};
	if (!m_parsed)
	{
		Log(result);
		return result;
	}

	result = BoolResult { false, "GetTextArgument() - not text argument exist" };
	if (!m_textArgs.empty())
	{
		result = BoolResult(true, "GetTextArgument()");
		value.clear();
		
		for (auto & m_textArg : m_textArgs)
		{
			value += m_textArg + " ";
		}
		value = std::regex_replace(value, std::regex(" +$"), "");
	}
	
	Log(result);
	return result;
}

BoolResult CmdArgParser::Exists(std::string argFlag)
{
	BoolResult result { false, "Exists() - CmdArgParser::ParseArguments() not executed"};
	if (!m_parsed)
	{
		Log(result);
		return result;
	}
	
	auto it = m_argNames.find(argFlag);

	result = BoolResult{ !(it == m_argNames.end() || !it->second->parsed), "Exists() - flag(" + argFlag + ") not found" };
	
	Log(result);
	return result;
}

void CmdArgParser::PrintHelp()
{
	std::cout << std::endl << m_execName << std::endl;
	std::cout << std::endl << "Usage:" << std::endl;
	std::cout << "\t" << m_usage << std::endl;
	std::cout << std::endl;
	std::cout << "Options: " << std::endl;
	
	std::vector<std::shared_ptr<Argument>>::const_iterator it;
	for (it = m_args.cbegin(); it != m_args.cend(); ++it)
	{
		std::cout << "-" << (*it)->argShort << "\t--" << (*it)->argLong << "\t\t" << (*it)->desc << std::endl;
	}
}

void CmdArgParser::Log(const BoolResult &result)
{
	if (static_cast<bool>(result))
	{
		if (!result.ErrorMessage().empty())
		{
			logger().trace(result.ErrorMessage());
		}
	}
	else
	{
		logger().error(result.ErrorMessage());
	}
}
}
