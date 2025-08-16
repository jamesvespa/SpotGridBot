#include <Poco/Logger.h>
#include "Utils/XmlConfigReader.h"
#include "GridConfig.h"

const std::string TAG_GRID_CONFIG = "GridConfig";

const std::string ATTR_INSTRUMENT = "instrument";
const std::string ATTR_BASE_PRICE = "base_price";
const std::string ATTR_LEVELS_BELOW = "levels_below";
const std::string ATTR_LEVELS_ABOVE = "levels_above";
const std::string ATTR_STEP_PERCENT = "step_percent";
const std::string ATTR_PERCENT_ORDER_QTY = "percent_order_qty";
const std::string ATTR_MAX_POSITION = "max_position";

const std::string TAG_SESSION_CONFIG = "SessionConfig";
const std::string TAG_SESSION = "Session";

using namespace UTILS;

bool GridConfig::LoadConfig(const std::string &path)
{
	poco_information_f1(logger(), "Loading definitions using: %s", path);
	return LoadConfig(GetConfigDoc(path));
}

bool GridConfig::LoadConfig(const UTILS::XmlDocPtr &pDoc)
{
	 try
	 {
	 	std::string errMsg;

	 	if (auto *baseNode = UTILS::GetConfigNode(pDoc, TAG_GRID_CONFIG, &errMsg))
	 	{
	 		poco_information_f1(logger(), "Reading %s attributes from XML", TAG_SESSION);

	 		m_instrument = UTILS::GetXmlAttribute(baseNode, ATTR_INSTRUMENT, "");
	 		m_basePrice = std::stod(UTILS::GetXmlAttribute(baseNode, ATTR_BASE_PRICE, ""));
	 		m_levelsBelow = std::stoi(UTILS::GetXmlAttribute(baseNode, ATTR_LEVELS_BELOW, ""));
	 		m_levelsAbove = std::stoi(UTILS::GetXmlAttribute(baseNode, ATTR_LEVELS_ABOVE, ""));
	 		m_stepPercent = std::stod(UTILS::GetXmlAttribute(baseNode, ATTR_STEP_PERCENT, ""));
	 		m_percentOrderQty = std::stod(UTILS::GetXmlAttribute(baseNode, ATTR_PERCENT_ORDER_QTY, ""));
	 		m_maxPosition = std::stod(UTILS::GetXmlAttribute(baseNode, ATTR_MAX_POSITION, ""));
	 	}
	 	else
	 	{
	 		poco_error(logger(), "Error loading config: Invalid base node");
	 		return false;
	 	}
	 }
	 catch (std::exception &e)
	 {
	 	poco_error_f1(logger(), "Error loading config: %s", std::string(e.what()));
	 	return false;
	 }

	return true;
}
