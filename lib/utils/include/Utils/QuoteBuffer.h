#pragma once

#include <cstring> //memcpy
#include <memory>
#include "ObjectPool.h"

//Simple wrapper to ensure byte array is cleanup after publishing..

using byte = unsigned char;

namespace UTILS {

    // The memory allocated inside of this class is what needs to come from a pool.  Make it a buffer with a
    // fixed size.
    class QuoteBuffer
    {
        static constexpr size_t BUFFER_SIZE = 256;
    public:
        explicit QuoteBuffer(short size, bool header=false) : m_size(size), m_header(header)
        {
            // if (header)
            // {
            //     m_quoteBuffer = new byte[sizeof(short) + m_size];
            // }
            // else
            // {
            //     m_quoteBuffer = new byte[m_size];
            // }
        }

        ~QuoteBuffer()
        {
        }

        unsigned char* GetRawBuffer()
        {
            return m_quoteBuffer;
        }

        short GetSize() const
        {
            return m_size;
        }

        void SetRawBuffer(const unsigned char* buffer, short size)
        {
            if (m_header)
            {
                std::memcpy( m_quoteBuffer, &m_size, sizeof(short));
                std::memcpy( m_quoteBuffer + sizeof(short), buffer, sizeof(byte) * size);
            }
            else
            {
                std::memcpy( m_quoteBuffer, buffer, sizeof(byte) * size );
            }
        }

    private:
        unsigned char  m_quoteBuffer[BUFFER_SIZE]{0};
        short  m_size;
        bool   m_header;
        unsigned char* m_oversize{nullptr};
    };

    // This can't be const since the buffer is contained in the class
    // and needs to be writeable for some method calls.
    using QuoteBufferPtr = std::shared_ptr</*const*/ QuoteBuffer>;


    class QuoteBufferPool
    {
    public:
        static constexpr uint64_t POOL_SIZE = 1L << 20;
        // static QuoteBufferPtr getQuoteBuffer(const char* caller, bool objectPoolDisabled, std::thread::id tid, short size, bool header);
        static QuoteBufferPtr getQuoteBuffer(const char* caller, std::thread::id tid, short size, bool header);

    private:
        static ObjectPool<QuoteBuffer, POOL_SIZE> op;
    };

}
