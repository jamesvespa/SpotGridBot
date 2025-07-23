//
// Created by jameson 11.06.18.
//

#include "Utils/MessageData.h"

namespace UTILS {
namespace MESSAGE {
    ObjectPool<QuoteData, QuoteDataPool::POOL_SIZE> QuoteDataPool::op;
    ObjectPool<MDSnapshotData, MDSnapshotDataPool::POOL_SIZE> MDSnapshotDataPool::op;

    std::atomic<int> IMessageAdapter::m_stSeqNumGenerator;
} // namespace MESSAGE
} // namespace UTILS

