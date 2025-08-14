#pragma once

#include <chrono>
#include <mutex>
#include <shared_mutex>

namespace UTILS
{

/*! \brief This class template combines a value of the template type T with a
 * mutex variable. A variable of type \a Lockable can be directly passed to
 * \a std::lock_guard, \a std::unique_lock, or \a std::shared_lock. */
template <typename T, typename M = std::mutex>
class Lockable
{
public:
	/*! \brief Default constructor */
	Lockable() = default;
	
	/*! \brief Constructor that forwards the argument list to the constructor
	 * of the value. */
	template <typename ...Args>
	explicit Lockable(Args &&...args)
			: m_value(std::forward<Args>(args)...) { }
	
	/*! \brief Indirection operator. */
	T &operator*() { return m_value; }
	
	/*! \brief Indirection operator (const). */
	const T &operator*() const { return m_value; }
	
	/*! \brief Dereference operator. */
	T *operator->() { return &m_value; }
	
	/*! \brief Dereference operator (const). */
	const T *operator->() const { return &m_value; }
	
	/*! \brief \a Lock operation. */
	void lock() { m_mtx.lock(); }
	
	/*! \brief \a Lock attempt operation. */
	bool try_lock() { return m_mtx.try_lock(); }
	
	/*! \brief \a Unlock operation. */
	void unlock() { m_mtx.unlock(); }
	
	/*! \brief Reference to mutex variable. */
	M &Mutex() const { return m_mtx; }
	
	/*! \brief Const reference to value. */
	const T &Value() const { return m_value; }
	
	/*! \brief Reference to value. */
	T &Value() { return m_value; }
	
	/*! \brief Executes an action using a unique lock.
	 *
	 * @tparam A Type of the action to be executed
	 * @param action Action to be executed
	 */
	template <typename A>
	void Do(A action)
	{
		std::unique_lock lock { Mutex() };
		action(Value());
	}

private:
	/*! \brief Mutex variable */
	mutable M m_mtx;
	/*! \brief Value member */
	T m_value;
};

template <typename T>
class SharedLockable : public Lockable<T, std::shared_mutex>
{
public:
	/*! \brief Default constructor */
	SharedLockable() = default;
	
	/*! \brief Constructor that forwards the argument list to the constructor
	 * of the value. */
	template <typename ...Args>
	explicit SharedLockable(Args &&...args)
			: Lockable<T, std::shared_mutex>(std::forward<Args>(args)...) { }
	
	/*! \brief \a Shared lock operation. */
	void lock_shared() { this->Mutex().lock_shared(); }
	
	/*! \brief \a Shared lock attempt operation. */
	bool try_lock_shared() { return this->Mutex().try_lock_shared(); }
	
	/*! \brief \a Shared unlock operation. */
	void unlock_shared() { this->Mutex().unlock_shared(); }
	
	/*! \brief Executes a non-modifying action using a shared lock.
	 *
	 * @tparam A Type of the action to be executed
	 * @param action Action to be executed
	 */
	template <typename A>
	void DoShared(A action) const
	{
		std::shared_lock lock { this->Mutex() };
		action(this->Value());
	}
	
	/*! \brief Executes a non-modifying function using a shared lock.
	 *
	 * The given function is executed on the shared locked object. The return
	 * value of the function is passed to the caller.
	 *
	 * @tparam R Return type of the function to be executed
	 * @tparam F Type of the function to be executed
	 * @param function Function to be executed
	 */
	template <typename R, typename F>
	R Get(F function) const
	{
		std::shared_lock lock { this->Mutex() };
		return function(this->Value());
	}
};

}
