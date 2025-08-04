#include "Utils/Logging.h"
#include "Utils/ErrorHandler.h"
#include "Utils/Result.h"
#include "Utils/BackgroundWorker.hpp"
#include "MessageProcessor.h"

#include "JSONDocument.h"

namespace CORE {
namespace CRYPTO {

MessageProcessor::MessageProcessor()
		: Logging("MessageProcessor")
{
}

MessageProcessor::~MessageProcessor()
{
	Stop();
}

std::string MessageProcessor::GetMessageType(const std::shared_ptr<CRYPTO::JSONDocument> message) const
{
	return m_messageTypeDetector ? m_messageTypeDetector(message) : "Message type detector not registered.";
}

void MessageProcessor::Register(TMessageTypeDetector messageTypeDetector)
{
	m_messageTypeDetector = messageTypeDetector;
}

UTILS::BoolResult MessageProcessor::Register(const std::string &msgType, const TMessageHandler handler)
{
	if (!handler)
	{
		return UTILS::BoolResult(false, "NULL message handler ignored");
	}
	
	return m_messageHandlers.emplace(msgType, handler).second ? true : UTILS::BoolResult(false,
																						 "Handler for message '%s' has been already registered. Ignored",
																						 msgType);
}

UTILS::BoolResult MessageProcessor::ProcessMessage(const std::shared_ptr<JSONDocument> message)
{
	if (!message)
	{
		return UTILS::BoolResult(false, "NULL message");
	}
	const auto msgType = GetMessageType(message);
	if (auto handler = FindMessageHandler(msgType))
	{
		Enqueue(message, handler);
		return true;
	}
	return UTILS::BoolResult(false, "Not supported message: '%s'", msgType);
}

TMessageHandler MessageProcessor::FindMessageHandler(const std::string &msgType) const
{
	auto iter = m_messageHandlers.find(msgType);
	return iter != m_messageHandlers.cend() ? iter->second : nullptr;
}

/*! \brief Returns a number of registered message handlers */
size_t MessageProcessor::Size() const
{
	return m_messageHandlers.size();
}

/*! \brief Starts message processor. It must be started before processing any messages */
void MessageProcessor::Start()
{
	m_messageQueue.Start("MessageProcessorQueue", [](const std::shared_ptr<JSONDocument> &message, const TMessageHandler handler)
	{
		if (message && handler)
		{
			handler(message);
		}
	});
}

/*! \brief Stops message processor */
void MessageProcessor::Stop()
{
	m_messageQueue.Stop(true);
}

UTILS::BoolResult MessageProcessor::Enqueue(const std::shared_ptr<JSONDocument> message, const TMessageHandler handler)
{
	if (!message)
	{
		return UTILS::BoolResult(false, "NULL message ignored");
	}
	if (!handler)
	{
		return UTILS::BoolResult(false, "NULL message handler ignored");
	}
	
	return m_messageQueue.Enqueue(message, handler);
}

}
}
