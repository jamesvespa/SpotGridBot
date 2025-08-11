#pragma once

enum class OrderSide;

namespace CORE {
  class ConnectionManager;

  class IOrderManager {
  public:
    virtual std::string PlaceLimitOrder(const std::string &pair, OrderSide side, double p, double q)=0;
    virtual bool CancelOrder(const std::string &pair, const std::string &orderId)=0;
    virtual std::optional<Order> GetOrder(const std::string &pair, const std::string &orderId)=0;
    virtual double GetBalance(const std::string &asset)=0;
    virtual void setBalances(double _usdt, double _btc)=0;
    virtual void dumpBalances()=0;
  };
}
