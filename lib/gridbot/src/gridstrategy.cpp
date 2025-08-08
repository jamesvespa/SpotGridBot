// grid_bot.cpp
// PoC grid trading bot with enhanced mock: partial fills + slippage
#include <bits/stdc++.h>
#include <chrono>
#include <thread>
#include "gridstrategy.h"

using namespace std;
namespace STRATEGY {
  void GridStrategy::start() {
    placeInitialGrid();
  }
  void GridStrategy::onTicker() {
    checkFilledOrders();
  }
  void GridStrategy::placeInitialGrid() {
    double base = m_cfg.gridBasePrice;
    double step = m_cfg.stepPercent;
    for (int i=1;i<=m_cfg.levelsBelow;i++) {
      double price = base * (1.0 - step * i);
      string oid = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::BUY, price, m_cfg.perOrderQty);
      m_activeOrders.push_back(oid);
      m_orderMeta[oid] = {OrderSide::BUY, price, m_cfg.perOrderQty};
    }
    for (int i=1;i<=m_cfg.levelsAbove;i++) {
      double price = base * (1.0 + step * i);
      string oid = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::SELL, price, m_cfg.perOrderQty);
      m_activeOrders.push_back(oid);
      m_orderMeta[oid] = {OrderSide::SELL, price, m_cfg.perOrderQty};
    }
    Logger::info("Initial grid placed: " + to_string(m_activeOrders.size()) + " orders");
  }

  void GridStrategy::checkFilledOrders() {
    vector<string> toRemove;
    for (auto &oid : m_activeOrders) {
      auto maybe = m_ex->getOrder(m_cfg.pair, oid);
      if (!maybe) continue;
      Order o = *maybe;
      if (o.status == OrderStatus::FILLED) {
        if (m_orderMeta[oid].side==OrderSide::BUY) {
          double sellPrice = m_orderMeta[oid].price * (1.0 + m_cfg.stepPercent);
          double btc = m_ex->getBalance("BTC");
          if (btc > m_cfg.maxPositionBtc + 1e-12) {
            Logger::warn("Max position exceeded, not placing hedge sell");
          } else {
            string newId = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::SELL, sellPrice, m_orderMeta[oid].qty);
            m_activeOrders.push_back(newId);
            m_orderMeta[newId] = {OrderSide::SELL, sellPrice, m_orderMeta[oid].qty};
          }
        } else {
          double buyPrice = m_orderMeta[oid].price * (1.0 - m_cfg.stepPercent);
          double usdt = m_ex->getBalance("USDT");
          double cost = buyPrice * m_orderMeta[oid].qty;
          if (usdt + 1e-12 < cost) {
            Logger::warn("Insufficient USDT to place rebuy");
          } else {
            string newId = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::BUY, buyPrice, m_orderMeta[oid].qty);
            m_activeOrders.push_back(newId);
            m_orderMeta[newId] = {OrderSide::BUY, buyPrice, m_orderMeta[oid].qty};
          }
        }
        toRemove.push_back(oid);
      } else if (o.status == OrderStatus::PARTIALLY_FILLED) {
        // handle partial fills: we keep the order live but may place hedge for filled portion
        double filled = o.filled;
        double knownFilled = m_knownFills[oid];
        if (filled - knownFilled > 1e-12) {
          double delta = filled - knownFilled;
          m_knownFills[oid] = filled;
          Logger::info("Detected new partial fill " + oid + " delta=" + to_string(delta));
          // for filled portion, place opposite order at next step
          if (m_orderMeta[oid].side==OrderSide::BUY) {
            double sellPrice = m_orderMeta[oid].price * (1.0 + m_cfg.stepPercent);
            double btc = m_ex->getBalance("BTC");
            if (btc <= m_cfg.maxPositionBtc + 1e-12) {
              string newId = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::SELL, sellPrice, delta);
              m_activeOrders.push_back(newId);
              m_orderMeta[newId] = {OrderSide::SELL, sellPrice, delta};
            }
          } else {
            double buyPrice = m_orderMeta[oid].price * (1.0 - m_cfg.stepPercent);
            double usdt = m_ex->getBalance("USDT");
            double cost = buyPrice * delta;
            if (usdt + 1e-12 >= cost) {
              string newId = m_ex->placeLimitOrder(m_cfg.pair, OrderSide::BUY, buyPrice, delta);
              m_activeOrders.push_back(newId);
              m_orderMeta[newId] = {OrderSide::BUY, buyPrice, delta};
            }
          }
        }
      } else if (o.status == OrderStatus::REJECTED || o.status==OrderStatus::CANCELED) {
        toRemove.push_back(oid);
      }
    }
    for (auto &r : toRemove) {
      m_activeOrders.erase(remove(m_activeOrders.begin(), m_activeOrders.end(), r), m_activeOrders.end());
      m_orderMeta.erase(r);
      m_knownFills.erase(r);
    }
  }

  void GridStrategy::dumpStatus() {
    Logger::info("Active orders: " + to_string(m_activeOrders.size()));
    for (auto &oid : m_activeOrders) {
      auto m = m_orderMeta[oid];
      cout << " - " << oid << " " << (m.side==OrderSide::BUY?"BUY":"SELL")
           << " @"<< m.price << " qty="<<m.qty<<endl;
    }
  }
}
