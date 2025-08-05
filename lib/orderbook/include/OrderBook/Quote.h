//
// Created by helmut on 20.02.18.
//

#ifndef COROUT_QUOTE_H
#define COROUT_QUOTE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

#include "Utils/Utils.h"
#include "Utils/Lockable.h"
#include "Utils/CurrencyPair.h"
#include "Utils/ObjectPool.h"

#define UNLIMITED_QUOTE_AGE  (std::numeric_limits<int64_t>::max()) // no (realistic) age limit
//#define LIMITED_QUOTE_AGE  	 10'000'000'000 // limited quote age (10s), to avoid using stale quotes
#define LIMITED_QUOTE_AGE     300'000'000'000 // limited quote age (5min), to avoid using stale quotes

namespace CORE {
namespace BOOK {

/*! \brief This class contains all data of a single quote
*/
class Quote
{
public:
	/*! \brief Shared pointer to a quote. */
	using Ptr = std::shared_ptr<Quote>;
	/*! \brief Weak pointer to a quote. */
	using WkPtr = std::weak_ptr<Quote>;
	
	/*! \brief Enumeration of fields that are part of a quote. */
	enum Field
	{
		fldNone,
		fldLevel,
		fldLevelVolume,
		fldAggregateVolume,
		fldQuoteID,
		fldCompID,
		fldSession,
		fldPb,
		fldSeqNum,
		fldPrice,
		fldVolume,
		fldMinQuantity,
		fldKey,
		fldRefKey,
		fldSendingTime,
		fldReceiptTime,
		fldQuoteType,
		fldPositionNo,
		fldAdptReceiveTime
	};
	/*! \brief Enumeration of base types of the fields that are part of a quote. */
	enum FieldBaseType
	{
		fbtNone, fbtInt64, fbtString
	};
	
	Quote(int64_t adptReceiveTime, int64_t receiptTime, int64_t sortTime, std::string quoteID, int64_t seqnum,
		  int64_t price, int64_t volume, int64_t minQty, int64_t key, int64_t refKey, int64_t sendingTime, int quoteType, int positionNo,
		  std::string settlDate, std::string originator)
			: m_adptReceiveTime(adptReceiveTime), m_receiptTime(receiptTime), m_sortTime(sortTime), m_quoteID(std::move(quoteID)),
			  m_seqnum(seqnum), m_price(price), m_volume(volume), m_minQty(minQty), m_key(key), m_refKey(refKey), m_sendingTime(sendingTime),
			  m_quoteType(quoteType), m_positionNo(positionNo), m_settlDate(std::move(settlDate)), m_originator(std::move(originator)), m_used(false),
			  m_successor { 0, 0 } { }
	
	int64_t ReceiptTime() const { return m_receiptTime; }
	
	const std::string &QuoteID() const { return m_quoteID; }
	
	int64_t SeqNum() const { return m_seqnum; }
	
	int64_t Price() const { return m_price; }
	
	int64_t Volume() const { return m_volume; }
	
	int64_t MinQty() const { return m_minQty; }
	
	int64_t Key() const { return m_key; }
	
	int64_t RefKey() const { return m_refKey; }
	
	int64_t SendingTime() const { return m_sendingTime; }
	
	const std::string &SettlDate() const { return m_settlDate; }
	
	int QuoteType() const { return m_quoteType; } //!< Quote update type (QT_NEW, QT_UPDATE, QT_DELETE)
	
	int PositionNo() const { return m_positionNo; }
	
	const std::string &Originator() const { return m_originator; }
	
	bool Used() const { return m_used.load(); } //!< Has this quote already been used in an order?
	
	bool SetUsed();
	
	bool Valid() const { return m_successor.sent == 0; }
	
	bool Valid(int64_t &successorSent, int64_t &successorReceived) const;
	
	bool SetInvalid(Quote::Ptr successor);
	
	int64_t GetInt(Field fld) const;
	
	const std::string &GetString(Field fld) const;
	
	std::string ToString(UTILS::CurrencyPair cp) const;
	
	static Field GetField(const std::string &fieldName);
	
	static FieldBaseType GetBaseType(Field fld);
	
	int64_t SendingDelay() const { return AdptReceiveTime() - SendingTime(); }
	
	double SendingDelayMs() const { return double(SendingDelay()) / 1000000.0; }
	
	int64_t AgeSinceSend() const { return UTILS::CurrentTimestamp() - SendingTime(); }
	
	double AgeSinceSendMs() const { return double(AgeSinceSend()) / 1000000.0; }
	
	int64_t AgeSinceReceipt() const { return UTILS::CurrentTimestamp() - ReceiptTime(); }
	
	double AgeSinceReceiptMs() const { return double(AgeSinceReceipt()) / 1000000.0; }
	
	int64_t AdptReceiveTime() const { return m_adptReceiveTime; }
	
	int64_t ReceiptDelay() const { return ReceiptTime() - AdptReceiveTime(); }
	
	double ReceiptDelayMs() const { return double(ReceiptDelay()) / 1000000.0; }
	
	int64_t SortTime() const { return m_sortTime; }
	
	int64_t SortDelay() const { return SortTime() - ReceiptTime(); }
	
	double SortDelayMs() const { return double(SortDelay()) / 1000000.0; }
	
private:

	const int64_t m_adptReceiveTime; // time when quote was received by the adapter
	const int64_t m_receiptTime; // time when quote was received by the engine
	const int64_t m_sortTime; // time when quote was added to the sortbook
	const std::string m_quoteID;
	const int64_t m_seqnum;
	const int64_t m_price;
	const int64_t m_volume;
	const int64_t m_minQty;
	const int64_t m_key;
	const int64_t m_refKey;
	const int64_t m_sendingTime;
	const int m_quoteType; //!< Quote update type (QT_NEW, QT_UPDATE, QT_DELETE)
	const int m_positionNo;
	const std::string m_settlDate;
	const std::string m_originator;
	std::atomic_bool m_used;
	struct
	{
		std::atomic_int64_t sent;
		std::atomic_int64_t received;
	} m_successor;
};


class QuotePool
{
public:
	static constexpr uint64_t POOL_SIZE = 1L << 20;

	template <typename... Args>
	static Quote::Ptr getQuote(const char* caller, bool objectPoolDisabled, std::thread::id tid, Args... args)
	{
		if(objectPoolDisabled || op.isExhausted())
		{
			return std::make_unique<Quote>(std::forward<Args>(args)...);
		}

		return Quote::Ptr((op.getObject(caller, tid, std::forward<Args>(args)...)));
	}

private:
	static UTILS::ObjectPool<Quote, POOL_SIZE> op;
};


/*! \brief This class defines a group of quotes (e.g. all quotes of one level)
 * */
class QuoteGroup
{
public:
	/*! \brief  Shared pointer to a quote group */
	using Ptr = std::shared_ptr<QuoteGroup>;
	/*! \brief  Vector of shared pointers to a quote group */
	using QuoteVector = std::vector<Quote::Ptr>;
	
	QuoteGroup();
	
	explicit QuoteGroup(const QuoteVector &srcVec);
	
	/*! \brief Creates a new quote group
	 *
	 * Static function.
	 *
	 * @return Pointer to the new quote group
	 * */
	static Ptr Create() { return std::make_shared<QuoteGroup>(); }
	
	void AddQuote(const Quote::Ptr &q);
	
	void AddQuotes(const QuoteGroup::Ptr &quoteGroup);
	
	bool RemoveQuote(const CORE::BOOK::Quote::Ptr &q);
	
	void GetQuotes(QuoteVector &vec) const;
	
	/*! \brief Copies the quotes of the quote group into a vector
	 *
	 * Adds the quotes of the quote group fulfilling a condition
	 * to a given vector.
	 *
	 * @param vec     Vector to hold the quotes of the quote group
	 * @param cond    Condition to be fulfilled ()
	 * */
	template <typename F>
	void GetQuotes(QuoteVector &vec, F cond) const
	{
		std::shared_lock lock { m_quotes.Mutex() };
		for (const auto &q: *m_quotes)
		{
			if (cond(q))
			{
				vec.push_back(q);
			}
		}
	}
	
	template <typename A>
	void ForEachQuote(A action) const
	{
		QuoteVector quoteVec;
		GetQuotes(quoteVec);
		for (const auto &q: quoteVec)
		{
			action(q);
		}
	}
	
	template <typename F>
	Quote::Ptr FindFirstQuote(F cond) const
	{
		std::shared_lock lock { m_quotes.Mutex() };
		for (const auto &q: *m_quotes)
		{
			if (cond(q))
			{
				return q;
			}
		}
		return nullptr;
	}
	
	/**
	 * Checks whether of quotes of the group are of the same price
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return \a true if all quotes have the same price
	 */
	template <bool unusedOnly = true>
	bool SinglePrice() const
	{
		AggregateValues::Ptr av { GetOrCreateAggregateValues<unusedOnly>() };
		return av->minPrice == av->maxPrice;
	}
	
	/**
	 * Minimum price of all quotes of the group, in cpips
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Minimum price
	 */
	template <bool unusedOnly = true>
	int64_t MinPrice() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->minPrice;
	}
	
	/**
	 * Maximum price of all quotes of the group, in cpips
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Maximum price
	 */
	template <bool unusedOnly = true>
	int64_t MaxPrice() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->maxPrice;
	}
	
	/**
	 * Average price (Weighted average of all the quotes of the group, in cpips)
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Average price
	 */
	template <bool unusedOnly = true>
	int64_t AvgPrice() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->avgPrice;
	}
	
	/**
	 * Maximum volume of all quotes of the group, in 100ths of a unit
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Maximum volume
	 */
	template <bool unusedOnly = true>
	int64_t MaxVolume() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->maxVolume;
	}
	
	/**
	 * Total volume (sum of volumes of all quotes of the group, in 100ths of a unit)
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Total volume
	 */
	template <bool unusedOnly = true>
	int64_t TotalVolume() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->totalVolume;
	}
	
	/**
	 * Minimum quantity to be ordered (in 100ths of a unit)
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Minimum qty (maximum of all considered quotes)
	 */
	template <bool unusedOnly = true>
	int64_t MinQty() const
	{
		return GetOrCreateAggregateValues<unusedOnly>()->minQty;
	}
	
	int64_t PartialAvgPrice(int64_t volume, int64_t *numOfQuotes = nullptr) const;
	
	void InvalidateAggregateValues(bool onlyUsedStateChanged = false);
	
	bool HasQuotes() const
	{
		std::shared_lock lock { m_quotes.Mutex() };
		return !m_quotes->empty();
	}
	
	size_t QuoteCount() const
	{
		std::shared_lock lock { m_quotes.Mutex() };
		return m_quotes->size();
	}

private:
	struct AggregateValues
	{
		using Ptr = std::shared_ptr<const AggregateValues>;
		
		/*! \brief Minimum price of all quotes of the group */
		int64_t minPrice { 0 };
		
		/*! \brief Maximum price of all quotes of the group */
		int64_t maxPrice { 0 };
		
		/*! \brief Average price (Weighted average of all the quotes of the group) */
		int64_t avgPrice { 0 };
		
		/*! \brief Maximum volume of all quotes of the group, in 100ths of a unit */
		int64_t maxVolume { 0 };
		
		/*! \brief Total volume (sum of volumes of all quotes of the group) */
		int64_t totalVolume { 0 };
		
		/*! \brief Minimum quantity (minimum quantity to be ordered) */
		int64_t minQty { 0 };
		
	};
	
	mutable AggregateValues::Ptr m_aggregateValues[2]; //!< Array of shared ptrs to @a AggregateValues
	
	/*! \brief Const Reference to the vector of quotes */
	UTILS::SharedLockable<QuoteVector> m_quotes;
	
	/**
	 * Calculates the index for accessing @a m_aggregateValues from a boolean value.
	 *
	 * This is a @a constexpr function, i.e. it is executed at compile time if
	 * a constant value is passed to @a unusedOnly.
	 *
	 * @param unusedOnly @a true ... accept only unused quotes, @a false ... accept all quotes
	 * @return Index of for accessing @a m_aggregateValues
	 */
	constexpr static size_t AggIdx(bool unusedOnly) { return unusedOnly ? 0 : 1; }
	
	/**
	 * Retrieves or creates a structure of aggregate values
	 *
	 * @tparam unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Shared pointer to the const structure
	 */
	template <bool unusedOnly>
	AggregateValues::Ptr GetOrCreateAggregateValues() const
	{
		AggregateValues::Ptr av { std::atomic_load(&m_aggregateValues[AggIdx(unusedOnly)]) };
		if (!av)
		{
			std::shared_lock lock { m_quotes.Mutex() };
			av = CalculateAggregateValues(*m_quotes, unusedOnly);
			lock.unlock();
			std::atomic_store(&m_aggregateValues[AggIdx(unusedOnly)], av);
		}
		return av;
	}
	
	/**
	 * Creates a structure of aggregate values
	 *
	 * Static function
	 *
	 * @param quoteVector Quote vector to be used to calculate the aggregate values
	 * @param unusedOnly @a true ... consider of unused quotes, @a false ... consider all quotes
	 * @return Shared pointer to the const structure
	 */
	static AggregateValues::Ptr CalculateAggregateValues(const QuoteVector &quoteVector, bool unusedOnly);
};

} // namespace BOOK
} // namespace CORE

#endif //COROUT_QUOTE_H
