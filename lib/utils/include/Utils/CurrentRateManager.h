/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2018-06-28
 * @copyright   (c) 2018 BBF-IT
*/

#ifndef MERCURYUTILS_CURRENTRATEMANAGER_H
#define MERCURYUTILS_CURRENTRATEMANAGER_H

#include <memory>
#include <vector>
#include <set>
#include <mutex>
#include <optional>

#include "Utils/CurrencyPair.h"
#include "Utils/Lockable.h"

namespace UTILS {

/** @brief This class handles current bid and ask rates of any number of
 * currency pairs.
 */
class CurrentRateManager
{
public:
	
	/** @brief Sets the bid rate of a currency pair
	 *
	 * This function is used to register the current bid rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @param rate  Bid price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 * @return @a true if the rate was successfully registered
	 */
	bool SetBidRate(CurrencyPair cp, double rate);
	
	/** @brief Sets the ask rate of a currency pair
	 *
	 * This function is used to register the current ask rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @param rate  Ask price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 * @return @a true if the rate was successfully registered
	 */
	bool SetAskRate(CurrencyPair cp, double rate);
	
	/** @brief Sets the bid or ask rate of a currency pair
	 *
	 * This function is used to register the current bid or ask rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	* @param bid   @a true -> bid, @false -> ask
	 * @param rate  Bid or ask price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 * @return @a true if the rate was successfully registered
	 */
	bool SetRate(CurrencyPair cp, bool bid, double rate);
	
	/** @brief Gets the bid rate of a currency pair
	 *
	 * This function is used to determine the current bid rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @return rate  Bid price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 */
	double GetBidRate(CurrencyPair cp) const;
	
	/** @brief Gets the ask rate of a currency pair
	 *
	 * This function is used to determine the current ask rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @return rate  Ask price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 */
	double GetAskRate(CurrencyPair cp) const;
	
	/** @brief Gets the bid or ask rate of a currency pair
	 *
	 * This function is used to determine the current bid or ask rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @param bid   @a true -> bid, @false -> ask
	 * @return rate  Bid/ask price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 */
	double GetRate(CurrencyPair cp, bool bid) const;
	
	/** @brief Gets the mid rate of a currency pair
	 *
	 * This function is used to determine the current mid rate of a currency pair.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @return rate  Mid price of one unit of the base currency of @a cp,
	 * expressed in units of the quote currency
	 */
	double GetMidRate(CurrencyPair cp) const;
	
	std::unique_ptr<CurrentRateManager> MakeSnapshot(const std::set<Currency> &restrictToCurrencies = { });
	
	void SetDfltMidRates(std::map<CurrencyPair, double> rates);
	
	std::map<CurrencyPair, double> GetMidRates();
	
	std::map<CurrencyPair, double> DfltMidRates() { return *m_midRateDfltMap; }
	
	/**
	 * \brief retrieves static mid rate values for this session, either from last persisted values
	 * or default mapping
	 * @param cp 	currency pair
	 * @return 		mid rate or 0 if not found
	 */
	double GetDfltMidRate(const CurrencyPair &cp) const;

    void AddDfltUSDToCCY(Currency currency, double rate);
	
private:
	using CurrencyList = std::vector<Currency>; //!< Type alias for vector of Currency
	
	using CurrencyListPtr = std::unique_ptr<CurrencyList>; //!< Type alias for unique pointer to vector of Currency
	
	SharedLockable <std::map<CurrencyPair, double>> m_bidRateMap; //!< Map of bid rates
	
	std::set<Currency> m_currencies;
	
	mutable SharedLockable <std::map<CurrencyPair, CurrencyListPtr>> m_pathMap; //!< Map of paths from one ccy to another
	
	//!< default conversion of USD to any CCY
	static std::map<Currency, double> m_dfltUSDToCCY;
	
	mutable SharedLockable <std::map<CurrencyPair, double>> m_midRateDfltMap; //!< Map of default mid rates (updates with each restart)
	
	/** @brief Determines the shortest path from one currency to another
	 *
	 * This function determines the shortest path of existing exchange rates
	 * from the base currency of a given currency pair to its quote currency.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @param ignoreList List of currencies that must not be included it the path
	 * (this parameter is used internally for recursive calls to avoid infinite
	 * recursion)
	 * @return Pointer to a list of currencies that represents the path from
	 * @a cp.BaseCCY() to @a cp.QuoteCCY(), @a nullptr if no path was found
	 * or an error occurred
	 */
	CurrencyList *GetPath(CurrencyPair cp, CurrencyList &ignoreList) const;
	
	/** @brief Determines the shortest path from one currency to another
	 *
	 * This function determines the shortest path of existing exchange rates
	 * from the base currency of a given currency pair to its quote currency.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @return Pointer to a list of currencies that represents the path from
	 * @a cp.BaseCCY() to @a cp.QuoteCCY(), @a nullptr if no path was found
	 * or an error occurred
	 */
	CurrencyList *GetPath(CurrencyPair cp) const;
	
	/** @brief Tries to get the direct bid rate of a currency pair
	 *
	 * This function returns a valid bid rate only if it was directly inserted
	 * with SetBidRate(); otherwise it returns @a std::nullopt.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @return Optional rate. @a std::nullopt if no rate was inserted for this
	 * currency pair.
	 */
	std::optional<double> GetDirectBidRate(CurrencyPair cp) const;
	
	/** @brief Adds a path from now currency to another
	 *
	 * This function adds a path to the map of currency paths.
	 *
	 * @param cp    Currency pair (base currency/quote currency)
	 * @param newPath Currency list representing the path from @a cp.BaseCCY()
	 * to @a cp.QuoteCCY()
	 * @return Returns a pointer to the inserted currency list
	 */
	CurrencyList *AddPath(CurrencyPair cp, CurrencyListPtr newPath) const;
};

} // namespace CORE

#endif //MERCURYUTILS_CURRENTRATEMANAGER_H
