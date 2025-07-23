//
// Created by james on 22/07/22.
//
#pragma once

#include <Poco/Logger.h>
#include <ostream>

namespace UTILS {

    /*! \brief useful class to trace/log commands output */
    class CommandLogger {
    public:
        /*! \brief ctor
        * @param logger: reference to logger
        * @param ostr: output stream
        * @param name: command name
        * @param arguments: command arguments
        * @example: CommandLogger cmdLogger(logger(), ostr, name, it.first == OPT_COMMAND ? ("\"" + val + "\"") : val);
        * */
        CommandLogger(Poco::Logger& logger, std::ostream& ostr, const std::string& name, const std::string& arguments = "")
            : m_logger(logger), m_ostr(ostr), m_cmd(name + arguments)
        {
            // Collect all stream logging produced by command by redirecting the stream into string
            m_oldBuf = m_ostr.rdbuf(m_ss.rdbuf());
            Log(">>> BEGIN");
        }

        ~CommandLogger()
        {
            // Log all collected stream logging
            poco_information(m_logger, m_ss.str());
            // Print the stream logging in the original stream and restore output
            if (m_oldBuf)
            {
                m_ostr.rdbuf(m_oldBuf);
                m_ostr << m_ss.str() << std::endl;
            }
            Log("<<< END");
        }
    private:
        void Log(const std::string& prefix) const
        {
            poco_information_f2(m_logger, "%s CMD '%s'", prefix, m_cmd);
        }
        Poco::Logger& m_logger;
        std::ostream& m_ostr;
        std::string m_cmd;
        std::stringstream m_ss;
        std::streambuf* m_oldBuf {};
    }; // CommandLogger

} // ns UTILS
