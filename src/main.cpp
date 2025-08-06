
#include "Utils/ContextBase.h"
#include "Utils/AppHandler.h"

#include "Options.h"

#include <Poco/Logger.h>

#include "ConnectionManager.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Util/Application.h"
#include "Poco/URI.h"

using namespace CORE;
using namespace UTILS;

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    Poco::Logger& logger { Poco::Logger::get("main") };

    try
    {
        Poco::Net::initializeSSL();

        Poco::Net::HTTPStreamFactory::registerFactory();
        Poco::Net::HTTPSStreamFactory::registerFactory();

        CurrencyPair::InitializeCurrencyConfigs();

        BOOK::OrderBook m_orderBook;

        Options options(argc, argv);
        ConnectionManager connectionManager(options.ConfigPath(), options.LoggingPropsPath(), m_orderBook);
        connectionManager.Connect();

       	poco_information(logger, "SpotGridBot has started - press <enter> to exit ..");
        std::cin.get();
    }
    catch (Poco::Exception& e) // explicitly catch poco exceptions
    {
        std::cerr << "Poco exception: " << e.message() << std::endl;
        poco_fatal(logger, e.message());
    }

    Poco::Net::uninitializeSSL();
   	poco_information(logger, "SpotGridBot has stopped successfully.");

    return 0;
}
