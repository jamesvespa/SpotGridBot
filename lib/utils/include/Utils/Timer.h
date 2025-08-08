

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "Utils.h"
#include "Result.h"
#include "Lockable.h"

#ifndef MERCURYUTILS_TIMER_H
#define MERCURYUTILS_TIMER_H

//#define TEST_THREADS

namespace UTILS {

class ContextBase;

class Timer
{
public:
	struct Task; //!< forward declaration

#ifdef TEST_THREADS
	using Action = std::function<void(Task &, size_t)>; // size_t threadIndex
#else
	using Action = std::function<void(Task &)>;
	using ExceptionHandler = std::function<void(Task &, std::exception_ptr)>;
#endif
	
	class Task
	{
	public:
		/**
		 * @brief Constructor
		 * @param name	Name for the task
		 * @param action Action to be executed
		 * @param scheduledTime Time at which the event is to be triggered the first time
		 * @param intervalNs Interval between repeated invocation of the timer event, 0 -> one-time event
		 * @param exceptionHandler Callback for exception handling, if @a nullptr -> exception is re-thrown
		 */
		Task(std::string name, Action &&action, int64_t scheduledTime, int64_t intervalNs, ExceptionHandler &&exceptionHandler)
				: m_id(NewInt64Key()), m_name(std::move(name)), m_action(std::move(action)), m_scheduledTime(scheduledTime),
				  m_interval(intervalNs), m_exceptionHandler(std::move(exceptionHandler)) { }
		
		using Ptr = std::unique_ptr<Task>; //!< Type alias for unique pointer to Task
		
		int64_t Id() const { return m_id; } //!< Unique task ID
		const std::string &Name() const { return m_name; } //!< Task name
		int64_t ScheduledTime() const { return m_scheduledTime; } //!< Scheduled time of this task
		int64_t Delay() const { return CurrentTimestamp() - ScheduledTime(); } //!< Delay of this task
		int64_t NextInterval() const { return m_interval; } //!< Interval in ns until next invocation
#ifdef TEST_THREADS
		void Execute(size_t threadIndex) { m_action(*this, threadIndex); } //!< Executes the action
#else
		void Execute(); //!< Executes the action
#endif
		
		void SetScheduledTime(int64_t scheduledTime) { m_scheduledTime = scheduledTime; } //!< Set the scheduled time of the next invocation
		void SetNextInterval(int64_t intervalNs) { m_interval = intervalNs; } //!< Set the interval until the next invocation
		void SetAction(Action &&action) { m_action = std::move(action); } //!< Set the action for the next invocation
		void SetExceptionHandler(ExceptionHandler &&exceptionHandler) { m_exceptionHandler = std::move(exceptionHandler); } //!< Set the exception handler for the next invocation
		void Cancel() { SetNextInterval(0); } //!< Cancels the task
	private:
		const int64_t m_id; //!< Task ID
		const std::string m_name; //!< Task name
		Action m_action; //!< Action to be executed
		int64_t m_scheduledTime; //!< Time for the first execution of the task
		int64_t m_interval; //!< Interval in nanoseconds for repeated executions, 0 -> no repetitions
		ExceptionHandler m_exceptionHandler; //!< Exception handler
	};

	/**
	 * Constructor
	 *
	 * @param ctx Reference to global context object
	 */
	explicit Timer(ContextBase &ctx);
	
	/**
	 * Constructor
	 */
	Timer();
	
	/**
	 * @brief Destructor
	 */
	virtual ~Timer();
	
	/** @brief Is the timer running? */
	bool Running() const;
	
	/** @brief Is the timer in the state of shutting down? */
	bool IsShuttingDown() const { return m_shutdown.load(); }
	
	/**
	 * @brief Starts the timer
	 *
	 * @param name Name of the timer - used for naming timer threads
	 * @param nThreads Number of threads
	 * @return @a true on success
	 */
	BoolResult Start(const std::string &name, size_t nThreads = 1);
	
	/**
	 * @brief Stops the timer
	 * @param detachThreads @a true -> detaches threads, @a false -> joins threads
	 */
	void Stop(bool detachThreads = false);
	
	/**
	 * @brief Schedules a new event
	 * @param name	Name for the task
	 * @param action Action to be executed
	 * @param firstEventTime AbsoluteTime at which the event is to be
	 * triggered the first time
	 * @param repeatedInterval (optional) Interval between repeated
	 * invocation of the timer event, 0 -> one-time event
	 * @return Id of the created event
	 */
	Result <int64_t> ScheduleAt(std::string name, Action &&action, int64_t firstEventTime,
								std::chrono::nanoseconds repeatedInterval = std::chrono::nanoseconds::zero(), ExceptionHandler &&exceptionHandler = nullptr);
	
	/**
	 * @brief Schedules a new event
	 * @param action Action to be executed
	 * @param firstEventTime AbsoluteTime at which the event is to be
	 * triggered the first time, 0 -> random time between current time
	 * and current time + repeatedInterval
	 * @param repeatedInterval (optional) Interval between repeated
	 * invocation of the timer event, 0 -> one-time event
	 * @return Id of the created event
	 */
	Result <int64_t> ScheduleAt(Action &&action, int64_t firstEventTime, std::chrono::nanoseconds repeatedInterval = std::chrono::nanoseconds::zero(), ExceptionHandler &&exceptionHandler = nullptr)
	{
		return ScheduleAt("", std::move(action), firstEventTime, repeatedInterval, std::move(exceptionHandler));
	}
	
	/**
	 * @brief Schedules a new event
	 * @param name	Name for the task
	 * @param action Action to be executed
	 * @param firstInterval Duration in nanoseconds after which the
	 * event is to be triggered the first time
	 * @param repeatedIntervalNs (optional) Interval between repeated
	 * invocation of the timer event, 0 -> one-time event
	 * @return Id of the created event
	 */
	Result <int64_t> Schedule(std::string name, Action &&action, std::chrono::nanoseconds firstInterval,
							  std::chrono::nanoseconds repeatedInterval = std::chrono::nanoseconds::zero(), ExceptionHandler &&exceptionHandler = nullptr);
	
	/**
	 * @brief Schedules a new event
	 * @param action Action to be executed
	 * @param firstInterval Duration in nanoseconds after which the event
	 * is to be triggered the first time
	 * @param repeatedIntervalNs (optional) Interval between repeated
	 * invocation of the timer event, 0 -> one-time event
	 * @return Id of the created event
	 */
	Result <int64_t> Schedule(Action &&action, std::chrono::nanoseconds firstInterval,
							  std::chrono::nanoseconds repeatedInterval = std::chrono::nanoseconds::zero(), ExceptionHandler &&exceptionHandler = nullptr)
	{
		return Schedule("", std::move(action), firstInterval, repeatedInterval, std::move(exceptionHandler));
	}
	
	/**
	 * Return current task count
	 *
	 * Primarily for testing
	 *
	 * @return Current task count
	 */
	size_t TaskCount() const
	{
		std::lock_guard lock { m_mtx };
		return m_taskMap.size();
	}
	
	/**
	 * Cancels a task
	 * @param taskId ID of the task to be canceled
	 * @return @a true if successful
	 */
	BoolResult Cancel(int64_t taskId);
	
	/**
	 * Cancels all tasks
	 * @return @a true if successful
	 */
	BoolResult CancelAll();
	
	/**
	 * Returns the scheduled time of a task
	 * @param taskId ID of the task to be canceled
	 * @return @a Scheduled time
	 */
	Result<int64_t> GetScheduledTime(int64_t taskId) const;
	
	
	/**
	 * Reschedules the next execution of a task
	 * @param taskId ID of the task to be rescheduled
	 * @param nextEventTime Time of the next execution of the task
	 * @param newAction (optional) new action to be executed
	 * @param newExceptionHandler (optional) new exception handler
	 * @return @a true if successful
	 */
	BoolResult Reschedule(int64_t taskId, int64_t nextEventTime, Action &&newAction = nullptr, ExceptionHandler &&newExceptionHandler = nullptr);
	
	/**
	 * Executes an action after a given delay. If the function is repeatedly
	 * called within the delay period, it is rescheduled and will be executed only once.
	 *
	 * @param uniqueName Unique name to identify the action
	 * @param action Action to be executed
	 * @param delay Delay in nanoseconds
	 * @param exceptionHandler (optional) Exception handler to be used
	 * @return Task ID on success, error message on failure
	 * */
	Result<int64_t> ExecuteDelayed(const std::string &uniqueName, Action &&action, std::chrono::nanoseconds delay, ExceptionHandler &&exceptionHandler = nullptr);
private:
	
	ContextBase *m_ctx;
	
	SharedLockable <std::vector<std::thread>> m_threads; //!< vector of worker threads
	
	mutable std::mutex m_mtx; //!< Mutex for map access
	using TaskMap = std::map<int64_t, Task::Ptr>; //!< Type alias for task map
	TaskMap m_taskMap; //!< map: invocation time -> Task
	
	std::set<int64_t> m_pendingTasks; //!< IDs of the tasks currently pending (in execution)
	
	std::atomic_bool m_shutdown; //!< Atomic boolean to trigger shutdown on all open threads
	
	std::condition_variable m_cv; //!< Condition variable
	
	int64_t GetUnusedScheduleTime(int64_t proposedTime) const;
};

#ifdef TEST_THREADS
struct TestTaskData
{
	int64_t interval;
	int64_t duration { 0 };
	int64_t firstDelay { -1 };
};

struct TestResult
{
	TestResult() { }
	
	std::atomic_size_t dlyNeg, dly1us, dly10us, dly100us, dly1ms, dly10ms, dly100ms, dly1s, dlyMore;
	std::vector<size_t> callsPerThread;
	
	void Reset(size_t threads)
	{
		dlyNeg = dly1us = dly10us = dly100us = dly1ms = dly10ms = dly100ms = dly1s = dlyMore = 0;
		callsPerThread.resize(threads);
		for (size_t i { 0 }; i < threads; ++i)
		{
			callsPerThread[i] = 0;
		}
	}
	
	std::string ToString() const
	{
		std::stringstream ss;
		ss << "total events: " << dlyNeg + dly1us + dly10us + dly100us + dly1ms + dly10ms + dly100ms + dly1s + dlyMore;
		ss << ", delays: < 0: " << dlyNeg << ", < 1us: " << dly1us << ", < 10us: " << dly10us << ", < 100us: " << dly100us << ", < 1ms: "
		   << dly1ms;
		ss << ", < 10ms: " << dly10ms << ", < 100ms: " << dly100ms << ", < 1s: " << dly1s << ", >= 1s: " << dlyMore;
		ss << ", calls per thread: {";
		for (size_t i { 0 }; i < callsPerThread.size(); ++i)
		{
			ss << " " << callsPerThread[i];
		}
		ss << "}";
		return ss.str();
	}
};

void TimerTest(size_t threads, int64_t testDuration, const std::vector<TestTaskData> &tasks, TestResult &result, bool withOutput = false);
#endif // TEST_THREADS

} // namespace CORE

#endif //CRTRNS_TIMER_H
