/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2018-02-13
 * @copyright   (c) 2018 BBF-IT
*/

#ifndef COROUT_EVENTMANAGER_H
#define COROUT_EVENTMANAGER_H

#include <shared_mutex>
#include <mutex>

#include <Poco/Delegate.h>
#include <Poco/BasicEvent.h>

namespace UTILS {

/*! \brief Base template for event parameter classes.
 *
 * @param TSource   type of the event source object
 *
 */
template <typename TSource>
class EventParams
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and no other parameters.
	 * */
	explicit EventParams(TSource *source)
			: m_source(source) { }
	
	/*! \brief Default destructor. */
	virtual ~EventParams() = default;
	
	/*! \brief Default copy constructor. */
	EventParams(const EventParams & /* other */) = default;
	
	/*! \brief Default copy assignment operator. */
	EventParams &operator=(const EventParams & /* other */) = default;
	
	/*! \brief Default move constructor. */
	EventParams(EventParams && /* other */) = default;
	
	/*! \brief Default move assignment operator. */
	EventParams &operator=(EventParams && /* other */) = default;
	
	/*! \brief Event source. */
	TSource *Source() const { return m_source; }

private:
	/*! \brief Event source. */
	TSource *m_source;
};

/*! \brief Template for event parameter classes containing one parameter.
 *
 * @param TSource   type of the event source object
 * @param T1   type of the first parameter
 *
 */
template <typename TSource, typename T1>
class EventParams1 : public EventParams<TSource>
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and one other parameter.
	 * */
	EventParams1(TSource *source, T1 param1)
			: EventParams<TSource>(source), m_param1(param1) { }
	
	/*! \brief First parameter. */
	T1 Param1() const { return m_param1; }

private:
	/*! \brief First parameter. */
	T1 m_param1;
};

/*! \brief Template for event parameter classes containing two parameters.
 *
 * @param TSource   type of the event source object
 * @param T1   type of the first parameter
 * @param T2   type of the second parameter
 *
 */
template <typename TSource, typename T1, typename T2>
class EventParams2 : public EventParams1<TSource, T1>
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and two other parameters.
	 * */
	EventParams2(TSource *source, T1 param1, T2 param2)
			: EventParams1<TSource, T1>(source, param1), m_param2(param2) { }
	
	/*! \brief Second parameter. */
	T2 Param2() const { return m_param2; }

private:
	/*! \brief Second parameter. */
	T2 m_param2;
};

/*! \brief Template for event parameter classes containing three parameters.
 *
 * @param TSource   type of the event source object
 * @param T1   type of the first parameter
 * @param T2   type of the second parameter
 * @param T3   type of the third parameter
 *
 */
template <typename TSource, typename T1, typename T2, typename T3>
class EventParams3 : public EventParams2<TSource, T1, T2>
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and three other parameters.
	 * */
	EventParams3(TSource *source, T1 param1, T2 param2, T3 param3)
			: EventParams2<TSource, T1, T2>(source, param1, param2), m_param3(param3) { }
	
	/*! \brief Third parameter. */
	T3 Param3() const { return m_param3; }

private:
	/*! \brief Third parameter. */
	T3 m_param3;
};

/*! \brief Template for event parameter classes containing four parameters.
 *
 * @param TSource   type of the event source object
 * @param T1   type of the first parameter
 * @param T2   type of the second parameter
 * @param T3   type of the third parameter
 * @param T4   type of the fourth parameter
 *
 */
template <typename TSource, typename T1, typename T2, typename T3, typename T4>
class EventParams4 : public EventParams3<TSource, T1, T2, T3>
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and four other parameters.
	 * */
	EventParams4(TSource *source, T1 param1, T2 param2, T3 param3, T4 param4)
			: EventParams3<TSource, T1, T2, T3>(source, param1, param2, param3), m_param4(param4) { }
	
	/*! \brief Fourth parameter. */
	T4 Param4() const { return m_param4; }

private:
	/*! \brief Fourth parameter. */
	T4 m_param4;
};

/*! \brief Template for event parameter classes containing five parameters.
 *
 * @param TSource   type of the event source object
 * @param T1   type of the first parameter
 * @param T2   type of the second parameter
 * @param T3   type of the third parameter
 * @param T4   type of the fourth parameter
 * @param T5   type of the fifth parameter
 *
 */
template <typename TSource, typename T1, typename T2, typename T3, typename T4, typename T5>
class EventParams5 : public EventParams4<TSource, T1, T2, T3, T4>
{
public:
	/*! \brief Constructor.
	 *
	 * Constructs the parameter object from the event source and five other parameters.
	 * */
	EventParams5(TSource *source, T1 param1, T2 param2, T3 param3, T4 param4, T5 param5)
			: EventParams4<TSource, T1, T2, T3, T4>(source, param1, param2, param3, param4), m_param5(param5) { }
	
	/*! \brief Fifth parameter. */
	T5 Param5() const { return m_param5; }

private:
	/*! \brief Fifth parameter. */
	T5 m_param5;
};

/*! \brief Template for event management classes.
 *
 * @param TSource   type of the event source object
 * @param TEventType    enum type of event types
 * @param eventCount    number of event types
 *
 */
template <typename TSource, typename TEventType, int eventCount>
class EventManager
{
public:
	/*! \brief Alias for Poco::BasicEvent type, instantiated with the EventParams type. */
	using Event = Poco::BasicEvent<const EventParams<TSource> &>;
	
	/*! \brief Registers an event handler.
	 *
	 * Function template.
	 *
	 * @param TObj  type of the object that contains the event handler
	 * @param eventType type of the event to be handled
	 * @param pObj object containing the event handler
	 * @param NotifyMethod event handler
	 * */
	template <class TObj>
	void RegisterHandler(TEventType eventType, TObj *pObj, void (TObj::*NotifyMethod)(const EventParams<TSource> &))
	{
		std::unique_lock lock { m_mtxEvent };
		m_event[eventType] += Poco::Delegate<TObj, const EventParams<TSource> &, false>(pObj, NotifyMethod);
	}
	
	/*! \brief Unregisters an event handler.
	 *
	 * Function template.
	 *
	 * @param TObj  type of the object that contains the event handler
	 * @param eventType type of the event to be unregistered
	 * @param pObj object containing the event handler
	 * @param NotifyMethod event handler
	 * */
	template <class TObj>
	void UnregisterHandler(TEventType eventType, TObj *pObj, void (TObj::*NotifyMethod)(const EventParams<TSource> &))
	{
		std::unique_lock lock { m_mtxEvent };
		m_event[eventType] -= Poco::Delegate<TObj, const EventParams<TSource> &, false>(pObj, NotifyMethod);
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event without parameters.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 *
	 * @return \a true if successful
	 * */
	bool FireEvent(TEventType eventType, TSource *source)
	{
		return FireEvent(eventType, EventParams<TSource>(source));
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with one parameter.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 * @param param1    first parameter
	 *
	 * @return \a true if successful
	 * */
	template <typename T1>
	bool FireEvent(TEventType eventType, TSource *source, T1 param1)
	{
		return FireEvent(eventType, EventParams1<TSource, T1>(source, param1));
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with two parameters.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 * @param param1    first parameter
	 * @param param2    second parameter
	 *
	 * @return \a true if successful
	 * */
	template <typename T1, typename T2>
	bool FireEvent(TEventType eventType, TSource *source, T1 param1, T2 param2)
	{
		return FireEvent(eventType, EventParams2<TSource, T1, T2>(source, param1, param2));
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with three parameters.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 * @param param1    first parameter
	 * @param param2    second parameter
	 * @param param3    third parameter
	 *
	 * @return \a true if successful
	 * */
	template <typename T1, typename T2, typename T3>
	bool FireEvent(TEventType eventType, TSource *source, T1 param1, T2 param2, T3 param3)
	{
		return FireEvent(eventType, EventParams3<TSource, T1, T2, T3>(source, param1, param2, param3));
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with four parameters.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 * @param param1    first parameter
	 * @param param2    second parameter
	 * @param param3    third parameter
	 * @param param4    fourth parameter
	 *
	 * @return \a true if successful
	 * */
	template <typename T1, typename T2, typename T3, typename T4>
	bool FireEvent(TEventType eventType, TSource *source, T1 param1, T2 param2, T3 param3, T4 param4)
	{
		return FireEvent(eventType, EventParams4<TSource, T1, T2, T3, T4>(source, param1, param2, param3, param4));
	}
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with five parameters.
	 *
	 * @param eventType type of the event
	 * @param source    event source
	 * @param param1    first parameter
	 * @param param2    second parameter
	 * @param param3    third parameter
	 * @param param4    fourth parameter
	 * @param param5    fifth parameter
	 *
	 * @return \a true if successful
	 * */
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	bool FireEvent(TEventType eventType, TSource *source, T1 param1, T2 param2, T3 param3, T4 param4, T5 param5)
	{
		return FireEvent(eventType, EventParams5<TSource, T1, T2, T3, T4, T5>(source, param1, param2, param3, param4, param5));
	}
	
	/*! \brief Extracts the event source.
	 *
	 * Extracts the event source from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 *
	 * @return pointer to the event source
	 * */
	TSource *Unwrap(const EventParams<TSource> &params) const
	{
		return params.Source();
	}
	
	/*! \brief Extracts parameters.
	 *
	 * Extracts one parameter from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 * @param param1    reference to the first parameter
	 *
	 * @return pointer to the event source, \a nullptr if the params object cannot be casted to the required type
	 * */
	template <typename T1>
	TSource *Unwrap(const EventParams<TSource> &params, T1 &param1) const
	{
		try
		{
			auto *typedParams { dynamic_cast<const EventParams1<TSource, T1> *>(&params) };
			if (typedParams != nullptr)
			{
				param1 = typedParams->Param1();
				return params.Source();
			}
		}
		catch (...)
		{
		}
		return nullptr;
	}
	
	/*! \brief Extracts parameters.
	 *
	 * Extracts two parameters from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 * @param param1    reference to the first parameter
	 * @param param2    reference to the second parameter
	 *
	 * @return pointer to the event source, \a nullptr if the params object cannot be casted to the required type
	 * */
	template <typename T1, typename T2>
	TSource *Unwrap(const EventParams<TSource> &params, T1 &param1, T2 &param2) const
	{
		try
		{
			auto *typedParams { dynamic_cast<const EventParams2<TSource, T1, T2> *>(&params) };
			if (typedParams != nullptr)
			{
				param1 = typedParams->Param1();
				param2 = typedParams->Param2();
				return params.Source();
			}
		}
		catch (...)
		{
		}
		return nullptr;
	}
	
	/*! \brief Extracts parameters.
	 *
	 * Extracts three parameters from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 * @param param1    reference to the first parameter
	 * @param param2    reference to the second parameter
	 * @param param3    reference to the third parameter
	 *
	 * @return pointer to the event source, \a nullptr if the params object cannot be casted to the required type
	 * */
	template <typename T1, typename T2, typename T3>
	TSource *Unwrap(const EventParams<TSource> &params, T1 &param1, T2 &param2, T3 &param3) const
	{
		try
		{
			auto *typedParams { dynamic_cast<const EventParams3<TSource, T1, T2, T3> *>(&params) };
			if (typedParams != nullptr)
			{
				param1 = typedParams->Param1();
				param2 = typedParams->Param2();
				param3 = typedParams->Param3();
				return params.Source();
			}
		}
		catch (...)
		{
		}
		return nullptr;
	}
	
	/*! \brief Extracts parameters.
	 *
	 * Extracts four parameters from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 * @param param1    reference to the first parameter
	 * @param param2    reference to the second parameter
	 * @param param3    reference to the third parameter
	 * @param param4    reference to the fourth parameter
	 *
	 * @return pointer to the event source, \a nullptr if the params object cannot be casted to the required type
	 * */
	template <typename T1, typename T2, typename T3, typename T4>
	TSource *Unwrap(const EventParams<TSource> &params, T1 &param1, T2 &param2, T3 &param3, T4 &param4) const
	{
		try
		{
			auto *typedParams { dynamic_cast<const EventParams4<TSource, T1, T2, T3, T4> *>(&params) };
			if (typedParams != nullptr)
			{
				param1 = typedParams->Param1();
				param2 = typedParams->Param2();
				param3 = typedParams->Param3();
				param4 = typedParams->Param4();
				return params.Source();
			}
		}
		catch (...)
		{
		}
		return nullptr;
	}
	
	/*! \brief Extracts parameters.
	 *
	 * Extracts five parameters from a parameter object.
	 *
	 * @param params    const reference to the parameter object
	 * @param param1    reference to the first parameter
	 * @param param2    reference to the second parameter
	 * @param param3    reference to the third parameter
	 * @param param4    reference to the fourth parameter
	 * @param param5    reference to the fifth parameter
	 *
	 * @return pointer to the event source, \a nullptr if the params object cannot be casted to the required type
	 * */
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	TSource *Unwrap(const EventParams<TSource> &params, T1 &param1, T2 &param2, T3 &param3, T4 &param4, T5 &param5) const
	{
		try
		{
			auto *typedParams { dynamic_cast<const EventParams5<TSource, T1, T2, T3, T4, T5> *>(&params) };
			if (typedParams != nullptr)
			{
				param1 = typedParams->Param1();
				param2 = typedParams->Param2();
				param3 = typedParams->Param3();
				param4 = typedParams->Param4();
				param5 = typedParams->Param5();
				return params.Source();
			}
		}
		catch (...)
		{
		}
		return nullptr;
	}

private:
	/*! \brief Array of POCO event handlers. */
	Event m_event[eventCount];
	
	/*! \brief Mutex for access to array of POCO event handlers. */
	std::shared_mutex m_mtxEvent;
	
	/*! \brief Fires an event.
	 *
	 * Fires an event with a parameter object.
	 *
	 * @param eventType type of the event
	 * @param params    reference to a parameter object
	 * @return \a true if successful
	 * */
	bool FireEvent(TEventType eventType, const EventParams<TSource> &params)
	{
		if ((int) eventType >= 0 && (int) eventType < eventCount)
		{
			try
			{
                std::shared_lock lock(m_mtxEvent);
				m_event[eventType].notify(this, params);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
};

} // namespace CORE

#endif //COROUT_EVENTMANAGER_H
