#include "OrderManager.h"

#include "ConnectionManager.h"
#include "Utils/CurrencyPair.h"

namespace CORE {
    std::string OrderManager::PlaceLimitOrder(const UTILS::CurrencyPair cp, UTILS::Side side, double price, double quantity)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        Order o;
        o.id = "o" + std::to_string(nextId++);
        o.side = side;
        o.price = price;
        o.quantity = quantity;
        o.filled = 0.0;
        o.status = OrderStatus::NEW;
        m_orders[o.id] = o;

        m_connectionManager->OrderConnection()->SendOrder(cp, side, RESTAPI::EOrderType::Limit, UTILS::TimeInForce::GTC, price, quantity);
        Logger::info("Placed order " + o.id + " " + (side==UTILS::Side::BUY ? "BUY" : "SELL")
                     + " @" + std::to_string(price) + " qty=" + std::to_string(quantity));

        return o.id;
    }

    bool OrderManager::CancelOrder(const UTILS::CurrencyPair cp, const std::string &orderId)
    {
        std::lock_guard<std::mutex> g(m_mutex);

        if (!m_orders.count(orderId))
            return false;

        auto &o = m_orders[orderId];
        if (o.status == OrderStatus::FILLED || o.status == OrderStatus::CANCELED)
            return false;

        o.status = OrderStatus::CANCELED;
        Logger::info("Canceled order " + orderId);

        return true;
    }

    std::optional<Order> OrderManager::GetOrder(const UTILS::CurrencyPair cp, const std::string &orderId)
    {
        std::lock_guard<std::mutex> g(m_mutex);

        if (!m_orders.count(orderId))
            return {};

        return m_orders[orderId];
    }

    double OrderManager::GetBalance(const std::string &asset)
    {
        std::lock_guard<std::mutex> g(m_mutex);

        if (asset=="USDT")
            return usdt;

        if (asset=="BTC")
            return btc;

        return 0.0;
    }

    void OrderManager::setBalances(double _usdt, double _btc)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        usdt=_usdt;
        btc=_btc;
    }

    void OrderManager::dumpBalances()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        std::cout << "Balances: USDT=" << usdt << " BTC=" << btc << " price=" << price << std::endl;
    }
}