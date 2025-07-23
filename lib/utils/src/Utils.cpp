//
// Created by jameson 09.02.18.
//

#include <fstream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>

#include <algorithm>

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/Document.h"
#include "Poco/SAX/InputSource.h"

#include "Utils/Utils.h"

#include <regex>

#include "Utils/FixDefs.h"
#include "Utils/CurrencyPair.h"

namespace UTILS {

size_t UseThisCoreNumber = 1;

std::string QtyToString(const CurrencyPair& cp, int64_t qty)
{
	std::string result;
	if (qty == 0)
	{
		result = "0";
	}
	else if (!cp.IsFX())
	{
		result = Poco::format("%.4f", cp.QtyToDouble(qty));
	}
	else if (qty % 100'000 == 0)
	{
		qty /= 100'000;
		if (qty % 1'000'000 == 0)
		{
			qty /= 1'000'000;
			result = std::to_string(qty) + "b";
		}
		else if (qty % 1'000 == 0)
		{
			qty /= 1'000;
			result = std::to_string(qty) + "m";
		}
		else
		{
			result = std::to_string(qty) + "k";
		}
	}
	else if (qty % 100 == 0)
	{
		result = std::to_string(qty / 100);
	}
	else
	{
		result = Poco::format("%.2f", cp.QtyToDouble(qty));
	}
	return result;
}

int64_t StringToQty(const CurrencyPair& cp, const std::string &str)
{
	double dbl { 0.0 };
	if (!str.empty())
	{
		switch (std::tolower(str.back()))
		{
			case 'b': // billion
				dbl = parse<double>(str.substr(0, str.size() - 1), 0.0) * 1'000'000'000.0;
				break;
			case 'm': // million
				dbl = parse<double>(str.substr(0, str.size() - 1), 0.0) * 1'000'000.0;
				break;
			case 'k': // thousand
				dbl = parse<double>(str.substr(0, str.size() - 1), 0.0) * 1'000.0;
				break;
			default:
				dbl = parse<double>(str, 0.0);
				break;
		}
	}
	return cp.DoubleToQty(dbl);
}

std::string NanosecondsToString(int64_t ns)
{
	int64_t unit { 0 };
	std::stringstream ss;
	if (ns == 0)
	{
		ss << "0";
	}
	else
	{
		while (unit < 3 && ns % 1'000 == 0)
		{
			ns /= 1'000;
			++unit;
		}
		while (unit >= 3 && unit < 5 && ns % 60 == 0)
		{
			ns /= 60;
			++unit;
		}
		if (unit == 5 && ns % 24 == 0)
		{
			ns /= 24;
			++unit;
		}
		ss << ns;
		switch (unit)
		{
			case 0:
				ss << "ns";
				break;
			case 1:
				ss << "us";
				break;
			case 2:
				ss << "ms";
				break;
			case 3:
				ss << "s";
				break;
			case 4:
				ss << "m";
				break;
			case 5:
				ss << "h";
				break;
			case 6:
				ss << "d";
				break;
			default: // won't happen
				ss << "?";
				break;
		}
	}
	return ss.str();
}

int64_t StringToNanoseconds(const std::string &str)
{
	size_t len { str.size() };
	int64_t factor { 1 };
	if (len >= 2)
	{
		switch (std::toupper(str.back()))
		{
			case 'S': // [mun]?s
				switch (std::toupper(str[len - 2]))
				{
					case 'N': // ns -> nanoseconds
						len -= 2;
						factor = 1;
						break;
					case 'U': // us -> microseconds
						len -= 2;
						factor = 1'000;
						break;
					case 'M': // ms -> milliseconds
						len -= 2;
						factor = 1'000'000;
						break;
					default: // s -> seconds
						--len;
						factor = 1'000'000'000;
						break;
				}
				break;
			case 'M': // minutes
				--len;
				factor = 60'000'000'000;
				break;
			case 'H': // hours
				--len;
				factor = 3'600'000'000'000;
				break;
			case 'D': // days
				--len;
				factor = 86'400'000'000'000;
				break;
		}
	}
	while (len > 1 && str[len - 1] == ' ') // ignore blanks between number and unit
	{
		--len;
	}
	return (int64_t) (parse<double>(str.substr(0, len), 0.0) * factor);
}

/*! \brief Calculates the value of a pip
 *
 *  Checks whether a currency pair contains JPY and calculates the value of
 *  a pip accordingly - returning int64_t
 *
 *  double value of a pip = 0.0001 * qty / price  for USD
 *  pip = 0.01 * qty / price                      for JPY
 *
 * @param p                                       number of pips
 * @param price                                   exchange rate
 * @param qty                                     notional amount
 * @param instrument                              currency pair
 * */

/*! \brief Creates a lower case copy of a string
 *
 * Takes a string parameter and returns a lower case
 * copy of it.
 *
 * @param s    String to be transformed to lower case
 * @return Lower case version of \a s
 */
std::string tolower(const std::string &s)
{
	std::string result { s };
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

/*! \brief Creates an upper case copy of a string
 *
 * Takes a string parameter and returns an upper case
 * copy of it.
 *
 * @param s    String to be transformed to upper case
 * @return Upper case version of \a s
 */
std::string toupper(const std::string &s)
{
	std::string result { s };
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

bool iequal_pred(unsigned char a, unsigned char b)
{
	return std::tolower(a) == std::tolower(b);
}

/*! \brief Case-insensitive string comparison
 *
 * Compares two strings case-insensitively.
 *
 * @param a,b Strings to compare
 * @return true if the two strings are equal
 */
bool iequal(const std::string &a, const std::string &b)
{
	if (a.length() == b.length())
	{
		return std::equal(b.begin(), b.end(), a.begin(), iequal_pred);
	}
	else
	{
		return false;
	}
}

int replaceAll(std::string &str, const std::string &from, const std::string &to)
{
	int count { 0 };
	size_t start_pos { 0 };
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
		++count;
	}
	return count;
}

/*! \brief Measures time it takes an action to perform
 *
 * Executes the specified action and measures the time it takes
 * to perform.
 *
 * @param action    Action to be measured
 * @return Measured time in microseconds
 */
Poco::Clock::ClockDiff MeasureTime(std::function<void()> action)
{
	Poco::Stopwatch watch;
	watch.start();
	action();
	watch.stop();
	return watch.elapsed();
}

/*! \brief Determines the PID of a running process
 *
 * This functions scans all running processes for the specified command
 * name and return the associated PID.
 *
 * @param name    Name ("comm value") of the process to be found
 * @return  PID of the process, -1 if no process matching the specified
 * name was found
 */
pid_t GetPid(const char *name)
{
	DIR *dir;
	struct dirent *ent;
	char *endptr;
	char buf[512];
	long thisPid { ::getpid() };
	
	if (!(dir = opendir("/proc")))
	{
		perror("can't open /proc");
		return -1;
	}
	
	while ((ent = readdir(dir)) != nullptr)
	{
		/* if endptr is not a null character, the directory is not
		 * entirely numeric, so ignore it */
		long lpid { strtol(ent->d_name, &endptr, 10) };
		if (*endptr != '\0')
		{
			continue;
		}
		
		/* if lpid is the pid of the current process, ignore it */
		if (lpid == thisPid)
		{
			continue;
		}
		
		/* try to open the cmdline file */
		snprintf(buf, sizeof(buf), "/proc/%ld/comm", lpid);
		FILE *fp = fopen(buf, "r");
		
		if (fp)
		{
			if (fgets(buf, sizeof(buf), fp) != nullptr)
			{
				char *pc { strchr(buf, '\n') };
				if (pc)
				{
					*pc = '\0';
				}
				if (!strcmp(buf, name))
				{
					fclose(fp);
					closedir(dir);
					return (pid_t) lpid;
				}
			}
			fclose(fp);
		}
		
	}
	
	closedir(dir);
	return -1;
}

/*! \brief Simple HEX dump routine
 *
 * Source: https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
 *
 * @param ptr    Pointer to the first byte of the memory block to be dumped
 * @param buflen    Number of bytes to be dumped
 */
void hexdump(void *ptr, int buflen)
{
	unsigned char *buf = (unsigned char *) ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16)
	{
		printf("%06x: ", i);
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				printf("%02x ", buf[i + j]);
			}
			else
			{
				printf("   ");
			}
		}
		printf(" ");
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		printf("\n");
	}
}

std::string NewUUID()
{
	std::string uuidStr { "" };
	FILE *fp = fopen("/proc/sys/kernel/random/uuid", "r");
	if (fp)
	{
		const int uuidLen { 36 };
		char buf[uuidLen + 1];
		if (fgets(buf, sizeof(buf), fp) != nullptr)
		{
			buf[uuidLen] = '\0';
			uuidStr = buf;
		}
		fclose(fp);
	}
	return uuidStr;
}

/*! \brief Returns a random integer number
 *
 * The function returns a random 32bit integer number.
 *
 * @param minValInclusive   lower bound (minimal value)
 * @param maxValExclusive   upper bound (maximum value + 1)
 * @return 32bit integer number in the range [minValInclusive,maxValExclusive)
 */
int RandomInt(int minValInclusive, int maxValExclusive)
{
	return (int) (random() % (maxValExclusive - minValInclusive)) + minValInclusive;
}

/*! \brief Returns a random integer number
 *
 * The function returns a random 32bit integer number.
 *
 * @param maxValExclusive   upper bound (maximum value + 1)
 * @return 32bit integer number in the range [0,maxValExclusive)
 */
int RandomInt(int maxValExclusive)
{
	return RandomInt(0, maxValExclusive);
}

void SleepForMilliseconds(long ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void SleepForMicroseconds(long us)
{
	std::this_thread::sleep_for(std::chrono::microseconds(us));
}


/*! \brief Returns a new 64bit integer key.
 *
 * The function returns a 64bit integer key which is guaranteed to be unique
 * and ascending within the running process.
 *
 * @return 64bit integer key
 */
int64_t NewInt64Key()
{
	static int64_t lastKey { 0 };
	static std::mutex mtx;
	std::lock_guard lock { mtx };
	int64_t currentKey { CurrentTimestamp() };
	if (currentKey <= lastKey)
	{
		++lastKey;
	}
	else
	{
		lastKey = currentKey;
	}
	return lastKey;
}

XmlDocPtr GetConfigDoc(const std::string &configPath, std::string *errorMessage)
{
	Poco::AutoPtr<Poco::XML::Document> pDoc = nullptr;
	std::string err;
	try
	{
		std::ifstream in { configPath };
		Poco::XML::InputSource src { in };
		Poco::XML::DOMParser parser;
		pDoc = parser.parse(&src);
	}
	catch (Poco::Exception &e)
	{
		err = e.message();
	}
	if (errorMessage)
	{
		*errorMessage = err;
	}
	return pDoc;
}

Poco::XML::Node *GetSubNode(Poco::XML::Node *node, const std::string &tag, std::string *errorMessage)
{
	Poco::XML::Node *subnd { nullptr };
	std::string err;
	if (node)
	{
		if (node->hasChildNodes())
		{
			Poco::XML::NodeList *children { node->childNodes() };
			if (children)
			{
				Poco::XML::Node *child;
				for (unsigned int i = 0; !subnd && i < children->length(); ++i)
				{
					child = children->item(i);
					if (child->nodeType() == Poco::XML::Node::ELEMENT_NODE && child->localName() == tag)
					{
						subnd = child;
					}
				}
				children->release();
			}
			else
			{
				err = "Error in GetSubNode: Internal error reading child nodes.";
			}
		}
		else
		{
			err = "Error in GetSubNode: No child nodes defined.";
		}
	}
	else
	{
		err = "Error in GetSubNode: Invalid base node";
	}
	if (errorMessage)
	{
		*errorMessage = err;
	}
	return subnd;
}

Poco::XML::Node *GetConfigNode(const XmlDocPtr &pDoc, const std::string &tag, std::string *errorMessage)
{
	Poco::XML::Node *cfgNode { nullptr };
	std::string err;
	if (pDoc)
	{
		Poco::XML::Node *pRoot { pDoc->firstChild() };
		while (pRoot && pRoot->nodeType() != Poco::XML::Node::ELEMENT_NODE)
		{
			pRoot = pRoot->nextSibling();
		}
		if (pRoot)
		{
			if (pRoot->localName() == tag)
			{
				cfgNode = pRoot;
			}
			else if (pRoot->localName() == TAG_CONFIGURATION)
			{
				cfgNode = GetSubNode(pRoot, tag, &err);
			}
			else
			{
				err = Poco::format("Invalid root node: %s", pRoot->localName());
			}
		}
		else
		{
			err = "Missing root node";
		}
	}
	else
	{
		err = "Error in GetConfigNode: Invalid document node";
	}
	if (errorMessage)
	{
		*errorMessage = err;
	}
	return cfgNode;
}

UTILS::XmlDocPtr LoadXml(const std::string& xml)
{
    std::stringstream ss(xml);
    Poco::XML::InputSource src { ss };
    Poco::XML::DOMParser parser;
    return parser.parse(&src);
}

void ForEachChildElement(const Poco::XML::Node *node, const std::string &name, const std::function<void(const Poco::XML::Node *)> &action)
{
	if (nullptr != node && node->hasChildNodes())
	{
		Poco::XML::NodeList *children { node->childNodes() };
		if (children)
		{
			Poco::XML::Node *child;
			for (unsigned int i = 0; i < children->length(); ++i)
			{
				child = children->item(i);
				if (child && child->nodeType() == Poco::XML::Node::ELEMENT_NODE && (name.empty() || name == child->localName()))
				{
					action(child);
				}
			}
			children->release();
		}
	}
}

void ForEachChildElement(const Poco::XML::Node *node, const std::function<void(const Poco::XML::Node *)> &action)
{
	return ForEachChildElement(node, "", action);
}

/*! \brief Checks if a given string is probably a string representation
 * of boolean value \a true.
 *
 * The function compares the given string to some common string
 * representations of boolean value \a true.
 *
 * @param str    String representing a boolean value
 * @return true if \a str is probably a representation of boolean
 * value \a true
 */
bool MeansTrue(const std::string &str)
{
	char c { !str.empty() ? (char) ::tolower(str[0]) : '\0' };
	return (c == 't' || c == 'y' || c == '1');
}

template <typename T, typename F>
T convert(const std::string &str, const std::regex &re, F func, T defaultValue)
{
	T result = { defaultValue };
	if (std::regex_match(str, re))
	{
		try
		{
			result = func(str);
		}
		catch (...)
		{
		}
	}
	return result;
}

/*! \brief Converts a string to string (returns a copy).
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return copy of the passed string.
 */
template <>
std::string parse<std::string>(const std::string &str, std::string defaultValue)
{
	return str;
}

/*! \brief Returns a string as a char array.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return char* representation of the passed string.
 */
template <>
const char *parse<const char *>(const std::string &str, const char *defaultValue)
{
	return str.c_str();
}

/*! \brief Returns a string as a boolean value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return boolean representation of the passed string.
 */
template <>
bool parse<bool>(const std::string &str, bool defaultValue)
{
	return MeansTrue(str);
}

/*! \brief Returns a string as an integer value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return integer representation of the passed string.
 */
template <>
int parse<int>(const std::string &str, int defaultValue)
{
	static const std::regex re("^[\\+-]?\\d+");
	return convert<int>(str, re, [](const std::string &s) { return std::stoi(s); }, defaultValue);
}

/*! \brief Returns a string as an unsigned integer value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return unsigned integer representation of the passed string.
 */
template <>
unsigned int parse<unsigned int>(const std::string &str, unsigned int defaultValue)
{
	static const std::regex re("\\d+");
	return convert<unsigned int>(str, re, [](const std::string &s) { return (unsigned int) std::stoul(s); }, defaultValue);
}

/*! \brief Returns a string as a long integer value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return long integer representation of the passed string.
 */
template <>
long int parse<long int>(const std::string &str, long int defaultValue)
{
	static const std::regex re("^[\\+-]?\\d+");
	return convert<long int>(str, re, [](const std::string &s) { return std::stol(s); }, defaultValue);
}

/*! \brief Returns a string as an unsigned long value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return unsigned long representation of the passed string.
 */
template <>
unsigned long parse<unsigned long>(const std::string &str, unsigned long defaultValue)
{
	static const std::regex re("^\\d+");
	return convert<unsigned long>(str, re, [](const std::string &s) { return std::stoul(s); }, defaultValue);
}

/*! \brief Returns a string as a long long value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return long long representation of the passed string.
 */
template <>
long long parse<long long>(const std::string &str, long long defaultValue)
{
	static const std::regex re("^[\\+-]?\\d+");
	return convert<long long>(str, re, [](const std::string &s) { return std::stoll(s); }, defaultValue);
}

/*! \brief Returns a string as a unsigned long long value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return unsigned long long representation of the passed string.
 */
template <>
unsigned long long parse<unsigned long long>(const std::string &str, unsigned long long defaultValue)
{
	static const std::regex re("^\\d+");
	return convert<unsigned long long int>(str, re, [](const std::string &s) { return std::stoull(s); }, defaultValue);
}

/*! \brief Returns a string as a char value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return first character of the passed string.
 */
template <>
char parse<char>(const std::string &str, char defaultValue)
{
	static const std::regex re(".{1,1}");
	return convert<char>(str, re, [](const std::string &s) { return s[0]; }, defaultValue);
}

/*! \brief Returns a string as a double value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return double representation of the passed string.
 */
template <>
double parse<double>(const std::string &str, double defaultValue)
{
	static const std::regex re("^[\\+-]?\\d+(\\.\\d+)?");
	return convert<double>(str, re, [](const std::string &s) { return std::stod(s); }, defaultValue);
}

/*! \brief Returns a string as a long double value.
 *
 * Instantiated variant of template function parse<T>().
 *
 * @param str string to be converted
 * @param defaultValue  value to be returned if the string cannot be converted
 *
 * @return long double representation of the passed string.
 */
template <>
long double parse<long double>(const std::string &str, long double defaultValue)
{
	static const std::regex re("^[\\+-]?\\d+(\\.\\d+)?");
	return convert<long double>(str, re, [](const std::string &s) { return std::stold(s); }, defaultValue);
}

std::string FormatTimestamp(const std::string &format, int64_t ts)
{
	std::string fmt { format };
	if (fmt.empty())
	{
		fmt = TSF_DATETIME;
	}
	if (ts <= 0)
	{
		ts = CurrentTimestamp();
	}
	// "%." -> show 0, 3, 6, or 9 post-comma digits (dependent on precision of timestamp)
	if (fmt.find("%.") != std::string::npos)
	{
		int64_t n { ts % 1000000000 };
		std::string str;
		if (n % 1000 == 0)
		{ // precision not greater than microseconds
			n /= 1000;
			if (n % 1000 == 0)
			{ // precision not greater than milliseconds
				n /= 1000;
				if (n % 1000 == 0)
				{  // precision not greater than seconds
					// do not add anything
				}
				else
				{ // add point and milliseconds
					str = Poco::format(".%03Ld", n);
				}
			}
			else
			{ // add point and microseconds
				str = Poco::format(".%06Ld", n);
			}
		}
		else
		{ // add point and nanoseconds
			str = Poco::format(".%09Ld", n);
		}
		replaceAll(fmt, "%.", str);
	}
	// "%N" -> always show 9 post-comma digits
	replaceAll(fmt, "%N", Poco::format("%09Ld", ts % 1000000000));
	//FIX8::Tickval tv { ts };
	const size_t bufSize { 256 };
	char buf[bufSize]={};
	struct tm tm { };
	//tv.as_tm(tm);
	std::strftime(buf, bufSize, fmt.c_str(), &tm);
	return std::string(buf);
}

int64_t ParseTimestamp(const std::string &format, const std::string &str)
{
	int64_t result { 0 };
	if (!str.empty())
	{
		std::string fmt { format };
		if (fmt.empty())
		{
			fmt = TSF_DATETIME;
		}
		size_t ix { fmt.find("%.") };
		bool withNanoseconds { false };
		if (ix != std::string::npos)
		{
			fmt = fmt.substr(0, ix);
			withNanoseconds = true;
		}
		struct tm tm { };
		char *endp { strptime(str.c_str(), fmt.c_str(), &tm) };
		if (endp)
		{
			long nsecs { 0 };
			if (withNanoseconds && *endp == '.')
			{
				std::string nanoStr { endp + 1 };
				size_t len { nanoStr.length() };
				if (len != 9)
				{
					nanoStr = (len > 9 ? nanoStr.substr(0, 9) : nanoStr + std::string(9 - len, '0'));
				}
				nsecs = parse<long>(nanoStr, 0);
			}
			// FIX8::Tickval tv(tm, nsecs);
			// result = tv;
			result = nsecs;
		}
	}
	return result;
}

std::vector<std::string_view> SplitStringView(std::string_view sv, char delimiter)
{
	std::vector<std::string_view> result;
	
	size_t next { 0 };
	for (size_t current { 0 }; current < sv.size(); current = next + 1)
	{
		next = sv.find(delimiter, current);
		
		if (current != next)
		{
			result.emplace_back(sv.substr(current, next - current));
		}
		
		if (next == std::string_view::npos)
		{
			break;
		}
	}
	
	return result;
}

std::string GetMessage(std::exception_ptr eptr)
{
	try
	{
		if (eptr)
		{
			std::rethrow_exception(eptr);
		}
		else
		{
			return "(no exception)";
		}
	}
	catch (const Poco::Exception &e)
	{
		return Poco::format("%s (%s)", e.displayText(), std::string(typeid(e).name()));
	}
	catch (const std::system_error &e)
	{
		return Poco::format("%s (std::system_error)", e.code().message());
	}
	catch (const std::exception &e)
	{
		return Poco::format("%s (%s)", std::string(e.what()), std::string(typeid(e).name()));
	}
	catch (const std::string &s)
	{
		return Poco::format("%s (std::string)", s);
	}
	catch (...)
	{
		return "(unknown exception type)";
	}
}

std::string OrdStatusToString(char orderStatus)
{
	std::string result;
	switch (orderStatus)
	{
		case ORDSTATUS_NOTSENT:
			result = "NOT_SENT";
			break;
		case ORDSTATUS_SENT:
			result = "SENT";
			break;
		case ORDSTATUS_TIMED_OUT:
			result = "TIMED_OUT";
			break;
		case ORDSTATUS_NEW:
			result = "NEW";
			break;
		case ORDSTATUS_PARTIALLY_FILLED:
			result = "PARTIALLY_FILLED";
			break;
		case ORDSTATUS_FILLED:
			result = "FILLED";
			break;
		case ORDSTATUS_DONE_FOR_DAY:
			result = "DONE_FOR_DAY";
			break;
		case ORDSTATUS_CANCELED:
			result = "CANCELED";
			break;
		case ORDSTATUS_REPLACED:
			result = "REPLACED";
			break;
		case ORDSTATUS_PENDING_CANCEL:
			result = "PENDING_CANCEL";
			break;
		case ORDSTATUS_STOPPED:
			result = "STOPPED";
			break;
		case ORDSTATUS_REJECTED:
			result = "REJECTED";
			break;
		case ORDSTATUS_SUSPENDED:
			result = "SUSPENDED";
			break;
		case ORDSTATUS_PENDING_NEW:
			result = "PENDING_NEW";
			break;
		case ORDSTATUS_CALCULATED:
			result = "CALCULATED";
			break;
		case ORDSTATUS_EXPIRED:
			result = "EXPIRED";
			break;
		case ORDSTATUS_ACCEPTED_FOR_BIDDING:
			result = "ACCEPTED_FOR_BIDDING";
			break;
		case ORDSTATUS_PENDING_REPLACE:
			result = "PENDING_REPLACE";
			break;
		default:
			result = UTILS::Format("(%c)", orderStatus);
			break;
	}
	return result;
}

std::string ExecTypeToString(char execType)
{
	std::string result;
	switch (execType)
	{
		case EXECTYPE_NONE:
			result = "NONE";
			break;
		case EXECTYPE_CANCEL_REJECT:
			result = "CANCEL_REJECT";
			break;
		case EXECTYPE_TIMED_OUT:
			result = "TIMED_OUT";
			break;
		case EXECTYPE_PARTIAL_FILL:
			result = "PARTIAL_FILL";
			break;
		case EXECTYPE_FILL:
			result = "FILL";
			break;
		case EXECTYPE_REPLACE:
			result = "REPLACE";
			break;
		case EXECTYPE_TRADE:
			result = "TRADE";
			break;
		case EXECTYPE_TRADE_CANCEL:
			result = "TRADE_CANCEL";
			break;
		case EXECTYPE_TRADE_CORRECT:
			result = "TRADE_CORRECT";
			break;
		case EXECTYPE_ORDER_STATUS:
			result = "ORDER_STATUS";
			break;
		default:
			result = OrdStatusToString(execType);
			break;
	}
	return result;
}

bool SetThreadAffinity(size_t coreId, std::string &errMsg)
{
	bool result { false };
	const pthread_t pid = pthread_self();
	// cpu_set_t: This data set is a bitset where each bit represents a CPU.
	cpu_set_t cpuset;
	// CPU_ZERO: This macro initializes the CPU set set to be the empty set.
	CPU_ZERO(&cpuset);
	// CPU_SET: This macro adds cpu to the CPU set set.
	CPU_SET(coreId, &cpuset);
	
	// pthread_setaffinity_np: The pthread_setaffinity_np() function sets the CPU affinity mask of the thread thread to the CPU set pointed to
	// by cpuset. If the call is successful, and the thread is not currently running on one of the CPUs in cpuset, then it is migrated to one of
	// those CPUs.
	if (pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuset) != 0)
	{
		errMsg = "pthread_setaffinity_np() failed";
	}
	else
	{
		// Check what is the actual affinity mask that was assigned to the thread.
		// pthread_getaffinity_np: The pthread_getaffinity_np() function returns the CPU affinity mask of the thread thread in the buffer
		// pointed to by cpuset.
		if (pthread_getaffinity_np(pid, sizeof(cpu_set_t), &cpuset) == 0 && CPU_ISSET(coreId, &cpuset))
		{
			result = true;
		}
		else
		{
			errMsg = "pthread_getaffinity_np() failed";
		}
	}
	
	return result;
}

void SetThreadAffinity(size_t coreId)
{
	std::string errMsg;
	SetThreadAffinity(coreId, errMsg);
}

std::function<std::optional<int64_t>(const std::string &)> coreMappingFunction; // global core mapping function

void SetCoreMappingFunction(std::function<std::optional<int64_t>(const std::string &)> &&func)
{
	coreMappingFunction = std::move(func);
}

int64_t SetThreadAffinityFromConfig(std::string &errMsg)
{
	int64_t result { -1 };
	std::string threadName { GetThreadName() };
	if (coreMappingFunction)
	{
		int64_t coreId { coreMappingFunction(threadName).value_or(-1) };
		if (coreId > 0 && coreId <= std::thread::hardware_concurrency())
		{
			if (SetThreadAffinity(coreId, errMsg))
			{
				result = coreId;
			}
		}
		else if (coreId != -1)
		{
			errMsg = UTILS::Format("Invalid core id mapped to thread \"%s\": %Ld", threadName, coreId);
		}
		else
		{
			errMsg = Format("No core id mapped to thread \"%s\"", threadName);
		}
	}
	else
	{
		errMsg = "Core mapping function not defined";
	}
	return result;
}

int64_t SetThreadAffinityFromConfig()
{
	std::string errMsg;
	return SetThreadAffinityFromConfig(errMsg);
}

bool SetThreadPriority(int priority, std::string &errMsg)
{
	bool result { false };
	const pthread_t pid = pthread_self();
	
	struct sched_param params;
	
	params.sched_priority = priority;
	
	if (pthread_setschedparam(pid, SCHED_FIFO, &params) == 0)
	{
		result = true;
	}
	else
	{
		errMsg = "pthread_setschedparam() failed";
	}
	
	return result;
}

std::string GetThreadName(pthread_t threadHandle)
{
	const static size_t bufLength { 16 };
	char buf[bufLength];
	pthread_getname_np(threadHandle, buf, bufLength);
	return std::string(buf);
}

void SetThreadName(pthread_t threadHandle, const std::string &name)
{
/*
	const auto processor_count = std::thread::hardware_concurrency();
	if (UseThisCoreNumber < 95 && UseThisCoreNumber < processor_count)
	{
		const pthread_t pid = pthread_self();
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(UseThisCoreNumber, &cpuset);
		pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
		UseThisCoreNumber = UseThisCoreNumber + 2;
	}
*/
	const static size_t maxLength { 15 };
	if (name.size() > maxLength)
	{
		pthread_setname_np(threadHandle, name.substr(0, maxLength).c_str());
	}
	else
	{
		pthread_setname_np(threadHandle, name.c_str());
	}
}

bool IsValidSettlDate(const std::string &str)
{
	static const std::regex re("^20[1-9][0-9][01][0-9][0-3][0-9]$");
	return std::regex_match(str, re);
}


/**
 * \brief Reads a content of text file into a string
 * @param path - path to the source file
 * @return the string content of the file or "" if file does not exist or empty
 */
std::string ReadFileIntoString(const std::string& path)
{
    std::ifstream input_file(path);
    return input_file.is_open()
        ? std::string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>())
        : "";
}

/**
 * \brief Writes a string into a text file
 * @param str - string
 * @param path - path to the destination file
 * @return true in success
 */
bool WriteStringIntoFile(const std::string& str, const std::string& path)
{
    std::ofstream output_file(path);
    if (!output_file.is_open())
    {
        return false;
    }
    output_file << str;
    return output_file.good();
}

} // UTILS

