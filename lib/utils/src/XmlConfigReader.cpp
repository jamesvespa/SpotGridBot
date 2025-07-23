/**
 * @file
 * @author      thomas <thomas.grim-schlink@bbf-it.at>
 * @date        2020-12-20
 * @version     2020.12.20		initial version
 * @copyright   (c) 2020 BBF-IT
 * @brief		unittest for cmd argument parser
*/

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/TreeWalker.h>

#include <Poco/SAX/InputSource.h>

#include <Poco/Logger.h>

#include "Utils/XmlConfigReader.h"

namespace UTILS
{

BoolResult XmlConfigReader::ParseFile(const std::string &xmlFile)
{
	BoolResult result { true };
	if (m_document)
	{
		m_document.reset();
		m_nodeAttrMap.clear();
	}
	
	std::ifstream in(xmlFile);
	if (in.good())
	{
		try
		{
			Poco::XML::InputSource src(in);
			Poco::XML::DOMParser parser;
			m_document = parser.parse(&src);
			m_workNode = m_document->firstChild();
		}
		catch (Poco::Exception &e)
		{
			result = BoolResult { false, e.displayText() };
		}
	}
	else
	{
		result = BoolResult { false, "ParseFile() - file('" + xmlFile + "') not exist" };
	}
	
	Log(result);
	return result;
}

BoolResult XmlConfigReader::ParseXmlPart(const std::string &xmlPart)
{
	BoolResult result { true };
	if (m_document)
	{
		m_document.reset();
		m_nodeAttrMap.clear();
	}

	try
	{
		Poco::XML::DOMParser parser;
		m_document = parser.parseString(xmlPart);
		m_workNode = m_document->firstChild();
	}
	catch (Poco::Exception &e)
	{
		result = BoolResult { false, e.displayText() };
	}

	Log(result);
	return result;
}

void XmlConfigReader::Log(const BoolResult &result)
{
	if (static_cast<bool>(result))
	{
		if (!result.ErrorMessage().empty())
		{
			logger().trace(result.ErrorMessage());
		}
	}
	else
	{
		logger().error(result.ErrorMessage());
	}
}

BoolResult XmlConfigReader::SwitchWorkBase(const std::string &newBase, bool fromBase)
{
	BoolResult result;
	if (newBase.empty())
	{
		result =  { false, "SwitchWorkBase() - newBase empty" };
	}
	else if(m_document.isNull() && fromBase)
	{
		result =  { false, "SwitchWorkBase() - no parsed document" };
	}
	else if (nullptr == m_workNode && !fromBase)
	{
		result =  { false, "SwitchWorkBase() - no valid work node" };
	}
	else
	{
		result = BoolResult { false, "SwitchWorkBase() - " + newBase + " not found, or more then one element exists!" };
		Poco::XML::Node *newWorkNode = nullptr;
		
		if (fromBase)
		{
			newWorkNode = NodePtr(newBase, m_document->firstChild());
		}
		else
		{
			newWorkNode = NodePtr(newBase, m_workNode->firstChild());
		}
		
		if (nullptr != newWorkNode)
		{
			result = BoolResult { true, "SwitchWorkBase() - switched to " + newBase };
			m_workNode = newWorkNode;
		}
	}

	return result;
}

BoolResult XmlConfigReader::SwitchWorkBase(Poco::XML::Node *newBase)
{
	BoolResult result { true, "SwitchWorkBase()" };
	if (nullptr == newBase)
	{
		result =  { false, "SwitchWorkBase() - newBase invalid!" };
	}
	else
	{
		m_workNode = newBase;
	}
	
	return result;
}

BoolResult XmlConfigReader::SwitchWorkBase(bool toParent, bool toBase)
{
	BoolResult result { true, "SwitchWorkBase()" };
	if ((toParent && toBase) || (!toParent && !toBase))
	{
		result = BoolResult { false, "SwitchWorkBase() - wrong input" };
	}
	else if (m_document.isNull() || nullptr == m_workNode)
	{
		result =  { false, "SwitchWorkBase() - no parsed document or newBase invalid!" };
	}
	else if (toParent)
	{
		if (m_workNode->parentNode() && m_workNode->parentNode()->nodeType() == Poco::XML::NodeFilter::SHOW_ELEMENT)
		{
			m_workNode = m_workNode->parentNode();
		}
		else
		{
			result = BoolResult { false, "SwitchWorkBase() - no parent exist" };
		}
	}
	else
	{
		m_workNode = m_document->firstChild();
	}
	return result;
}

BoolResult XmlConfigReader::AttrFromElem(std::map<std::string, std::string> &attributes)
{
	return AttrFromElem(attributes, m_workNode);
}

BoolResult XmlConfigReader::AttrFromElem(std::map<std::string, std::string> &attributes, const std::string &base)
{
	Poco::XML::Node *workNode = nullptr;
	
	if (base.empty() || nullptr == (workNode = NodePtr(base, m_document->firstChild())))
	{
		return BoolResult { false, "AttrFromElem() - base empty or not found!" };
	}
	
	return AttrFromElem(attributes, workNode);
}

BoolResult XmlConfigReader::AttrFromElem(std::map<std::string, std::string> &attributes, Poco::XML::Node *base)
{
	BoolResult result { false, "AttrFromElem() - no attributes exist!" };
	
	if (nullptr == base || !base->hasAttributes())
	{
		return result;
	}
	
	result = BoolResult { true, "AttrFromElem()" };
	
	if (Poco::XML::NamedNodeMap *attrs { base->attributes() }; attrs)
	{
		for (unsigned long i = 0; i < attrs->length(); i++)
		{
			attributes.insert(std::make_pair(attrs->item(i)->nodeName(), attrs->item(i)->nodeValue()));
		}
		attrs->release();
	}
	m_nodeAttrMap.insert(std::make_pair(base, std::make_unique<std::map<std::string, std::string>>(attributes)));
	
	
	return result;
}

std::string XmlConfigReader::Attribute(std::string attr, Poco::XML::Node *node)
{
	std::string result;
	if (attr.empty())
	{
		return result;
	}
	
	if (nullptr == node)
	{
		node = m_workNode;
	}
	
	auto attribute = m_nodeAttrMap.find(node);
	if (attribute != m_nodeAttrMap.end())
	{
		auto value = attribute->second->find(attr);
		if (value != attribute->second->end())
		{
			result = value->second;
		}
	}
	else
	{
		std::map<std::string, std::string> attributes;
		if (static_cast<bool>(AttrFromElem(attributes, node)))
		{
			auto value = attributes.find(attr);
			if (value != attributes.end())
			{
				result = value->second;
			}
		}
	}
	
	return result;
}

std::map<int, std::string> XmlConfigReader::SplitInput(const std::string &input)
{
	std::map<int, std::string> result;
	std::string::size_type prev_pos = 0, pos = 0;
	int level = 0;
	
	while((pos = input.find('/', pos)) != std::string::npos)
	{
		std::string substring( input.substr(prev_pos, pos-prev_pos) );
		result.insert(std::pair(level, substring));
		++level;
		prev_pos = ++pos;
	}
	result.insert(std::pair(level, input.substr(prev_pos, pos-prev_pos)));
	
	return result;
}

Poco::XML::Node *XmlConfigReader::NodePtr(std::string newBase, Poco::XML::Node *base)
{
	if (newBase.empty() || (m_document.isNull() && nullptr == base))
	{
		return nullptr;
	}

	std::vector<Poco::XML::Node *> result { NodePtrs(std::move(newBase), base) };

	if (result.size() != 1)
	{
		return nullptr;
	}
	return result.at(0);
}

std::vector<Poco::XML::Node *> XmlConfigReader::NodePtrs(std::string newBase, Poco::XML::Node *base)
{
	std::vector<Poco::XML::Node *> result;
	if (newBase.empty() || (m_document.isNull() && nullptr == base))
	{
		return result;
	}

	auto baseMap = SplitInput(newBase);
	Poco::XML::Node *newNode = nullptr, *workNode = nullptr;

	if (nullptr == base)
	{
		workNode = m_document->firstChild();
	}
	else
	{
		if (nullptr == (workNode = base->firstChild()))
		{
			workNode = base->nextSibling();
		}
	}

	for (size_t i = 0; i < baseMap.size(); i++)
	{
		while (workNode)
		{
			if (workNode->nodeType() == Poco::XML::NodeFilter::SHOW_ELEMENT)
			{
				if (workNode->nodeName() == baseMap.at(i))
				{
					newNode = workNode;
					if ((i + 1) == baseMap.size())
					{
						result.push_back(workNode);
					}
				}
			}

			workNode = workNode->nextSibling();
		}
		if (nullptr != newNode && newNode->firstChild() != nullptr)
		{
			workNode = newNode->firstChild();
		}
	}

	return result;
}

std::string XmlConfigReader::WorkPath()
{
	std::string result { m_workNode->nodeName() };
	
	auto workNode = m_workNode;
	while (nullptr != workNode->parentNode())
	{
		workNode = workNode->parentNode();
		if (workNode->nodeType() == Poco::XML::NodeFilter::SHOW_ELEMENT)
		{
			result = workNode->nodeName() + "/" + result;
		}
	}
	
	return result;
}

}
