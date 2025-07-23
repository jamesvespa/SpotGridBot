#include "Utils/QuoteBuffer.h"

namespace UTILS {

ObjectPool<QuoteBuffer, QuoteBufferPool::POOL_SIZE> QuoteBufferPool::op;

// QuoteBufferPtr QuoteBufferPool::getQuoteBuffer(const char* caller, bool objectPoolDisabled, std::thread::id tid, short size, bool header)
QuoteBufferPtr QuoteBufferPool::getQuoteBuffer(const char* caller, std::thread::id tid, short size, bool header)
{
	if (size > op.maxObjectSize() || op.isExhausted())
	{
		// use a regular allocation
		return std::move(std::make_unique<QuoteBuffer>(size, header));
	}

	return std::move(op.getObject(caller, tid, size, header));
}

}
