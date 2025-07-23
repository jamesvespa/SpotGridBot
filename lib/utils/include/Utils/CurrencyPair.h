/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2018-05-15
 * @copyright   (c) 2018 BBF-IT
*/
#pragma once

#include <memory>
#include <string>
#include <map>
#include <cmath>

#include "Utils/Result.h"
#include "Utils/Lockable.h"

namespace UTILS
{

class CurrentRateManager;

const char* const INVALID_CURRENCY = "???";
const int CurrencyDefaultPrecision = 8;
const int64_t CurrencyDefaultPipFactor = 100;

const std::string CurrencyConfigFile = "CurrencyConfig.xml";

/*! \brief This class represents a single currency. An object of this type is
 * constructable either from a string or from an enum value (Currency::Value).
 * It is assignable from a string (copy assignment operator) and can be
 * assigned to a string (via implicit conversion operator) */
class Currency
{
    friend class CurrencyPair;
public:
	// mapValToStr, mapStrToVal
	// Note: to add a new currency:
    // 1) add value here
    // 2) add mapping in CurrencyPair.cpp in Currency::ValToStrMapping::ValToStrMapping()
	enum Value
	{
		NONE,
		USD,
		EUR,
		GBP,
		CHF,
		JPY,
		AUD,
		CAD,
		CNY,
		SEK,
		NZD,
		MXN,
		SGD,
		HKD,
		NOK,
		KRW,
		TRY,
		RUB,
		INR,
		BRL,
		ZAR,
		DKK,
		ILS,
		PLN,
		CZK,
		NGN,
		HUF,
		CNH,
		GHS,
		KES,
		RON,
		BGN,
		TWD,
		ZMW,
		XAG,
		XAU,
		XPD,
		XPT,
		XAF,
		SAR,
		KWD,
		QAR,
		OMR,
		BHD,
		MYR,
		THB,
		PKR,
		MAD,
		BGD,
		AED,
		IDR,
		EGP,
		IQD,
		UAH,
		VND,
		JOD,
		XOF,
		LKR,
		TND,
		UGX,
		UZS,
		DZD,
		IRR,
		DOP,
		ISK,
		HRK,
		RSD,
		PHP,
		COP,
		ARS,
        //---
        END_OF_FX, // should end of known FX currencies
		INVALID = 999999
	};

	Currency(Value val)
			: m_value(val) { }

	Currency(std::string_view sv)
			: m_value(StrToVal(sv)) { }

	Currency()
			: Currency(NONE) { }

	/*! \brief Copy constructor. */
	Currency(const Currency &other)
			: Currency(other.m_value) { }

	/*! \brief Copy assignment operator. */
	Currency &operator=(const Currency &other)
	{
		m_value = other.m_value;
		return *this;
	}

	/*! \brief Copy assignment operator. */
	Currency &operator=(std::string_view sv)
	{
		m_value = StrToVal(sv);
		return *this;
	}

	/*! \brief Implicit conversion operator. */
	operator std::string() const
	{
	    return std::string(m_valToStrMapping.m_valToStr[m_value]);
	}

	operator std::string_view() const
	{
	    return m_valToStrMapping.m_valToStr[m_value];
	}

	/*! \brief Return currency as enum value. */
	operator Value() const { return m_value; }

	/*! \brief Checks whether the currency is empty (not set). */
	bool Empty() const { return m_value == NONE; }

	/*! \brief Checks whether the currency is set to an invalid value. */
	bool Invalid() const { return m_value == INVALID; }

	/*! \brief Checks whether the currency is set to a valid value. */
	bool Valid() const { return !Empty() && !Invalid(); }

	/*! \brief Checks whether the currency is set to a valid value.
	 *
	 * Static function.
	 *
	 * @param str String representation of a currency.
	 * @return \a true if \a str represents a valid currency, \a false otherwise.
	 * */
	static bool Valid(std::string_view sv) { return StrToVal(sv) != INVALID; }

	/*! \brief Returns the string representation of the currency. */
	std::string_view ToStringView() const
	{
	    return m_valToStrMapping.m_valToStr[m_value];
	}

	/*! \brief Returns the string representation of the currency. */
	std::string ToString() const { return std::string(ToStringView()); }

	/*! \brief Overload of the ostream << operator. */
	friend std::ostream &operator<<(std::ostream &stream, const Currency &ccy)
	{
		return stream << ccy.ToString();
	}

    struct ValToStrMapping {
        ValToStrMapping();
    	std::map<Value, std::string> m_valToStr;
	    std::map<std::string, Value> m_strToVal;
    	std::map<Value, std::pair<int, int64_t>> m_valToPrecision; // <Currency: <precision:pip>>
    };
    static const ValToStrMapping& GetValToStrMapping()
    {
        return m_valToStrMapping;
    }

	/*! \brief Registers a new currency. The currency value and symbol must be unique
	* Currency should not create a conflict when creating pairs with other currencies
	* @param value - unique currency id
	* @param symbol - unique currency symbol
	* @return - returns true in success.
	*/
    static BoolResult RegisterCurrency(const Value value, const std::string& symbol);

	/*! \brief Returns false if there is a conflict in existing currencies */
    static BoolResult CheckRegisteredCurrencies();

    /*! \brief Loads currencies from config file
    * @param path: path to config file with currencies
    * @return: true in success
    * */
    static BoolResult InitializeCurrenciesFromFile(const std::string& path);

    /*! \brief Loads currencies from xml document
    * @param config: xml doc
    * @return: true in success
    * */
    static BoolResult InitializeCurrenciesFromXml(UTILS::XmlDocPtr config);

    /*! \brief Loads currencies from "CurrencyConfig.xml" file and traces output in cout
    * @return: true in success
    * */
    static BoolResult InitializeCurrencies(std::string cfgFile = CurrencyConfigFile);

private:
	static Value StrToVal(std::string_view sv);

	Value m_value;
    static ValToStrMapping m_valToStrMapping;
};

/*! \brief This class represents a currency pair. An object of this type is
 * constructable either from a string or from two Currency objects. It is
 * assignable from a string (copy assignment operator) and can be assigned
 * to a string (via implicit conversion operator) */
class CurrencyPair
{
public:
	/*! \brief Construct currency pair from string (implicit constructor). */
	CurrencyPair(std::string_view sv) { Set(sv); }

	/*! \brief Construct currency pair from base currency and quote currency. */
	CurrencyPair(Currency baseCCY, Currency quoteCCY)
			: m_baseCCY(baseCCY), m_quoteCCY(quoteCCY) { }

	/*! \brief Default constructor (set base currency and quote currency to NONE). */
	CurrencyPair()
			: CurrencyPair(Currency::NONE, Currency::NONE) { }

	/*! \brief Copy constructor. */
	CurrencyPair(const CurrencyPair &other)
			: CurrencyPair(other.m_baseCCY, other.m_quoteCCY) { }

	/*! \brief Copy assignment operator. */
	CurrencyPair &operator=(const CurrencyPair &other)
	{
		Set(other.m_baseCCY, other.m_quoteCCY);
		return *this;
	}

	/*! \brief Copy assignment operator. */
	CurrencyPair &operator=(std::string_view sv)
	{
		Set(sv);
		return *this;
	}

	/*! \brief Lower-than operator. */
	bool operator<(const CurrencyPair &other) const
	{
		return Valid() && BaseCCY() == other.BaseCCY() ? QuoteCCY() < other.QuoteCCY() : BaseCCY() < other.BaseCCY();
	}

	/*! \brief Implicit conversion operator. */
	operator const std::string &() const { return ToString(); }

	const std::string &ToString() const;

	/*! \brief Overload of the ostream << operator. */
	friend std::ostream &operator<<(std::ostream &stream, const CurrencyPair &ccyPair)
	{
		return stream << ccyPair.ToString();
	}

	/*! \brief Checks whether the currency pair is empty
	 *
	 * The currency pair is empty iff both base currency
	 * and quote currency are empty.
	 *
	 * @return \a true if the currency pair is empty
	 * */
	bool Empty() const { return BaseCCY().Empty() && QuoteCCY().Empty(); }

    bool IsFX() const { return m_baseCCY.m_value < Currency::Value::END_OF_FX; }

	/*! \brief Checks whether the currency pair is invalid
	 *
	 * The currency pair is invalid iff it is neither empty nor valid.
	 *
	 * @return \a true if the currency pair is invalid
	 * */
	bool Invalid() const { return !Empty() && !Valid(); }

	/*! \brief Checks whether the currency pair is valid
	 *
	 * The currency pair is valid iff both base currency
	 * and quote currency are valid.
	 *
	 * @return \a true if the currency pair is valid
	 * */
	bool Valid() const { return BaseCCY().Valid() && QuoteCCY().Valid(); }

	static bool ValidMask(std::string_view mask);

	bool MatchesMask(std::string_view mask) const;

	int64_t PipFactor() const;

	int Precision() const;

	int64_t RoundingFactor() const;

	/*! \brief "CentiPip" factor (pip factor * 100) */
	int64_t CpipFactor() const { return PipFactor() * 100;}

	/*! \brief Converts a double currency value to "CentiPips" */
	int64_t DblToCpip(double dbl) const { return llround(dbl * CpipFactor()); }

	/*! \brief Converts "CentiPips" to a a double currency value */
	double CpipToDbl(int64_t cpip) const { return (double) (cpip) / CpipFactor(); }

    const double QtyToDouble(int64_t qty) const
    {
        return IsFX() ? (double(qty) / QUANTITY_DECIMAL_FACTOR) : (double(qty) / QUANTITY_DECIMAL_FACTOR_CRYPTO);
    }

    const int64_t DoubleToQty(double qty) const
    {
        return llround(qty * (IsFX() ? QUANTITY_DECIMAL_FACTOR : QUANTITY_DECIMAL_FACTOR_CRYPTO));
    }

    int64_t Round(int64_t cpip, bool down, int64_t roundBy) const;

	int64_t RoundForStreaming(int64_t cpip, bool down) const;

	/*! \brief WorkElem currency (first currency in the pair) */
	const Currency &BaseCCY() const { return m_baseCCY; }

	/*! \brief Quote currency (second currency in the pair) */
	const Currency &QuoteCCY() const { return m_quoteCCY; }

	/*! \brief Equality operator */
	bool operator==(const CurrencyPair &other) const
	{
		return Valid() && other.Valid() && BaseCCY() == other.BaseCCY() && QuoteCCY() == other.QuoteCCY();
	}

	/*! \brief Inequality operator */
	bool operator!=(const CurrencyPair &other) const
	{
		return !this->operator==(other);
	}

	/*! \brief Returns the opposite currency
	 *
	 * @param ccy One of the currencies of the instrument
	 * @return Currency opposite to \a ccy (if \a ccy is the base currency,
	 * then the quote currency is returned, and vice versa)
	 * */
	Currency OtherCCY(const Currency &ccy) const
	{
		return ccy == BaseCCY() ? QuoteCCY() : (ccy == QuoteCCY() ? BaseCCY() : Currency(Currency::INVALID));
	}

	/*! \brief Reverse operator.
	 *
	 * @return Opposite currency pair: EUR/USD -> USD/EUR
	 * */
	CurrencyPair operator~() const { return Valid() ? CurrencyPair(QuoteCCY(), BaseCCY()) : *this; }

	struct CpToStrMapping
	{
		CpToStrMapping()
		{
			// Regular FX currencies
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::CAD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::HKD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::NZD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::USD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CAD, Currency::HKD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::MXN, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::TRY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::ILS ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::AUD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::CAD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::CNH ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::CZK ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::GBP ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::HKD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::HUF ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::NZD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::RON ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::RUB ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::TRY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::TRY, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::USD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::AUD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::CAD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::CZK ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::HUF ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::NZD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::USD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::TRY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::RUB ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::HKD, Currency::JPY ), std::make_pair(3, 100) );
            m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NOK, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::USD ), std::make_pair(5, 10000) );
            m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::CAD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::AED ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::CAD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::CHF ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::CNH ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::CZK ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::RUB ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::HKD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::HUF ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::ILS ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::ILS ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::KWD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::PLN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::QAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::RON ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::EUR, Currency::RUB ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::SAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::THB ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::TRY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::KES ), std::make_pair(2, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::ZAR, Currency::MXN ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::ZAR, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::HKD), std::make_pair(5, 10000) );



			m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::HKD ), std::make_pair(6, 100000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SGD, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::HUF ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::HKD ), std::make_pair(6, 100000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::DKK, Currency::SEK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NOK, Currency::JPY ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SEK, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::SEK ), std::make_pair(3, 100) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::DKK, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::ILS ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NOK, Currency::HKD ), std::make_pair(6, 100000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::SEK, Currency::HKD ), std::make_pair(6, 100000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::CZK ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::DKK, Currency::JPY ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::PLN, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CZK, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::PLN, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::USD, Currency::GHS ), std::make_pair(4, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::NOK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::DKK, Currency::HKD ), std::make_pair(5, 100000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::GBP, Currency::HKD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::PLN, Currency::HUF ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CZK, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::ZAR ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::NZD, Currency::SGD ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CNH, Currency::JPY ), std::make_pair(4, 1000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::HUF, Currency::JPY ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::AUD, Currency::DKK ), std::make_pair(5, 10000) );
			m_cpToPrecision.emplace( CurrencyPair( Currency::CHF, Currency::ZAR ), std::make_pair(5, 10000) );


			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::AUD), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::CAD), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::CHF), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::DKK), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::EUR), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::GBP), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::HKD), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::JPY), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::NOK), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::NZD), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::SEK), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::SGD), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::THB), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::TRY), std::make_pair(6, 100000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::USD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAG, Currency::ZAR), std::make_pair(6, 100000));

			// Gold (XAU) pairs
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::AUD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::CAD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::CHF), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::DKK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::EUR), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::GBP), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::HKD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::JPY), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::NOK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::NZD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::SEK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::SGD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::THB), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::TRY), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::USD), std::make_pair(2, 10));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XAU, Currency::ZAR), std::make_pair(4, 1000));

			// Palladium (XPD) pairs
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::AUD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::CAD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::CHF), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::DKK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::EUR), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::GBP), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::HKD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::JPY), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::NOK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::NZD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::SEK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::SGD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPD, Currency::USD), std::make_pair(3, 100));

			// Platinum (XPT) pairs
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::AUD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::CAD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::CHF), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::DKK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::EUR), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::GBP), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::HKD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::JPY), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::NOK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::NZD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::SEK), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::SGD), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::THB), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::TRY), std::make_pair(4, 1000));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::USD), std::make_pair(3, 100));
			m_cpToPrecision.emplace(CurrencyPair(Currency::XPT, Currency::ZAR), std::make_pair(4, 1000));
		}

		std::map<CurrencyPair, std::pair<int, int64_t>> m_cpToPrecision; // <CurrencyPair: <precision:pip>>
	};

	static const CpToStrMapping& GetCpToStrMapping()
	{
		return m_cpToStrMapping;
	}

	/*! \brief Loads currencies from config file
* @param path: path to config file with currencies
* @return: true in success
* */
	static BoolResult InitializeCurrencyPairsFromFile(const std::string& path);

	/*! \brief Loads currencies from xml document
	* @param config: xml doc
	* @return: true in success
	* */
	static BoolResult InitializeCurrencyPairsFromXml(UTILS::XmlDocPtr config);

	/*! \brief Loads currencies from "CurrencyConfig.xml" file and traces output in cout
	* @return: true in success
	* */
	static BoolResult InitializeCurrencyPairs(const std::string& cfgFile = CurrencyConfigFile);

	static void InitializeCurrencyConfigs(const std::string& cfgFile = CurrencyConfigFile);

private:
	/*! \brief WorkElem and quote currencies. */
	Currency m_baseCCY, m_quoteCCY;

	/*! \brief Static cache of CurrencyPair string representations */
	static SharedLockable<std::map<CurrencyPair, std::unique_ptr<std::string>>> m_nameCache;

	static CpToStrMapping m_cpToStrMapping;

	/*! \brief Set base and quote currencies. */
	void Set(Currency baseCCY, Currency quoteCCY)
	{
		m_baseCCY = baseCCY;
		m_quoteCCY = quoteCCY;
	}

	/*! \brief Set base and quote currencies from string. */
	void Set(std::string_view sv)
	{
	    Parse(sv, m_baseCCY, m_quoteCCY);
    }

	static void Parse(std::string_view sv, Currency &ccy1, Currency &ccy2);
};


/*! \brief This class makes faster to obtain currency pairs from symbols, especially irregular ones
* like crypto-currencies. Constructing real currency pair using internal container from irregular symbol
* like "EOSDOWNTRXDOWN" involves linear search. This hash would find a valid currency pair once, construct it
* and return a copy of it by symbol, what is much faster
* */
class CurrencyPairHash {
public:
    /*! \brief Returns currency pair by symbol. If currency pair is valid it is stored in the hash,
    * if not - ot's just returned. So note: the result may be invalid
    * @param symbol - currency symbol like "EUR/USD" or "BTCUSDT"
    * @return - currency pair
    * */
    CurrencyPair GetCurrencyPair(const std::string& symbol) const;

    /*! \brief Returns hash map */
    using THashMap = std::unordered_map<std::string, CurrencyPair>;
    const THashMap& GetHash() const
    {
        return m_hash;
    }

private:
    mutable THashMap m_hash;
}; // CurrencyPairHash

} // ns UTILS

namespace std {

template <>
class hash<UTILS::Currency>
{
public:
	size_t operator()(const UTILS::Currency &ccy) const
	{
		return std::hash<uint>()(uint(ccy));
	}
};

template <>
class hash<UTILS::CurrencyPair>
{
public:
	size_t operator()(const UTILS::CurrencyPair &cp) const
	{
		return std::hash<uint>()((uint(cp.BaseCCY()) << 16u) | uint(cp.QuoteCCY()));
	}
};

}
