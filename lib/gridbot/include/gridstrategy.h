//
// Created by james on 08/08/2025.
//
#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include "IOrderManager.h"
#include "Utils/CurrencyPair.h"

namespace STRATEGY {

  class GridStrategy {
  public:
    GridStrategy(std::shared_ptr<CORE::IOrderManager> orderManager, GridConfig cfg) : m_orderManager(orderManager), m_cfg(cfg), m_cp(m_cfg.pair)
    {
    }

    ~GridStrategy() = default;

    void start();
    void onTicker();
    void placeInitialGrid();
    void checkFilledOrders();
    void dumpStatus();

  private:
    std::shared_ptr<CORE::IOrderManager> m_orderManager;
    GridConfig m_cfg;
    std::vector<std::string> m_activeOrders;
    struct Meta { UTILS::Side side; double price; double qty; };

    std::unordered_map<std::string, Meta> m_orderMeta;
    std::unordered_map<std::string, double> m_knownFills;
    UTILS::CurrencyPair m_cp;
  };
}
