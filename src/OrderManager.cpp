#include "OrderManager.h"

#include "ConnectionManager.h"
#include "Utils/CurrencyPair.h"

namespace CORE {
    std::string OrderManager::PlaceLimitOrder(const UTILS::CurrencyPair cp, UTILS::Side side, double price, double quantity)
    {
        std::lock_guard<std::mutex> g(m_mutex);

        CRYPTO::JSONDocument response(m_connectionManager->OrderConnection()->SendOrder(cp, side, RESTAPI::EOrderType::Limit, UTILS::TimeInForce::GTC, price, quantity));

        if (response.GetValue<std::string>("status").c_str()=="true")
        {
            Order order;
            order.side = side;
            order.price = price;
            order.quantity = quantity;
            order.filled = 0.0;
            order.status = OrderStatus::NEW;

            CRYPTO::JSONDocument sr(response.GetValue<std::string>("success_response"));
            order.id = sr.GetValue<std::string>("order_id");
            m_orders[order.id] = order;

            Logger::info("Placed order " + order.id + " " + (side==UTILS::Side::BUY ? "BUY" : "SELL") + " @" + std::to_string(price) + " qty=" + std::to_string(quantity));

            return order.id;
        }

        return "";
    }

    bool OrderManager::CancelOrder(const UTILS::CurrencyPair cp, const std::string &orderId)
    {
        std::lock_guard<std::mutex> g(m_mutex);

        if (!m_orders.count(orderId))
            return false;

        auto &order = m_orders[orderId];
        if (order.status == OrderStatus::FILLED || order.status == OrderStatus::CANCELED)
            return false;

        order.status = OrderStatus::CANCELED;
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