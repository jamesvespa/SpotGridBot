#pragma once

#include <cmath>
#include <fstream>
#include <memory>
#include <vector>
#include <map>
#include <cmath>
#include <functional>
#include <thread>
#include <future>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>

#include <Poco/AutoPtr.h>
#include <Poco/Format.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/Stopwatch.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Timestamp.h>

#ifdef LOGGING
#define IF_LOGGING(_stmt_)  { _stmt_; }
#else
#define IF_LOGGING(_stmt_)  {}
#endif

namespace Poco::XML {
class Document;
}

using namespace std::literals;

namespace UTILS
{

#define COMM_VALUE              "Engine"
#define COMM_VALUE_MEMCHECK     "memcheck-amd64-"

#define TAG_CONFIGURATION   "Configuration"

#define CC_RESET        "\033[0m"
#define CC_BRIGHT       "\033[1m"
#define CC_DIM          "\033[2m"
#define CC_BACKGROUND   "\033[3m"
#define CC_UNDERSCORE   "\033[4m"
#define CC_BLINK        "\033[5m"
#define CC_REVERSE      "\033[7m"

#define CC_BLACK        "\033[30m"
#define CC_RED          "\033[31m"
#define CC_GREEN        "\033[32m"
#define CC_YELLOW       "\033[33m"
#define CC_BLUE         "\033[34m"
#define CC_PURPLE       "\033[35m"
#define CC_CYAN         "\033[36m"
#define CC_WHITE        "\033[37m"

const size_t QUANTITY_DECIMAL_FACTOR = 100;
const size_t QUANTITY_DECIMAL_FACTOR_CRYPTO = 100000000;

/*! \brief Converts a double representation of a currency value into a cpip value
 * (centipip = 1/100 of a pip)
 *
 * @param dbl       currency value
 * @param factor    pip factor (either PRICE_DECIMAL_PRECISION or PRICE_DECIMAL_PRECISION_JPY)
 * @return          value in cpips
 * */

class CurrencyPair;

std::string QtyToString(const CurrencyPair& cp, int64_t qty);

int64_t StringToQty(const CurrencyPair& cp, const std::string &str);

/*! current timestamp in nanoseconds */
inline int64_t CurrentTimestamp() { return 0; }

std::string NanosecondsToString(int64_t ns);

int64_t StringToNanoseconds(const std::string &str);

inline double AddDecimalPlaces(int num, int precision)
{
	double denominator = pow(10, precision);
	return static_cast<double>(num) / denominator;
}

template<typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

template <typename ...Args>
std::string Format(const std::string &fmt, Args &&...args)
{
	constexpr size_t argc { sizeof...(Args) };
	if constexpr (argc == 0)
	{
		return fmt;
	}
	else
	{
		static std::mutex mtx;
		std::lock_guard lock { mtx };
		return Poco::format(fmt, std::forward<Args>(args)...);
	}
}

#define TSF_DATETIME    "%F %T%."
#define TSF_DATE    	"%F"
#define TSF_TIME        "%T%."
#define TSF_SETTLDATE  	"%Y%m%d"

std::string FormatTimestamp(const std::string &format, int64_t ts = 0);

int64_t ParseTimestamp(const std::string &format, const std::string &str);

inline std::string FormatTimestamp(int64_t ts = 0) { return FormatTimestamp(TSF_DATETIME, ts); }

inline std::string CharToString(char c) { return std::string(1, c); }

std::string tolower(const std::string &s);

std::string toupper(const std::string &s);

bool iequal(std::string const &a, std::string const &b);

int replaceAll(std::string &str, const std::string &from, const std::string &to);

Poco::Clock::ClockDiff MeasureTime(std::function<void()> action);

/*! \brief Measures time it takes a function to perform
 *
 * Executes the specified function, measures the time it takes
 * to perform, and saves its return value.
 *
 * @param function    Function to be measured
 * @param returnValue    Reference to the variable to store the return value
 * @return Measured time in microseconds
 */
template <typename T>
Poco::Clock::ClockDiff MeasureTime(std::function<T()> function, T &returnValue)
{
	Poco::Stopwatch watch;
	watch.start();
	returnValue = function();
	watch.stop();
	return watch.elapsed();
}

pid_t GetPid(const char *name);

void hexdump(void *ptr, int buflen);

int RandomInt(int minValInclusive, int maxValExclusive);

int RandomInt(int maxValExclusive);

void SleepForMilliseconds(long ms);

void SleepForMicroseconds(long us);

std::string NewUUID();

int64_t NewInt64Key();

/*! \brief Returns the index of a vector element.
 *
 * The template function takes references to a vector and an element
 * of its base type, and returns the 0-based index of the element
 * in the vector.
 *
 * @param vec     reference to the vector to be searched
 * @param element reference to the element to be looked for
 *
 * @return 0-based index of the element in the vector. -1 if the
 * vector does not contain the element.
 */
template <typename T>
ptrdiff_t IndexOf(const std::vector<T> &vec, const T &element)
{
	auto it { std::find(vec.begin(), vec.end(), element) };
	return (it != vec.end() ? std::distance(vec.begin(), it) : -1);
}

/*! \brief Returns a reference to the first element in a given vector
 * fulfilling a condition defined by a given predicate.
 *
 * The template function takes a reference to a vector and a predicate,
 * and iterates over the elements of the vector until it finds an
 * element that fulfils the condition defined by the predicate.
 *
 * @param vec     reference to the vector to be searched
 * @param predicate boolean function to test the elements of the vector
 * @param defaultValue value to be returned if the vector does not contain
 * any element that meets the condition defined by the predicate
 *
 * @return reference to the first element that meets the condition defined
 * by \a predicate.
 */
template <typename T>
const T &FindFirst(const std::vector<T> &vec, std::function<bool(const T &)> predicate, const T &defaultValue = T(0))
{
	for (const T &item : vec)
	{
		if (predicate(item))
		{
			return item;
		}
	}
	return defaultValue;
}

using XmlDocPtr = Poco::AutoPtr<Poco::XML::Document>;

XmlDocPtr GetConfigDoc(const std::string &configPath, std::string *errorMessage = nullptr);

Poco::XML::Node *GetConfigNode(const XmlDocPtr &pDoc, const std::string &tag, std::string *errorMessage = nullptr);

Poco::XML::Node *GetSubNode(Poco::XML::Node *node, const std::string &tag, std::string *errorMessage = nullptr);

bool MeansTrue(const std::string &str);

/*! \brief Converts a string to a value of a given type.
 *
 * The template function takes a string a returns the corresponding value
 * of the template type. This function cannot be called directly, only
 * its instantiated variants.
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return converted value.
 */
template <typename T>
T parse(const std::string &str, T defaultValue = { })
{
	if constexpr (std::is_constructible<T, std::string>::value)
	{
		return T(str);
	}
	else
	{
		throw Poco::Exception(Format("No specialization for type %s", std::string(typeid(T).name())));
	}
}

template <>
std::string parse<std::string>(const std::string &str, std::string defaultValue);

template <>
char *parse<char *>(const std::string &str, char *defaultValue);

template <>
const char *parse<const char *>(const std::string &str, const char *defaultValue);

template <>
bool parse<bool>(const std::string &str, bool defaultValue);

template <>
int parse<int>(const std::string &str, int defaultValue);

template <>
unsigned int parse<unsigned int>(const std::string &str, unsigned int defaultValue);

template <>
long int parse<long int>(const std::string &str, long int defaultValue);

template <>
unsigned long parse<unsigned long>(const std::string &str, unsigned long defaultValue);

template <>
long long parse<long long>(const std::string &str, long long defaultValue);

template <>
unsigned long long parse<unsigned long long>(const std::string &str, unsigned long long defaultValue);

template <>
char parse<char>(const std::string &str, char defaultValue);

template <>
double parse<double>(const std::string &str, double defaultValue);

template <>
long double parse<long double>(const std::string &str, long double defaultValue);

template <typename T>
T GetXmlAttribute(const Poco::XML::Node *node, const std::string &name, T defaultValue)
{
	T result { defaultValue };
	if (node && node->hasAttributes() && !name.empty())
	{
		Poco::XML::NamedNodeMap *attrs { node->attributes() };
		if (attrs)
		{
			Poco::XML::Node *attr { attrs->getNamedItem(name) };
			if (attr)
			{
				result = parse<T>(attr->nodeValue(), defaultValue);
			}
			attrs->release();
		}
	}
	return result;
}

template <typename T>
std::optional<T> GetXmlAttribute(const Poco::XML::Node *node, const std::string &name)
{
	std::optional<T> result { std::nullopt };
	if (node && node->hasAttributes() && !name.empty())
	{
		Poco::XML::NamedNodeMap *attrs { node->attributes() };
		if (attrs)
		{
			Poco::XML::Node *attr { attrs->getNamedItem(name) };
			if (attr)
			{
				result = parse<T>(attr->nodeValue(), {});
			}
			attrs->release();
		}
	}
	return result;
}

/*! \brief Loads xml from a string
 * @param xml valid xml string
 * @return pointer to constructed xml doc
 */
UTILS::XmlDocPtr LoadXml(const std::string& xml);

void ForEachChildElement(const Poco::XML::Node *node, const std::string &name, const std::function<void(const Poco::XML::Node *)> &action);

void ForEachChildElement(const Poco::XML::Node *node, const std::function<void(const Poco::XML::Node *)> &action);

template <class InputIterator, class Predicate, class Action>
void foreach(InputIterator first, InputIterator last, Action action, Predicate pred = nullptr)
{
	while (first != last)
	{
		if (!pred || pred(*first))
		{
			action(*first++);
		}
		else
		{
			first++;
		}
	}
}

template <class T, class Predicate, class Action>
void foreach(const T &container, Action action, Predicate pred = nullptr)
{
	foreach(container.begin(), container.end(), action, pred);
}

template <class Predicate>
bool WaitUntil(Predicate predicate, long timeoutMs, long checkIntervalMs = 1)
{
	Poco::Timestamp timeStamp;
	while (!predicate() && timeStamp.elapsed() / 1000 < timeoutMs)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
	}
	return predicate();
}

/*! \brief Returns the contents of the vector as a string.
 *
 * The template function takes references to a vector and a delimiter string
 * and returns a concatenation of the vector elements as delimited string.
 *
 * @param vec     reference to the vector to be searched
 * @param delimiter delimiter string (default: ", ")
 *
 * @return Vector content as string
 */
template <typename C>
std::string VecToStr(const C &vec, const std::string &delimiter = ", ")
{
	std::ostringstream oss;
	if (!vec.empty())
	{
		for (const auto &item : vec)
		{
			if (!oss.str().empty())
			{
				oss << delimiter;
			}
			oss << item;
		}
	}
	return oss.str();
}

/*! \brief Returns the contents of the vector as a string.
 *
 * The template function takes references to a vector and a delimiter string
 * and returns a concatenation of the vector elements as delimited string.
 *
 * @param vec     reference to the vector to be searched
 * @param selector selector function to convert a single vector element to an output string
 * @param delimiter delimiter string (default: ", ")
 *
 * @return Vector content as string
 */
template <typename C, typename F>
std::string VecToStrWithSelect(const C &vec, F selector, const std::string &delimiter = ", ")
{
	std::ostringstream oss;
	if (!vec.empty())
	{
		for (const auto &item : vec)
		{
			if (!oss.str().empty())
			{
				oss << delimiter;
			}
			oss << selector(item);
		}
	}
	return oss.str();
}

/*! \brief Splits a string and adds the items to a vector.
 *
 * Template function.
 *
 * The function splits a given strings, converts the elements
 * and adds them to a given vector.
 *
 * @param str     string to be split
 * @param vec     reference to the vector to take the items
 * @param delimiter delimiter string (default: ", ")
 *
 * @return Number of items added to the vector
 */
template <typename T>
int64_t SplitString(const std::string &str, std::vector<T> &vec, const std::string &delimiter = ",")
{
	int64_t cnt { 0 };
	Poco::StringTokenizer tokens { str, delimiter, Poco::StringTokenizer::TOK_TRIM };
	for (const std::string &token : tokens)
	{
		++cnt;
		vec.push_back(parse<T>(token, { }));
	}
	return cnt;
}

std::vector<std::string_view> SplitStringView(std::string_view sv, char delimiter = ',');

std::string GetMessage(std::exception_ptr eptr);

template <typename T>
std::optional<T> MakeOptional(const T &val, const T &nullVal = {})
{
	if (val == nullVal)
	{
		return std::nullopt;
	}
	else
	{
		return { val };
	}
}

std::string OrdStatusToString(char orderStatus);

std::string ExecTypeToString(char execType);

/** Pins the current thread to a specific core
 *
 * Taken from: https://bytefreaks.net/programming-2/cc-set-affinity-to-threads-example-code
 *
 * @param coreId Id of the core to which the current thread is to be pinned
 * @param errMsg (out) String reference to receive error message
 * @return @a true if successful
 */
bool SetThreadAffinity(size_t coreId, std::string &errMsg);

/** Pins the current thread to a specific core
 *
 * Taken from: https://bytefreaks.net/programming-2/cc-set-affinity-to-threads-example-code
 *
 * @param coreId Id of the core to which the current thread is to be pinned
 * @return @a true if successful
 */
void SetThreadAffinity(size_t coreId);

/**
 * Defines a function to map thread name to core ID
 * @param func Right-value reference to core mapping function
 */
void SetCoreMappingFunction(std::function<std::optional<int64_t>(const std::string &)> &&func);

/**
 * Uses the core mapping function to determine the core ID, and pins the thread to that core.
 * @param errMsg (out) String reference to receive error message
 * @return Core ID, or -1 if invalid
 */
int64_t SetThreadAffinityFromConfig(std::string &errMsg);

/**
 * Uses the core mapping function to determine the core ID, and pins the thread to that core.
 * @return Core ID, or -1 if invalid
 */
int64_t SetThreadAffinityFromConfig();

/** Sets the priority of the current thread
 *
 * Succeeds only with root privileges!
 *
 * @param priority Priority value to be set (1..lowest, 99..highest)
 * @return @a true if successful
 */
bool SetThreadPriority(int priority, std::string &errMsg);

std::string GetThreadName(pthread_t threadHandle);

inline std::string GetThreadName() { return GetThreadName(pthread_self()); }

void SetThreadName(pthread_t threadHandle, const std::string &name);

inline void SetThreadName(const std::string &name) { SetThreadName(pthread_self(), name); }

bool IsValidSettlDate(const std::string &str);

/**
 * @brief  Returns an underlying type of a type. Usefull when dealing with enum classes
 * Example:
    enum class ETopicTokens
    {
        Instance        = 0,
        TradingType     = 1,
        CurrencyPair    = 2
    };
    GetUnderlyingType(ETopicTokens::TradingType) returns 1
 * @return  underlying type
 */
template <typename T> constexpr auto GetUnderlyingType(const T a)
{
    return static_cast<typename std::underlying_type<T>::type>(a);
}


/**
 * \brief Reads a content of text file into a string
 * @param path - path to the source file
 * @return the string content of the file or "" if file does not exist or empty
 */
std::string ReadFileIntoString(const std::string& path);

/**
 * \brief Writes a string into a text file
 * @param str - string
 * @param path - path to the destination file
 * @return true in success
 */
bool WriteStringIntoFile(const std::string& str, const std::string& path);


/**
 * \brief creates a unique file name and removed the file in destructor
 */
class CScopedFile final {
public:
    CScopedFile(const std::string& fileName) : m_fileName(fileName)
    {
        // Make sure we have unique file
        auto name = fileName;
        for (int i = 0;; ++i)
        {
            // Check file exists (note: better to use !std::filesystem::exists(name) but it's not available on some pipelines)
            if (!std::ifstream(name.c_str()).good())
                break;
            name = std::to_string(i) + "_" + fileName;
        }
        m_fileName = name;
    }
    CScopedFile(const CScopedFile&) = delete;
    CScopedFile& operator=(const CScopedFile&) = delete;

    ~CScopedFile()
    {
        unlink(m_fileName.c_str());
    }

    const std::string& GetFileName() const
    {
        return m_fileName;
    }

private:
    std::string m_fileName;
}; // CScopedFile


/**
 * @brief	Event simulator
 */
class CEvent final {
public:
  CEvent(bool aIsSet = false) : mFlag(aIsSet) {}
  CEvent(const CEvent&) = delete;
  CEvent& operator=(const CEvent&) = delete;

public:
  // Sets event
  void Set()
  {
    Set(true);
  }

  void Set(bool aState)
  {
      mFlag.store(aState);
      mCond.notify_all();
  }

  // Resets event
  void Reset()
  {
    Set(false);
  }

  bool IsSet() const
  {
    return mFlag.load();
  }

  // Waiting for event is set till timeout
  bool Wait(const unsigned long aMilliseconds = std::numeric_limits<unsigned long>::max())
  {
    return WaitFor(aMilliseconds, true);
  }

  // Waiting for event is reset till timeout
  bool WaitForReset(const unsigned long aMilliseconds = std::numeric_limits<unsigned long>::max())
  {
    return WaitFor(aMilliseconds, false);
  }

private:
  // Internal helper
  bool WaitFor(const unsigned long aMilliseconds, bool aWaitForSet)
  {
      std::unique_lock<std::mutex> lock(mMtx);
      mCond.wait_for(
        lock,
        std::chrono::milliseconds(aMilliseconds),
        [&]() { return IsSet() == aWaitForSet; } );
      return IsSet() == aWaitForSet;
  }

private:
  std::mutex mMtx;
  std::atomic<bool> mFlag;
  std::condition_variable mCond;
}; // CEvent


} // ns UTILS
