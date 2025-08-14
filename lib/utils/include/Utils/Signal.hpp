#pragma once


#include <functional>
#include <map>

namespace UTILS
{

/**
 * @brief   A signal object may call multiple slots with the same signature. \n
 *          You can connect functions to the signal which will be called when the emit() method on the
 *          signal object is invoked. Any argument passed to emit() will be passed to the given functions.
 */
template<typename... Args>
class Signal
{
public:
	/** @brief Constructor */
	Signal() : m_currentId(0)
	{ }

	/** @brief Copy constructor */
	Signal(Signal const &other)	: m_currentId(0)
	{ }

	/** @brief Copy assignment operator. */
	Signal &operator=(Signal const &other)
	{
		DisconnectAll();
		return *this;
	}

	/**
	 * @brief connects a member function to this Signal \n
	 * @return signal id, can used for Disconnect()
	 */
	template<typename T>
	int ConnectMember(T *inst, void (T::*func)(Args...))
	{
		return Connect([=](Args... args)
		{
			(inst->*func)(args...);
		});
	}

	/**
	 * @brief connects a const member function to this Signal \n
	 * @return signal id, can used for Disconnect()
	 */
	template<typename T>
	int ConnectMember(T *inst, void (T::*func)(Args...) const)
	{
		return Connect([=](Args... args)
		{
			(inst->*func)(args...);
		});
	}

	/**
	 * @brief connects a std::function to the Signal. \n
	 * @return signal id, can used for Disconnect() the function
	 */
	int Connect(std::function<void(Args...)> const &slot) const
	{
		m_slots.insert(std::make_pair(++m_currentId, slot));
		return m_currentId;
	}

	/**
	 * @brief	disconnect a previously connected function
	 * @param[in] id return value of Connect()
	 */
	void Disconnect(int id) const
	{
		m_slots.erase(id);
	}

	/**
	 * @brief disconnects all previously connected functions
	 */
	void DisconnectAll() const
	{
		m_slots.clear();
	}

	/**
	 * @brief calls all connected functions
	 */
	void Emit(Args... p)
	{
		for (auto it : m_slots)
		{
			it.second(p...);
		}
	}

private:
	mutable std::map<int, std::function<void(Args...)>> m_slots;
	mutable int m_currentId;
};


}

