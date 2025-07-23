//
// Created by helmut on 1/18/19.
//

#ifndef CORE_SETTINGSRECORD_H
#define CORE_SETTINGSRECORD_H

#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include "Utils/CurrencyPair.h"
#include "Utils/Logging.h"
#include "Utils/ErrorHandler.h"
#include "Utils/Lockable.h"

namespace Poco::XML {
	class Node;
}

namespace CORE {
namespace DATA {

using AttributeVariant = std::variant<std::string, int64_t, double, bool>; //!<  Variant type for attribute values

using AttributeMap = std::map<std::string, AttributeVariant>; //!< Attribute map (name -> value)
using AttributeMapVector = std::vector<std::pair<std::string, AttributeMap>>; //!< Vector of attribute maps

using AttributeCache = std::map<std::string, std::optional<AttributeVariant>>; //!< Attribute map (name -> optional value, for cache)
using AttributeCacheMap = std::map<UTILS::CurrencyPair, AttributeCache>; //!< Attribute cache map (currency pair -> attribute cache)

/*! \brief Container class for instrument-specific settings
 *
 * This class enables the representation of instrument-specific settings.
 * The settings may be loaded either from an XML structure or from a BSON
 * array (coming from a Mongo document).
 */
class Settings : UTILS::Logging, public UTILS::ErrorHandler
{
public:
	/*! \brief Default constructor
	 */
	Settings()
			: Logging("Settings"), ErrorHandler(pLogger()) { }
	
	/*! \brief Copy constructor
	 */
	Settings(const Settings &rhs)
			: Settings()
	{
		m_attributeMapVector = rhs.m_attributeMapVector;
	}
	
	/*! \brief Copy assignment operator
	 */
	Settings &operator=(const Settings &rhs)
	{
		m_attributeMapVector = rhs.m_attributeMapVector;
		ClearCache();
		return *this;
	}
	
	bool Load(const Poco::XML::Node *node, const std::string &elementName);
	
	template <typename T>
	std::optional<T> Get(UTILS::CurrencyPair cp, const std::string &name) const
	{
		std::optional<T> result { std::nullopt };
		std::optional<AttributeVariant> val { GetVariant(cp, name) };
		if (val.has_value())
		{
			try
			{
				result = std::get<T>(val.value());
			}
			catch (...)
			{
				if constexpr (std::is_same_v<T, std::string>)
				{
					try
					{
						std::visit([&result](const auto &tval)
								   {
									   using V = std::decay_t<decltype(tval)>;
									   if constexpr (std::is_same_v<V, std::string>)
									   {
										   result = tval;
									   }
									   else
									   {
										   result = std::to_string(tval);
									   }
								   }, val.value());
					}
					catch (...)
					{
						result = std::nullopt;
					}
				}
				else
				{
					result = std::nullopt;
				}
			}
		}
		return result;
	}

	/**
	 * Determines the exclusivity interval for a given ccy pair at a given time
	 *
	 * @param cp Currency pair
	 * @param ts (optional) Timestamp (default: current time)
	 * @return Exclusivity interval in nanoseconds on success, @a error message on failure
	 */
	UTILS::Result<int64_t> GetExclusivityInterval(UTILS::CurrencyPair cp, int64_t ts = 0) const;
	
	/*! \brief Iterates over the vector of instrument specific settings
	 *
	 * Function template
	 *
	 * Calls a given function for each map of instrument specific settings.
	 * If the callback function returns \a true, the loop is exited.
	 *
	 * @tparam F	Function type
	 * @param  func Function to be executed
	 * @return \a true if the callback function returned \a true, \a false
	 * if no matching attribute map was found, or if the callback function
	 * returned \a false was all elements.
	 */
	template <typename F>
	bool ForEachMap(F func) const
	{
		bool result { false };
		for (const auto &item : m_attributeMapVector)
		{
			result = func(item.first, item.second);
			if (result)
			{
				break;
			}
		}
		return result;
	}
	
	/*! \brief Iterates over the vector of instrument specific settings
	 *
	 * Function template
	 *
	 * Calls a given function for each map of instrument specific settings
	 * that matches a given currency pair. If the callback function returns
	 * true, the loop is exited.
	 *
	 * @tparam F	Function type
	 * @param  cp   Currency pair to match with the instrument mask of the
	 * settings map
	 * @param  func Function to be executed
	 * @return \a true if the callback function returned \a true, \a false
	 * if no matching attribute map was found, or if the callback function
	 * returned \a false was all elements.
	 */
	template <typename F>
	bool ForEachMap(UTILS::CurrencyPair cp, F func) const
	{
		return ForEachMap([cp, &func](const std::string &mask, const AttributeMap &map)
						  {
							  return cp.MatchesMask(mask) && func(mask, map);
						  });
	}
	void ClearAll();
private:
	AttributeMapVector m_attributeMapVector; //!< Instrument specific settings
	mutable UTILS::Lockable<AttributeCacheMap> m_attributeCacheMap; //!< Cache of instrument specific settings

	bool Add(const Poco::XML::Node *node);
	
	void AddMap(const std::string &mask, AttributeMap &&map);
	
	std::optional<AttributeVariant> GetVariant(UTILS::CurrencyPair cp, const std::string &name) const;
	
	void ClearCache() const;
};

} // namespace DATA
} // namespace CORE


#endif //CORE_SETTINGSRECORD_H
