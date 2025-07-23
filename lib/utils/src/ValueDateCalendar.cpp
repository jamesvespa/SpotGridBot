/**
* @file
* @author Helmut <james.doll@bbf-it.at>
* @date 5/15/24
* @version * @copyright (c) 2024 BBF-IT
*/

#include "Utils/ValueDateCalendar.h"

namespace UTILS {

constexpr int64_t NANOSECONDS_PER_HOUR = (60ll * 60ll * 1'000'000'000ll);
constexpr int64_t NANOSECONDS_PER_DAY = (24ll * NANOSECONDS_PER_HOUR);

//static
int64_t ValueDateCalendar::ExtractDate(int64_t ts)
{
	return ts / NANOSECONDS_PER_DAY * NANOSECONDS_PER_DAY;
}

//static
int64_t ValueDateCalendar::ExtractTime(int64_t ts)
{
	return ts % NANOSECONDS_PER_DAY;
}

//static
bool ValueDateCalendar::IsDst(int64_t timestamp, TimeZone timeZone)
{
	FIX8::Tickval tv { timestamp };
	struct tm tm { };
	tv.as_tm(tm);
	int month { tm.tm_mon + 1 }; // [0..11] -> [1..12]
	bool dst { false };
	switch (timeZone)
	{
		case tzAmericaNewYork:
			switch (month)
			{
				case 3: // after 2nd Sunday of March -> DST
					dst = tm.tm_mday - tm.tm_wday > 7;
					if (dst && tm.tm_wday == 0 && tm.tm_mday <= 14)
					{
						dst = tm.tm_hour >= 2;
					}
					break;
				case 11: // before 1st Sunday of November -> DST
					dst = tm.tm_mday - tm.tm_wday < 1;
					if (!dst && tm.tm_wday == 0 && tm.tm_mday <= 7)
					{
						dst = tm.tm_hour < 2;
					}
					break;
				default:
					dst = month > 3 && month < 11;
					break;
			}
			break;
		case tzPacificAuckland:
			switch (month)
			{
				case 4: // before 1st Sunday of April -> DST
					dst = tm.tm_mday - tm.tm_wday < 1;
					if (!dst && tm.tm_wday == 0 && tm.tm_mday <= 7)
					{
						dst = tm.tm_hour < 3;
					}
					break;
				case 9: // after last Sunday of September -> DST
					dst = tm.tm_mday - tm.tm_wday >= 24;
					if (dst && tm.tm_wday == 0 && tm.tm_mday >= 24)
					{
						dst = tm.tm_hour >= 2;
					}
					break;
				default:
					dst = month < 4 || month > 9;
					break;
			}
			break;
	}
	return dst;
}

//static
int64_t ValueDateCalendar::ToUtc(int64_t timestamp, TimeZone timeZone)
{
	int64_t offset { timeZone == tzAmericaNewYork ? -5 : +12 };
	if (IsDst(timestamp, timeZone)) // DST: add one hour
	{
		++offset;
	}
	return timestamp - offset * NANOSECONDS_PER_HOUR;
}

//static
int64_t ValueDateCalendar::GetDayOfWeek(int64_t ts)
{
	FIX8::Tickval tv { ts };
	struct tm tm { };
	tv.as_tm(tm);
	return tm.tm_wday;
}

// static
ValueDateCalendar::RollTime ValueDateCalendar::GetRollTime(CurrencyPair cp)
{
	RollTime result { };
	if (cp.BaseCCY() == Currency::NZD || cp.QuoteCCY() == Currency::NZD) // NZD pairs: 07:00 NZST/NZDT
	{
		result.time = 7ll * NANOSECONDS_PER_HOUR; //!< 07:00 NZST/NZDT
		result.timeZone = tzPacificAuckland;
	}
	else // all other ccy pairs: 17:00 EST/EDT
	{
		result.time = 17ll * NANOSECONDS_PER_HOUR; //!< 17:00 EST/EDT
		result.timeZone = tzAmericaNewYork;
	}
	return result;
}

// static
int64_t ValueDateCalendar::GetSpotOffset(CurrencyPair cp)
{
	int64_t result { 2 };
	switch (cp.BaseCCY())
	{
		case Currency::EUR:
			result = cp.QuoteCCY() == Currency::RUB ? 1 : 2;
			break;
		case Currency::USD:
			result =
					(cp.QuoteCCY() == Currency::CAD || cp.QuoteCCY() == Currency::PHP || cp.QuoteCCY() == Currency::RUB || cp.QuoteCCY() == Currency::TRY)
					? 1 : 2;
			break;
		default:
			result = 2;
			break;
	}
	return result;
}

// static
int64_t ValueDateCalendar::GetDefaultSpotDate(CurrencyPair cp, int64_t transactionTimeUtc, int64_t spotOffset)
{
	int64_t transactionDate { GetTradeDate(cp, transactionTimeUtc).ValueOr(ExtractDate(transactionTimeUtc)) };
	if (spotOffset < 0)
	{
		spotOffset = GetSpotOffset(cp);
	}
	int64_t valueDate { transactionDate + spotOffset * NANOSECONDS_PER_DAY }; // value date = 2 days after transaction date
	
	int64_t wday { GetDayOfWeek(valueDate) };
	if (wday == 0 || wday == 6) // value date falls on Saturday or Sunday -> add another 2 days
	{
		valueDate += 2ll * NANOSECONDS_PER_DAY;
	}
	return valueDate;
}

Result<ValueDateCalendar::CcyMap *> ValueDateCalendar::GetValueDateMap(int64_t transactionDate)
{
	const auto &it { m_valueDateMap.find(ExtractDate(transactionDate)) };
	return COND_RESULT(it != m_valueDateMap.end(), &it->second,
					   Format("No value date map for transaction date %s", FormatTimestamp("%F", transactionDate)));
}

Result<const ValueDateCalendar::CcyMap *> ValueDateCalendar::GetValueDateMap(int64_t transactionDate) const
{
	const auto &it { m_valueDateMap.find(ExtractDate(transactionDate)) };
	return COND_RESULT(it != m_valueDateMap.end(), &it->second,
					   Format("No value date map for transaction date %s", FormatTimestamp("%F", transactionDate)));
}

// static
Result<int64_t> ValueDateCalendar::GetTradeDate(CurrencyPair cp, int64_t transactionTimeUtc)
{
	if (!cp.Valid())
	{
		return { setError, "Invalid currency pair" };
	}
	int64_t transactionDate { ExtractDate(transactionTimeUtc) };
	if (transactionDate <= 0)
	{
		return { setError, "Invalid transaction timestamp" };
	}
	else
	{
		RollTime rollTime { GetRollTime(cp) };
		if (ExtractTime(transactionTimeUtc) >= ExtractTime(ToUtc(transactionDate + rollTime.time, rollTime.timeZone)))
		{
			if (rollTime.timeZone == tzPacificAuckland && GetDayOfWeek(transactionDate) == 5)
			{
				// NZD -> roll time not effective on Fridays
			}
			else
			{
				transactionDate += NANOSECONDS_PER_DAY; // next day
			}
		}
		// ensure transaction date does not fall on weekend
		switch (GetDayOfWeek(transactionDate))
		{   // if next day is:
			case 0: // Sunday
				transactionDate += NANOSECONDS_PER_DAY; // + another day -> Monday
				break;
			case 6: // Saturday:
				transactionDate += 2ll * NANOSECONDS_PER_DAY; // + another 2 days -> Monday
				break;
			default: // Mon-Fri
				// ok, do nothing
				break;
		}
	}
	return transactionDate;
}

Result<int64_t> ValueDateCalendar::GetValueDate(CurrencyPair cp, int64_t transactionTimeUtc, bool fallbackToDefault) const
{
	if (!cp.Valid())
	{
		return { setError, "Invalid currency pair" };
	}
	if (transactionTimeUtc <= 0)
	{
		return { setError, "Invalid transaction time" };
	}
	Result<int64_t> valueDate;
	Result<int64_t> transactionDate { GetTradeDate(cp, transactionTimeUtc) };
	fallbackToDefault = fallbackToDefault || !cp.IsFX();
	if (transactionDate)
	{
		Result<const CcyMap *> valueDateMap { GetValueDateMap(transactionDate.Value()) };
		if (valueDateMap)
		{
			const auto &it { valueDateMap.Value()->find(cp) };
			valueDate = COND_RESULT(it != valueDateMap.Value()->end(), it->second,
									Format("No value date for %s and transaction time %s", cp.ToString(),
										   FormatTimestamp(TSF_DATETIME, transactionTimeUtc)));
			
		}
		else
		{
			valueDate = { setError, valueDateMap.ErrorMessage() };
		}
	}
	else
	{
		valueDate = { setError, transactionDate.ErrorMessage() };
	}
	if (!valueDate && fallbackToDefault)
	{
		valueDate = GetDefaultSpotDate(cp, transactionTimeUtc);
	}
	return valueDate;
}

std::string ValueDateCalendar::GetInfo() const
{
	std::string result;
	if (m_valueDateMap.empty())
	{
		result = "NO VALUE DATES LOADED";
	}
	else
	{
		std::string first { FormatTimestamp("%F", m_valueDateMap.begin()->first) }, last { FormatTimestamp("%F", m_valueDateMap.rbegin()->first) };
		result = Format("value dates for transactions between %s and %s available", first, last);
	}
	return result;
}

} // UTILS