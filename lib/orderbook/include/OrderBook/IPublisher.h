#pragma once

#include "Quote.h"

namespace ORDERBOOK {
    class IPublisher
    {
    public:
        virtual ~IPublisher() = default;
        virtual void Publish(UTILS::CurrencyPair cp, int entry, int level, CORE::BOOK::Quote::Ptr quote, bool bid) = 0;
    };
}
