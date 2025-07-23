//
// Created by james on 10/06/22.
//
#pragma once

#include <csignal>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include "ContextBase.h"
#include "Utils.h"

namespace UTILS
{
    // Common Options
    const std::string CMD_OPT_HELP = "h";
    const std::string CMD_OPT_EXIT = "x";

    /**
     * @return of executable from path
     */
    inline std::string GetExecName(const std::string& path)
    {
        const auto p = path.rfind("/");
        return p != std::string::npos ? path.substr(p + 1) : path;
    }


    /*! \brief Provides general application handling
     */
    template <typename Context> class AppHandler {
    public:
        AppHandler(Context& ctx, const std::string& appName, int argc, char** argv)
        {
            stCtxPtr = &ctx;
            RegisterSignalHandler(
                [](int signum)
                {
                    switch (signum)
                    {
                        // The following signals terminate the program
                        case SIGINT:
                        case SIGQUIT:
                        case SIGTERM:
                            stCtxPtr->TriggerShutdown(sdtProgramShutdown, 0, "Terminate signal received");
                            break;
                            // SIGUSR1 -> Update options
                        case SIGUSR1:
                        {
                            std::ifstream istr(stCtxPtr->PathOptFIFO);
                            if (istr.is_open())
                            {
                                std::ofstream ostr(stCtxPtr->PathResponseFIFO);
                                if (ostr.is_open())
                                {
                                    if (stCtxPtr->Options().ReadFromStream(istr))
                                    {
                                        if (stCtxPtr->Ready.load())
                                            stCtxPtr->Options().Process(ostr);
                                        else
                                            std::cout << "The process is not ready yet" << std::endl;
                                    }
                                    else
                                    {
                                        ostr.close();
                                    }
                                }
                                istr.close();
                            }
                            break;
                        }
                    } // switch
                });

            pid_t pid { GetPid(appName.c_str()) };
            if (pid < 0)
            {
                pid = GetPid(COMM_VALUE_MEMCHECK);
            } // valgrind execution -> embedded in memcheck

            if (!ctx.Options().IsStateGood())
            {
                // Command line parsing error
                std::cerr
                    << "Error parsing command line: "
                    << ctx.Options().GetErrorState().ErrorMessage() << std::endl;
                _exit(1);
            }


            // Common processing of Help and Exit options
            const auto& cmd = ctx.Options().GetProcessor();
            bool shouldExit = false;
            if (cmd.Find(CMD_OPT_HELP))
            {
                std::cout << "Usage:\n\t" << cmd.GetUsage(argv[0]) << std::endl << std::endl;
                std::cout << "Options:\n" << cmd.GetOptStr() << std::endl;
                shouldExit = true;
            }
            if (cmd.Find(CMD_OPT_EXIT) && pid < 0)
            {
                if (pid >= 0)
                {
                    std::cout << appName << " is running (PID: " << pid << ")" << std::endl;
                }
                else
                {
                    std::cout << appName << " is not running." << std::endl;
                    shouldExit = true;
                }
            }

            if (pid >= 0) // process already running -> relay command line arguments to running process and exit
            {
                std::cout << "Relaying options to running process." << std::endl;
                RelayOptions(ctx, pid, argc, argv);
                shouldExit = true;
            }

            if (shouldExit)
            {
                _exit(0);
            }
        }

        /*! Register all signals to be handled */
        static void RegisterSignalHandler(__sighandler_t handler)
        {
            for (int sig : { SIGINT, SIGQUIT, SIGTERM, SIGUSR1 })
            {
                signal(sig, handler);
            }
        }

        static void RelayOptions(Context& ctx, pid_t pid, int argc, char** argv)
        {
            unlink(ctx.PathOptFIFO.c_str());
            unlink(ctx.PathResponseFIFO.c_str());
            try
            {
                const auto throwFailure = [&ctx]()
                {
                    throw std::runtime_error(UTILS::Format(
                        std::string("Failed to create FIFO %s: %d\n"),
                        ctx.PathResponseFIFO, errno));
                };
                if (mkfifo(ctx.PathOptFIFO.c_str(), S_IRUSR | S_IWUSR) == 0)
                {
                    if (mkfifo(ctx.PathResponseFIFO.c_str(), S_IRUSR | S_IWUSR) == 0)
                    {
                        try
                        {
                            kill(pid, SIGUSR1);
                            std::ofstream ostr { ctx.PathOptFIFO };
                            if (ostr.is_open())
                            {
                                ctx.Options().SaveOptions(ostr, argc, argv);
                                ostr.close();
                            }
                            std::ifstream istr { ctx.PathResponseFIFO };
                            if (istr.is_open())
                            {
                                std::string line;
                                while (getline(istr, line))
                                {
                                    std::cout << line << std::endl;
                                }
                                istr.close();
                            }
                        }
                        catch (...)
                        {
                            throw std::runtime_error(
                                "Error writing options to FIFO: " + UTILS::GetMessage(std::current_exception()));
                        }
                        unlink(ctx.PathResponseFIFO.c_str());
                    }
                    else
                    {
                        throwFailure();
                    }
                    unlink(ctx.PathOptFIFO.c_str());
                }
                else
                {
                   throwFailure();
                }
            } // try
            catch (const std::exception& e)
            {
                std::cerr << e.what();
                exit(1);
            } // catch
        }

        // Static ptr to the context used in signal handler.
        // It must be explicitly added to the code in a cpp module (main.cpp or so), example:
        // template <> FullContext* UTILS::AppHandler<FullContext>::stCtxPtr {};
        static Context* stCtxPtr;
    }; // AppHandler


} // ns UTILS
