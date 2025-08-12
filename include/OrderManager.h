#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "exchange.h"
#include "IOrderManager.h"
#include "Utils/CurrencyPair.h"
#include "Utils/FixTypes.h"

namespace CORE {

  class OrderManager : public CORE::IOrderManager {
  public:
    OrderManager(std::shared_ptr<CORE::ConnectionManager> connectionManager) : m_connectionManager(connectionManager)
    {
    }

    std::string PlaceLimitOrder(const UTILS::CurrencyPair cp, UTILS::Side side, double price, double quantity);
    bool CancelOrder(const UTILS::CurrencyPair cp, const std::string &orderId);
    std::optional<Order> GetOrder(const UTILS::CurrencyPair cp, const std::string &orderId);
    double GetBalance(const std::string &asset);
    void setBalances(double _usdt, double _btc);
    void dumpBalances();

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
