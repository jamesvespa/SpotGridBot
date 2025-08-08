

#include "Utils/Latency.h"
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>

using namespace UTILS::Latency;

/* Latency config XML example
    <LatencyConfig logging_period="60">
        <Metrics>
            <Metric name="Fix.SessionBase::handle_application.Delay" logging_period="30" active="true">
                <Parameter name="long_delay" value="4000000"/>
            </Metric>
        </Metrics>
    </LatencyConfig>
*/

CLatencyEntry::CLatencyEntry(UTILS::XmlDocPtr config, Poco::Logger &logger, const std::string &name, TPopulateValuesFunc populateValuesFunc,
							 TBeforeLoggingValuesFunc beforeLoggingValuesFunc, const int64_t loggingStartTime)
		: m_logger(logger), m_name(name), m_populateValuesFunc(populateValuesFunc), m_beforeLoggingValuesFunc(beforeLoggingValuesFunc),
		  m_loggingPeriodStart(loggingStartTime)
{
	if (!m_populateValuesFunc)
	{
		throw Poco::Exception("CLatencyEntry: populateValuesFunc must be specified");
	}
	
	int loggingPeriodInSec = DefaultLoggingPeriodSeconds;
	if (config)
	{
		if (const auto *latencyConfigNode = UTILS::GetConfigNode(config, CFG_TAG_LATENCY_CONFIG))
		{
			// General logging period
			loggingPeriodInSec = ReadLoggingPeriod(*latencyConfigNode, DefaultLoggingPeriodSeconds);
			// Find settings for the metric
			if (const auto *metricNode = latencyConfigNode->getNodeByPath(CFG_TAG_LATENCY_METRICS))
			{
				if (const auto *metrics = metricNode->childNodes())
				{
					for (long unsigned int i = 0; i < metrics->length(); ++i)
					{
						const auto *metric = metrics->item(i);
						if (UTILS::GetXmlAttribute<std::string>(metric, CFG_ATTR_NAME) == name)
						{
							// Activity flag
							const char *const str_true = "true";
							m_active = UTILS::GetXmlAttribute<std::string>(metric, CFG_ATTR_ACTIVE, str_true) == str_true;
							// Check if we have special logging period
							loggingPeriodInSec = ReadLoggingPeriod(*metric, loggingPeriodInSec);
							// Read custom metric parameters
							if (const auto *parameters = metric->childNodes())
							{
								for (long unsigned int i = 0; i < parameters->length(); ++i)
								{
									const auto *parameter = parameters->item(i);
									if (parameter->nodeName() == CFG_TAG_LATENCY_PARAMETER)
									{
										const auto attrName = UTILS::GetXmlAttribute<std::string>(parameter, CFG_ATTR_NAME);
										if (attrName && !attrName.value().empty())
										{
											m_parameters[attrName.value()] = UTILS::GetXmlAttribute(parameter, CFG_ATTR_VALUE, 0);
										}
									}
								} // for parameters
								parameters->release();
							} // if parameters
							break; // we expect only one setting for the metric
						} // if metric name
					} // for metrics
					metrics->release();
				} // if metrics
			} // if metricNode
		} // if latencyConfigNode
	} // if config
	m_loggingPeriod = CalcLoggingPeriod(loggingPeriodInSec);
	
	poco_information_f3(m_logger, "*** Set up latency: name= '%s', logging_period = %s, number of parameters = %s", m_name,
						std::to_string(m_loggingPeriod), std::to_string(m_parameters.size()));
	if (!m_parameters.empty())
	{
		poco_information(m_logger, "  Parameters:");
		for (const auto &param: m_parameters)
			poco_information_f2(m_logger, "    %s -> %s", param.first, std::to_string(param.second));
	}
}


// Reads logging period from XML with validation
int CLatencyEntry::ReadLoggingPeriod(const Poco::XML::Node &node, const int defaultValue)
{
	const auto loggingPeriodInSec = UTILS::GetXmlAttribute(&node, CFG_ATTR_LOGGING_PERIOD, defaultValue);
	if (loggingPeriodInSec <= 0)
	{
		poco_error_f2(m_logger, "Invalid logging period in latency '%s': %s", m_name, loggingPeriodInSec);
		return defaultValue;
	}
	return loggingPeriodInSec;
}


/**
 * @brief Should be called where we collect the metric
 */
void CLatencyEntry::Record(const int64_t currentTime)
{
	if (!IsActive())
	{
		return; // not active - do nothing
	}
	
	// Collect values
	m_populateValuesFunc(currentTime, m_values, *this);
	if (currentTime - m_loggingPeriodStart >= m_loggingPeriod)
	{
		// Time to log results
		// Prepare values (if function specified)
		if (m_beforeLoggingValuesFunc)
		{
			m_beforeLoggingValuesFunc(m_values);
		}
		
		if (!m_values.empty())
		{
			// Generate output in sorted order
			std::map<std::string, TValues::mapped_type> sortedValues;
			sortedValues.insert(m_values.cbegin(), m_values.cend());
			std::stringstream ss;
			for (const auto &pair: sortedValues)
			{
				ss << pair.first << ":" << pair.second << "; ";
			}
			poco_information_f2(m_logger, "Latency: %s -> %s", m_name, ss.str());
			// Reset data for the next period
			m_values.clear();
		}
		
		m_loggingPeriodStart = currentTime;
	}
}
