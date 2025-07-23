//
// Created by james on 1/12/24.
//

#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <cstddef>
#include <atomic>
#include <memory>
#include <iostream>
#include <thread>
#include <mutex>

//#define DEBUG_OBJECT_DELETER

namespace UTILS {
constexpr bool OBJECT_POOL_VERBOSE = false;

/*
 * Pool use cases:
 *  1) plain buffers a la EFVI - no constructors involved
 *  2) pool of objects - constructors and destructors are called
 *
 *  Memory needs to be returned to the pool without intervention from
 *  the user, thus what gets returned must be some kind of smart ptr.
 *
 *  An object pool is a specialization of a buffer pool.
 *  It would be nice to have the dynamicism available in a class that
 *  could be used for multiple inheritance.  The dynamic aspect is
 *  an enhancement that is not necessary for the first iteration.
 */

// BufferPool - allocates a slab of memory and dispenses chunks using a unique_ptr with custom deleter to
// return chunks to the pool.  Template arguments are a buffer/chunk size and number of chunks to create.
// This class is supposed to stay simple.  Dynamic resizing, using the memory to construct objects in place, etc.
// Are handled in derived or containing classes.  If the Custom deleter is a level removed, might be able
// to pass back a regular unique_ptr.
// Anywhere we use placement new we need to call the dtor explicitly.
// This data structure works like a stack.  Objects are added to the free list
// by pushing them on the top via pointer manipulation.  New objects are created
// in the most recently freed memory, which is what will be at the top of the stack.
template <uint64_t BufferSize, uint64_t BufferCount>
class BufferPool
{
    static_assert(BufferCount > 0, "BufferCount must be greater than 0.");
    static_assert(BufferSize > 0, "BufferSize must be greater than 0.");
public:
    class CustomDeleter
    {
    public:
        CustomDeleter() = default;
        explicit CustomDeleter(BufferPool* pool) : m_pool(pool) {}

        void operator()(unsigned char* buffer)
        {
            if(OBJECT_POOL_VERBOSE) {
                std::cout << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ <<
                "]\tCustom deleter got called for [" << static_cast<void*>(buffer) << "]!!!!!" << std::endl;
            }

            if(m_pool)
            {
                m_pool->releaseBuffer(buffer);
            }
        }

    private:
        BufferPool* m_pool{nullptr};
    };

    using BufferPtr = std::unique_ptr<unsigned char, CustomDeleter>;

    BufferPool()
    {
        constexpr auto bytes_to_allocate = BufferSize * BufferCount;

#if defined(DEBUG_BUFFER_POOL)
        std::cout << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ <<
        "]\t" << BufferSize << " bytes per buffer being allocated for " << BufferCount << " object pool: " << bytes_to_allocate << std::endl;
#endif

        m_pool = static_cast<unsigned char*>(malloc(bytes_to_allocate));
        std::lock_guard<std::mutex> lck(m_poolMutex);
        m_freeList = m_pool;
        initBuffers(m_pool);
    }

    ~BufferPool()
    {
#if defined(DEBUG_BUFFER_POOL)
            std::cout << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ <<
                "]\tPool memory being freed." << std::endl;
#endif

        free(m_pool);
    }

    // This is equivalent to popping a value off of a stack where what's popped is the pointer to the new buffer.
    auto getBuffer(std::thread::id tid)
    {
        return  BufferPtr(getRawBuffer(), this);
    }

    auto bufferSize() const { return BufferSize; }

    bool isExhausted() const { return m_buffersInUse >= m_capacity; }

protected:

    // This is equivalent to popping a value off of a stack where what's popped is a raw pointer to the new buffer.
    // This does not return a smart pointer because the ObjectPool provides its own with a customer deleter,
    // which is not possible here since there is no class type being contained.
    auto getRawBuffer(const char* caller, std::thread::id tid)
    {
        std::lock_guard<std::mutex> lck(m_poolMutex);
        if (m_buffersInUse >= m_capacity)
        {
            std::cerr << "Memory pool capacity of " << m_capacity << " objects is exhausted. Reverting to regular heap allocation.";
            return static_cast<unsigned char*>(malloc(BufferSize));
        }
        auto newBuffer = m_freeList;
        m_freeList = *reinterpret_cast<unsigned char**>(newBuffer);

#if defined(DEBUG_BUFFER_POOL)
        std::cout << "[tid: " << tid << "]\tNew buffer: [" << static_cast<void*>(newBuffer) << "]\tNew m_freeList head: [" << static_cast<void*>(m_freeList.load()) << "]";
        std::cout << "\tbuffersInUse: " << m_buffersInUse.load() << std::endl;
#endif

        ++m_buffersInUse;
        return  newBuffer;
    }


    // Derived classes can call this function in their own custom deleters if they need
    // to do something different for pool management, i.e.,
    // use raw pointers to control the lifecycle to release buffer in a callback.
    void releaseBuffer(const char* caller, unsigned char* buffer)
    {
        std::lock_guard<std::mutex> lck(m_poolMutex);
        *reinterpret_cast<unsigned char**>(buffer) = m_freeList;
        m_freeList = buffer;
        --m_buffersInUse;

#if defined(DEBUG_BUFFER_POOL)
        std::cout << "Next buffer: " << static_cast<void*>(reinterpret_cast<char**>(buffer)) << "\t"
            << "new m_freeList head: "
            << static_cast<void*>(m_freeList) << "\tbuffersInUse: " << m_buffersInUse.load()
            << std::endl;
#endif
    }


    // Make each item point to the next segment in the buffer since
    // that will be the next available slot at start up.
    // This function can be used by a derived class thatmakes the pool expandable.
    void initBuffers(unsigned char* blockStart)
    {
#if defined(DEBUG_BUFFER_POOL)
        std::cout << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ <<
        "]\tSetting up links for the pool." << std::endl;
#endif

        for(uint64_t offset = 0; offset < BufferCount ; ++offset)
        {
            auto curr = (blockStart + offset * BufferSize);
            auto next = curr + BufferSize;

            // take the memory of the char* (the buffer) and interpret it as a pointer value
            auto freeAddr = reinterpret_cast<unsigned char**>(curr);
            *freeAddr = next;

#if defined (DEBUG_POOL_INIT)
            // Debug statements to make sure the pool links are intialized correctly:
            std::cout << "offset: " << offset << "\t";
            std::cout << "curr: " << static_cast<void*>(curr) << "\t";
            std::cout << "next: " << static_cast<void*>(next) << std::endl;
#endif
        }
    }

private:
    unsigned char* m_pool  {nullptr};
    unsigned char* m_freeList{nullptr};
    uint64_t m_buffersInUse{0};
    const uint64_t m_capacity {BufferCount};
    std::mutex m_poolMutex;
};

// DynamicBufferPool - BufferPool that expands dynamically once the originally allocated memory is exhausted.
// Adds a function to allocate additional slabs and connect them to the pool. Takes an initial size,
// and a max size to expand to.
class DynamicBufferPool //: public BufferPool
{

};


// Uses the BufferPool, passing in sizeof(T) as the argument for buffer/chunk size.
// Adds typing on top of the BufferPool by getting the memory buffers and
// passing them to placement new for calling ctors gand getting a T*. The dtors must be handled
// explicity because of the use of placement new, so a custom deleter for
// cleaning up objects before returning them to the pool is needed.
template <typename T, uint64_t PoolItemCount>
class ObjectPool : public BufferPool<sizeof(T), PoolItemCount>
{
    // Because pointers to other free buffers are kept in the memory for T instances, T must be
    // at least as large as the pointer type that will be used to manage the free list.
    static_assert(sizeof(T) >= sizeof(unsigned char*), "sizeof(T) must be >= sizeof(unsigned char*");
    static_assert(PoolItemCount > 0, "PoolSize must be greater than 0.");

    // There needs to be a different custom deleter from BufferPool that will call the destructor of T.
    class ObjectDeleter
    {
    public:
        ObjectDeleter() = default;
        ObjectDeleter(ObjectPool* pool) : m_pool(pool) {}

        void operator()(T* item)
        {
#if defined DEBUG_OBJECT_DELETER
                std::cout << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ <<
                "]\tCustom deleter got called for [" << item << "]!!!!!" << std::endl;
#endif

            item->~T();
            m_pool->releaseBuffer(__PRETTY_FUNCTION__, reinterpret_cast<unsigned char*>(item));
        }

    private:
        ObjectPool* m_pool{nullptr};
    };

public:
    using ItemPtr = std::unique_ptr<T,  ObjectDeleter>;

    ObjectPool() = default;

    // This is equivalent to popping a value off of a stack where what's popped is the pointer to the new object.
    template <typename ...Args>
    ItemPtr getObject(std::thread::id tid, Args &&...args)
    {
        if (!this->isExhausted())
        {
            auto newBuffer = this->getRawBuffer(tid);
            auto newItemPtr = new(newBuffer) T(std::forward<Args>(args)...);
            return  ItemPtr(newItemPtr, this);
        }

        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // This is equivalent to popping a value off of a stack where what's popped is the pointer to the new object.
    template <typename ...Args>
    ItemPtr getObject(const char* caller, std::thread::id tid, Args &&...args)
    {
        auto newBuffer = this->getRawBuffer(caller, tid);
        auto newItemPtr = new(newBuffer) T(std::forward<Args>(args)...);
        return  ItemPtr(newItemPtr, this);
    }

    auto maxObjectSize() { return this->bufferSize(); }
};

// CallbackPool - Use when passing a buffer to an API that returns it via a callback (e.g., zmq).
// The buffer can then be explicitly returned to the pool.
template<size_t chunk_size, size_t chunk_count>
class CallbackPool// : public BufferPool<chunk_size, chunk_count>
{
    /**** NOT IMPLEMENTED ****/

public:
    // This provides a means for consumers of the class to return the memory to the pool when the callback is invoked.
//    void returnBufferToPool(unsigned char* buffer){ this->getBufferPool().releaseBuffer(buffer); }
};

};

#endif //OBJECTPOOL_H
