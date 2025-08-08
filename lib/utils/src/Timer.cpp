

#include <random>

#include "Utils/ContextBase.h"
#include "Utils/StopWatch.h"
#include "Utils/Timer.h"

#include <iomanip>
#include <iostream>

namespace UTILS {

void Timer::Task::Execute()
{
	try
	{
		if (m_action)
		{
			m_action(*this);
		}
	}
	catch (...)
	{
		if (m_exceptionHandler)
		{
			m_exceptionHandler(*this, std::current_exception());
		}
		else
		{
			std::cerr << "Exception thrown in timer task " << Id() << " (" << Name() << "): " << GetMessage(std::current_exception()) << std::endl;
			throw;
		}
	}
}

Timer::Timer(ContextBase &ctx)
		: m_ctx(&ctx), m_shutdown(false)
{
}

Timer::Timer()
		: m_ctx(nullptr), m_shutdown(false)
{
}

Timer::~Timer()
{
	if (Running())
	{
		Stop(false);
	}
}

bool Timer::Running() const
{
	std::shared_lock lock { m_threads.Mutex() };
	return !m_threads->empty();
}

BoolResult Timer::Start(const std::string &name, size_t nThreads)
{
	std::unique_lock lock { m_threads.Mutex() };
	if (!m_threads->empty())
	{
		return { setError, "Timer already running" };
	}
	else if (name.empty() || name.size() > 12)
	{
		return { setError, "Invalid name: \"%s\"", name };
	}
	else if (nThreads < 1 || nThreads > 99)
	{
		return { setError, "Invalid number of threads: %Lu", nThreads };
	}
	m_threads->reserve(nThreads);
	for (size_t i { 0 }; i < nThreads; ++i)
	{
		m_threads->emplace_back([this, name, i]()
								{
									std::stringstream ss;
									ss << name << "_" << std::setfill('0') << std::setw(2) << i;
									pthread_setname_np(pthread_self(), ss.str().c_str());
									std::unique_lock lock { m_mtx };
									TaskMap::iterator it { m_taskMap.begin() };
									while (!IsShuttingDown())
									{
										if (it != m_taskMap.end())
										{
											int64_t timeout { it->first - CurrentTimestamp() };
											if (timeout > 0)
											{
												m_cv.wait_for(lock, std::chrono::nanoseconds(timeout));
											}
										}
										else
										{
											m_cv.wait(lock);
										}
										it = m_taskMap.begin();
										if (!IsShuttingDown() && it != m_taskMap.end() && it->first <= CurrentTimestamp())
										{
											auto nh { m_taskMap.extract(it) };
											Task &task { *nh.mapped() };
											m_pendingTasks.emplace(task.Id());
											lock.unlock();
											std::unique_ptr<StopWatch> sw;
											const bool doLogging { m_ctx && m_ctx->IsLoggingFlagSet(8) };
											int64_t taskDuration { 0 };
											if (doLogging)
											{
												sw = std::make_unique<StopWatch>(task.Name(), StopWatch::Data::ParamMap({{ "taskId",    task.Id() },
																														 { "taskName",  task.Name() },
																														 { "timerName", name },
																														 { "threadIdx", int(i) },
																														 { "delay",     double(
																																 task.Delay()) / 1'000'000.0 },
																														 { "event",     "start" }}));
												m_ctx->StoreStopWatch(*sw, "TM");
												taskDuration = CurrentTimestamp();
											}
#ifdef TEST_THREADS
											task.Execute(i);
#else
											task.Execute();
#endif
											if (doLogging)
											{
												taskDuration = CurrentTimestamp() - taskDuration;
												sw = std::make_unique<StopWatch>(task.Name(), StopWatch::Data::ParamMap({{ "taskId",       task.Id() },
																														 { "taskName",     task.Name() },
																														 { "timerName",    name },
																														 { "threadIdx",    int(i) },
																														 { "taskDuration", double(
																																 taskDuration) / 1'000'000.0 },
																														 { "event",        "stop" }}));
												m_ctx->StoreStopWatch(*sw, "TM");
											}
											lock.lock();
											// only if nh.key() has not meanwhile been cancelled
											if (m_pendingTasks.erase(task.Id()) > 0 && task.NextInterval() > 0)
											{
												const int64_t scheduledTime { GetUnusedScheduleTime(
														std::max(task.ScheduledTime() + task.NextInterval(), CurrentTimestamp())) };
												task.SetScheduledTime(scheduledTime);
												nh.key() = scheduledTime;
												m_taskMap.insert(move(nh));
											}
											it = m_taskMap.begin();
											if (doLogging)
											{
												sw = std::make_unique<StopWatch>(task.Name(), StopWatch::Data::ParamMap({{ "taskId",            task.Id() },
																														 { "taskName",          task.Name() },
																														 { "timerName",         name },
																														 { "threadIdx",         int(
																																 i) },
																														 { "nextInterval",      double(
																																 task.NextInterval()) / 1'000'000.0 },
																														 { "queuedTasks",       int64_t(
																																 m_taskMap.size()) },
																														 { "timeUntilNextTask", (it != m_taskMap.end()
																																				 ? double(
																																		 it->first - CurrentTimestamp()) / 1'000'000.0
																																				 : 0.0) },
																														 { "event",             "done" }}));
												m_ctx->StoreStopWatch(*sw, "TM");
											}
										}
									}
								});
	}
	return true;
}

void Timer::Stop(bool detachThreads)
{
	CancelAll(); // cancel all tasks
	
	std::unique_lock lock { m_threads.Mutex() };
	if (!m_threads->empty())
	{
		m_shutdown = true; //!< set shutdown flag
		
		m_cv.notify_all(); //!< wake up all worker threads
		for (auto &thread : *m_threads)
		{
			if (thread.joinable()) //!< check whether joinable (should always return true)
			{
				if (detachThreads)
				{
					thread.detach(); //!< detach thread
				}
				else
				{
					thread.join(); //!< or join thread
				}
			}
		}
		m_threads->clear(); //!< clear thread vector
		
		m_shutdown = false; //!< clear shutdown flag
	}
}

Result<int64_t> Timer::ScheduleAt(std::string name, Action &&action, int64_t firstEventTime, std::chrono::nanoseconds repeatedInterval,
								  ExceptionHandler &&exceptionHandler)
{
	if (action == nullptr)
	{
		return { setError, "No valid action" };
	}
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	if (firstEventTime <= 0) //
	{
		static std::random_device rd;  //Will be used to obtain a seed for the random number engine
		static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
		
		std::uniform_int_distribution<int64_t> dis(0, repeatedInterval.count());
		firstEventTime = CurrentTimestamp() + dis(gen);
	}
	std::unique_lock lock { m_mtx };
	const int64_t scheduledTime { GetUnusedScheduleTime(std::max(firstEventTime, CurrentTimestamp())) };
	const auto &[it, success] { m_taskMap.emplace(scheduledTime,
												  std::make_unique<Task>(std::move(name), std::move(action), scheduledTime, repeatedInterval.count(),
																		 std::move(exceptionHandler))) };
	int64_t taskId { success ? it->second->Id() : 0 };
	lock.unlock();
	m_cv.notify_one();
	if (success)
	{
		return taskId;
	}
	else
	{
		return { setError, "Adding task to map failed" };
	}
}

Result<int64_t> Timer::Schedule(std::string name, Action &&action, std::chrono::nanoseconds firstInterval, std::chrono::nanoseconds repeatedInterval,
								ExceptionHandler &&exceptionHandler)
{
	return ScheduleAt(std::move(name), std::move(action), CurrentTimestamp() + firstInterval.count(), repeatedInterval, std::move(exceptionHandler));
}

BoolResult Timer::Cancel(int64_t taskId)
{
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	bool found { false };
	std::unique_lock lock { m_mtx };
	for (TaskMap::iterator it { m_taskMap.begin() }; it != m_taskMap.end(); ++it)
	{
		if (it->second->Id() == taskId)
		{
			found = true;
			m_taskMap.erase(it);
			break;
		}
	}
	if (!found)
	{
		found = m_pendingTasks.erase(taskId) > 0;
	}
	lock.unlock();
	return { found, "ID %Ld not found", taskId };
}

BoolResult Timer::CancelAll()
{
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	std::unique_lock lock { m_mtx };
	m_taskMap.clear();
	m_pendingTasks.clear();
	return true;
}

Result<int64_t> Timer::GetScheduledTime(int64_t taskId) const
{
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	int64_t scheduledTime { 0 };
	bool taskPending { false };
	std::unique_lock lock { m_mtx };
	for (const auto &it : m_taskMap)
	{
		if (it.second->Id() == taskId)
		{
			scheduledTime = it.second->ScheduledTime();
			break;
		}
	}
	if (scheduledTime == 0 && m_pendingTasks.find(taskId) != m_pendingTasks.end())
	{
		taskPending = true;
	}
	lock.unlock();
	if (scheduledTime > 0)
	{
		return scheduledTime;
	}
	else
	{
		if (taskPending)
		{
			return { setError, "Task %Ld is currently executing", taskId };
		}
		else
		{
			return { setError, "ID %Ld not found", taskId };
		}
	}
}

BoolResult Timer::Reschedule(int64_t taskId, int64_t nextEventTime, Action &&newAction, ExceptionHandler &&newExceptionHandler)
{
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	bool found { false };
	std::unique_lock lock { m_mtx };
	for (TaskMap::iterator it { m_taskMap.begin() }; it != m_taskMap.end(); ++it)
	{
		if (it->second->Id() == taskId)
		{
			found = true;
			const int64_t scheduledTime { GetUnusedScheduleTime(std::max(nextEventTime, CurrentTimestamp())) };
			auto nh { m_taskMap.extract(it) };
			Task &task { *nh.mapped() };
			task.SetScheduledTime(scheduledTime);
			if (newAction)
			{
				task.SetAction(std::move(newAction));
			}
			if (newExceptionHandler)
			{
				task.SetExceptionHandler(std::move(newExceptionHandler));
			}
			nh.key() = scheduledTime;
			m_taskMap.insert(std::move(nh));
			break;
		}
	}
	lock.unlock();
	if (found)
	{
		m_cv.notify_one();
		return true;
	}
	else
	{
		return { setError, "ID %Ld not found", taskId };
	}
}

Result<int64_t> Timer::ExecuteDelayed(const std::string &uniqueName, Action &&action, std::chrono::nanoseconds delay,
									  ExceptionHandler &&exceptionHandler)
{
	if (uniqueName.empty())
	{
		return { setError, "No valid unique name" };
	}
	if (action == nullptr)
	{
		return { setError, "No valid action" };
	}
	if (!Running())
	{
		return { setError, "Timer not running" };
	}
	if (IsShuttingDown())
	{
		return { setError, "Timer is shutting down" };
	}
	if (uniqueName.empty())
	{
		return { setError, "Invalid unique name" };
	}
	Result<int64_t> result;
	std::unique_lock lock { m_mtx };
	for (TaskMap::iterator it { m_taskMap.begin() }; it != m_taskMap.end(); ++it)
	{
		if (it->second->Name() == uniqueName)
		{
			const int64_t scheduledTime { GetUnusedScheduleTime(CurrentTimestamp() + delay.count()) };
			auto nh { m_taskMap.extract(it) };
			Task &task { *nh.mapped() };
			result = task.Id();
			task.SetScheduledTime(scheduledTime);
			task.SetAction(std::move(action));
			if (exceptionHandler)
			{
				task.SetExceptionHandler(std::move(exceptionHandler));
			}
			nh.key() = scheduledTime;
			m_taskMap.insert(std::move(nh));
			break;
		}
	}
	lock.unlock();
	if (result)
	{
		m_cv.notify_one();
	}
	else
	{
		result = Schedule(uniqueName, std::move(action), delay, std::chrono::nanoseconds::zero(), std::move(exceptionHandler));
	}
	return result;
}

int64_t Timer::GetUnusedScheduleTime(int64_t proposedTime) const
{
	while (m_taskMap.find(proposedTime) != m_taskMap.end())
	{
		++proposedTime;
	}
	return proposedTime;
}

#ifdef TEST_THREADS
void TimerTest(size_t threads, int64_t testDuration, const std::vector<TestTaskData> &tasks, TestResult &result, bool withOutput)
{
	Timer tm;
	tm.Start("Test", threads);
	const int64_t baseTime { CurrentTimestamp() };
	result.Reset(threads);
	size_t taskIdx { 0 };
	std::mutex mtx;
	std::vector<std::string> timelines;
	timelines.resize(threads);
	const size_t n { 10000 };
	for (size_t i { 0 }; i < threads; ++i)
	{
		timelines[i] = std::string(n, '.');
	}
	for (const TestTaskData &taskData : tasks)
	{
		tm.ScheduleAt([&result, &taskData, withOutput, taskIdx, &timelines, baseTime, testDuration, n, &mtx](Timer::Task &task, size_t threadIndex)
					  {
						  const int64_t delay { task.Delay() };
						  const size_t tlStartIdx { (CurrentTimestamp() - baseTime) * n / testDuration };
						  if (delay < 0)
						  {
							  ++result.dlyNeg;
						  }
						  else if (delay < 1'000)
						  {
							  ++result.dly1us;
						  }
						  else if (delay < 10'000)
						  {
							  ++result.dly10us;
						  }
						  else if (delay < 100'000)
						  {
							  ++result.dly100us;
						  }
						  else if (delay < 1'000'000)
						  {
							  ++result.dly1ms;
						  }
						  else if (delay < 10'000'000)
						  {
							  ++result.dly10ms;
						  }
						  else if (delay < 100'000'000)
						  {
							  ++result.dly100ms;
						  }
						  else if (delay < 1'000'000'000)
						  {
							  ++result.dly1s;
						  }
						  else
						  {
							  ++result.dlyMore;
						  }
						  ++result.callsPerThread[threadIndex];
						  double relTimeSeconds;
						  if (withOutput)
						  {
							  relTimeSeconds = double(CurrentTimestamp() - baseTime) / double(std::chrono::seconds(1));
							  std::cout << std::setw(10) << relTimeSeconds << ": TASK " << taskIdx << ", TIMER EVENT THREAD " << threadIndex
										<< " STARTING" << std::endl;
						  }
						  if (taskData.duration > 0)
						  {
							  std::this_thread::sleep_for(std::chrono::nanoseconds(taskData.duration));
						  }
						  const size_t tlEndIdx { std::min(std::max((CurrentTimestamp() - baseTime) * n / testDuration, tlStartIdx + 1), n) };
						  mtx.lock();
						  for (size_t tlIdx { tlStartIdx }; tlIdx < tlEndIdx; ++tlIdx)
						  {
							  char c { taskIdx < 10 ? char('0' + taskIdx) : char('A' + (taskIdx - 10)) };
							  timelines[threadIndex][tlIdx] = c;
						  }
						  mtx.unlock();
						  if (withOutput)
						  {
							  relTimeSeconds = double(CurrentTimestamp() - baseTime) / double(std::chrono::seconds(1));
							  std::cout << std::setw(10) << relTimeSeconds << ": TASK " << taskIdx << ", TIMER EVENT THREAD " << threadIndex
										<< " FINISHED" << std::endl;
						  }
					  }, baseTime + (taskData.firstDelay < 0 ? taskData.interval : taskData.firstDelay), taskData.interval);
		++taskIdx;
	}
	std::this_thread::sleep_for(std::chrono::nanoseconds(testDuration));
	tm.Stop();
	std::cout << std::endl << threads << "THREADS, TIMELINE RESOLUTION: " << double(testDuration) / double(n) / 1'000'000.0 << "ms" << std::endl;
	for (size_t idx { 0 }; idx < threads; ++idx)
	{
		std::cout << "THREAD " << std::setw(2) << idx << ": " << timelines[idx] << std::endl;
	}
}
#endif // TEST_THREADS

} // namespace CORE

