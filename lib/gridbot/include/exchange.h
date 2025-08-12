//
// Created by james on 08/08/2025.
//
#pragma once

#include <optional>
#include <string>
#include <fstream>
#include <memory>
#include <unordered_map>

#include "Utils/FixTypes.h"

// Very small JSON parsing helper tuned for our simple config file.
// Not a full JSON parser — acceptable for PoC.

struct SimpleConfig {
  std::string pair = "BTC/USDT";
  double gridBasePrice = 30000.0;
  int levelsBelow = 4;
  int levelsAbove = 4;
  double stepPercent = 0.005;
  double perOrderQty = 0.001;
  double maxPositionBtc = 2.0;
  double feeRate = 0.001;
  double partialFillMinPct = 0.3;
  double partialFillMaxPct = 1.0;
  double slippageMaxPct = 0.002;
  int tickDelayMs = 500;
  int simulateTicks = 200;
};

static std::string loadConfig(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    std::cerr << "[WARN] Cannot open config.json -- using defaults\n";
    return std::string();
  }
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

  return content;
}

// ---------- Utilities ----------
struct Logger {
  static void info(const std::string &s){ std::cout << "[INFO] " << s << std::endl; }
  static void warn(const std::string &s){ std::cout << "[WARN] " << s << std::endl; }
  static void debug(const std::string &s){ std::cout << "[DEBUG] " << s << std::endl; }
};

struct Ticker { double bid; double ask; double last; };

enum class OrderSide { BUY, SELL };
enum class OrderStatus { NEW, PARTIALLY_FILLED, FILLED, CANCELED, REJECTED };

struct Order {
  std::string id;
  UTILS::Side side;
  double price;
  double quantity;      // original quantity
  double filled = 0.0;  // filled so far
  OrderStatus status = OrderStatus::NEW;
};

class ExchangeAPI {
public:
  virtual ~ExchangeAPI() = default;
  virtual Ticker getTicker(const std::string &pair) = 0;
  virtual std::string placeLimitOrder(const std::string &pair, UTILS::Side side, double price, double qty) = 0;
  virtual bool cancelOrder(const std::string &pair, const std::string &orderId) = 0;
  virtual std::optional<Order> getOrder(const std::string &pair, const std::string &orderId) = 0;
  virtual double getBalance(const std::string &asset) = 0;
};

// ---------- Enhanced Mock Exchange ----------
class MockExchange : public ExchangeAPI {
public:
  MockExchange(double initial_price, double fee_rate = 0.001,
               double partialMin = 0.3, double partialMax = 1.0, double slippageMaxPct = 0.002)
    : price(initial_price), fee(fee_rate), partialMinPct(partialMin), partialMaxPct(partialMax),
      slippageMax(slippageMaxPct) {
    usdt = 10000.0;
    btc = 0.1;
    srand((unsigned)time(nullptr));
  }

  Ticker getTicker(const std::string &pair) override {
    std::lock_guard<std::mutex> g(m);
    double spread = price * 0.0005;
    return {price - spread, price + spread, price};
  }

  std::string placeLimitOrder(const std::string &pair, UTILS::Side side, double p, double q) override {
    std::lock_guard<std::mutex> g(m);
    Order o;
    o.id = "o" + std::to_string(nextId++);
    o.side = side;
    o.price = p;
    o.quantity = q;
    o.filled = 0.0;
    o.status = OrderStatus::NEW;
    orders[o.id] = o;
    Logger::info("Placed order " + o.id + " " + (side == UTILS::Side::BUY ? "BUY" : "SELL")
                 + " @" + std::to_string(p) + " qty=" + std::to_string(q));
    return o.id;
  }

  bool cancelOrder(const std::string &pair, const std::string &orderId) override {
    std::lock_guard<std::mutex> g(m);
    if (!orders.count(orderId)) return false;
    auto &o = orders[orderId];
    if (o.status == OrderStatus::FILLED || o.status == OrderStatus::CANCELED) return false;
    o.status = OrderStatus::CANCELED;
    Logger::info("Canceled order " + orderId);
    return true;
  }

  std::optional<Order> getOrder(const std::string &pair, const std::string &orderId) override {
    std::lock_guard<std::mutex> g(m);
    if (!orders.count(orderId)) return {};
    return orders[orderId];
  }

  double getBalance(const std::string &asset) override {
    std::lock_guard<std::mutex> g(m);
    if (asset=="USDT") return usdt;
    if (asset=="BTC") return btc;
    return 0.0;
  }

  // simulate a price move and attempt to (partially) fill orders based on crossing
  void simulatePriceMove(double toPrice) {
    std::lock_guard<std::mutex> g(m);
    price = toPrice;
    // iterate orders and check if price crosses
    for (auto &kv : orders) {
      auto &o = kv.second;
      if (o.status == OrderStatus::FILLED || o.status == OrderStatus::CANCELED || o.status==OrderStatus::REJECTED) continue;
      bool cross = false;
      if (o.side==UTILS::Side::BUY && price <= o.price + 1e-12) cross = true;
      if (o.side==UTILS::Side::SELL && price >= o.price - 1e-12) cross = true;
      if (!cross) continue;
      // determine partial fill pct
      double pct = partialMinPct + (double)(rand())/RAND_MAX * (partialMaxPct - partialMinPct);
      pct = std::min(1.0, std::max(0.0, pct));
      double remaining = o.quantity - o.filled;
      double fillQty = remaining * pct;
      if (fillQty < 1e-12) continue;
      // slippage applied to execution price
      double slip = ( (double)rand()/RAND_MAX * 2.0 - 1.0 ) * slippageMax;
      double execPrice = o.price * (1.0 + slip);
      if (o.side==UTILS::Side::BUY) {
        double cost = fillQty * execPrice;
        if (usdt + 1e-12 < cost) {
          o.status = OrderStatus::REJECTED;
          Logger::warn("Order " + o.id + " rejected insufficient USDT for buy");
          continue;
        }
        usdt -= cost;
        double received = fillQty * (1.0 - fee); // fee deducted from asset received
        btc += received;
      } else { // SELL
        if (btc + 1e-12 < fillQty) {
            o.status = OrderStatus::REJECTED;
            Logger::warn("Order " + o.id + " rejected insufficient BTC for sell");
            continue;
        }
        btc -= fillQty;
        double proceeds = fillQty * execPrice * (1.0 - fee); // fee deducted from proceeds
        usdt += proceeds;
      }
      o.filled += fillQty;
      if (o.filled + 1e-12 >= o.quantity) {
        o.status = OrderStatus::FILLED;
        Logger::info("Order " + o.id + " FILLED qty=" + std::to_string(o.filled) + " execPrice=" + std::to_string(execPrice));
      } else {
        o.status = OrderStatus::PARTIALLY_FILLED;
        Logger::info("Order " + o.id + " PARTIALLY_FILLED qty=" + std::to_string(o.filled) + " execPrice=" + std::to_string(execPrice));
      }
    }
  }

  void setBalances(double _usdt, double _btc) { std::lock_guard<std::mutex> g(m); usdt=_usdt; btc=_btc; }

  void dumpBalances() {
    std::lock_guard<std::mutex> g(m);
    std::cout << "Balances: USDT=" << usdt << " BTC=" << btc << " price=" << price << std::endl;
  }

private:
  std::mutex m;
  double price;
  double fee;
  double partialMinPct;
  double partialMaxPct;
  double slippageMax;
  std::unordered_map<std::string,Order> orders;
  long long nextId = 1;
  double usdt = 10000.0;
  double btc = 0.0;
};

// ---------- Grid Strategy ----------
struct GridConfig {
  std::string pair = "BTC/USDT";
  double gridBasePrice = 30000.0;
  int levelsBelow = 4;
  int levelsAbove = 4;
  double stepPercent = 0.005;
  double perOrderQty = 0.001;
  double maxPositionBtc = 1.0;
};