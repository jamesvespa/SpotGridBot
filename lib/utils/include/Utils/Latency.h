/**
 * @file
 * @author      Alex R <jamesander.razancev@bbf-it.at>
 * @brief       Latency class
 * @date        2022.01.26
 * @version     2022.01.31
 * @copyright   (c) 2022 BBF-IT
*/

#pragma once

#include <Poco/Logger.h>
#include <Poco/DOM/Document.h>

#include "Utils/ContextBase.h"

namespace UTILS {
namespace Latency {

const char* const CFG_TAG_LATENCY_CONFIG = "LatencyConfig";
const char* const CFG_TAG_LATENCY_METRICS = "Metrics";
const char* const CFG_TAG_LATENCY_PARAMETER = "Parameter";
const char* const CFG_ATTR_LOGGING_PERIOD = "logging_period";
const char* const CFG_ATTR_NAME = "name";
const char* const CFG_ATTR_VALUE = "value";
const char* const CFG_ATTR_ACTIVE = "active";

/*! \brief Latency entry class
*
* To be used to log latency metrics in a standardized form
* Latency: <metric name> -> <field1 name>:<field1 value>[, <field2 name>:<field2 value>, <field2 name>:<field2 value>...]
* Example:
*    Latency: MercuryFix.SessionBase::handle_application.Delay -> total:255555, long_delays:37777, avg:4002000, min:322222, max:4500034
*/
class CLatencyEntry final {
public:
    using TValues = std::unordered_map<std::string, int64_t>;
    using TPopulateValuesFunc =
        std::function<void(const int64_t currentTime, TValues& values, const CLatencyEntry& le)>;
    using TBeforeLoggingValuesFunc = std::function<void(TValues& values)>;
    static const int DefaultLoggingPeriodSeconds = 60;

    /**
     * @brief Constructor
     * @param config                    configuration XML
     * @param logger                    poco loggger
     * @param name                      latency metric name (should be unique and well recognizable by external log analysers)
     * @param loggingPeriodNs           log period in nanoseconds (how often the metric is logged)
     * @param populateValuesFunc        functor responsible for populating values in TValues map. For performance reasons
     *                                  the keys are pointers (const char*) to the names, so a good idea to use a variable
     *                                  with stored name rather than passing strings by value, because to constants
     *                                  "myname" and "myname" may have different addresses
     * @param beforeLoggingValuesFunc   optional functor responsible for preparing the collected name:value pairs for the output
     */
    CLatencyEntry(
        UTILS::XmlDocPtr config,
        Poco::Logger& logger,
        const std::string& name,
        TPopulateValuesFunc populateValuesFunc,
        TBeforeLoggingValuesFunc beforeLoggingValuesFunc = nullptr,
        const int64_t loggingStartTime = UTILS::CurrentTimestamp());
    CLatencyEntry(const CLatencyEntry&) = delete;
    CLatencyEntry& operator=(const CLatencyEntry&) = delete;

    /**
     * @brief Should be called where we collect the metric
     */
    void Record(const int64_t currentTime = UTILS::CurrentTimestamp());

    /**
     * @brief Returns parameter by name
     * @param   name - parameter name
     * @param   defaultVal - default value if parameter not found
     */
    // Returns metric parameter by name
    int64_t GetParameter(const std::string& name, const int64_t defaultVal = 0) const
    {
        const auto iter = m_parameters.find(name);
        return iter == m_parameters.cend() ? defaultVal : iter->second;
    }

    /**
     * @brief Returns true if metric is active
     */
    bool IsActive() const
    {
        return m_active;
    }

    const std::string& GetName() const
    {
        return m_name;
    }

    const TValues& GetValues() const
    {
        return m_values;
    }

    const TValues& GetParameters() const
    {
        return m_parameters;
    }

    int64_t GetLoggingPeriod() const
    {
        return m_loggingPeriod;
    }

    int64_t GetLoggingStartTime() const
    {
        return m_loggingPeriodStart;
    }

    static int64_t CalcLoggingPeriod(const int loggingPeriodInSec)
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(loggingPeriodInSec)).count();
    }

private:
    // Reads logging period from XML with validation
    int ReadLoggingPeriod(const Poco::XML::Node& node, const int defaultValue);

    Poco::Logger& m_logger;
    TValues m_values;
    TValues m_parameters;
    std::string m_name;
    TPopulateValuesFunc m_populateValuesFunc;
    TBeforeLoggingValuesFunc m_beforeLoggingValuesFunc;
    int64_t m_loggingPeriodStart;
    int64_t m_loggingPeriod;
    bool m_active = true;
}; // CLatencyEntry

} // ns Latency
} // ns UTILS