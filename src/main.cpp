
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
#include "OrderManager.h"

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

        auto m_orderBook = std::make_shared<BOOK::OrderBook>();

        string configPath = "config.json";
        if (argc > 1) configPath = argv[1];

        auto cfg = std::make_unique<CRYPTO::JSONDocument>(loadConfig(configPath));

        Options options(argc, argv);
        auto m_connectionManager = make_shared<ConnectionManager>(options.ConfigPath(), options.LoggingPropsPath(), m_orderBook);
        m_connectionManager->Connect(); //connect market data and populate orderbook.

        auto m_orderManager = make_shared<OrderManager>(m_connectionManager);

        sleep(2); //need to implement

        GridConfig gcfg;
        gcfg.pair = cfg->GetValue<std::string>("pair");
        auto cp = UTILS::CurrencyPair(gcfg.pair);
        gcfg.gridBasePrice = cp.CpipToDbl(m_orderBook->GetMidPrice(cp)); //set the price of the bot from the order book.
        gcfg.levelsAbove = cfg->GetValue<int>("levelsAbove");
        gcfg.levelsBelow = cfg->GetValue<int>("levelsBelow");
        gcfg.stepPercent = cfg->GetValue<double>("stepPercent");
        gcfg.perOrderQty = cfg->GetValue<double>("perOrderQty");
        gcfg.maxPositionBtc = cfg->GetValue<int>("maxPositionBtc");

        STRATEGY::GridStrategy strat(m_orderManager, gcfg);
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
