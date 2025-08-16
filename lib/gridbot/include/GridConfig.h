#pragma once

#include <string>
#include "Utils/ErrorHandler.h"
#include "Utils/Logging.h"
#include "Utils/Utils.h"

// ---------- Grid Strategy ----------
class GridConfig : public UTILS::Logging, public UTILS::ErrorHandler {
public:
    GridConfig(const std::string &path) : Logging("GridConfig") {
        LoadConfig(path);
    }

    bool LoadConfig(const std::string &path);
    bool LoadConfig(const UTILS::XmlDocPtr &pDoc);

    std::string m_instrument;
    double m_basePrice;
    int m_levelsBelow;
    int m_levelsAbove;
    double m_stepPercent;
    double m_perOrderQty;
    double m_maxPosition;
};
