//
// Created by james on 08/08/2025.
//
#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include "exchange.h"

namespace STRATEGY {

  class GridStrategy {
  public:
    GridStrategy(std::shared_ptr<ExchangeAPI> ex, GridConfig cfg) : m_ex(ex), m_cfg(cfg) {

    }
    ~GridStrategy() = default;

    void start();
    void onTicker();
    void placeInitialGrid();
    void checkFilledOrders();
    void dumpStatus();

  private:
    std::shared_ptr<ExchangeAPI> m_ex;
    GridConfig m_cfg;
    std::vector<std::string> m_activeOrders;
    struct Meta { OrderSide side; double price; double qty; };

    std::unordered_map<std::string, Meta> m_orderMeta;
    std::unordered_map<std::string, double> m_knownFills;
  };
}