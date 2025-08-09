
#include <Poco/Logger.h>

#include "Utils/ContextBase.h"
#include "Utils/AppHandler.h"

#include "Options.h"

#include "ConnectionManager.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Util/Application.h"
#include "Poco/URI.h"

#include "exchange.h"
#include "gridstrategy.h"

using namespace CORE;
using namespace UTILS;
using namespace std;

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

        string configPath = "config.json";
        if (argc > 1) configPath = argv[1];

        auto cfg = std::make_unique<CRYPTO::JSONDocument>(loadConfig(configPath));

        Options options(argc, argv);
        ConnectionManager connectionManager(options.ConfigPath(), options.LoggingPropsPath(), m_orderBook);
        connectionManager.Connect();

        auto ex = make_shared<MockExchange>(0, 0.001, 0.001, 0.01, 1.0);
        ex->setBalances(10000.0, 0.1);

        sleep(2); //need to implement

        GridConfig gcfg;
        gcfg.pair = cfg->GetValue<std::string>("pair");
        auto cp = UTILS::CurrencyPair(gcfg.pair);
        gcfg.gridBasePrice = cp.CpipToDbl(m_orderBook.GetMidPrice(cp));
        gcfg.levelsAbove = cfg->GetValue<int>("levelsAbove");
        gcfg.levelsBelow = cfg->GetValue<int>("levelsBelow");
        gcfg.stepPercent = cfg->GetValue<double>("stepPercent");
        gcfg.perOrderQty = cfg->GetValue<double>("perOrderQty");
        gcfg.maxPositionBtc = cfg->GetValue<int>("maxPositionBtc");

        STRATEGY::GridStrategy strat(ex, gcfg);
        strat.start();

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
