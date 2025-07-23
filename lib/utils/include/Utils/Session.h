//
// Created by james on 04/05/22.
//
#pragma once

#include "Utils/MessageData.h"

namespace UTILS {
	/** @brief Session interface */
    class ISession {
    public:
        virtual ~ISession() = default;

        // Creating all supported messages:
        //---------------------------------
        virtual std::unique_ptr<MESSAGE::IMessageAdapter> CreateNewOrderSingle(MESSAGE::NewOrderSingleData::Ptr data) const = 0;
        //...TODO: the list of supported messages will be expanded

        /**
         * Sends message adapter
         * @param msg - message adapter
         * @param custom_seqnum
         * @param no_increment
         */
        virtual bool SendMessage(const MESSAGE::IMessageAdapter& msg, unsigned custom_seqnum = 0, bool no_increment = false) = 0;
    };

} // ns UTILS
