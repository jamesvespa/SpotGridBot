//
// Created by james on 29/09/2020.
//

#ifndef QUOTEGROUP_H
#define QUOTEGROUP_H

#include <memory>
#include "Quote.h"

using namespace ORDERBOOK;

namespace ORDERBOOK
{
/*! \brief This class defines a group of quotes (e.g. all quotes of one level)
 * */
        class QuoteGroup
        {
        public:
            /*! \brief  Shared pointer to a quote group */
            using Ptr = std::shared_ptr<QuoteGroup>;
            /*! \brief  Vector of shared pointers to a quote group */
            using QuoteVector = std::vector<CORE::BOOK::Quote::Ptr>;
        
            QuoteGroup()
                    : m_minPrice(0), m_maxPrice(0), m_avgPrice(0), m_maxVolume(0), m_totalVolume(0), m_minQty(0)
            {
            }
    
            explicit QuoteGroup(const QuoteVector &srcVec)
                    : QuoteGroup()
            {
                for (const auto &q : srcVec)
                {
                    m_quotes->emplace_back(q);
                }
                RecalcAggregateValues();
            }
            
        
            /*! \brief Creates a new quote group
             *
             * Static function.
             *
             * @return Pointer to the new quote group
             * */
            static Ptr Create() { return std::make_shared<QuoteGroup>(); }
    
            void AddQuote(const CORE::BOOK::Quote::Ptr &q)
            {
                {
                    std::unique_lock lock { m_quotes };
                    m_quotes->push_back(q);
                }
                AdjustAggregateValues(q);
            }

            /*! \brief Adds a vector of quotes to the quote group
             *
             * Adds a vector of quotes to the quote group and updates min price, max price, average
             * price, and total volume of the quote group.
             *
             * @param vec     Vector of quotes to be added to the quote group
             * */
            void AddQuotes(const QuoteGroup::Ptr &quoteGroup)
            {
                {
                    std::unique_lock lock { m_quotes };
                    quoteGroup->ForEachQuote([this](const CORE::BOOK::Quote::Ptr &q)
                                             {
                                                 m_quotes->emplace_back(q);
                                             });
                }
                RecalcAggregateValues();
            }
        
            void GetQuotes(QuoteVector &vec) const
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
            int64_t PartialAvgPrice(int64_t volume, int64_t *numOfQuotes = nullptr) const
            {
                int64_t result { 0 }, aggregateVolume { 0 }, quoteCount { 0 };
        
                if (volume > 0)
                {
                    try
                    {
                        std::shared_lock lock { m_quotes.Mutex() };
                        for (const auto q : *m_quotes)
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
                for (const auto &q : *m_quotes)
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
                for (const auto &q : quoteVec)
                {
                    action(q);
                }
            }
        
            template <typename F>
            CORE::BOOK::Quote::Ptr FindFirstQuote(F cond) const
            {
                std::shared_lock lock { m_quotes.Mutex() };
                for (const auto &q : *m_quotes)
                {
                    if (cond(q))
                    {
                        return q;
                    }
                }
                return nullptr;
            }
        
            /*! \brief Checks whether of quotes of the group are of the same price
             *
             * @return \a true if all quotes have the same price
             * */
            bool SinglePrice() const { return MinPrice() == MaxPrice(); }
        
            /*! \brief Minimum price of all quotes of the group, in cpips */
            int64_t MinPrice() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_minPrice;
            }
        
            /*! \brief Maximum price of all quotes of the group, in cpips */
            int64_t MaxPrice() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_maxPrice;
            }
        
            /*! \brief Average price (Weighted average of all the quotes of the group, in cpips) */
            int64_t AvgPrice() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_avgPrice;
            }
        
            /*! \brief Maximum volume of all quotes of the group, in 100ths of a unit */
            int64_t MaxVolume() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_maxVolume;
            }
        
            /*! \brief Total volume (sum of volumes of all quotes of the group, in 100ths of a unit) */
            int64_t TotalVolume() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_totalVolume;
            }
        
            /*! \brief Minimum quantity to be ordered (in 100ths of a unit) */
            int64_t MinQty() const
            {
                std::lock_guard lock { m_mtxAggregateValues };
                return m_minQty;
            }
    
            void RecalcAggregateValues()
            {
                std::lock_guard lockValues { m_mtxAggregateValues };
                m_totalVolume = m_maxVolume = m_minQty = m_minPrice = m_maxPrice = m_avgPrice = 0;
                std::shared_lock lockQuotes { m_quotes };
                for (const auto q : *m_quotes)
                {
                    AdjustAggregateValues(q, false);
                }
            }
        
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
            mutable std::mutex m_mtxAggregateValues;
        
            /*! \brief Minimum price of all quotes of the group */
            int64_t m_minPrice;
        
            /*! \brief Maximum price of all quotes of the group */
            int64_t m_maxPrice;
        
            /*! \brief Average price (Weighted average of all the quotes of the group) */
            int64_t m_avgPrice;
        
            /*! \brief Maximum volume of all quotes of the group, in 100ths of a unit */
            int64_t m_maxVolume;
        
            /*! \brief Total volume (sum of volumes of all quotes of the group) */
            int64_t m_totalVolume;
        
            /*! \brief Minimum quantity (minimum quantity to be ordered) */
            int64_t m_minQty;
        
            /*! \brief Const Reference to the vector of quotes */
            UTILS::SharedLockable<QuoteVector> m_quotes;
        
            void AdjustAggregateValues(const CORE::BOOK::Quote::Ptr &q, bool doLock = true)
            {
                if (q && !q->Used())
                {
                    std::unique_lock lock { m_mtxAggregateValues, std::defer_lock };
                    if (doLock)
                    {
                        lock.lock();
                    }
                    if (m_totalVolume == 0 || m_minPrice > q->Price())
                    {
                        m_minPrice = q->Price();
                    }
                    if (m_totalVolume == 0 || m_maxPrice < q->Price())
                    {
                        m_maxPrice = q->Price();
                    }
                    if (m_maxVolume < q->Volume())
                    {
                        m_maxVolume = q->Volume();
                    }
                    m_totalVolume += q->Volume();
                    if (m_minPrice == m_maxPrice)
                    {
                        m_avgPrice = m_minPrice;
                    }
                    else
                    {
                        m_avgPrice += (q->Price() - m_avgPrice) * q->Volume() / m_totalVolume;
                    }
                    m_minQty = std::max(m_minQty, q->MinQty());
                    if (doLock)
                    {
                        lock.unlock();
                    }
                }
            }
        };
    }



#endif //QUOTEGROUP_H
