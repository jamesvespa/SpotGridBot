//
// Created by james on 10/08/22.
//
#include "Tools.h"
#include <openssl/hmac.h>

namespace CORE {
    namespace CRYPTO {
        namespace TOOLS {

        UTILS::MESSAGE::ExecutionReportData CreateEmptyExecutionReportData()
        {
            return UTILS::MESSAGE::ExecutionReportData(
                "",                         // std::string orderId,
                "",                         // std::string clOrdID,
                ORDTYPE_MARKET,             // char ordType,
                UTILS::CurrencyPair(),      // UTILS::CurrencyPair instrument,
                UTILS::Currency(),          // UTILS::Currency currency,
                "",                         // std::string execID,
                "",                         // std::string settlDate,
                EXECTYPE_NONE,              // char execType,
                ORDSTATUS_NOTSENT,          // char ordStatus,
                UTILS::Side::INVALID,       // char side,
                0,                          // int64_t orderQty,
                0,                          // int64_t orderPx,
                0,                          // int64_t lastQty,
                0,                          // int64_t lastPx,
                0,                          // int64_t leavesQty,
                0,                          // int64_t cumQty,
                0,                          // int64_t avgPx,
                "",                         // std::string text,
                "",                         // std::string account,
                "",                         // std::string orderText,
                "",                         // std::string username,
                "",                         // std::string quoteId,
                UTILS::TimeInForce::INVALID, // char tif
                ""	//custompb
            );
        }

        //------------------------------------------------------------------------------
        /*! \brief Encrypts data by key using sha256 encryption
        * @param key: the key (in crypto it would be a Secret key)
        * @param data: data to be encrypted
        * @return: encrypted data string
        * */
        std::string EncryptWithHMAC(const std::string& key, const std::string& data)
        {
            const auto* result = HMAC(EVP_sha256(), key.c_str(), key.size(),
                                      reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
                                      NULL, NULL);

            const int result_len = 32;
            const size_t encrSize = 64;
            char res_hexstring[encrSize];
            for (int i = 0; i < result_len; ++i)
            {
                sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
            }

            std::string signature;
            for (size_t i = 0; i < encrSize; ++i)
            {
                signature += res_hexstring[i];
            }
            return signature;
        }

        // iso8601 - time format e.g. 2022-02-19T20:05:22Z
        // return utc
        uint64_t from_iso_string(const std::string& dateTime)
        {
            struct tm t{0};
            if (sscanf(dateTime.c_str(), "%d-%d-%dT%d:%dZ", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min) != 5)
            {
                std::cout << "Invalid date format " << dateTime << std::endl;
                return 0;
            }

            t.tm_year = t.tm_year - 1900;
            t.tm_mon = t.tm_mon - 1;
            time_t localTime = mktime(&t);
            time_t utcTime = localTime - timezone;

            return std::chrono::system_clock::from_time_t(utcTime).time_since_epoch().count();
        }

        //ms time, return iso8601 - time format e.g. 2022-02-19T20:05:22Z
        const std::string to_iso_string(const int64_t ms)
        {
            auto epoch = std::chrono::time_point<std::chrono::high_resolution_clock>();
            auto since_epoch = std::chrono::milliseconds(ms);
            auto timestamp_ms = epoch + since_epoch;

            std::time_t time = std::chrono::system_clock::to_time_t(timestamp_ms);
            char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
            std::strftime(std::data(timeString), std::size(timeString), "%FT%TZ", std::gmtime(&time));

            return std::string(timeString);
        }
    }
  }
}

