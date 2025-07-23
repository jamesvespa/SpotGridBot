//
// Created by james on 23/05/23.
//
#pragma once

#include "Utils/Result.h"
#include "JSONDocument.h"
#include "Config.h"

namespace CORE {
namespace CRYPTO {

class IConnection
{
public:
	virtual bool IsActive()=0;
	virtual void Start()=0;
	virtual void SetActive(bool)=0;
	virtual UTILS::BoolResult Connect()=0;
	virtual bool IsConnected() const=0;
	virtual void Disconnect()=0;
	virtual const Settings &GetSettings() const=0;
};

}
}