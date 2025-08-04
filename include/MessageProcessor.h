#pragma once

#include "Utils/Logging.h"
#include "Utils/ErrorHandler.h"
#include "Utils/Result.h"
#include "Utils/BackgroundWorker.hpp"
#include "IConnection.h"

#include "JSONDocument.h"

namespace CORE {
namespace CRYPTO {

using TMessageTypeDetector = std::function<std::string(const std::shared_ptr<JSONDocument> message)>;
using TMessageHandler = std::function<void(const std::shared_ptr<CRYPTO::JSONDocument>)>;

/*! \brief Message processor for JSON messages */
class MessageProcessor final : UTILS::Logging, UTILS::ErrorHandler
{
public:
	MessageProcessor();
	
	~MessageProcessor();
	
	MessageProcessor(const MessageProcessor &) = delete;
	
	MessageProcessor &operator=(const MessageProcessor &) = delete;
	
	/*! \brief detects message type from JSON message
	* @param message: JSON document
	* @return: message type as string
	* */
	std::string GetMessageType(const std::shared_ptr<CRYPTO::JSONDocument> message) const;
	
	void Register(TMessageTypeDetector messageTypeDetector);
	
	UTILS::BoolResult Register(const std::string &msgType, const TMessageHandler handler);
	
	/*! \brief This method should be called to process incoming messages
	* @param message: JSON document
	* @return: true in success, error in error case
	* */
	UTILS::BoolResult ProcessMessage(const std::shared_ptr<JSONDocument> message);
	
	/*! \brief Returns message handler by type
	* @param msgType: message type
	* @return: message handler in success, nullptr if not found
	* */
	TMessageHandler FindMessageHandler(const std::string &msgType) const;
	
	/*! \brief Returns a number of registered message handlers */
	size_t Size() const;
	
	/*! \brief Starts message processor. It must be started before processing any messages */
	void Start();
	
	/*! \brief Stops message processor */
	void Stop();
	
	/*! \brief Explicitly enqueues a JSON document and a handler processing
	* This is convenient when caller knows the handler and wants process message directly in the queue
	* @param message: JSON document
	* @param handler: message handler
	* @return: true in success, error if message or handler is nullptr
	* */
	UTILS::BoolResult Enqueue(const std::shared_ptr<JSONDocument> message, const TMessageHandler handler);

private:
	UTILS::BackgroundWorker<const std::shared_ptr<JSONDocument>, TMessageHandler> m_messageQueue;
	using TMessageHandlers = std::unordered_map<std::string, std::function<void(const std::shared_ptr<JSONDocument>)>>;
	TMessageHandlers m_messageHandlers;
	TMessageTypeDetector m_messageTypeDetector;
};

}
}
