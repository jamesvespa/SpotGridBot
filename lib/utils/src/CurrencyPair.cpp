//
// Created by jameson 15.05.18.
//

#include <sstream>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Document.h>
#include <Poco/String.h>

#include "Utils/CurrencyPair.h"

#include <iostream>

#include "Utils/CurrentRateManager.h"


namespace UTILS {

////////////////////////////////////////////////////////////////////////////////
// Currency
////////////////////////////////////////////////////////////////////////////////


Currency::ValToStrMapping::ValToStrMapping()
{
	/*! static map for mapping currency values to their string representations */
	m_valToStr = {{ NONE,    "" },
				   { USD,     "USD" },
				   { EUR,     "EUR" },
				   { GBP,     "GBP" },
				   { CHF,     "CHF" },
				   { JPY,     "JPY" },
				   { AUD,     "AUD" },
				   { CAD,     "CAD" },
				   { CNY,     "CNY" },
				   { SEK,     "SEK" },
				   { NZD,     "NZD" },
				   { MXN,     "MXN" },
				   { SGD,     "SGD" },
				   { HKD,     "HKD" },
				   { NOK,     "NOK" },
				   { KRW,     "KRW" },
				   { TRY,     "TRY" },
				   { RUB,     "RUB" },
				   { INR,     "INR" },
				   { BRL,     "BRL" },
				   { ZAR,     "ZAR" },
				   { DKK,     "DKK" },
				   { ILS,     "ILS" },
				   { PLN,     "PLN" },
				   { CZK,     "CZK" },
				   { NGN,     "NGN" },
				   { HUF,     "HUF" },
				   { CNH,     "CNH" },
				   { GHS,     "GHS" },
				   { KES,     "KES" },
				   { RON,     "RON" },
				   { BGN,     "BGN" },
				   { TWD,     "TWD" },
				   { ZMW,     "ZMW" },
				   { XAG,     "XAG" },
				   { XAU,     "XAU" },
				   { XPD,     "XPD" },
				   { XPT,     "XPT" },
				   { XAF,     "XAF" },
				   { SAR,     "SAR" },
				   { KWD,     "KWD" },
				   { QAR,     "QAR" },
				   { OMR,     "OMR" },
				   { BHD,     "BHD" },
				   { MYR,     "MYR" },
				   { THB,     "THB" },
				   { PKR,     "PKR" },
				   { MAD,     "MAD" },
				   { BGD,     "BGD" },
				   { AED,     "AED" },
				   { IDR,     "IDR" },
				   { EGP,     "EGP" },
				   { IQD,     "IQD" },
				   { UAH,     "UAH" },
				   { VND,     "VND" },
				   { JOD,     "JOD" },
				   { XOF,     "XOF" },
				   { LKR,     "LKR" },
				   { TND,     "TND" },
				   { UGX,     "UGX" },
				   { UZS,     "UZS" },
				   { DZD,     "DZD" },
				   { IRR,     "IRR" },
				   { DOP,     "DOP" },
				   { ISK,     "ISK" },
				   { HRK,     "HRK" },
				   { RSD,     "RSD" },
				   { PHP,     "PHP" },
				   { COP,     "COP" },
				   { ARS,     "ARS" },
				   { XPD,     "XPD" },
				   { XPT,     "XPT" },
			//---
				   { INVALID, INVALID_CURRENCY }};
	
	/*! static map for mapping string representations of currencies to currency values */
	for (const auto &iter: m_valToStr)
	{
		m_strToVal.emplace(iter.second, iter.first);
	}
}

Currency::ValToStrMapping Currency::m_valToStrMapping;
CurrencyPair::CpToStrMapping CurrencyPair::m_cpToStrMapping;

/*! \brief Converts a string to a currency value
 *
 * Static function.
 *
 * @param str String representation of the currency (e.g. "EUR", "USD")
 * @return Currency value (\a Currency::NONE if \a str was empty,
 * \a Currency::INVALID if \a str could not be mapped to a currency value)
 * */
//static
Currency::Value Currency::StrToVal(std::string_view sv)
{
	const auto it { m_valToStrMapping.m_strToVal.find(std::string(sv)) };
	if (it != m_valToStrMapping.m_strToVal.cend())
	{
		return it->second;
	}
	else
	{
		return sv.empty() ? NONE : INVALID;
	}
}


/*! \brief Registers a new currency. The currency value and symbol must be unique
* Currency should not create a conflict when creating pairs with other currencies
* @param value - unique currency id (use GetNextCurrencyValue() to ensure you have unique one)
* @param symbol - unique currency symbol
* @return - returns true in success.
*/
// static
BoolResult Currency::RegisterCurrency(const Value value, const std::string &symbol)
{
	if (m_valToStrMapping.m_valToStr.find(value) != m_valToStrMapping.m_valToStr.cend())
	{
		return BoolResult(false, "Currency with id %s already exists", std::to_string(value));
	}
	
	if (m_valToStrMapping.m_strToVal.find(symbol) != m_valToStrMapping.m_strToVal.cend())
	{
		return BoolResult(false, "Currency with symbol '%s' already exists", symbol);
	}
	
	m_valToStrMapping.m_valToStr.emplace(value, symbol);
	m_valToStrMapping.m_strToVal.emplace(symbol, value);
	
	const auto res = CheckRegisteredCurrencies();
	if (!res)
	{
		// Remove conflicting entry
		m_valToStrMapping.m_valToStr.erase(value);
		m_valToStrMapping.m_strToVal.erase(symbol);
	}
	
	return res;
}


/*! \brief Returns false if there is a conflict in existing currencies */
// static
BoolResult Currency::CheckRegisteredCurrencies()
{
	// Checking that all possible combinations of currency pairs are valid
	for (const auto &iter1: m_valToStrMapping.m_strToVal)
	{
		for (const auto &iter2: m_valToStrMapping.m_strToVal)
		{
			if (Currency(iter1.second).Valid() && Currency(iter2.second).Valid() && iter1.second != iter2.second)
			{
				CurrencyPair cp(iter1.first + iter2.first);
				if (cp.Invalid()) // this would happen very unlikely
				{
					return BoolResult(false, "Invalid currency pair '%s'", iter1.first + "/" + iter2.first);
				}
				
				if (cp.BaseCCY() != Currency(iter1.first) || cp.QuoteCCY() != Currency(iter2.first))
				{
					return BoolResult(false, "Currency pair '%s' conflicts with another pair '%s'", iter1.first + "/" + iter2.first, cp.ToString());
				}
			}
		}
	}
	
	return true;
}


/*! \brief Loads currencies from config file
* @param path: path to config file with currencies
* @Config File Format:
    <Currencies>
        <Currency name="BNB" id="1001" desc="Crypto currency">
        <Currency name="USDT" id = "1002">
        ...
    </Currencies>
* */
// static
BoolResult Currency::InitializeCurrenciesFromFile(const std::string &path)
{
	std::string errMsg;
	auto doc = UTILS::GetConfigDoc(path, &errMsg);
	return (!doc || !errMsg.empty()) ? BoolResult(false,
												  UTILS::Format("Failed to load the currency config from '%s': the file does not exist or invalid",
																path)) : InitializeCurrenciesFromXml(doc);
}

/*! \brief Loads currencies from "CurrencyConfig.xml" file and traces output in cout
* @return: true in success
* */
BoolResult Currency::InitializeCurrencies(std::string cfgFile)
{
	static bool init { false };
	if (init)
	{
		return BoolResult(init);
	}
	
	const auto res = Currency::InitializeCurrenciesFromFile(cfgFile);
	if (!res)
	{
		std::cout << "!!! Errors found when initializing currencies from '" << cfgFile << "':" << std::endl << res.ErrorMessage() << std::endl;
	}
	else
	{
		std::cout << "Currencies have been loaded successfully from '" << cfgFile << "'" << std::endl;
		init = true;
	}
	
	return res;
}

//static
void CurrencyPair::InitializeCurrencyConfigs(const std::string &cfgFile)
{
	Currency::InitializeCurrencies(cfgFile);
	InitializeCurrencyPairs(cfgFile);
}

/*! \brief Loads currencies from "CurrencyConfig.xml" file and traces output in cout
* @return: true in success
* */
BoolResult CurrencyPair::InitializeCurrencyPairs(const std::string &cfgFile)
{
	static bool init { false };
	if (init)
	{
		return BoolResult(init);
	}
	
	const auto res = CurrencyPair::InitializeCurrencyPairsFromFile(cfgFile);
	if (!res)
	{
		std::cout << "!!! Errors found when initializing currency pairs from '" << cfgFile << "':" << std::endl << res.ErrorMessage() << std::endl;
	}
	else
	{
		std::cout << "Currencies have been loaded successfully from '" << cfgFile << "'" << std::endl;
		init = true;
	}
	
	return res;
}

BoolResult CurrencyPair::InitializeCurrencyPairsFromFile(const std::string &path)
{
	std::string errMsg;
	auto doc = UTILS::GetConfigDoc(path, &errMsg);
	return (!doc || !errMsg.empty()) ? BoolResult(false,
												  UTILS::Format("Failed to load the currency config from '%s': the file does not exist or invalid",
																path)) : InitializeCurrencyPairsFromXml(doc);
}

namespace {
const std::string NODE_CONFIG = "CurrencyConfig";
const std::string NODE_CURRENCIES = "Currencies";
const std::string NODE_CURRENCY = "Currency";
const std::string NODE_PIPFACTORS = "PipFactors";
const std::string NODE_CURRENCYPAIR = "CurrencyPair";
const std::string ATTR_NAME = "name";
const std::string ATTR_BASE = "base";
const std::string ATTR_QUOTE = "quote";
const std::string ATTR_ID = "id";
const std::string ATTR_DESC = "desc";
const std::string ATTR_PRECISION = "precision";
const std::string ATTR_PIP = "pip";
} // anon ns


/*! \brief Loads currencies from xml document
* @param config: xml doc
* */
// static
BoolResult Currency::InitializeCurrenciesFromXml(UTILS::XmlDocPtr config)
{
	if (!config)
	{
		return BoolResult(false, "NULL config");
	}
	
	std::string errMsg;
	auto node = GetConfigNode(config, NODE_CONFIG, &errMsg);
	if (!node)
	{
		return BoolResult(false, errMsg);
	}
	
	const auto addOutputError = [&errMsg](const std::string &err)
	{
		errMsg += (errMsg.empty() ? "" : "\n") + err;
	};
	
	if (auto *items { node->childNodes() }; items)
	{
		for (long unsigned int i = 0; i < items->length(); ++i)
		{
			Poco::XML::Node *currencyNode = items->item(i);
			
			if (currencyNode->nodeName() == NODE_CURRENCIES && currencyNode->hasChildNodes())
			{
				if (Poco::XML::NodeList *currencies { currencyNode->childNodes() }; currencies)
				{
					for (long unsigned int currency = 0; currency < currencies->length(); ++currency)
					{
						auto *valueNode = currencies->item(currency);
						if (valueNode->nodeType() == Poco::XML::Node::ELEMENT_NODE)
						{
							if (valueNode->nodeName() == NODE_CURRENCY)
							{
								const auto name = Poco::trim(UTILS::toupper(GetXmlAttribute(valueNode, ATTR_NAME, "")));
								const Value id = static_cast<Value>(GetXmlAttribute(valueNode, ATTR_ID, 0));
								const std::string idStr = GetXmlAttribute(valueNode, ATTR_ID, "");
								const auto desc = Poco::trim(std::string(GetXmlAttribute(valueNode, ATTR_DESC, "")));
								
								const auto currencySignature = "'" + name + "/" + idStr + (desc.empty() ? "" : ("/" + desc)) + "'";
								
								if (name.empty())
								{
									addOutputError("Empty currency name in " + currencySignature + " - ignored");
									continue;
								}
								
								if (id <= 0 || id >= Currency::INVALID)
								{
									std::stringstream ss;
									ss << "Invalid currency id in " + currencySignature;
									ss << ": it must be > 0 and < " << Currency::INVALID << " - ignored";
									addOutputError(ss.str());
									continue;
								}
								
								// Try to add currency
								const auto res = RegisterCurrency(id, name);
								if (!res)
								{
									addOutputError(res.ErrorMessage());
									continue;
								}
							} // if NODE_CURRENCY
						}
					}
					currencies->release();
				}
			}
		}
		items->release();
	}
	
	return errMsg.empty() ? true : BoolResult(false, errMsg);
}


BoolResult CurrencyPair::InitializeCurrencyPairsFromXml(UTILS::XmlDocPtr config)
{
	if (!config)
	{
		return BoolResult(false, "NULL config");
	}
	
	std::string errMsg;
	auto node = GetConfigNode(config, NODE_CONFIG, &errMsg);
	if (!node)
	{
		return BoolResult(false, errMsg);
	}
	
	const auto addOutputError = [&errMsg](const std::string &err)
	{
		errMsg += (errMsg.empty() ? "" : "\n") + err;
	};
	
	if (auto *items { node->childNodes() }; items)
	{
		for (long unsigned int i = 0; i < items->length(); ++i)
		{
			Poco::XML::Node *currencyNode = items->item(i);
			
			if (currencyNode->nodeName() == NODE_PIPFACTORS && currencyNode->hasChildNodes())
			{
				const auto defaultPrecision = GetXmlAttribute(currencyNode, ATTR_PRECISION, CurrencyDefaultPrecision);
				const auto defaultPip = GetXmlAttribute(currencyNode, ATTR_PIP, CurrencyDefaultPipFactor);
				
				if (Poco::XML::NodeList *currencies { currencyNode->childNodes() }; currencies)
				{
					for (long unsigned int currency = 0; currency < currencies->length(); ++currency)
					{
						auto *valueNode = currencies->item(currency);
						if (valueNode->nodeType() == Poco::XML::Node::ELEMENT_NODE)
						{
							if (valueNode->nodeName() == NODE_CURRENCYPAIR)
							{
								const auto base = Currency(Poco::trim(UTILS::toupper(GetXmlAttribute(valueNode, ATTR_BASE, ""))));
								const auto quote = Currency(Poco::trim(UTILS::toupper(GetXmlAttribute(valueNode, ATTR_QUOTE, ""))));
								
								if (!base.Valid() || !quote.Valid())
								{
									addOutputError("Invalid base or quote currency");
									continue;
								}
								
								auto cp { CurrencyPair(base, quote) };
								
								auto pip = GetXmlAttribute(valueNode, ATTR_PIP, defaultPip);
								const auto precision = GetXmlAttribute(valueNode, ATTR_PRECISION, defaultPrecision);
								
								if (pip <= 0)
								{
									pip = 1; // to avoid division by zero due to bad configuration
								}
								
								// Precisions
								m_cpToStrMapping.m_cpToPrecision[cp] = std::make_pair(precision, pip);
							} // if NODE_CURRENCY
						}
					} // for
					currencies->release();
				}
			}
		}
		items->release();
	} // if
	
	return errMsg.empty() ? true : BoolResult(false, errMsg);
}


////////////////////////////////////////////////////////////////////////////////
// CurrencyPair
////////////////////////////////////////////////////////////////////////////////


// static
SharedLockable<std::map<CurrencyPair, std::unique_ptr<std::string>>> CurrencyPair::m_nameCache;

/*! \brief Convert currency pair to string.
 *
 * @return String representation of the currency pair
 * */
const std::string &CurrencyPair::ToString() const
{
	static const std::string emptyString { "" };
	if (Empty())
	{
		return emptyString;
	}
	else
	{
		std::shared_lock slock { m_nameCache.Mutex() };
		const auto &it { m_nameCache->find(*this) };
		if (it != m_nameCache->end())
		{
			return *it->second;
		}
		else
		{
			slock.unlock();
			std::stringstream ss;
			ss << m_baseCCY.ToStringView() << "/" << m_quoteCCY.ToStringView();
			std::unique_lock ulock { m_nameCache.Mutex() };
			return *m_nameCache->emplace(*this, std::make_unique<std::string>(ss.str())).first->second;
		}
	}
}

bool CurrencyPair::ValidMask(std::string_view mask)
{
	switch (mask.length())
	{
		case 1: // "*"
			return mask[0] == '*';
		case 3: // either "*/*" or single ccy (e.g. "EUR")
			return mask == "*/*" || Currency::Valid(mask);
		case 5: // e.g. "*/JPY", "EUR/*"
			if (mask[1] == '/')
			{
				return mask[0] == '*' && Currency::Valid(mask.substr(2, 3));
			}
			else if (mask[3] == '/')
			{
				return mask[4] == '*' && Currency::Valid(mask.substr(0, 3));
			}
			else
			{
				return false;
			}
		case 6: // e.g. "EURUSD"
			return Currency::Valid(mask.substr(0, 3)) && Currency::Valid(mask.substr(3, 3));
		case 7: // e.g. "EUR/USD"
			return mask[3] == '/' && Currency::Valid(mask.substr(0, 3)) && Currency::Valid(mask.substr(4, 3));
		default:
			return false;
	}
}

/*! \brief Checks whether the currency pair matches a given mask
 *
 * Checks if a given instrument matches a given mask.
 *
 * @param mask pattern constraining the set of currency pairs to be matched.
 * \a mask can take the following forms:
 * \a * or \a * / * ... matches any instrument,
 * \a EUR ... matches all instruments containing \a EUR,
 * \a EUR/ * ... matches all instruments with \a EUR in first position,
 * \a * /EUR ... matches all instruments with \a EUR in second position,
 * \a EUR/USD ... matches only \a EUR/USD
 * @return \a true if the currency pair matches \a mask
 * */
bool CurrencyPair::MatchesMask(std::string_view mask) const
{
	bool match { false };
	if (Valid())
	{
		switch (mask.length())
		{
			case 1:
				match = (mask[0] == '*');
				break;
			case 3:
				match = (mask == "*/*" || Currency(mask) == BaseCCY() || Currency(mask) == QuoteCCY());
				break;
			case 5:
				if (mask[1] == '/')
				{
					match = (mask[0] == '*' && Currency(mask.substr(2, 3)) == QuoteCCY());
				}
				else if (mask[3] == '/')
				{
					match = (mask[4] == '*' && Currency(mask.substr(0, 3)) == BaseCCY());
				}
				else
				{
					match = false;
				}
				break;
			case 6:
				match = Currency(mask.substr(0, 3)) == BaseCCY() && Currency(mask.substr(3, 3)) == QuoteCCY();
				break;
			case 7:
				match = mask[3] == '/' && Currency(mask.substr(0, 3)) == BaseCCY() && Currency(mask.substr(4, 3)) == QuoteCCY();
				break;
			default:
				match = false;
				break;
		}
	}
	return match;
}

/*! \brief Returns the Pip factor of the currency pair
 *
 * @return Pip factor of the currency pair (i.e. factor that converts currency
 * units to pips)
 * */
int64_t CurrencyPair::PipFactor() const
{
	if (Valid())
	{
		const auto &it { CurrencyPair::m_cpToStrMapping.m_cpToPrecision.find(*this) };

		if (it != CurrencyPair::m_cpToStrMapping.m_cpToPrecision.end())
		{
			return it->second.second; // pip factor
		}

		return 10'000; //default precision is 5/6 for most FX pairs.
	}

	return 1;
}

/*! \brief Returns the number of significant post-comma digits
 *
 * @return Number of significant post-comma digits
 * */
int CurrencyPair::Precision() const
{
	if (Valid())
	{
		// Try to find precision in registered map
		const auto &it { CurrencyPair::m_cpToStrMapping.m_cpToPrecision.find(*this) };
		if (it != CurrencyPair::m_cpToStrMapping.m_cpToPrecision.end())
		{
			return it->second.first; // precision
		}

		//For cps not defined use following defaults
		if (BaseCCY() == Currency::XAU)
		{
			return 2;
		}
		else if (QuoteCCY() == Currency::JPY || QuoteCCY() == Currency::HUF)
		{
			return 3;
		}
		else if (QuoteCCY() == Currency::CZK || QuoteCCY() == Currency::RUB || QuoteCCY() == Currency::RON)
		{
			return 4;
		}
		else
		{
			return 5;
		}
	}
	
	return 0;
}


/*! \brief Returns the rounding factor for this CurrencyPair
 *
 * Prices can be streamed and traded with a resolution of tenth of a pip,
 * except EUR/MXN and XAU/USD where the allowed resolution is 1 pip.
 * Internally we represent prices as cpip value (hundredth of a pip).
 * This function rounds a cpip value up or down, so that the least
 * significant digits are zero.
 *
 * @return Rounding factor
 * */
int64_t CurrencyPair::RoundingFactor() const
{
	int64_t result { 10 };
	
	if (QuoteCCY() == Currency::MXN && BaseCCY() == Currency::EUR)
	{
		result = 100; //!< Special case EUR/MXN -> round to pip (= 100 cpips)
	}
	return result;
}

/*! \brief Rounds a cpip value to the lower or higher value having zero as
 * least significant digit.
 *
 * Internally we represent prices as cpip value (hundredth of a pip).
 * This function rounds a cpip value up or down, so that the least
 * significant digits are zero.
 *
 * @param cpip  Cpip value tpo be rounded
 * @param down  \a true -> round down, \a false -> round up
 * @param roundBy Value by which @a cpip is to be rounded (in cpips)
 * @return Rounded cpip value
 * */
int64_t CurrencyPair::Round(int64_t cpip, bool down, int64_t roundBy) const
{
	if (roundBy > 0 && cpip % roundBy != 0)
	{
		cpip /= roundBy;
		if (!down)
		{
			++cpip;
		}
		cpip *= roundBy;
	}
	return cpip;
}

/*! \brief Rounds a cpip value to the lower or higher value having zero as
 * least significant digit.
 *
 * Prices can be streamed and traded with a resolution of tenth of a pip,
 * except EUR/MXN where the allowed resolution is 1 pip.
 * Internally we represent prices as cpip value (hundredth of a pip).
 * This function rounds a cpip value up or down, so that the least
 * significant digits are zero.
 *
 * @param cpip  Cpip value tpo be rounded
 * @param down  \a true -> round down, \a false -> round up
 * @return Rounded cpip value
 * */
int64_t CurrencyPair::RoundForStreaming(int64_t cpip, bool down) const
{
	int64_t roundBy { 10 };
	
	if (QuoteCCY() == Currency::MXN && BaseCCY() == Currency::EUR) //!< Special case EUR/MXN
	{
		roundBy = 100;
	}
	
	return Round(cpip, down, roundBy);
}

/*! \brief Parses the string representation of a currency pair
 *
 * The string representation must be must be of any of the following forms:
 * empty
 * standard 6 characters: "EURUSD" - the fastest result
 * any number of characters, with '/' in the middle: "EUR/USDT"
 * any number of characters like "EURUSDT" - the slowest way because it looks inside internal container
 * */
//static
void CurrencyPair::Parse(std::string_view sv, Currency &ccy1, Currency &ccy2)
{
	if (sv.empty())
	{
		ccy1 = Currency::NONE;
		ccy2 = Currency::NONE;
		return;
	}
	
	// Check "standard" currency pair (each currency has 3 letters - most of the cases)
	if (sv.length() == 6)
	{
		ccy1 = Currency(sv.substr(0, 3));
		ccy2 = Currency(sv.substr(3, 3));
		if (ccy1.Valid() && ccy2.Valid())
		{
			return;
		}
	}
	
	// Check delimiter
	auto p = sv.find('/');
	if (p == std::string_view::npos)
	{
		p = sv.find('-'); // also accept '-' as delimiter
	}
	if (p != std::string_view::npos)
	{
		ccy1 = Currency(sv.substr(0, p));
		ccy2 = Currency(sv.substr(p + 1));
		return;
	}
	
	// Not standard currency - try to find any in the map (PRESUME THERE IS NO DELIMITER)
	for (const auto &iter: Currency::m_valToStrMapping.m_strToVal)
	{
		if (iter.second == Currency::NONE)
		{
			continue;
		}
		
		const auto p = sv.find(iter.first);
		if (p != std::string_view::npos)
		{
			if (p == 0)
			{
				// Found from the left
				ccy1 = Currency(iter.first);
				ccy2 = Currency(sv.substr(iter.first.size()));
			}
			else if (p == sv.length() - iter.first.length())
			{
				// Found from right
				ccy1 = Currency(sv.substr(0, p));
				ccy2 = Currency(iter.first);
			}
			else
			{
				continue;
			} // ignore, if found in the middle
			
			if (ccy1.Valid() && ccy2.Valid())
			{
				return;
			}
		} // if
	}
	
	// Nothing found:
	ccy1 = Currency::INVALID;
	ccy2 = Currency::INVALID;
}


////////////////////////////////////////////////////////////////////////////////
// CurrencyPairHash
////////////////////////////////////////////////////////////////////////////////


/*! \brief Returns currency pair by symbol. If currency pair is valid it is stored in the hash,
* if not - ot's just returned. So note: the result may be invalid
* @param symbol - currency symbol like "EUR/USD" or "BTCUSDT"
* @return - currency pair
* */
CurrencyPair CurrencyPairHash::GetCurrencyPair(const std::string &symbol) const
{
	// Try to find currency in the hash
	auto iter = m_hash.find(symbol);
	if (iter == m_hash.cend())
	{
		CurrencyPair cp(symbol);
		if (cp.Valid())
		{
			m_hash.emplace(symbol, cp);
		}
		
		return cp;
	}
	return iter->second;
}

} // ns UTILS