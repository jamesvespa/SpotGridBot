/**
* @file
* @author Helmut <james.doll@bbf-it.at>
* @date 5/15/24
* @version * @copyright (c) 2024 BBF-IT
*/

#ifndef MERCURYUTILS_VALUEDATECALENDAR_H
#define MERCURYUTILS_VALUEDATECALENDAR_H

#include "Utils/CurrencyPair.h"
#include "Utils/Result.h"

namespace UTILS {

/*! \brief Value date calendar
 *
 * This class loads value date information from the DB and enables to
 * determine the value date from the transaction date.
 */
class ValueDateCalendar
{
public:
	friend class ValueDateCalendarLoader;
	
	enum TimeZone
	{
		tzAmericaNewYork, tzPacificAuckland
	};
	
	struct RollTime
	{
		int64_t time;
		TimeZone timeZone;
	};
	
	/** Checks whether the map of value dates is available.
	 *
	 * @param transactionDate Transaction date to check
	 * @return \a true if the map of value dates is available for
	 * the given transaction date
	 */
	bool HasValueDates(int64_t transactionDate) const { return m_valueDateMap.find(ExtractDate(transactionDate)) != m_valueDateMap.end(); }
	
	/** Determines the trade date
	 *
	 * Static function.
	 *
	 * This function calculates the transaction date to be used for value date
	 * determination by considering the roll time associated with the given
	 * currency pair.
	 *
	 * @param cp Currency pair
	 * @param transactionTimeUtc UTC transaction timestamp
	 * @return Trade date to be used for value date
	 * calculation
	 */
	static UTILS::Result <int64_t> GetTradeDate(UTILS::CurrencyPair cp, int64_t transactionTimeUtc);
	
	/** Retrieves the value date for a currency pair and a UTC transaction time
	 *
	 * @param cp Currency pair (with tenor)
	 * @param transactionTimeUtc UTC transaction time
	 * @param tenor tenor for settlement type
	 * @return Positive result with the value date for \a cp and
	 * \a transactionTimeUtc, or negative result with error message
	 * if the value date could not be retrieved
	 */
	UTILS::Result<int64_t> GetValueDate(UTILS::CurrencyPair cp, int64_t transactionTimeUtc, bool fallbackToDefault = false) const;
	
	/** Retrieves the roll time for a currency pair
	 *
	 * Static function.
	 *
	 * @param cp Currency pair
	 * @return Roll time for \a cp
	 */
	static RollTime GetRollTime(UTILS::CurrencyPair cp);
	
	/** Retrieves the value date offset for a given ccy pair
	 *
	 * @param cp Currency pair
	 * @return Value date offset in days
	 */
	static int64_t GetSpotOffset(UTILS::CurrencyPair cp);
	
	/** Retrieves the default value date for a currency pair and
	 * a UTC transaction time
	 *
	 * Static function.
	 *
	 * @param cp Currency pair
	 * @param transactionTimeUtc UTC transaction time
	 * @return spotOffset (optional) Value date offset in days (default: call GetSpotOffset())
	 * @return Default value date for \a cp and \a transactionTimeUtc
	 */
	static int64_t GetDefaultSpotDate(UTILS::CurrencyPair cp, int64_t transactionTimeUtc, int64_t spotOffset = -1);
	
	/** Extract the date from a timestamp
	 *
	 * Static function
	 *
	 * This function subtracts the time information from a given timestamp.
	 *
	 * @param ts timestamp
	 * @return Date information taken from \a ts
	 */
	static int64_t ExtractDate(int64_t ts);
	
	/** Extract the time from a timestamp
	 *
	 * Static function
	 *
	 * This function subtracts the date information from a given timestamp.
	 *
	 * @param ts timestamp
	 * @return Time information taken from \a ts
	 */
	static int64_t ExtractTime(int64_t ts);
	
	std::string GetInfo() const;
	
protected: // accessible for tests
	using CcyMap = std::map<UTILS::CurrencyPair, int64_t>;
	
	using ValueDateMap = std::map<int64_t, CcyMap>;
	
	/** Sets the value date map
	 *
	 * ONLY FOR TESTING!
	 *
	 * @param valueDateMap Value date map to be set
	 */
	void SetValueDateMap(ValueDateMap valueDateMap) { m_valueDateMap = std::move(valueDateMap); }
	
	/** Returns the value date map for a given transaction date
	 *
	 * @param transactionDate Transaction date
	 * @return Positive result containing a pointer to a value date map,
	 * or a negative result containing an error message.
	 */
	UTILS::Result<CcyMap *> GetValueDateMap(int64_t transactionDate);
	
	/** Returns the value date map for a given transaction date
	 *
	 * @param transactionDate Transaction date
	 * @return Positive result containing a pointer to a value date map,
	 * or a negative result containing an error message.
	 */
	UTILS::Result<const CcyMap *> GetValueDateMap(int64_t transactionDate) const;
	
	/** Checks whether a given timestamp falls into the period of
	 * Daylight Saving Time (DST)
	 *
	 * The function uses the currently used schemes for switching between ST and DST:
	 *
	 * New York time (America/New York):
	 *
	 * EST -> EDT: 2nd Sunday in March 02:00 -> 03:00
	 * EDT -> EST: 1st Sunday in November 02:00 -> 01:00
	 *
	 * New Zealand time (Pacific/Auckland):
	 *
	 * NZST -> NZDT: last Sunday in September 02:00 -> 03:00
	 * NZDT -> NZST: 1st Sunday in April 03:00 -> 02:00
	 *
	 * @param timestamp timestamp
	 * @param timeZone either tzAmericaNewYork (EST/EDT), or tzPacificAuckland (NZST/NZDT)
	 * @return \a true if the timestamp is within the DST period
	 */
	static bool IsDst(int64_t timestamp, TimeZone timeZone);
	
	/** Converts an EST/EDT timestamp to UTC
	 *
	 * This function converts an EST/EDT timestamp to UTC time.
	 *
	 * @param timestamp timestamp
	 * @return UTC timestamp
	 */
	static int64_t ToUtc(int64_t timestamp, TimeZone timeZone);
	
	/** Returns the weekday of a timestamp
	 *
	 * This function determines the weekday of a given timestamp
	 *
	 * @param ts	Timestamp
	 * @return Weekday (0..Sun, 1..Mon, 2..Tue, 3..Wed, 4..Thu, 5..Fri, 6..Sat)
	 */
	static int64_t GetDayOfWeek(int64_t ts);

private:
	ValueDateMap m_valueDateMap; //!< Map of value dates
	
};

} // UTILS

#endif //MERCURYUTILS_VALUEDATECALENDAR_H
