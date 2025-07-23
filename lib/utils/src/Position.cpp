//
// Created by jameson 07.06.18.
//

#include "Utils/Position.h"

namespace UTILS {

const std::string Position::stateNames[] = { "Open", "Pending", "Filled" };

/*! Constructor expecting all parameters that constitute a position
 *
 * @param instrument Instrument of the position
 * @param dealtCurrency Currency to be dealt
 * @param side BUY/SELL
 * @param price Price of the position, in cpips
 * @param openQuantity Initially open quantity
 * */
Position::Position(CurrencyPair instrument, Currency dealtCurrency, UTILS::Side side, int64_t price, int64_t openQuantity)
		: m_instrument(instrument), m_dealtCurrency(
		dealtCurrency == instrument.BaseCCY() || dealtCurrency == instrument.QuoteCCY() ? dealtCurrency : Currency(Currency::INVALID)), m_side(side),
		  m_price(price), m_quantity { openQuantity, 0, 0 }, m_pnl(0)
{
}

/*! Constructor expecting all parameters that constitute a position
 *
 * This constructor variant implicitly sets the base currency of the
 * given instrument as dealt currency
 *
 * @param instrument Instrument of the position
 * @param side BUY/SELL
 * @param price Price of the position, in cpips
 * @param openQuantity Initially open quantity
 * */
Position::Position(CurrencyPair instrument, UTILS::Side side, int64_t price, int64_t openQuantity)
		: Position(instrument, instrument.BaseCCY(), side, price, openQuantity)
{
}

Position::Position(const Position &originalPosition)
{
	m_instrument = originalPosition.m_instrument;
	m_dealtCurrency = originalPosition.m_dealtCurrency;
	m_side = originalPosition.m_side;
	m_price = originalPosition.m_price;
	for (int i(0); i < QTYSTATE_COUNT; ++i)
	{
		m_quantity[i] = originalPosition.m_quantity[i];
	}
	m_pnl = originalPosition.m_pnl;
}

/*! Constructor that copies property values from a model position
 *
 * This constructor variant sets the properties from a model position,
 * allowing to modify price and/or quantity
 *
 * @param model Model position
 * @param price Price of the position, in cpips
 * @param openQuantity Initially open quantity
 * */
Position::Position(const Position &model, int64_t price, int64_t openQuantity)
{
	m_instrument = model.m_instrument;
	m_dealtCurrency = model.m_dealtCurrency;
	m_side = model.m_side;
	m_price = price > 0 ? price : model.m_price;
	for (int i(0); i < QTYSTATE_COUNT; ++i)
	{
		m_quantity[i] = i == qsOpen ? (openQuantity > 0 ? openQuantity : model.m_quantity[i]) : 0;
	}
	m_pnl = 0;
}

/*! Constructor taking a string representation of the position data
 *
 * This constructor variant sets the properties from a string representation.
 * String format <Ccy pair>_<Buy/Sell><Quantity>@<Price>
 * Example: EURUSD_B1M@1.2 (buy 1.000.000 EUR at a rate of 1.2 USD)
 * The following varinats of the syntax are also accepted:
 * EUR/USD:Buy 1000000 @ 1.2, EURUSD buy 1000K@1.2
 *
 * @param stringRepresentation String representation of the position
 * */
Position::Position(const std::string &stringRepresentation)
		: Position(CurrencyPair(), Side(), 0, 0)
{
	static const std::regex base_regex(R"( *([A-Z/]+)[_: ]([BbSs][A-Za-z]*) *([0-9.]+[mk]?) *([A-Z][A-Z][A-Z])? *@ *([0-9.]+) *)");
	std::cmatch base_match;
	std::string repr { stringRepresentation };
	if (repr.find('@') == std::string::npos)
	{
		repr += "@0";
	} // default price: 0
	if (std::regex_match(repr.c_str(), base_match, base_regex))
	{
		if (base_match.size() == 6)
		{
			m_instrument = base_match[1].str();
			m_dealtCurrency = base_match[4].str().empty() ? m_instrument.BaseCCY() : Currency(base_match[4].str());
			m_side = std::toupper(base_match[2].str()[0]) == 'S' ? Side::SELL : Side::BUY;
			for (int i(0); i < QTYSTATE_COUNT; ++i)
			{
				m_quantity[i] = i == qsOpen ? StringToQty(m_instrument, base_match[3].str()) : 0;
			}
			std::string prcString { base_match[5].str() };
			if (prcString.find('.') != std::string::npos)
			{
				m_price = m_instrument.DblToCpip(parse<double>(prcString));
			}
			else
			{
				m_price = parse<int64_t>(prcString);
			}
		}
	}
}

/*! Processes an order of a given quantity
 *
 * This function is used to notify the position of an order placement.
 *
 * @param qty Ordered quantity (in 100ths of a unit)
 * @return \a true if \a qty may be ordered, \a false if the currently
 * open quantity is to low, or if the value of \a qty is invalid
 * */
bool Position::Order(int64_t qty)
{
	return qty > 0 && MoveQuantity(qty, qsOpen, qsPending);
}

/*! Processes a fill.
 *
 * This function moves the given quantity from either \a open
 * or \a pending to \a filled. Additionally, it updates the PnL value,
 * and optionally stores the profit of this single fill in an output
 * parameter.
 *
 * @param qsFrom Source state of the quantity (either \a open or \a pending)
 * @param qty Filled quantity (in 100ths of a unit)
 * @param price Price at which the order has been filled (in cpips)
 * @param profit Pointer to an 64bit integer to store the profit
 * (in cpips); may be \a nullptr -> profit is not stored
 * @return \a true if \a qty may be filled, \a false if the currently
 * pending quantity is to low, or if the value of \a qty or \a price is
 * invalid
 * */
bool Position::Fill(State qsFrom, int64_t qty, int64_t price, int64_t *profit)
{
	if (qty > 0 && price > 0 && MoveQuantity(qty, qsFrom, qsFilled))
	{
		int64_t amt { amount(qty, Sign(m_price - price)) };
		m_pnl += amt;
		if (profit)
		{
			*profit = amt;
		}
		return true;
	}
	else
	{
		return false;
	}
}

int64_t Position::QuantityToBase(int64_t qty, int64_t price) const
{
	int64_t result { qty };
	
	if (m_dealtCurrency != m_instrument.BaseCCY())
	{
		if (price <= 0)
		{
			price = m_price;
		}
		result = m_instrument.DoubleToQty(m_instrument.QtyToDouble(qty) / m_instrument.CpipToDbl(price));
	}
	return result;
}

int64_t Position::BaseToQuantity(int64_t qty, int64_t price) const
{
	int64_t result { qty };
	
	if (m_dealtCurrency != m_instrument.BaseCCY())
	{
		if (price <= 0)
		{
			price = m_price;
		}
		result = m_instrument.DoubleToQty(m_instrument.QtyToDouble(qty) * m_instrument.CpipToDbl(price));
	}
	return result;
}

/*! Processes a reject
 *
 * This function is used to notify the position that an order has been
 * rejected. It moves the given quantity from pending quantity back
 * to open quantity.
 *
 * @param qty Rejected quantity (in 100ths of a unit)
 * @return \a true if \a qty may be rejected, \a false if the currently
 * pending quantity is to low, or if the value of \a qty is invalid
 * */
bool Position::Reject(int64_t qty)
{
	return qty > 0 && MoveQuantity(qty, qsPending, qsOpen);
}

/*! Processes a late fill that occurred to a position already canceled
 *
 * This function is used to notify the position that an order has been
 * filled after it was already canceled, i.e. the wrong side was filled.
 * It moves the given quantity from pending quantity back to open quantity.
 * Additionally, it updates the price of the position, as the quantity
 * has to be reversely hedged with the price it was filled.
 *
 * @param qty Filled quantity (in 100ths of a unit)
 * @param price Price at which the order has been filled (in cpips)
 * @return \a true if operation was successfully executed, \a false if
 * the currently pending quantity is to low, or if the value of \a qty
 * or \a price is invalid.
 * */
bool Position::CanceledFill(int64_t qty, int64_t price)
{
	int64_t oldOpenQty { QuantityOpen() };
	if (qty > 0 && price > 0 && MoveQuantity(qty, qsPending, qsOpen))
	{
		m_price = (oldOpenQty * m_price + qty * price) / (oldOpenQty + qty);
		return true;
	}
	else
	{
		return false;
	}
	
}

/*! Processes a late reject that occurred to a position already canceled
 *
 * This function is used to notify the position that an order has been
 * rejected after it was already canceled.
 * It discard the given quantity. i.e. removes it from the currently
 * pending quantity.
 *
 * @param qty Rejected quantity (in 100ths of a unit)
 * @return \a true if operation was successfully executed, \a false if
 * the currently pending quantity is to low, or if the value of \a qty
 * is invalid.
 * */
bool Position::CanceledReject(int64_t qty)
{
	if (qty > 0 && qty <= m_quantity[qsPending])
	{
		m_quantity[qsPending] -= qty;
		return true;
	}
	else
	{
		return false;
	}
}

/*! Cancels the position
 *
  * @param revertFilled @a true -> swap side and revert filled qty, @a false -> only drop open qty

 * This function makes the following changes to the position:
 *          \li The side is switched: BUY <-> SELL
 *          \li Price is modified according to the PnL generated so far
 *          \li PnL is reset to 0
 *          \li Currently open quantity is discarded
 *          \li Currently filled quantity becomes open quantity
 *          \li Filled quantity is reset to 0
 *
 * If none of the quantity is filled yet, the only action (apart from
 * switching the side) is resetting the open quantity to 0.
 * */
void Position::Cancel(bool revertFilled)
{
	if (revertFilled)
	{
		m_side = ~m_side;
	}
	if (revertFilled && m_quantity[qsFilled] > 0)
	{
		m_price = m_price + Sign(price(m_quantity[qsFilled], m_pnl));
		m_pnl = 0;
		m_quantity[qsOpen] = m_quantity[qsFilled];
		m_quantity[qsFilled] = 0;
	}
	else
	{
		m_quantity[qsOpen] = 0;
//        m_price = 0; don't reset price, needed for PnL calculation
	}
}

int64_t Position::CancelOpenQty(int64_t qty, bool allOrNothing)
{
	if (qty < 0)
	{
		return 0;
	}
	if (qty == 0 || (!allOrNothing && qty > m_quantity[qsOpen]))
	{
		qty = m_quantity[qsOpen];
	}
	if (qty <= m_quantity[qsOpen])
	{
		m_quantity[qsOpen] -= qty;
	}
	else
	{
		qty = 0;
	}
	return qty;
}

Position::Ptr Position::Skim()
{
	Position::Ptr result { Create(*this) };
	(*result)->m_quantity[qsOpen] = (*result)->m_quantity[qsPending] = 0;
	this->m_quantity[qsFilled] = 0;
	this->m_pnl = 0;
	return result;
}

bool Position::AddOpenQuantity(UTILS::Side side, int64_t price, int64_t qty)
{
	if (!side.Valid() || price <= 0)
	{
		return false;
	}
	if (qty <= 0)
	{
		return qty == 0; // qty == 0 => ok, qty < 0 => error
	}
	if (side != Side() && Quantity() > 0)
	{
		return false; // wrong side => error
	}
	const CurrencyPair cp { Instrument() };
	m_price = cp.DblToCpip((cp.CpipToDbl(m_price) * cp.QtyToDouble(m_quantity[qsOpen]) + cp.CpipToDbl(price) * cp.QtyToDouble(qty)) / cp.QtyToDouble(m_quantity[qsOpen] + qty));
	m_quantity[qsOpen] += qty;
	return true;
}

bool Position::ReduceOpenQuantity(int64_t qty)
{
	if (qty < 0 || qty > m_quantity[qsOpen])
	{
		return false;
	}
	m_quantity[qsOpen] -= qty;
	return true;
}

/*! \brief Moves the given quantity from one state to another
 *
 * @param qty Quantity to be moved
 * @param qsFrom State from which \a qty is to be moved
 * @param qsTo State to which \a qty is to be moved
 * @result \a true if \a was successfully moved from \a qsFrom to \a qsTo
 * */
bool Position::MoveQuantity(int64_t qty, State qsFrom, State qsTo)
{
	bool ok { qsFrom != qsTo && m_quantity[qsFrom] >= qty };
	if (ok)
	{
		m_quantity[qsFrom] -= qty;
		m_quantity[qsTo] += qty;
	}
	return ok;
}

double Position::QuantityFilledUSD(const CurrentRateManager *crm) const
{
	return m_instrument.QtyToDouble(QuantityFilled()) * GetRate(m_instrument.BaseCCY(), Currency("USD"), crm);
}

//static
double Position::GetRate(Currency ccyFrom, Currency ccyTo, const CurrentRateManager *crm)
{
	double rate { 0.0 };
	if (ccyFrom == ccyTo)
	{
		rate = 1.0;
	}
	else
	{
		if (crm)
		{
			rate = crm->GetMidRate(CurrencyPair(ccyFrom, ccyTo));
		}
	}
	return rate;
}


/*! \brief Average executed price per unit of the position (in cpips), calculated form Price() and PnL() */
int64_t Position::ExecutedPrice() const
{
	if (QuantityFilled() > 0)
	{
		return m_price - Sign(m_pnl * 100 / QuantityFilled());
	}
	else
	{
		return 0;
	}
}

} // CORE