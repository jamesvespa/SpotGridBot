/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2018-15-30
 * @copyright   (c) 2018 BBF-IT
*/

#ifndef MERCURYUTILS_STOPWATCH_H
#define MERCURYUTILS_STOPWATCH_H

#include <memory>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <mutex>

namespace UTILS {

/*! Clock to be used by the stopwatch */
#define CLOCK_STOPWATCH     CLOCK_MONOTONIC

#define INVISIBLE   "*INVISIBLE*"

/*! \brief This macro takes the current time and adds it to the list
 * of time points taken by the given stopwatch. It adds the overhead
 * caused by the operation to the correction value of the stopwatch.
 *
 * @param _sw_  Stopwatch to be used for taking the time
 * @param _tag_ Tag to be assigned to the taken time
 * @param _params_ Map of parameters (name/value pairs)
 * */
#define TAKE_TIME_PARAMS(_sw_, _tag_, _params_) \
{ \
    timespec _ts { 0, 0 }; \
    clock_gettime(CLOCK_STOPWATCH, &_ts); \
    (_sw_).AddTime(_ts, (_tag_), (_params_)); \
}

/*! \brief This macro takes the current time and adds it to the list
 * of time points taken by the given stopwatch. It adds the overhead
 * caused by the operation to the correction value of the stopwatch.
 *
 * @param _sw_  Stopwatch to be used for taking the time
 * @param _tag_ Tag to be assigned to the taken time
 * */
#define TAKE_TIME(_sw_, _tag_) \
    TAKE_TIME_PARAMS((_sw_), (_tag_), StopWatch::Data::ParamMap({}))

/*! \brief This class manages a vector of time points, associated with a tag,
 * an index, and a description. */
class StopWatch
{
public:
	class Data
	{
	public:
		using Ptr = std::unique_ptr<Data>;
		using PtrToConst = std::unique_ptr<const Data>;
		using ParamMap = std::map<std::string, std::variant<std::string, char, int64_t, int, double, bool, const char *>>;
		
		/*! \brief Structure defining a time point, associated with a tag,
		 * an index, and a description */
		struct Time
		{
			Time(timespec ts, int64_t time, int64_t diff, std::string tag, ParamMap params = { })
					: time(time), diff(diff), tag(std::move(tag)), params(std::move(params)) { }
			
			/*! Nanoseconds since the last reset, or the construction of the stopwatch. */
			int64_t time;
			/*! Nanoseconds since the last time point. */
			int64_t diff;
			/*! Tag associated with the time point. */
			std::string tag;
			/*! Name/value map of parameters. */
			ParamMap params;
		};
		
		/*! Type of the vector of time points. */
		using TimeVec = std::vector<Time>;
		
		/*! Constructor
		 *
		 * @param name  Name to be assigned to the stopwatch
		 * */
		explicit Data(std::string name, ParamMap params = { })
				: m_name(std::move(name)), m_params(std::move(params)), m_timestamp(GetTimestamp())
		{
		}
		
		/*! \brief Name associated with the stopwatch */
		const std::string &Name() const { return m_name; }
		
		/*! Name/value map of parameters. */
		const ParamMap &Params() const { return m_params; }
		
		/*! \brief Timestamp of the stopwatch (in nanoseconds since Epoch) */
		int64_t Timestamp() const { return m_timestamp; }
		
		/*! \brief Overall duration (relative timestamp of the last time point) */
		int64_t Duration() const { return m_times.empty() ? 0 : m_times.back().time; }
		
		/*! \brief Const vector of time points */
		const TimeVec &Times() const { return m_times; }
		
		void GetTags(std::vector<std::string> &tagVector) const;
		
		void GetTimes(const std::string &tag, TimeVec &timeVector) const;
		
		void GetTimes(const std::string &tag, TimeVec &timeVector, int64_t &min, int64_t &max, int64_t &sum, double &avg) const;
		
		template <typename A>
		void ForEachTag(A action) const
		{
			std::vector<std::string> tagVec;
			GetTags(tagVec);
			for (const auto &tag : tagVec)
			{
				TimeVec timeVec;
				int64_t min, max, sum;
				double avg;
				GetTimes(tag, timeVec, min, max, sum, avg);
				if (!timeVec.empty())
				{
					action(tag, timeVec, min, max, sum, avg);
				}
			}
		}
		
		/*! \brief Vector of time points */
		TimeVec &Times() { return m_times; }
	
	private:
		/*! \brief Name associated with the stopwatch */
		std::string m_name;
		/*! Name/value map of parameters. */
		ParamMap m_params;
		/*! \brief Timestamp of the stopwatch (in nanoseconds since Epoch, obtained from CLOCK_REALTIME) */
		int64_t m_timestamp;
		/*! \brief Vector of time points */
		TimeVec m_times;
	};
	
	/*! Constructor
	 *
	 * @param name  Name to be assigned to the stopwatch
	 * */
	explicit StopWatch(std::string name, Data::ParamMap params = { })
			: m_data(std::make_unique<Data>(std::move(name), std::move(params))), m_correction(0), m_base(GetTime()) { }
	
	Data::PtrToConst Release();
	
	void AddTime(timespec ts, const std::string &tag, Data::ParamMap params = { });

private:
	/*! \brief Timing data */
	Data::Ptr m_data;
	/*! \brief Mutex for adding a time point */
	std::mutex m_mtx;
	/*! \brief Correction value (nanoseconds spent for adding time points to the vector) */
	int64_t m_correction;
	/*! \brief WorkElem for calculating the time values (timestamp obtained from CLOCK_MONOTONIC) */
	timespec m_base;
	
	static int64_t TimespecToInt64(timespec ts) { return ts.tv_sec * 1'000'000'000 + ts.tv_nsec; }
	
	/*! \brief Current \a timespec value (seconds and nanoseconds) */
	static int64_t GetTimestamp()
	{
		timespec ts { 0, 0 };
		clock_gettime(CLOCK_REALTIME, &ts);
		return TimespecToInt64(ts);
	}
	
	/*! \brief Current \a timespec value (seconds and nanoseconds) */
	static timespec GetTime()
	{
		timespec ts { 0, 0 };
		clock_gettime(CLOCK_STOPWATCH, &ts);
		return ts;
	}
	
	/*! \brief Nanoseconds between two \a timespec values */
	static int64_t Diff(timespec ts1, timespec ts2)
	{
		return (ts2.tv_sec - ts1.tv_sec) * 1'000'000'000 + (ts2.tv_nsec - ts1.tv_nsec);
	}
	
};

} // namespace CORE

#endif //MERCURYUTILS_STOPWATCH_H
