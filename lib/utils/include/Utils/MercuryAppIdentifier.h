#ifndef MERCURYAPPIDENTIFIER_H
#define MERCURYAPPIDENTIFIER_H

#include "LocalizedException.h"

namespace UTILS::MercuryAppIdentifier {

/**
 * @brief Used to identify from which mercury app some function/method is called.
 */
enum MercuryAppIdentifier {
    mercuryAppEngine, mercuryAppAdapters, mercuryAppBubbleWrap
};

/**
 * @brief Converts MercuryEngine to "ME", MercuryAdapters to "MA", and analogous for all other apps.
 *
 * @throws LocalizedException when provided identifier cannot be converted to std::string.
 */
inline std::string toStringAbbreviationUppercase(const MercuryAppIdentifier identifier) {
    switch (identifier) {
    case mercuryAppEngine:
        return "ME";
    case mercuryAppAdapters:
        return "MA";
    case mercuryAppBubbleWrap:
        return "MBW";
    default:
        throw LocalizedException(__func__, "Wrong value for `const MercuryAppIdentifier identifier` param. Maybe this function needs to be "
                                           "extended because new MercuryAppIdentifier value was added?");
    }
}

}

#endif //MERCURYAPPIDENTIFIER_H
