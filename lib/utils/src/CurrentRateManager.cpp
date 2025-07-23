//
// Created by jameson 28.06.18.
//

#include <shared_mutex>

#include <Poco/Logger.h>
#include <set>
#include "Utils/CurrentRateManager.h"

namespace UTILS {

//static
std::map<Currency, double>CurrentRateManager::m_dfltUSDToCCY {{ Currency::AUD, 1.499 },
																	{ Currency::CAD, 1.33 },
																	{ Currency::CHF, 0.977 },
																	{ Currency::CNH, 6.998 },
																	{ Currency::CZK, 22.88 },
																	{ Currency::DKK, 6.828 },
																	{ Currency::EUR, 0.914 },
																	{ Currency::GBP, 0.776 },
																	{ Currency::HKD, 7.765 },
																	{ Currency::HUF, 309.3 },
																	{ Currency::ILS, 3.428 },
																	{ Currency::JPY, 109.7 },
																	{ Currency::MXN, 18.79 },
																	{ Currency::NOK, 9.291 },
																	{ Currency::NZD, 1.562 },
																	{ Currency::PLN, 3.904 },
																	{ Currency::RUB, 64.19 },
																	{ Currency::SEK, 9.669 },
																	{ Currency::SGD, 1.389 },
																	{ Currency::TRY, 5.987 },
																	{ Currency::THB, 34.26 },
																	{ Currency::USD, 1.0 },
																	{ Currency::ZAR, 15.08 },
																	{ Currency::XAU, 0.00058 },
															  		{ Currency::XAG, 0.03197 }
                                                                    };

void CurrentRateManager::AddDfltUSDToCCY(Currency currency, double rate)
{
    m_dfltUSDToCCY.emplace(currency, rate);
}

void CurrentRateManager::SetDfltMidRates(std::map<CurrencyPair, double> rates)
{
	std::unique_lock lock { m_midRateDfltMap.Mutex() };
	*m_midRateDfltMap = std::move(rates);
}

double CurrentRateManager::GetDfltMidRate(const CurrencyPair &cp) const
{
	double result { 0.0 };
	
	std::shared_lock slock { m_midRateDfltMap.Mutex() };
	
	if (auto it { m_midRateDfltMap->find(cp) }; it == m_midRateDfltMap->end())
	{
		slock.unlock();
		auto dfltIt { m_dfltUSDToCCY.find(cp.QuoteCCY()) };
		double usdToQuoteCcy { dfltIt != m_dfltUSDToCCY.end() ? dfltIt->second : 0. };
		dfltIt = m_dfltUSDToCCY.find(cp.BaseCCY());
		double usdToBaseCcy { dfltIt != m_dfltUSDToCCY.end() ? dfltIt->second : 0. };
		if (usdToQuoteCcy > 0 && usdToBaseCcy > 0)
		{
			result = usdToQuoteCcy / usdToBaseCcy;
			std::unique_lock ulock { m_midRateDfltMap.Mutex() };
			m_midRateDfltMap->emplace(cp, result); // add calculated rate to map
		}
		else
		{
			result = GetMidRate(cp);
			if (result > 0)
			{
				std::unique_lock ulock { m_midRateDfltMap.Mutex() };
				m_midRateDfltMap->emplace(cp, result); // add calculated rate to map
			}
		}
	}
	else
	{
		result = it->second;
	}
	return result;
}

bool CurrentRateManager::SetBidRate(CurrencyPair cp, double rate)
{
	if (rate > 0 && cp.Valid())
	{
		std::unique_lock lock { m_bidRateMap.Mutex() };
		const auto &it { m_bidRateMap->find(cp) };
		if (it != m_bidRateMap->end())
		{
			it->second = rate;
		}
		else
		{
			m_bidRateMap->emplace(cp, rate);
			m_currencies.insert(cp.BaseCCY());
			m_currencies.insert(cp.QuoteCCY());
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CurrentRateManager::SetAskRate(CurrencyPair cp, double rate)
{
	if (rate > 0)
	{
		return SetBidRate(~cp, 1.0 / rate);
	}
	else
	{
		return false;
	}
}

bool CurrentRateManager::SetRate(CurrencyPair cp, bool bid, double rate)
{
	if (bid)
	{
		return SetBidRate(cp, rate);
	}
	else
	{
		return SetAskRate(cp,rate);
	}
}

double CurrentRateManager::GetBidRate(CurrencyPair cp) const
{
	if (cp.Valid())
	{
		if (cp.BaseCCY() == cp.QuoteCCY())
		{
			return 1.0;
		}
		else
		{
			std::optional<double> maybeRate { GetDirectBidRate(cp) };
			if (maybeRate)
			{
				return maybeRate.value();
			}
			else // no direct rate -> traverse path from source to dest ccy
			{
				CurrencyList *ccyPath { GetPath(cp) };
				if (ccyPath && !ccyPath->empty())
				{
					double rate { 1.0 };
					Currency lastCcy { Currency::INVALID };
					for (auto &ccy : *ccyPath)
					{
						if (lastCcy != Currency::INVALID)
						{
							rate *= GetBidRate(CurrencyPair(lastCcy, ccy));
						}
						lastCcy = ccy;
					}
					return rate;
				}
				else
				{
					return 0.0;
				}
			}
		}
	}
	else
	{
		return 0.0;
	}
	
}

double CurrentRateManager::GetAskRate(CurrencyPair cp) const
{
	double reverseBidRate { GetBidRate(~cp) };
	if (reverseBidRate != 0)
	{
		return 1.0 / reverseBidRate;
	}
	else
	{
		return 0.0;
	}
}

double CurrentRateManager::GetRate(CurrencyPair cp, bool bid) const
{
	if (bid)
	{
		return GetBidRate(cp);
	}
	else
	{
		return GetAskRate(cp);
	}
}

std::map<CurrencyPair, double> CurrentRateManager::GetMidRates()
{
	std::map<CurrencyPair, double> midRates { };
	std::shared_lock lock { m_bidRateMap.Mutex() };
	for (auto &rates : *m_bidRateMap)
	{
		double midRate = (rates.second + GetAskRate(rates.first)) / 2.;
		midRates.emplace(rates.first, midRate);
	}
	
	return midRates;
}


double CurrentRateManager::GetMidRate(CurrencyPair cp) const
{
	return (GetBidRate(cp) + GetAskRate(cp)) / 2.0;
}

std::unique_ptr<CurrentRateManager> CurrentRateManager::MakeSnapshot(const std::set<Currency> &restrictToCurrencies)
{
	std::unique_ptr<CurrentRateManager> snapshot { std::make_unique<CurrentRateManager>() };
	std::shared_lock lock { m_bidRateMap.Mutex() };
	for (const auto &it : *m_bidRateMap)
	{
		CurrencyPair cp { it.first };
		if (restrictToCurrencies.empty() || (restrictToCurrencies.find(cp.BaseCCY()) != restrictToCurrencies.end() && restrictToCurrencies.find(
				cp.QuoteCCY()) != restrictToCurrencies.end()))
		{
			snapshot->SetBidRate(cp, it.second);
		}
	}
	return snapshot;
}


CurrentRateManager::CurrencyList *CurrentRateManager::GetPath(CurrencyPair cp) const
{
	CurrencyList ignoreList;
	std::shared_lock lock { m_bidRateMap.Mutex() };
	if (m_currencies.find(cp.BaseCCY()) == m_currencies.end() || m_currencies.find(cp.QuoteCCY()) == m_currencies.end())
	{
		return nullptr;
	}
	return GetPath(cp, ignoreList);
}

CurrentRateManager::CurrencyList *CurrentRateManager::GetPath(CurrencyPair cp, CurrencyList &ignoreList) const
{
	if (cp.Valid())
	{
		std::shared_lock lockRead { m_pathMap.Mutex() };
		const auto &it { m_pathMap->find(cp) };
		if (it != m_pathMap->end()) // already in m_pathMap?
		{
			return it->second.get();
		}
		else // not in m_pathMap
		{
			lockRead.unlock(); // release lock
			if (GetDirectBidRate(cp)) // already in m_bidRateMap?
			{
				return AddPath(cp, std::make_unique<CurrencyList>(CurrencyList { cp.BaseCCY(), cp.QuoteCCY() }));
			}
			else // neither in m_pathMap nor in m_bidRateMap
			{
				ignoreList.push_back(cp.BaseCCY()); // add base currency to ignore list (to avoid infinite recursion)
				CurrencyList *bestSubPath { nullptr }, *tmpSubPath;
				for (const auto &it2 : *m_bidRateMap)
				{
					if (it2.first.BaseCCY() == cp.BaseCCY() && std::find(ignoreList.begin(), ignoreList.end(),
																		 it2.first.QuoteCCY()) == ignoreList.end())
					{
						tmpSubPath = GetPath(CurrencyPair(it2.first.QuoteCCY(), cp.QuoteCCY()), ignoreList);
						if (tmpSubPath && (!bestSubPath || tmpSubPath->size() < bestSubPath->size()))
						{
							bestSubPath = tmpSubPath;
						}
					}
				}
				ignoreList.pop_back(); // remove base currency from ignore list
				if (bestSubPath)
				{
					CurrencyListPtr newPath { std::make_unique<CurrencyList>(CurrencyList { cp.BaseCCY() }) };
					newPath->insert(newPath->end(), bestSubPath->begin(), bestSubPath->end());
					return AddPath(cp, std::move(newPath));
				}
				else
				{
					return nullptr;
				}
			}
		}
	}
	else
	{
		return nullptr;
	}
}

std::optional<double> CurrentRateManager::GetDirectBidRate(CurrencyPair cp) const
{
	std::optional<double> result { std::nullopt };
	std::shared_lock lock { m_bidRateMap.Mutex() };
	const auto &it { m_bidRateMap->find(cp) };
	if (it != m_bidRateMap->end())
	{
		result = it->second;
	}
	return result;
}

CurrentRateManager::CurrencyList *CurrentRateManager::AddPath(CurrencyPair cp, CurrencyListPtr newPath) const
{
	std::unique_lock lock { m_pathMap.Mutex() };
	const auto &[itNew, result] = m_pathMap->emplace(std::pair<CurrencyPair, CurrencyListPtr>(cp, std::move(newPath)));
	return itNew->second.get(); // return new or existing currency list
}

} // namespace CORE