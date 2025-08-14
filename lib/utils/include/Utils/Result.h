#pragma once

#include <optional>

#include "Utils/Utils.h"

namespace UTILS {

/*! \brief This macro creates an result object depending on a condition
 *
 * @param _type_ Type of the result to be created
 * @param _condition_ Condition that determines whether the result object represents a success
 * @param _value_ Value to be set if \a _condition_ evaluates to \a true
 * @param _error_ Error message to be set if \a _condition_ evaluates to \a false
 * */
#define CONDITIONAL_RESULT(_type_, _condition_, _value_, _error_) ((_condition_) ? Result<_type_>(_value_) : Result<_type_>(setError, _error_))
#define COND_RESULT(condition_, _value_, _error_) CONDITIONAL_RESULT(decltype(_value_), condition_, _value_, _error_)

/*! \brief Empty struct used for constructor disambiguation.
 *
 * If an object of this type is passed to the constructor of the \a Result
 * type, the following argument is interpreted as error message.
 * */
struct SetErrorT
{
};

const extern SetErrorT setError;

#define EMSG_NO_ERROR            ""                        //!< Error message in \a Success state
#define EMSG_UNINITIALIZED        "(uninitialized)"        //!< Error message of an uninitialized object
#define EMSG_UNSPECIFIED_ERROR    "(unspecified error)"    //!< Error message of an object if \a Fail state if no error message was set

/*! \brief This class template stores either a value of an arbitrary type or
 * an error message.
 *
 * A result object can take one of two states:
 *
 * 1. In \a Success state, it contains a result value of the template type,
 * and contains an empty error message
 * 2. In \a Fail state, it does not contain a result value, and contains
 * a non-empty error message
 *
 * @tparam T Type of the return value
 * */
template <typename T>
class Result
{
public:
	
	/*! \brief Default constructor.
	 *
	 * Sets the result to \a std::nullopt and the error message to
	 * the value "(uninitialized)".
	 *
	 * The created object is in \a Fail state.
	 * */
	Result()
			: Result(std::nullopt, EMSG_UNINITIALIZED) { }
	
	/*! \brief Constructor setting a value.
	 *
	 * Sets the result to the given value and clears the error message.
	 *
	 * The created object is in \a Success state.

	 * @param value Value to be set.
	 * */
	Result(T value)
			: Result(std::move(value), EMSG_NO_ERROR) { }
	
	/*! \brief Constructor setting an error message.
	 *
	 * Sets the result to \a std::nullopt, and the error message to the
	 * given string.
	 *
	 * The created object is in \a Fail state.
	 *
	 * @param SetErrorType \a setError
	 * @param errorMessageFmt May be either a string or a format string
	 * defining the mask that is filled with the following \a args
	 * @param args Arguments to be inserted into the format string
	 * */
	template <typename ...Args>
	Result(SetErrorT, const std::string &errorMessageFmt, Args &&...args)
			: Result(std::nullopt, UTILS::Format(errorMessageFmt, std::forward<Args>(args)...)) { }
	
	/*! \brief Conversion operator to bool.
	 *
	 * @return \a true if the result object is in \a Success state.
	 * */
	explicit operator bool() const { return Success(); }
	
	/*! \brief Result value.
	 *
	 * @return Const reference to the result value if the result object is
	 * in \a Success state, otherwise an exception of type
	 * \a std::bad_optional_access is thrown.
	 * */
	const T &Value() const { return m_value.value(); }
	
	/*! \brief Result value.
	 *
	 * @return Reference to the result value if the result object is
	 * in \a Success state, otherwise an exception of type
	 * \a std::bad_optional_access is thrown.
	 * */
	T &Value() { return m_value.value(); }
	
	/*! \brief Result value.
	 *
	 * @param defaultValue Returned value if the result object is in \a Fail
	 * state
	 * @return Result value if the result object is in \a Success state,
	 * otherwise \a defaultValue.
	 * */
	const T ValueOr(T defaultValue) const { return m_value.value_or(defaultValue); }
	
	/*! \brief Error message.
	 *
	 * @return Error message if the result object is in \a Fail state,
	 * empty string otherwise.
	 * */
	const std::string &ErrorMessage() const { return m_errorMessage; }
	
	/*! \brief Check whether the object is in \a Success state.
	 *
	 * @return \a true if the result object is in \a Success state.
	 * */
	bool Success() const { return m_value.has_value(); }
	
	/*! \brief Check whether the result object is in initial state.
	 *
	 * @return \a true if the result object is initial state (neither value nor error set).
	 * */
	bool NotSet() const { return !Success() && m_errorMessage == EMSG_UNINITIALIZED; }
	
protected:
	/*! \brief Constructor setting value and error message.
	 *
	 * Protected. Sets the result to a given value, and the error message
	 * to the given string.
	 *
	 * If \a equals to \a std::nullopt, then the created object is in
	 * \a Fail state, otherwise it is in \a Success state.
	 *
	 * @param value	Optional value to be set
	 * @param errorMessage errorMessage to be set if \a value is
	 * \a std::nullopt. If \a value is \a std::nullopt and \a errorMessage
	 * is empty, then "(unspecified error)" is set as error message.
	 * */
	template <typename ...Args>
	Result(std::optional<T> value, std::string errorMessage)
			: m_value(std::move(value)),
			  m_errorMessage(m_value.has_value() ? EMSG_NO_ERROR : (errorMessage.empty() ? EMSG_UNSPECIFIED_ERROR : std::move(errorMessage))) { }

private:
	
	std::optional<T> m_value;    //!< Optional value
	std::string m_errorMessage; //!< Error message
};

/*! \brief This class represents a boolean result that stores an error
 * message if it is set to \a false
 *
 * Uses Result<bool> as private base class.
 * */
class BoolResult : private Result<bool>
{
public:
	/*! \brief Constructor setting the result value to \a true or \a false.
	 *
	 * Depending on the given condition, the result value becomes either
	 * \a true or \a false. If the condition is \a false then the error message
	 * is set to the given error string.
	 *
	 * The created object is in \a Success state if the condition is \a true,
	 * otherwise it is in \a Fail state.
	 *
	 * @param condition Condition that determines the result value
	 * @param errorMessageFmt May be either a string or a format string
	 * defining the mask that is filled with the following \a args
	 * @param args Arguments to be inserted into the format string
	 * */
	template <typename ...Args>
	BoolResult(bool condition, const std::string &errorMessageFmt, Args &&...args)
			: Result<bool>(condition ? std::optional(true) : std::nullopt,
						   condition ? EMSG_NO_ERROR : Format(errorMessageFmt, std::forward<Args>(args)...)) { }
	
	/*! \brief Constructor setting an error message.
	 *
	 * Sets the result to \a false, and the error message to the
	 * given string.
	 *
	 * The created object is in \a Fail state.
	 *
	 * @param SetErrorType \a setError
	 * @param errorMessageFmt May be either a string or a format string
	 * defining the mask that is filled with the following \a args
	 * @param args Arguments to be inserted into the format string
	 * */
	template <typename ...Args>
	BoolResult(SetErrorT, const std::string &errorMessageFmt, Args &&...args)
			: BoolResult(false, errorMessageFmt, std::forward<Args>(args)...) { }
	
	/*! \brief Default constructor.
	 *
	 * Sets the result to false and the error message to
	 * the value "(uninitialized)".
	 *
	 * The created object is in \a Fail state.
	 * */
	BoolResult()
			: BoolResult(false, EMSG_UNINITIALIZED) { }
	
	/*! \brief Constructor setting the result value to \a true or \a false.
	 *
	 * Depending on the given condition, the result value becomes either
	 * \a true or \a false. If the condition is \a false then the error message
	 * is set to the value "(unspecified error)".
	 *
	 * The created object is in \a Success state if the condition is \a true,
	 * otherwise it is in \a Fail state.
	 *
	 * @param condition Condition that determines the result value
	 * */
	BoolResult(bool condition)
			: BoolResult(condition, EMSG_UNSPECIFIED_ERROR) { }
	
	/**
	 * Constructing a @a BoolResult from any @a Result<T>
	 * @tparam T Template type of Result
	 * @param r Source result
	 */
	template <typename T>
	BoolResult(const Result<T> &r)
			: BoolResult(r.Success(), r.ErrorMessage()) { }
	
	using Result<bool>::operator bool; //!< Operator adopted from private base class
	using Result<bool>::ErrorMessage; //!< Function adopted from private base class
	using Result<bool>::NotSet; //!< Function adopted from private base class
};

} // CORE

