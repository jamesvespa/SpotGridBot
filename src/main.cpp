
#include "Utils/ContextBase.h"
#include "Utils/AppHandler.h"

#include "Options.h"

#include <Poco/Logger.h>

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
        auto cfg = loadConfig(configPath);

        auto ex = make_shared<MockExchange>(cfg.gridBasePrice, cfg.feeRate,
                                           cfg.partialFillMinPct, cfg.partialFillMaxPct, cfg.slippageMaxPct);
        ex->setBalances(10000.0, 0.1);

        GridConfig gcfg;
        gcfg.pair = cfg.pair;
        gcfg.gridBasePrice = cfg.gridBasePrice;
        gcfg.levelsAbove = cfg.levelsAbove;
        gcfg.levelsBelow = cfg.levelsBelow;
        gcfg.stepPercent = cfg.stepPercent;
        gcfg.perOrderQty = cfg.perOrderQty;
        gcfg.maxPositionBtc = cfg.maxPositionBtc;

        STRATEGY::GridStrategy strat(ex, gcfg);
        strat.start();

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
