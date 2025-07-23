//
// Created by jameson 30.05.18.
//
#include <algorithm>
#include "Utils//StopWatch.h"

namespace UTILS {

void StopWatch::Data::GetTags(std::vector<std::string> &tagVector) const
{
	tagVector.clear();
	for (const auto &tm : Times())
	{
		if (tm.tag != INVISIBLE && std::find(tagVector.begin(), tagVector.end(), tm.tag) == tagVector.end())
		{
			tagVector.push_back(tm.tag);
		}
	}
}

void StopWatch::Data::GetTimes(const std::string &tag, TimeVec &timeVector) const
{
	timeVector.clear();
	for (const auto &tm : Times())
	{
		if (tm.tag == tag)
		{
			timeVector.emplace_back(tm);
		}
	}
}

void StopWatch::Data::GetTimes(const std::string &tag, TimeVec &timeVector, int64_t &min, int64_t &max, int64_t &sum, double &avg) const
{
	GetTimes(tag, timeVector);
	if (!timeVector.empty())
	{
		min = max = sum = 0;
		for (const auto &tm : timeVector)
		{
			if (min == 0 || tm.diff < min)
			{
				min = tm.diff;
			}
			if (max == 0 || tm.diff > max)
			{
				max = tm.diff;
			}
			sum += tm.diff;
		}
		avg = (double) sum / timeVector.size();
	}
}

/*! \brief Release stopwatch data.
 *
 * Returns a unique pointer to the stopwatch data. Subsequent calls to
 * \a AddTime() have no effect.
 *
 * @return Unique pointer to the stopwatch data.
 * */
StopWatch::Data::PtrToConst StopWatch::Release()
{
	std::lock_guard lock { m_mtx };
	return std::move(m_data);
}

/*! \brief Adds a time point to the vector.
 *
 * To be used only by the macros TAKE_TIME and TAKE_TIME_PARAMS
 * */
void StopWatch::AddTime(timespec ts, const std::string &tag, Data::ParamMap params)
{
	std::lock_guard lock { m_mtx };
	if (m_data)
	{
		int64_t time { Diff(m_base, ts) - m_correction }, diff;
//    if (m_data.Times().empty()) // fails after reset
		if (m_correction == 0)
		{ // either times vector empty or Reset() occurred
			diff = time;
		}
		else
		{
			diff = time - m_data->Times().back().time;
		}
		m_data->Times().emplace_back(ts, time, diff, tag, params);
		m_correction += Diff(ts, GetTime());
	}
}

} // namespace CORE
