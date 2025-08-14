

#pragma once


#include <pthread.h>
#include <algorithm>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <deque>
#include <unordered_set>
#include <iterator>
#include <iomanip>

#include "Utils/Utils.h"
#include "Utils/Result.h"
#include "Utils/StopWatch.h"
#include "Utils/Lockable.h"
#include "Utils/Timer.h"
#include "Utils/BgwMonitor.h"

#define MAX_QUEUESIZE       100'000ul

#define QUEUE_NEW           0
#define QUEUE_UPDATE        1
#define QUEUE_DELETE        2

namespace UTILS {

/** @brief This class defines a thread pool that processes items taken from
 * a dynamic queue.
 *
 * @tparam Args Argument list defining a single queue element.
 */
template <typename ...Args>
class BackgroundWorker
{
public:
	/** @brief Constructor */
	explicit BackgroundWorker()
			: m_maxQueueSize(MAX_QUEUESIZE), m_shutdown(false), m_waitingSince(0), m_idleThreads(0), m_autoflush(false), m_batchSize(0),
			  m_timer(nullptr) { }
	
	/*autoflush constructor, we take a batchSize (threshold value that when exceeded we force a flush).
	 * and we take a start and periodic - these are start time and period time - for a timed flush */
	explicit BackgroundWorker(ContextBase &ctx, long batchSize, long start, long periodic)
			: m_maxQueueSize(MAX_QUEUESIZE), m_shutdown(false), m_waitingSince(0), m_idleThreads(0), m_autoflush(true), m_batchSize(batchSize)
	{
		m_timer = std::make_unique<UTILS::Timer>(ctx);
		m_timer->Start("Autoflush");
		m_timer->Schedule("Onflush", [this](Timer::Task &task)
		{
			OnFlush();
		}, std::chrono::microseconds(start), std::chrono::microseconds(periodic));
	}
	
	/** @brief Destructor
	 *
	 * Ends all worker threads. Queue elements not yet processed are canceled.
	 */
	virtual ~BackgroundWorker()
	{
		if (Running())
		{
			Stop(true, false);
		}
		
		if (m_timer)
		{
			m_timer->Stop();
		}
	}
	
	/** @brief Adds a new item to the queue
	 *
	 * This function creates a new item from the given arguments and places
	 * it at the end of the processing queue.
	 *
	 * @param args Arguments defining the new item to be queued.
	 * @return @a true if the item was successfully added to the queue,
	 * @a false if the background worker is being stopped, or if the maximum
	 * queue size has bee reached.
	 */
	BoolResult Enqueue(Args ...args) { return EnqueueTagged(std::nullopt, std::forward<Args>(args)...); }
	
	/** @brief Adds a new item with an optional sequence tag to the queue
	 *
	 * This function creates a new item from the given arguments and places
	 * it at the end of the processing queue. If a sequence tag is provided,
	 * the item will not be processed in parallel with other items containing
	 * the same tag.
	 *
	 * @tparam T Type of the sequence tag
	 * @tparam Hash Functional type defining a hash function. Default: @a std::hash<T>
	 * @param optTag Optional sequence tag (@a std::nullopt -> no sequence tagging)
	 * @param args Arguments defining the new item to be queued.
	 * @return @a true if the item was successfully added to the queue,
	 * @a false if the background worker is being stopped, or if the maximum
	 * queue size has been reached.
	 */
	template <typename T, typename Hash = std::hash<T>>
	BoolResult Enqueue(const std::optional<T> &optTag, Args ...args)
	{
		SequenceTag seqTag { std::nullopt };
		if (optTag)
		{
			seqTag = Hash()(optTag.value());
		}
		return EnqueueTagged(seqTag, std::forward<Args>(args)...);
	}
	
	/** @brief Maximum number of items in the queue
	 *
	 * @return The maximum number of queued items allowed
	 */
	size_t MaxQueueSize() const { return m_maxQueueSize.load(); }
	
	/** @brief Number of items in the queue
	 *
	 * @return The number of the queued items not yet processed
	 */
	size_t QueueSize() const
	{
		std::unique_lock lock { m_mtx };
		return m_queue.size();
	}
	
	/** @brief Number of threads in the thread pool. */
	size_t RunningThreads() const
	{
		std::shared_lock lock { m_threads.Mutex() };
		return m_threads->size();
	}
	
	/** @brief Number of idle threads (i.e. threads that are waiting for the
	 * next item to be queued). */
	size_t IdleThreads() const { return m_idleThreads; }

	/** @brief Returns true if autoflush is set */
	int AutoFlush() const
    {
        return m_autoflush;
    }

	/** @brief Returns batch size */
    int BatchSize() const
    {
        return m_batchSize;
    }

	/** @brief Number of busy threads (i.e. threads that are currently
	 * processing an item). */
	size_t BusyThreads() const { return RunningThreads() - IdleThreads(); }
	
	int64_t BlockedForNsec() const
	{
		std::unique_lock lock { m_mtx };
		return m_waitingSince != 0 && Running() ? CurrentTimestamp() - m_waitingSince : 0;
	}
	
	/** @brief Is the background worker running (i.e. processes queued items)? */
	bool Running() const
	{
		std::shared_lock lock { m_threads.Mutex() };
		return !m_threads->empty();
	}
	
	/** @brief Starts the background worker.
	 *
	 * @param action Action processing a single item.
	 * @param workerThreads Number of threads to process the queued items
	 * (default: 1)
	 * @param maxQueueSize Maximum queue size (default: @a MAX_QUEUESIZE)
	 * @param pinToCore @a true -> pin-to-core worker threads
	 * @return @a true if the worker was successfully started, @a false
	 * if it has already been running, or an invalid number of worker threads
	 * or maximum queue size is provided.
	 * */
	template <typename A>
	BoolResult Start(const std::string &name, A action, size_t workerThreads = 1, size_t maxQueueSize = MAX_QUEUESIZE, bool pinToCore = true)
	{
		std::unique_lock lock { m_threads.Mutex() };
		if (!m_threads->empty())
		{
			return { setError, "Worker already running" };
		}
		if (workerThreads < 1)
		{
			return { setError, "Invalid number of worker threads: %Lu", workerThreads };
		}
		if (maxQueueSize < 1)
		{
			return { setError, "Invalid maximum queue size: %Lu", maxQueueSize };
		}
		m_maxQueueSize = maxQueueSize;
		m_threads->reserve(workerThreads);
		m_monitor.Start(workerThreads); //!< start utilization monitoring
		for (size_t i { 0 }; i < workerThreads; ++i)
		{
			m_threads->emplace_back([this, i, action, pinToCore, name]()
									{
										if (!name.empty())
										{
											std::stringstream ss;
											ss << name << "_" << std::setfill('0') << std::setw(2) << i;
											SetThreadName(ss.str());
											if (pinToCore)
											{
												SetThreadAffinityFromConfig();
											}
										}
										worker(action);
									});
		}
		return true;
	}
	
	void ClearQueue()
	{
		std::unique_lock lock2 { m_mtx };
		m_queue.clear();
		m_waitingSince = 0;
	}
	/** @brief Stops the background worker.
	*
	* @param cancelQueue @a true -> cancels all queued items not yet
	* processed, @a false -> continue processing items until the queue
	* is empty.
	* @param detachThreads @a true -> detach worker threads, @a false ->
	* wait for threads to finish.
	* */
	void Stop(bool cancelQueue = false, bool detachThreads = false)
	{
		std::unique_lock lock { m_threads.Mutex() };
		if (!m_threads->empty())
		{
			m_shutdown = true; //!< set shutdown flag
			
			m_monitor.Stop(); //!< stop utilization monitoring
			
			if (cancelQueue) //!< cancelQueue -> dump queue items not yet processed
			{
				std::unique_lock lock2 { m_mtx };
				m_queue.clear();
				m_waitingSince = 0;
			}
			if (detachThreads)
			{
				std::unique_lock lock2 { m_mtx };
				m_openSequences.clear();
			}
			m_cv.notify_all(); //!< wake up all worker threads
			m_cvPause.notify_all();
			for (auto &thread: *m_threads)
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
			m_idleThreads = 0;
			
			m_shutdown = false; //!< clear shutdown flag
		}
	}
	
	bool IsPaused() { return m_pause.load(); }    //!< getter pause flag
	void Pause() { m_pause = true; }            //!< set pause flag, pause background worker
	void UnPause()                                //!< remove pause flag, unpause background worker, wake up paused thread
	{
		m_pause = false;
		m_cvPause.notify_all();
	}
	
	void OnFlush() //<! signal 'auto flush' when timer expires..
	{
		std::shared_lock lock { m_mtx };
		if (m_queue.size())
		{
			m_cv.notify_one(); //<! Max timer has been reached and queue has msgs to be processed.
		}
	}
	
	/**
	 * Starts a new monitoring task
	 * @return Shared pointer to new monitoring task
	 */
	BgwMonitor::Task::Ptr StartMonitoringTask()
	{
		return m_monitor.IsActive() ? m_monitor.StartTask(BusyThreads()) : nullptr;
	}

private:
	using ArgTuple = std::tuple<Args...>; //!< tuple type of arguments
	
	constexpr static size_t ArgCount = sizeof...(Args); //!< argument count
	
	/** Argument type
	 *
	 * @tparam index @a 0 -> first argument type, ...
	 */
	template <size_t index> using ArgType = typename std::tuple_element<index, ArgTuple>::type;
	
	using SequenceTagType = size_t; //!< Type of sequence tag
	using SequenceTag = std::optional<SequenceTagType>; //!< Type of optional sequence tag
	
	/** @brief Type of a single queued item. */
	struct Element
	{
		SequenceTag sequenceTag; //!< optional sequence tag
		ArgTuple args; //!< argument tuple
	};
	
	using ElementPtr = std::unique_ptr<Element>; //!< unique pointer to a queued item
	
	using Queue = std::deque<ElementPtr>; //!< Type of the queue
	
	std::atomic_size_t m_maxQueueSize; //!< maximum queue size
	
	Queue m_queue; //!< queue
	
	std::unordered_set<SequenceTagType> m_openSequences; //!< set of sequences currently being processed
	
	mutable std::shared_mutex m_mtx; //!< Mutex for accessing the processing queue
	
	/** Condition variable used to notify the worker threads of new items. */
	std::condition_variable_any m_cv;
	
	/** Condition variable used to notify the worker threads of Unpause. */
	std::condition_variable_any m_cvPause;
	
	/** @a true -> worker thread are shutting down (@a Stop() has been called). */
	std::atomic_bool m_shutdown;
	
	int64_t m_waitingSince; //!< Time when the current next element became the head of the queue
	
	std::atomic_size_t m_idleThreads; //!< number of threads waiting for new items
	
	SharedLockable<std::vector<std::thread>> m_threads; //!< vector of worker threads
	
	bool m_autoflush;                       //!< is autoflush enabled..
	long unsigned int m_batchSize;          //!< msg threshold, for autoflushing
	std::unique_ptr<UTILS::Timer> m_timer;  //!< autoflush timer
	
	/** @brief Adds a new item to the queue
	 *
	 * This function creates a new item from the given arguments and places
	 * it at the end of the processing queue.
	 *
	 * @param sequenceTag Arguments defining the new item to be queued.
	 * @param args Arguments defining the new item to be queued.
	 * @return @a true if the item was successfully added to the queue,
	 * @a false if the background worker is being stopped, or if the maximum
	 * queue size has bee reached.
	 */
	BoolResult EnqueueTagged(SequenceTag sequenceTag, Args ...args)
	{
		if (!m_shutdown)
		{
			std::unique_lock lock { m_mtx };
			if (const size_t queueSize { m_queue.size() }; queueSize < m_maxQueueSize.load())
			{
				if (queueSize == 0)
				{
					m_waitingSince = CurrentTimestamp();
				}
				m_queue.emplace_back(ElementPtr(new Element { sequenceTag, ArgTuple { std::forward<Args>(args)... }}));
				
				if (m_autoflush)
				{
					if (queueSize >= m_batchSize) //number of msgs exceeds batch size, so call auto flush
					{
						m_cv.notify_one(); //signal auto flush when batch size is reached ..
					}
				}
				else
				{
					m_cv.notify_one(); //signal flush after every msg..
				}
				
				return true;
			}
			else
			{
				return { setError, "Maximum queue size (%Lu) reached", m_maxQueueSize.load() };
			}
		}
		else
		{
			return { setError, "Background worker is being stopped" };
		}
	}
	
	/** @brief Calls the action with the arguments unwrapped from
	 * the queued tuple. */
	template <typename A, std::size_t... Is>
	void call_action_with_tuple(A action, ArgTuple &argTuple, std::index_sequence<Is...>)
	{
		action(std::get<Is>(argTuple)...);
	}
	
	template <typename A, std::size_t... Is>
	auto call_action_with_tuple_and_return_value(A action, ArgTuple &argTuple, std::index_sequence<Is...>)
	{
		return action(std::get<Is>(argTuple)...);
	}
	
	/** @brief Worker loop.
	 *
	 * \tparam A Type of the function to process a single item
	 * \param action Action to process a single item
	 * */
	template <typename A>
	void worker(A action)
	{
		std::unique_lock lock { m_mtx };
		typename Queue::iterator it { m_queue.begin() };
		while (!m_shutdown || it != m_queue.end())
		{
			if (!m_shutdown && it == m_queue.end())
			{
				IncIdleThreads();
				m_cv.wait(lock); //!< wait for an item to be queued
				DecIdleThreads();
			}
			if (!m_shutdown && IsPaused())
			{
				IncIdleThreads();
				m_cvPause.wait(lock);
				DecIdleThreads();
			}
			/// iterate the queue to get an item that can be processed
			for (it = m_queue.begin(); it < m_queue.end(); ++it)
			{
				const ElementPtr &elem { *it };
				/// accept if the item either has no sequence tag or a sequence tag that is currently not being processed
				if (!elem->sequenceTag || m_openSequences.find(elem->sequenceTag.value()) == m_openSequences.end())
				{
					break;
				}
			}
			if (it != m_queue.end())
			{
				ElementPtr elem { std::move(*it) };  //!< fetch the accepted item from the queue
				it = m_queue.erase(it); //!< remove the item from the queue
				if (m_queue.empty())
				{
					m_waitingSince = 0;
				}
				else if (it == m_queue.begin())
				{
					m_waitingSince = CurrentTimestamp();
				}
				
				if (elem->sequenceTag)
				{
					m_openSequences.insert(elem->sequenceTag.value()); //!< mark the sequence as open
				}
				lock.unlock(); //!< release the queue before starting to process the popped item
				try
				{
					call_action_with_tuple(action, elem->args, std::index_sequence_for<Args...>());
				}
				catch (...)
				{
					/// ignore exceptions in action
				}
				lock.lock(); //!< processing finished -> lock queue
				if (elem->sequenceTag)
				{
					m_openSequences.erase(elem->sequenceTag.value()); //!< mark the sequence as closed
				}
				it = m_queue.begin(); //!< reset the queue iterator
			}
		}
	}
	
	/** Pins the current thread to a specific core
	 *
	 * Taken from: https://bytefreaks.net/programming-2/cc-set-affinity-to-threads-example-code
	 *
	 * @param coreId Id of the core to which the current thread is to be pinned
	 * @return @a true if successful
	 */
	static BoolResult SetThreadAffinity(size_t coreId)
	{
		std::string errMsg;
		return { UTILS::SetThreadAffinity(coreId, errMsg), errMsg };
	}
	
	/** Sets the priority of the current thread
	 *
	 * Succeeds only with root privileges!
	 *
	 * @param priority Priority value to be set (1..lowest, 99..highest)
	 * @return @a true if successful
	 */
	static BoolResult SetThreadPriority(int priority)
	{
		std::string errMsg;
		return { UTILS::SetThreadPriority(priority, errMsg), errMsg };
	}
	
	std::atomic_bool m_pause { false };    //!< flag for pause running process
	
	BgwMonitor m_monitor; //!< Instance of class @a BgwMonitor, used to monitor thread utilization
	
	/**
	 * Increments the number of idle threads and updates all monitoring tasks.
	 * @param incBy (optional; default: 1) Increment (explicitly set only by @a DecIdleThreads)
	 */
	void IncIdleThreads(int incBy = 1)
	{
		if (incBy != 0)
		{
			m_monitor.UpdateIdleThreads(m_idleThreads += incBy);
		}
	}
	
	/**
	 * Decrements the number of idle threads by 1 and updates all monitoring tasks.
	 */
	void DecIdleThreads() { IncIdleThreads(-1); }
};

} // namespace CORE
