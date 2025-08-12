#pragma once
#include <mutex>

#include "IOrderManager.h"

namespace CORE {

  class OrderManager : public CORE::IOrderManager {
  public:
    OrderManager(std::shared_ptr<CORE::ConnectionManager> connectionManager) : m_connectionManager(connectionManager) {
    }

    std::string PlaceLimitOrder(const UTILS::CurrencyPair cp, OrderSide side, double p, double q) override {
      std::lock_guard<std::mutex> g(m_mutex);
      Order o;
      o.id = "o" + std::to_string(nextId++);
      o.side = side;
      o.price = p;
      o.quantity = q;
      o.filled = 0.0;
      o.status = OrderStatus::NEW;
      m_orders[o.id] = o;

      Logger::info("Placed order " + o.id + " " + (side==OrderSide::BUY?"BUY":"SELL")
                   + " @" + std::to_string(p) + " qty=" + std::to_string(q));

      return o.id;
    }

    bool CancelOrder(const UTILS::CurrencyPair cp, const std::string &orderId) override {
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

    std::optional<Order> GetOrder(const UTILS::CurrencyPair cp, const std::string &orderId) override {
      std::lock_guard<std::mutex> g(m_mutex);

      if (!m_orders.count(orderId))
        return {};

      return m_orders[orderId];
    }

    double GetBalance(const std::string &asset) override {
      std::lock_guard<std::mutex> g(m_mutex);

      if (asset=="USDT")
        return usdt;

      if (asset=="BTC")
        return btc;

      return 0.0;
    }

    void setBalances(double _usdt, double _btc) override {
      std::lock_guard<std::mutex> g(m_mutex);
      usdt=_usdt;
      btc=_btc;
    }

    void dumpBalances() override {
      std::lock_guard<std::mutex> g(m_mutex);
      std::cout << "Balances: USDT=" << usdt << " BTC=" << btc << " price=" << price << std::endl;
    }

    std::shared_ptr<CORE::ConnectionManager> GetConnectionManager() { return m_connectionManager; }

  private:
    std::mutex m_mutex;
    double price;
    double fee;
    double partialMinPct;
    double partialMaxPct;
    double slippageMax;
    std::unordered_map<std::string,Order> m_orders;
    long long nextId = 1;
    double usdt = 10000.0;
    double btc = 0.0;

    std::shared_ptr<CORE::ConnectionManager> m_connectionManager;
  };
}
