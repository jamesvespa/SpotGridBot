//
// Created by james aleksandra on 8/30/23.
//

#ifndef MERCURYUTILS_ONETOONEQUEUE_H
#define MERCURYUTILS_ONETOONEQUEUE_H


#include <memory>
#include <atomic>
#include <optional>
#include <thread>
#include "utils.h"
#include "Utils/BgwMonitor.h"

namespace UTILS {

/**
 * Lock-free queue assuming one producer and one consumer thread.
 *
 * @tparam T Type of the queue element
 * @tparam maxCount Maximum queue element count
 */
template <typename T, size_t maxCount>
class OneToOneQueue
{
public:
	
	OneToOneQueue()
			: m_buffer { }, m_ixHead(0), m_ixTail(0), m_shutdown(false), m_cancelQueue(false) { }
	
	~OneToOneQueue() { Stop(true); }
	
	/**
	 * Enqueue a value
	 * @param value Value to be added to the queue
	 * @return @a true -> @a value was successfully enqueued, @a false -> @a maxCount is reached
	 */
	bool Enqueue(T &&value)
	{
		if (!m_shutdown.load(std::memory_order_relaxed)) [[likely]]
		{
			const size_t ixT
					{ GetTailIndex() };        // current tail index can be stored in a local variable - assuming that no other thread can change m_ixTail
			const size_t queueSize { Dist(GetHeadIndex(), ixT) };
			if (queueSize == maxCount)  // check whether buffer is full
			{
				return false;
			}
			m_buffer[ixT] = std::move(value);           // move value to current tail
			SetTailIndex(Inc(ixT));                     // update tail index
			m_monitor.UpdateQueueSize(queueSize + 1);
			return true;
		}
		return false;
	}
	
	/**
		 * Dequeue a value
	 * @param ref (out) reference to the variable to hold the dequeued element
	 * @return @a true -> @a value was successfully dequeued, @a false -> @a queue is empty
	 */
	bool Dequeue(T &ref)
	{
		const size_t ixH
				{ GetHeadIndex() };        // current head index can be stored in a local variable - assuming that no other thread can change m_ixHead
		const size_t queueSize { Dist(ixH, GetTailIndex()) };
		if (queueSize == 0)                  // check whether buffer is empty
		{
			return false;
		}
		ref = std::move(m_buffer[ixH]);             // move current head element to ref
		m_buffer[ixH] = {};
		SetHeadIndex(Inc(ixH));                     // update head index
		m_monitor.UpdateQueueSize(queueSize - 1);
		return true;
	}

	bool Dequeue(std::size_t idx, T& ref)
	{
		if (Empty() || idx > maxCount)
		{
			return false;
		}
		ref = std::move(m_buffer[idx]);
		m_buffer[idx] = {};
		// no change to head
		return true;
	}

	bool Insert(std::size_t pos, T&& value)
	{
		if (pos > maxCount) [[unlikely]]
		{
			return false;
		}
		m_buffer[pos] = std::move(value);
		// no change to tail
		return true;
	}
	
	size_t QueueSize() const { return Dist(m_ixHead, m_ixTail); }
	
	bool Empty() const { return QueueSize() == 0; }
	
	bool Full() const { return QueueSize() == maxCount; }
	
	size_t MaxQueueSize() { return maxCount; }
	
	/** @brief Is the background worker running (i.e. processes queued items)? */
	bool Running() const
	{
		return m_workerThread.joinable();
	}

	/** @brief Stops the ring buffer.
*
* @param cancelQueue @a true -> cancels all queued items not yet
* processed, @a false -> continue processing items until the queue
* is empty.
* */
	void Stop(bool cancelQueue = false)
	{
		if (m_workerThread.joinable())
		{
			m_cancelQueue = cancelQueue;
			m_shutdown = true;
			m_monitor.Stop(); //!< stop utilization monitoring
			m_workerThread.join();
			m_shutdown = false;
		}
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
	void Start(const std::string &name, A action, bool pinToCore = true)
	{
		m_monitor.Start(1);
		m_workerThread = std::thread([this, name, action, pinToCore]()
									 {
										 bool busy { false };
										 if (!name.empty())
										 {
											 UTILS::SetThreadName(name);
											 if (pinToCore)
											 {
												 UTILS::SetThreadAffinityFromConfig();
											 }
										 }
										 T element;
										 while (!m_shutdown.load(std::memory_order_relaxed))[[likely]]
										 {
                                             if (! m_pauseDequeue.load(std::memory_order_relaxed))    [[likely]]
                                             {
                                                 if (Dequeue(element))
                                                 {
                                                     if (!busy) // change from idle to busy -> update monitor
                                                     {
                                                         busy = true;
                                                         m_monitor.UpdateIdleThreads(0);
                                                     }
                                                     action(std::move(element)); // pass dequeued element to action
                                                 }
                                                 else
                                                 {
                                                     if (busy) // change from busy to idle -> update monitor
                                                     {
                                                         busy = false;
                                                         m_monitor.UpdateIdleThreads(1);
                                                     }
                                                 }
                                             }
										 }
										 if (!m_cancelQueue) // cancelQueue == false => process remaining elements
										 {
											 while (Dequeue(element))
											 {
												 action(std::move(element)); // pass dequeued element to action
											 }
										 }
									 });
	}
	
	/** @brief Number of busy threads set to 1 as ringbuffer is single threaded */
	size_t BusyThreads() const { return 1; }
	
	/**
 	* Starts a new monitoring task
 	* @return Shared pointer to new monitoring task
 	*/
	UTILS::BgwMonitor::Task::Ptr StartMonitoringTask()
	{
		return m_monitor.IsActive() ? m_monitor.StartTask(BusyThreads(), QueueSize()) : nullptr;
	}

	inline size_t GetTailIndex() const { return m_ixTail.load(std::memory_order_relaxed); }
    inline size_t GetHeadIndex() const { return m_ixHead.load(std::memory_order_relaxed); }
protected:
    void PauseDequeue()
    {
        m_pauseDequeue.store(true, std::memory_order_relaxed);
    }
    void ResumeDequeue()
    {
        m_pauseDequeue.store(false, std::memory_order_relaxed);
    }
private:

	static constexpr size_t size { maxCount + 1 }; // set array size to maxCount+1, to be able to distinguish empty queue from full queue

	T m_buffer[size];
	std::atomic_size_t m_ixHead;  // read access on enqueuing, read+write access on dequeuing
	std::atomic_size_t m_ixTail;  // read+write access on enqueuing, read access on dequeuing

	std::thread m_workerThread; // worker for dequeueing elements and executing action

	std::atomic_bool m_shutdown;
	std::atomic_bool m_cancelQueue;
    std::atomic_bool m_pauseDequeue{false};

	UTILS::BgwMonitor m_monitor; //!< Instance of class @a BgwMonitor, used to monitor thread utilization

	inline void SetHeadIndex(size_t val) { m_ixHead.store(val, std::memory_order_relaxed); }


	inline void SetTailIndex(size_t val) { m_ixTail.store(val, std::memory_order_relaxed); }
	
	static inline size_t Inc(size_t ix) { return ix < size - 1 ? ix + 1 : 0; }
	
	static inline size_t Dist(size_t ixH, size_t ixT) { return ixH <= ixT ? ixT - ixH : ixT + size - ixH; }
	
};

} // namespace UTILS

#endif //MERCURYUTILS_ONETOONEQUEUE_H
