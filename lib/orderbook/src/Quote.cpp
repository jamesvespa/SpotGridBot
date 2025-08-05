//
// Created by helmut on 20.02.18.
//

#include <Poco/Format.h>

#include "OrderBook/Quote.h"
#include "Utils/FixDefs.h"

using namespace UTILS;

namespace CORE {
namespace BOOK {

ObjectPool<Quote, QuotePool::POOL_SIZE> QuotePool::op;

/*! \brief Set the \a used flag of this quote to \a true
 *
 * @return \a true if successful, \a false if \a used flag was already set
 * */
bool Quote::SetUsed()
{
	bool result { false };
	if (!Used())
	{
		bool oldValue { false };
		result = m_used.compare_exchange_strong(oldValue, true);
	}
	return result;
}

bool Quote::Valid(int64_t &successorSent, int64_t &successorReceived) const
{
	successorSent = m_successor.sent;
	successorReceived = m_successor.received;
	return Valid();
}

bool Quote::SetInvalid(Quote::Ptr successor)
{
	bool result { false };
	if (Valid() && successor.get() != this)
	{
		const int64_t tsSent { successor ? successor->SendingTime() : CurrentTimestamp() };
		const int64_t tsReceived { successor ? successor->ReceiptTime() : CurrentTimestamp() };
		if (tsSent != 0 && tsReceived != 0)
		{
			int64_t oldValue { 0 };
			result = m_successor.sent.compare_exchange_strong(oldValue, tsSent);
			if (result)
			{
				m_successor.received = tsReceived;
			}
		}
	}
	return result;
}

/*! \brief Field value as 64bit-integer.
 *
 * @param fld   Field (enum value)
 * @return Value of the field as 64bit-integer
 * */
int64_t Quote::GetInt(Field fld) const
{
	switch (fld)
	{
		case fldPrice:
			return Price();
		case fldVolume:
			return Volume();
		case fldMinQuantity:
			return MinQty();
		case fldKey:
			return Key();
		case fldRefKey:
			return RefKey();
		case fldSendingTime:
			return SendingTime();
		case fldReceiptTime:
			return ReceiptTime();
		case fldQuoteType:
			return QuoteType();
		case fldPositionNo:
			return PositionNo();
		case fldSeqNum:
			return SeqNum();
		default:
			throw Poco::Exception(Format("Invalid integer field type: %d", (int) fld));
	}
}

Quote::Field Quote::GetField(const std::string &fieldName)
{
	std::string upperName { UTILS::toupper(fieldName) };
	if (upperName == "LEVEL")
	{
		return Quote::fldLevel;
	}
	else if (upperName == "LEVELVOLUME")
	{
		return Quote::fldLevelVolume;
	}
	else if (upperName == "AGGREGATEVOLUME")
	{
		return Quote::fldAggregateVolume;
	}
	else if (upperName == "QUOTEID")
	{
		return Quote::fldQuoteID;
	}
	else if (upperName == "COMPID")
	{
		return Quote::fldCompID;
	}
	else if (upperName == "SESSION")
	{
		return Quote::fldSession;
	}
	else if (upperName == "PB")
	{
		return Quote::fldPb;
	}
	else if (upperName == "SEQNUM")
	{
		return Quote::fldSeqNum;
	}
	else if (upperName == "PRICE")
	{
		return Quote::fldPrice;
	}
	else if (upperName == "VOLUME")
	{
		return Quote::fldVolume;
	}
	else if (upperName == "MINQUANTITY")
	{
		return Quote::fldMinQuantity;
	}
	else if (upperName == "KEY")
	{
		return Quote::fldKey;
	}
	else if (upperName == "REFKEY")
	{
		return Quote::fldRefKey;
	}
	else if (upperName == "SENDINGTIME")
	{
		return Quote::fldSendingTime;
	}
	else if (upperName == "RECEIPTTIME")
	{
		return Quote::fldReceiptTime;
	}
	else if (upperName == "QUOTETYPE")
	{
		return Quote::fldQuoteType;
	}
	else if (upperName == "POSITIONNO")
	{
		return Quote::fldPositionNo;
	}
	else
	{
		return Quote::fldNone;
	}
}

/*! \brief Returns the base type of a field.
 *
 * Static function.
 *
 * @param fld   Field (enum value)
 * @return Base type of the field
 * */
Quote::FieldBaseType Quote::GetBaseType(Field fld)
{
	switch (fld)
	{
		case fldLevel:
		case fldLevelVolume:
		case fldAggregateVolume:
		case fldPrice:
		case fldVolume:
		case fldMinQuantity:
		case fldKey:
		case fldRefKey:
		case fldSendingTime:
		case fldReceiptTime:
		case fldQuoteType:
		case fldSeqNum:
			return fbtInt64;
		case fldQuoteID:
		case fldCompID:
		case fldPb:
		case fldSession:
			return fbtString;
		case fldNone:
		default:
			return fbtNone;
	}
}

/*! \brief Creates a string representation of a quote.
 *
 * @return String representation of the quote
 * */
std::string Quote::ToString(CurrencyPair cp) const
{
	std::stringstream ss;
	ss << "[";
	if (Used())
	{
		ss << "*";
	}
	if (!Valid())
	{
		ss << "!";
	}
	switch (QuoteType())
	{
		case QT_SNAPSHOT:
			ss << "S ";
			break;
		case QT_NEW:
			ss << "N ";
			break;
		case QT_UPDATE:
			ss << "U ";
			break;
		case QT_DELETE:
			ss << "D ";
			break;
		default:
			ss << "? ";
			break;
	}
	if (!QuoteID().empty())
	{
		ss << QuoteID() << ": ";
	}
	ss << "/" << SeqNum();
	ss << " " << Price() << " ";
	if (MinQty() != Volume())
	{
		ss << QtyToString(cp, MinQty()) << "-";
	}
	
	ss << QtyToString(cp, Volume()) << ", age: " << Format("%.3f+%.3f", SendingDelayMs(), AgeSinceReceiptMs());
	ss << "]";
	return ss.str();
}

/*! \brief QuoteGroup constructor
 * */
QuoteGroup::QuoteGroup() : m_aggregateValues { nullptr, nullptr }
{
}

QuoteGroup::QuoteGroup(const QuoteVector &srcVec)
		: QuoteGroup()
{
	for (const auto &q : srcVec)
	{
		m_quotes->emplace_back(q);
	}
}

/*! \brief Adds a quote to the quote group
 *
 * Adds a quote to the quote group and updates min price, max price, average
 * price, and total volume of the quote group.
 *
 * @param q     Quote to be added to the quote group
 * */
void QuoteGroup::AddQuote(const Quote::Ptr &q)
{
	{
		std::unique_lock lock { m_quotes.Mutex() };
		m_quotes->push_back(q);
	}
	InvalidateAggregateValues();
}

/*! \brief Removes a quote from the quote group
 *
 * Removes a quote from the quote group and updates min price, max price, average
 * price, and total volume of the quote group.
 *
 * @param q     Quote to be removed from the quote group
 * */
bool QuoteGroup::RemoveQuote(const CORE::BOOK::Quote::Ptr &q)
{
	{
		std::unique_lock lock { m_quotes };
		auto it = std::find(m_quotes->begin(),m_quotes->end(),q);
		if(it!=m_quotes->end())
		{
			m_quotes->erase(it);
		}
		else
		{
			return false;
		}
	}
	
	InvalidateAggregateValues();
	return true;
}

// static
QuoteGroup::AggregateValues::Ptr QuoteGroup::CalculateAggregateValues(const QuoteVector &quoteVector, bool unusedOnly)
{
	std::shared_ptr<AggregateValues> av { std::make_shared<AggregateValues>() };
	for (const auto &q : quoteVector)
	{
		if (!(unusedOnly && q->Used()))
		{
			if (av->totalVolume == 0 || av->minPrice > q->Price())
			{
				av->minPrice = q->Price();
			}
			av->maxPrice = std::max(av->maxPrice, q->Price());
			av->maxVolume = std::max(av->maxVolume, q->Volume());
			av->totalVolume += q->Volume();
			av->minQty = std::max(av->minQty, q->MinQty());
		}
	}
	if (av->totalVolume == 0 || av->minPrice == av->maxPrice) // trivial standard case: avgPrice = minPrice = maxPrice
	{
		av->avgPrice = av->minPrice;
	}
	else // minPrice =/= maxPrice -> avgPrice (weighed average of all prices) has to be calculated
	{
		int64_t sum { 0 };
		for (const auto &q : quoteVector)
		{
			if (!(unusedOnly && q->Used()))
			{
				sum += q->Price() * q->Volume();
			}
		}
		av->avgPrice = sum / av->totalVolume;
	}
	return av;
}

/*! \brief Adds a vector of quotes to the quote group
 *
 * Adds a vector of quotes to the quote group and updates min price, max price, average
 * price, and total volume of the quote group.
 *
 * @param vec     Vector of quotes to be added to the quote group
 * */
void QuoteGroup::AddQuotes(const QuoteGroup::Ptr &quoteGroup)
{
	{
		std::unique_lock lock { m_quotes.Mutex() };
		quoteGroup->ForEachQuote([this](const Quote::Ptr &q)
								 {
									 m_quotes->emplace_back(q);
								 });
	}
	InvalidateAggregateValues();
}

void QuoteGroup::InvalidateAggregateValues(bool onlyUsedStateChanged)
{
	std::atomic_store(&m_aggregateValues[AggIdx(true)], AggregateValues::Ptr(nullptr));
	if (!onlyUsedStateChanged)
	{
		std::atomic_store(&m_aggregateValues[AggIdx(false)], AggregateValues::Ptr(nullptr));
	}
}

/*! \brief Copies the quotes of the quote group into a vector
 *
 * Adds the quotes of the quote group to a given vector.
 *
 * @param vec     Vector to hold the quotes of the quote group
 * */
void QuoteGroup::GetQuotes(QuoteVector &vec) const
{
	std::shared_lock lock { m_quotes.Mutex() };
	for (const auto &q : *m_quotes)
	{
		vec.push_back(q);
	}
}


/*! \brief Returns the average price of the first quotes of a quote group
 *
 * This function iterates the quotes of this quote group until the required
 * volume is reached, and calculates the average price of the quotes.
 *
 * @param volume    Volume to be covered
 * @param numOfQuotes Pointer to int64_t to receive the number of quotes used
 * @return Average price of the quotes that cover the required volume
 * */
int64_t QuoteGroup::PartialAvgPrice(int64_t volume, int64_t *numOfQuotes) const
{
	int64_t result { 0 }, aggregateVolume { 0 }, quoteCount { 0 };
	
	if (volume > 0)
	{
		try
		{
			std::shared_lock lock { m_quotes.Mutex() };
			for (const auto &q : *m_quotes)
			{
				// consider only unused quotes that have a minQty lower that the required volume
				if (!q->Used() && volume >= q->MinQty())
				{
					int64_t currentVolume { std::min(q->Volume(), volume) };
					if (currentVolume > 0)
					{
						aggregateVolume += currentVolume;
						result += (q->Price() - result) * currentVolume / aggregateVolume;
						++quoteCount;
						volume -= currentVolume; // decrement required volume by current quote volume
						if (volume <= 0)
						{
							break; // all volume covered -> quit loop
						}
					}
				}
			}
		}
		catch (...)
		{
			result = 0;
			quoteCount = 0;
		}
	}
	if (numOfQuotes)
	{
		*numOfQuotes = quoteCount;
	}
	return result;
}

} // namespace BOOK
} // namespace CORE
