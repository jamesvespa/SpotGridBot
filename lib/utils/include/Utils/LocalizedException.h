#ifndef LOCALIZEDEXCEPTION_H
#define LOCALIZEDEXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

namespace UTILS
{

/**
 * @brief Exception which is able to inform about from which function it was thrown in its message.
 */
class LocalizedException : public std::exception {
private:
    std::string fullMessage;

public:
    /**
     * @param functionName Name of the function/method from which the exception is thrown. Must be supplied by the caller.
     *                     Usually should be set to the value of `__func__` standard macro.
     * @param errorMessage Message describing the reason why exception was thrown.
     *
     */
    LocalizedException(const std::string& functionName, const std::string& errorMessage) {
        std::ostringstream oss;
        oss << "[ exception thrown from " << functionName << " ] - " << errorMessage;
        fullMessage = oss.str();
    }

    const char* what() const noexcept override {
        return fullMessage.c_str();
    }
};

}
#endif //LOCALIZEDEXCEPTION_H
