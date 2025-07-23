#pragma once

#include "JSONDocument.h"
#include <algorithm>
#include <memory>

namespace CORE {
namespace COINBASE {
    struct change
    {
        using Ptr = std::shared_ptr<change>;
        std::string side;
        std::string price;
        std::string size;
    };

    class L2Update
    {
    public:
        L2Update(const std::shared_ptr<CRYPTO::JSONDocument> msg) : m_json(msg)
        {
            auto changes = m_json->GetArray("changes");
            Poco::Dynamic::Array da = *changes;

            for (size_t i=0; i < changes->size(); i++)
            {
                m_changes.emplace_back(std::make_shared<COINBASE::change>());
                m_changes.back()->side = da[i][0].toString();
                m_changes.back()->price= da[i][1].toString();
                m_changes.back()->size = da[i][2].toString();
            }
            //Note we are reading the changes only, non need to read the full book...
        }

        const std::vector<change::Ptr>& GetChanges() const
        {
            return m_changes;
        }

    private:
        std::vector<change::Ptr> m_changes;
        const std::shared_ptr<CRYPTO::JSONDocument> m_json;
    };
} // ns COINBASE
} // ns CORE