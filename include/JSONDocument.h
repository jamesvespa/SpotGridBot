//
// Created by james on 11/06/22.
//

#pragma once

#include <iostream>
#include <string>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>
#include <Utils/Result.h>

namespace CORE {
namespace CRYPTO {

struct JSONError
{
	JSONError()
	{
	}
	JSONError(std::string s, long l):msg(s), code(l)
	{
	}
	
	std::string msg;
	long code;
};

class JSONDocument
{
public:
	explicit JSONDocument(const std::string &document)
	{
		m_parser.parse(document);
		auto result = m_parser.result();
		m_jsonObject = result.extract<Poco::JSON::Object::Ptr>();
	}
	
	template <typename T>
	T GetValue(const std::string &name) const
	{
		Poco::Dynamic::Var var = m_jsonObject->get(name); // Get the member Variable
		return var.isEmpty() ? T() : var.convert<T>();
	}
	
	Poco::JSON::Array::Ptr GetArray(const std::string &name) const
	{
		return m_jsonObject->getArray(name); // Get the member Variable;
	}
	
	/*! \brief returns subobject by name */
	Poco::JSON::Object::Ptr GetSubObject(const std::string &name) const
	{
		return m_jsonObject->get(name).extract<Poco::JSON::Object::Ptr>();
	}
	
	/*! \brief returns true if field exists */
	bool Has(const std::string &name) const
	{
		return m_jsonObject->has(name);
	}
	
	Poco::JSON::Object::Ptr GetJsonObject() const
	{
		return m_jsonObject;
	}

private:
	Poco::JSON::Parser m_parser;
	Poco::JSON::Object::Ptr m_jsonObject;
};


/*! \brief Parses Json document
* @param: json - json string
* @param: outError - optional error object
* @return: shared ptr to json doc or nullptr in error case
*  */
inline std::shared_ptr<JSONDocument> ParseJson(const std::string &json)
{
	try
	{
		return std::make_shared<JSONDocument>(json);
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	
	return nullptr;
}


/*! \brief creates formatted json message with code
* @example: {"code":3,"msg":"Invalid JSON: expected `,` or `]` at line 4 column 6"}
* */
inline std::string CreateJSONMessageWithCode(const std::string &msg, int code = 1)
{
	return UTILS::Format("{\"code\":%d,\"msg\":\"%s\"}", code, msg);
}


/*! \brief parser of error message
* @example: {"code":3,"msg":"Invalid JSON: expected `,` or `]` at line 4 column 6"}
* */
inline JSONError ParseJSONMessageWithCode(const std::shared_ptr<CRYPTO::JSONDocument> jd)
{
	return JSONError(jd->GetValue<std::string>("msg"), jd->GetValue<long>("code"));
}

}
}