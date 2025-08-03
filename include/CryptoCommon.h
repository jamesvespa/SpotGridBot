//
// Created by james on 29/11/22.
// Contains common classes to support for crypto
//
#pragma once

#include "JSONDocument.h"

namespace CORE {
namespace CRYPTO {
struct Level
{
	Level() { }
	
	Level(std::string &p, std::string &s)
			: price(p), size(s) { }
	
	std::string price;
	std::string size;
};

// Price msg, can be snapshot or incremental update
class PriceMessage
{
public:
	using Levels = std::vector<std::shared_ptr<Level>>;
	Levels Bids;
	Levels Asks;
};

}
}