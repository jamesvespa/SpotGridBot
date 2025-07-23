/**
 * @file
 * @author      james <james.doll@bbf-it.at>
 * @date        2018-06-07
 * @copyright   (c) 2018 BBF-IT
*/

#ifndef COROUT_POSITION_H
#define COROUT_POSITION_H

#include <shared_mutex>

#include "Utils/Utils.h"
#include "Utils/Lockable.h"
#include "Utils/FixTypes.h"
#include "Utils/CurrencyPair.h"
#include "CurrentRateManager.h"

namespace UTILS {

#define QTYSTATE_COUNT      3

/*! \brief This class encapsulates the parameters forming a position:
 * instrument, dealt currency, side, price, quantity.
 * In addition it represents the state of the position (open, pending, filled) */
class Position
{
public:
	
	/*! \brief Type alias for shared pointer to Position */
	using Lckbl = Lockable<Position, std::shared_mutex>;
	
	/*! \brief Type alias for shared pointer to Position */
	using Ptr = std::shared_ptr<Lckbl>;
	
	/*! \brief Type alias for weak pointer to Position */
	using WkPtr = std::weak_ptr<Lckbl>;
	
	/*! \brief This type defines the possible values for the state of the
	 * position (open, pending, filled).
	 *
	 * CAUTION: if modified -> don't forget to adapt values in qtyStateNames[]
	 * */
	enum State
	{
		qsOpen, qsPending, qsFilled
	};
	
	Position(CurrencyPair instrument, Currency dealtCurrency, UTILS::Side side, int64_t price, int64_t openQuantity);
	
	Position(CurrencyPair instrument, UTILS::Side side, int64_t price, int64_t openQuantity);
	
	Position(const Position &originalPosition);
	
	Position(const Position &model, int64_t price, int64_t openQuantity);
	
	explicit Position(const std::string &stringRepresentation);
	
	template <typename ...Args>
	static Position::Ptr Create(Args &&...args)
	{
		return std::make_shared<Lckbl>(Position(std::forward<Args>(args)...));
	}
	
	/*! \brief Is the position valid?
	 *
	 * @return \a true if instrument, dealt currency and side are valid
	 * */
	bool Valid() const
	{
		return m_instrument.Valid() && m_dealtCurrency.Valid() && m_side.Valid();
	}
	
	/*! \brief Instrument */
	const CurrencyPair &Instrument() const { return m_instrument; }
	
	/*! \brief Dealt currency (usually the base currency of the instrument) */
	const Currency &DealtCurrency() const { return m_dealtCurrency; }
	
	/*! \brief Side of the position (BUY/SELL) */
	const UTILS::Side &Side() const
	{
		return m_side;
	}
	
	int64_t QuantityToBase(int64_t qty, int64_t price = 0) const;
	
	int64_t BaseToQuantity(int64_t qty, int64_t price = 0) const;
	
	/*! \brief Is this a BUY position?
	 *
	 * @return \a true if this is a BUY position */
	bool Buy() const { return Side().Buy(); }
	
	/*! \brief Is this a SELL position?
	 *
	 * @return \a true if this is a SELL position */
	bool Sell() const { return Side().Sell(); }
	
	/*! \brief Is this a LONG position?
	 *
	 * @return \a true if this is a LONG position */
	bool Long() const { return Buy() == (DealtCurrency() == Instrument().BaseCCY()); }
	
	/*! \brief Is this a SHORT position?
	 *
	 * @return \a true if this is a SHORT position */
	bool Short() const { return Sell() == (DealtCurrency() == Instrument().BaseCCY()); }
	
	bool TermPosition() const { return DealtCurrency() != Instrument().BaseCCY(); }
	
	/*! \brief sign of the position?
	 *
	 * @return Negates a given value if this is a short position */
	int64_t Sign(int64_t value) const { return Long() ? +value : -value; }
	
	/*! \brief Price per unit of the position (in cpips) */
	int64_t Price() const
	{
		return m_price;
	}
	
	int64_t ExecutedPrice() const;
	
	/*! \brief Total quantity of the position */
	int64_t Quantity() const
	{
		return m_quantity[qsOpen] + m_quantity[qsPending] + m_quantity[qsFilled];
	}
	
	/*! \brief Open quantity of the position */
	int64_t QuantityOpen() const { return m_quantity[qsOpen]; }
	
	/*! \brief Pending quantity of the position (ordered but not filled). */
	int64_t QuantityPending() const { return m_quantity[qsPending]; }
	
	/*! \brief Filled quantity of the position. */
	int64_t QuantityFilled() const { return m_quantity[qsFilled]; }
	
	/*! \brief Filled quantity of the position, converted to USD. */
	double QuantityFilledUSD(const CurrentRateManager *crm = nullptr) const;
	
	/*! \brief \a true if this position is partially (but not completely) filled. */
	bool PartiallyFilled() const
	{
		return m_quantity[qsOpen] + m_quantity[qsPending] > 0 && m_quantity[qsFilled] > 0;
	}
	
	/*! \brief \a true if this position is completely filled. */
	bool Filled() const
	{
		return m_quantity[qsOpen] + m_quantity[qsPending] == 0;
	}
	
	/*! \brief \a true if this position is completely open. */
	bool Open() const
	{
		return m_quantity[qsPending] + m_quantity[qsFilled] == 0;
	}
	
	/*! \brief String representation of a state
	 *
	 * @param state enum value of a \a State
	 * @return String representation of \a state
	 * */
	static const std::string &StateName(State state) { return stateNames[(int) state]; }
	
	/*! \brief String representation of the quantities (in open, pending, and filled states)
	 *
	 * @return String representation of the quantity, split in open, pending, and filled quantities
	 * */
	std::string QtyString() const
	{
		return Format("(%.2f/%.2f/%.2f)", m_instrument.QtyToDouble(m_quantity[qsOpen]), m_instrument.QtyToDouble(m_quantity[qsPending]),
                      m_instrument.QtyToDouble(m_quantity[qsFilled]));
	}
	
	/*! \brief String representation of the position
	 *
	 * @return String representation of the position
	 * */
	std::string ToString() const
	{
		std::string result;
		if (Open())
		{
			result = Format("%s %.2f %s @ %.6f %s", Side().ToString(), Instrument().QtyToDouble(QuantityOpen()), DealtCurrency().ToString(),
								  Instrument().CpipToDbl(Price()), Instrument().OtherCCY(DealtCurrency()).ToString());
		}
		else if (Filled())
		{
			const std::string verb { Side() == Side::BUY ? "BOUGHT" : "SOLD" }, quoteCcy { Instrument().OtherCCY(DealtCurrency()).ToString() };
			result = Format("%s %.2f %s @ %.6f %s; PnL: %.4f %s", verb, Instrument().QtyToDouble(QuantityFilled()), DealtCurrency().ToString(),
								  Instrument().CpipToDbl(ExecutedPrice()), quoteCcy, Instrument().CpipToDbl(m_pnl), quoteCcy);
		}
		else
		{ // transaction in progress
			std::string verb { (Side() == Side::BUY ? "BUYING" : "SELLING") }, quoteCcy { Instrument().OtherCCY(DealtCurrency()).ToString() };
			result = Format("%s %.2f %s @ %.6f %s in progress: %s; PnL: %.4f %s", verb, Instrument().QtyToDouble(Quantity()), DealtCurrency().ToString(),
								  Instrument().CpipToDbl(Price()), Instrument().OtherCCY(DealtCurrency()).ToString(), QtyString(),
								  Instrument().CpipToDbl(m_pnl), quoteCcy);
		}
		return result;
	}
	
	/*! \brief Overload of the ostream << operator. */
	friend std::ostream &operator<<(std::ostream &stream, const Position &position)
	{
		return stream << position.ToString();
	}
	
	/*! \brief Short string representation of the position
	 *
	 * @return Short string representation of the position
	 * */
	std::string ID() const
	{
		std::string result { Format("%s%s_%c%s%s", Instrument().BaseCCY().ToString(), Instrument().QuoteCCY().ToString(), Side().ToString()[0],
										  QtyToString(Instrument(), Quantity()), DealtCurrency().ToString()) };
		if (Price() != 0)
		{
			result += "@" + std::to_string(Price());
		}
		if (!Valid())
		{
			result = "*" + result + "*";
		}
		return result;
	}
	
	bool Order(int64_t qty);
	
	/*! Processes a fill, moves quantity from \a pending to \a filled
	 *
	 * This function is used to notify the position that an order has been filled.
	 * It moves the given quantity from \a pending quantity to \a filled quantity.
	 * Additionally, it updates the PnL value, and optionally stores the profit
	 * of this single fill in an output parameter.
	 *
	 * @param qty Filled quantity (in 100ths of a unit)
	 * @param price Price at which the order has been filled (in cpips)
	 * @param profit Pointer to an 64bit integer to store the profit
	 * (in cpips); may be \a nullptr -> profit is not stored
	 * @return \a true if \a qty may be filled, \a false if the currently
	 * pending quantity is to low, or if the value of \a qty or \a price is
	 * invalid
	 * */
	bool Fill(int64_t qty, int64_t price, int64_t *profit = nullptr) { return Fill(qsPending, qty, price, profit); }
	
	/*! Processes a fill, moves quantity from \a open to \a filled
	 *
	 * This function is used to notify the position that an open quantity has
	 * been filled directly, i.e. without placing an order.
	 * It moves the given quantity from \a open quantity to \a filled quantity.
	 * Additionally, it updates the PnL value, and optionally stores the profit
	 * of this single fill in an output parameter.
	 *
	 * @param qty Filled quantity (in 100ths of a unit)
	 * @param price Price at which the order has been filled (in cpips)
	 * @param profit Pointer to an 64bit integer to store the profit
	 * (in cpips); may be \a nullptr -> profit is not stored
	 * @return \a true if \a qty may be filled, \a false if the currently
	 * open quantity is to low, or if the value of \a qty or \a price is
	 * invalid
	 * */
	bool DirectFill(int64_t qty, int64_t price, int64_t *profit = nullptr) { return Fill(qsOpen, qty, price, profit); }
	
	/**
	 * Increases the open quantity od the position.
	 *
	 * The new quantity of the position is the sum of the current qty and the quantity
	 * passed to the function. The new price is the weighed average of the current
	 * price and the price passed to the function.
	 *
	 * @param side Side (buy/sell). Must be identical with the side of this position.
	 * @param price Price of the increment; must be greater than 0.
	 * @param qty Quantity to add to this position; must be greater or equal 0.
	 * @return @a true if the position was updated successfully, @a false otherwise.
	 */
	bool AddOpenQuantity(UTILS::Side side, int64_t price, int64_t qty);
	
	/**
	 * Reduces the open quantity (without changing any other attribute)
	 *
	 * @param qty Quantity by which to reduce the open quantity of the position
	 * @return @a true if successful, @a false if @a qty is negative or larger than the current open quantity.
	 */
	bool ReduceOpenQuantity(int64_t qty);
	
	bool MoveQuantity(int64_t qty, State qsFrom, State qsTo);
	
	bool Reject(int64_t qty);
	
	bool CanceledFill(int64_t qty, int64_t price);
	
	bool CanceledReject(int64_t qty);
	
	void Cancel(bool revertFilled);
	
	void Cancel() { Cancel(true); }
	
	/**
	 * Cancels open quantity.
	 *
	 * @param qty (default: 0) Quantity to be canceled (default: 0 -> all open quantity)
	 * @param allOrNothing (default: false) in case that current open qty < @a qty:
	 * if @a allOrNothing = @a true: do not cancel any qty, otherwise: cancel all open qty
	 * @return Canceled open quantity
	 */
	int64_t CancelOpenQty(int64_t qty = 0, bool allOrNothing = false);
	
	Ptr Skim();
	
	void AddMarketPrice(int64_t price)
	{
		if (m_price == 0)
		{
			m_price = price;
		}
	}

private:
	static const std::string stateNames[];
	
	/*! Instrument. */
	CurrencyPair m_instrument;
	/*! Dealt currency. */
	Currency m_dealtCurrency;
	/*! Side to be hedged. */
	UTILS::Side m_side;
	/*! Price. */
	int64_t m_price;
	/*! Array of quantity (open, pending, filled). */
	int64_t m_quantity[QTYSTATE_COUNT];
	/*! Profit/loss. */
	int64_t m_pnl;
	
	/*! calculates the total amount given quantity and price */
	static int64_t amount(int64_t quantity, int64_t price) { return price * quantity / QUANTITY_DECIMAL_FACTOR; }
	
	/*! calculates the price given quantity and total amount*/
	static int64_t price(int64_t quantity, int64_t amount) { return amount * QUANTITY_DECIMAL_FACTOR / quantity; }
	
	bool Fill(State qsFrom, int64_t qty, int64_t price, int64_t *profit);
	
	static double GetRate(Currency ccyFrom, Currency ccyTo, const CurrentRateManager *crm = nullptr);
};

/*! \brief Thread-safe access to a Position object.
 *
 * This class provides thread-safe access to a Position object. It acquires
 * the lock during construction and releases it during destruction. During the
 * lifetime of the object, the position may be released and locked again using
 * the methods \a unlock() and \a lock(), respectively.
 *
 * @tparam L Type of the lock (e.g. \a unique_lock or \a shared_lock). This
 * type must provide \a lock() \a unlock() and \a owns_lock() methods.
 */
template <typename L>
class PositionAccess
{
public:
	/*! \brief Constructor
	 * @param position Shared pointer to the position to be accessed
	 */
	explicit PositionAccess(Position::Ptr position)
			: m_position(std::move(position)), m_lock(m_position ? std::make_unique<L>(m_position->Mutex()) : nullptr) { }
	
	/*! \brief Constructor
	 * @param position Weak pointer to the position to be accessed
	 */
	explicit PositionAccess(const Position::WkPtr &position)
			: PositionAccess(position.lock()) { }
	
	PositionAccess(const PositionAccess &other) = delete;
	
	PositionAccess &operator=(const PositionAccess &) = delete; // disable copy by assignment
	
	PositionAccess(const PositionAccess &&other) = delete;
	
	PositionAccess &operator=(const PositionAccess &&) = delete; // disable copy by assignment
	
	/*! \brief Reference to the position pointer */
	UTILS::Position::Ptr &PositionPtr() { return m_position; }
	
	/*! \brief Checks whether the PositionAccess object actually refers to an
	 * object. */
	bool Valid() const { return m_lock != nullptr; }
	
	/*! \brief Checks whether the PositionAccess object owns the lock to a
	 * Position object. */
	bool Locked() const { return m_lock != nullptr && m_lock->owns_lock(); }
	
	/*! \brief Indirection operator.
	 * @return Reference to the position.
	 * */
	UTILS::Position &operator*()
	{
#ifdef DEBUG
		if (Locked())
		{
			return m_position->Value();
		}
		else
		{
			throw Poco::Exception("(1) PositionReader/Writer not locked!");
		}
#else
		return m_position->Value();
#endif
	}
	
	/*! \brief Indirection operator (const).
	 * @return Const reference to the position.
	 * */
	const UTILS::Position &operator*() const
	{
#ifdef DEBUG
		if (Locked())
		{
			return m_position->Value();
		}
		else
		{
			throw Poco::Exception("(2) PositionReader/Writer not locked!");
		}
#else
		return m_position->Value();
#endif
	}
	
	/*! \brief Dereference operator.
	 * @return Pointer to the position.
	 * */
	UTILS::Position *operator->()
	{
#ifdef DEBUG
		if (Locked())
		{
			return &m_position->Value();
		}
		else
		{
			throw Poco::Exception("(3) PositionReader/Writer not locked!");
		}
#else
		return &m_position->Value();
#endif
	}
	
	/*! \brief Dereference operator (const).
	 * @return Const pointer to the position.
	 * */
	const UTILS::Position *operator->() const
	{
#ifdef DEBUG
		if (Locked())
		{
			return &m_position->Value();
		}
		else
		{
			throw Poco::Exception("(4) PositionReader/Writer not locked!");
		}
#else
		return &m_position->Value();
#endif
	}
	
	/*! \brief Locks the position */
	void lock()
	{
		if (Valid())
		{
			m_lock->lock();
		}
	}
	
	/*! \brief Unlocks the position */
	void unlock()
	{
		if (Valid())
		{
			m_lock->unlock();
		}
	}

private:
	UTILS::Position::Ptr m_position;
	std::unique_ptr<L> m_lock;
};

/*! \brief Shared access to a position object.
 *
 */
class PositionReader : public PositionAccess<std::shared_lock<std::shared_mutex>>
{
public:
	explicit PositionReader(Position::Ptr position)
			: PositionAccess(std::move(position)) { }
	
	explicit PositionReader(const Position::WkPtr &position)
			: PositionAccess(position) { }
};

class PositionWriter : public PositionAccess<std::unique_lock<std::shared_mutex>>
{
public:
	explicit PositionWriter(Position::Ptr position)
			: PositionAccess(std::move(position)) { }
	
	explicit PositionWriter(const Position::WkPtr &position)
			: PositionAccess(position) { }
};

} // CORE

#endif //COROUT_POSITION_H
