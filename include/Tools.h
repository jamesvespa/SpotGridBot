//
// Created by james on 10/08/22.
//
#pragma once

#include <string>
#include <Utils/MessageData.h>

namespace CORE {
namespace CRYPTO {
namespace TOOLS {

/*! \brief Encrypts data by key using sha256 encryption
* @param key: the key (in crypto it would be a Secret key)
* @param data: data to be encrypted
* @return: encrypted data string
* */
std::string EncryptWithHMAC(const std::string &key, const std::string &data);

/*! \brief Returns empty (default) ExecutionReportData */
UTILS::MESSAGE::ExecutionReportData CreateEmptyExecutionReportData();

// iso8601 - time format e.g. 2022-02-19T20:05:22Z return utc
uint64_t from_iso_string(const std::string &dateTime);

//ms time, return iso8601 - time format e.g. 2022-02-19T20:05:22Z
const std::string to_iso_string(const int64_t ms);

} // TOOLS
} // CRYPTO
} // CORE