/**
 * @file
 * @author      thomas <thomas.grim-schlink@bbf-it.at>
 * @date        2020-12-20
 * @copyright   (c) 2020 BBF-IT
 * @brief		cmd argument parser
*/

#ifndef MERCURYUTILS_XMLCONFIGREADER_H
#define MERCURYUTILS_XMLCONFIGREADER_H

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

#include "Utils/Logging.h"
#include "Utils/Result.h"

namespace UTILS
{

class XmlConfigReader : public Logging
{
public:
	explicit XmlConfigReader(const std::string &logName) : Logging(logName), m_workNode(nullptr)
	{ }
	~XmlConfigReader() = default;
	
	XmlConfigReader(XmlConfigReader const &) = delete;
	XmlConfigReader(XmlConfigReader &&) = delete;
	XmlConfigReader &operator=(XmlConfigReader const &) = delete;
	XmlConfigReader &operator=(XmlConfigReader &&) = delete;
	
	/**
	 * @brief		parse xml file and create Poco XML Document\n
	 * 				create Poco XML node as working node.
	 * @param[in] 	xmlFile file name of the xml file including path to the file
	 * @return		true on successful parsed, otherwise false with set ErrorMessage
	 */
	BoolResult ParseFile(const std::string &xmlFile);
	
	/**
	 * @brief		parse xml string and create Poco XML Document\n
	 * 				create Poco XML node as working node.
	 * @param[in] 	xmlPart xml document or part of a xml document as string
	 * @return		true on successful parsed, otherwise false with set ErrorMessage
	 */
	BoolResult ParseXmlPart(const std::string &xmlPart);
	
	/**
	 * @brief	returns the name of the current root element from XML document
	 * @return 	string with name of root / base element
	 */
	std::string BaseName() { return !m_document.isNull() ? m_document->documentElement()->nodeName() : ""; }

	/**
	 * @brief	returns the pointer of the current root element from XML document
	 * @return 	Poco AutoPtr to root / base element
	 */
	Poco::XML::AutoPtr<Poco::XML::Element> BaseElemPtr() { return !m_document.isNull() ? m_document->documentElement() : nullptr; }

	
	/**
	 * @brief	returns the name of the current / selected working element / node from XML document
	 * @return 	string with name of working element / node
	 */
	std::string WorkName() { return m_workNode ? m_workNode->nodeName() : ""; }

	/**
	 * @brief	returns the pointer of the current / selected working element / node from XML document
	 * @return 	Poco AutoPtr to working element / node
	 */
	Poco::XML::Node *WorkNodePtr() { return m_workNode ? m_workNode : nullptr; }
	
	
	/**
	 * @brief	looks for the  full path of the current working node
	 * @return 	full path of working node
	 */
	std::string WorkPath();
	
	/**
	 * @brief		switch the work base to new node
	 * @param[in]	newBase node pointer of the new work base
	 * @return 		true if ok, otherwise false with set ErrorMessage
	 */
	virtual BoolResult SwitchWorkBase(Poco::XML::Node *newBase);
	
	/**
	 * @brief		switch the work base to new node \n
	 * 				new node defined by path to new work base, if fromBase set newBase path must start on base node, otherwise on current work node
	 * @param[in]	newBase path of new work base separator "/"
	 * paramm[in]	fromBase if true path start base
	 * @return 		true if ok, otherwise false with set ErrorMessage
	 */
	virtual BoolResult SwitchWorkBase(const std::string &newBase, bool fromBase);

	BoolResult SwitchWorkBase(const std::string &newBase) { return SwitchWorkBase(newBase, true); }

	/**
	 * @brief		switch the work base to new node
	 * @param[in]	toParent switch to parent
	 * paramm[in]	toBase switch to base
	 * @return 		true if ok, otherwise false with set ErrorMessage
	 */
	BoolResult SwitchWorkBase(bool toParent, bool toBase);
	
	/**
	 * @brief		returns all attributes as name / value map from current working element / node
	 * @param[out] 	attributes filled map with attribute name / value
	 * @return 		true true if attributes found
	 */
	BoolResult AttrFromElem(std::map<std::string, std::string> &attributes);
	
	/**
	 * @brief		returns all attributes as name / value map from element / node with given base name
	 * @param[out] 	attributes filled map with attribute name / value
	 * @param[in]	base element / node name use looking for attributes
	 * @return 		true true if attributes found
	 */
	BoolResult AttrFromElem(std::map<std::string, std::string> &attributes, const std::string &base);
	
	/**
	 * @brief		returns all attributes as name / value map from element / node from given node pointer
	 * @param[out] 	attributes filled map with attribute name / value
	 * @param[in]	base element / node node pointer
	 * @return 		true true if attributes found
	 */
	BoolResult AttrFromElem(std::map<std::string, std::string> &attributes, Poco::XML::Node *base);
	
	
	/**
	 * @brief		looking for attribute value from given attribute name inside given element node
	 * @param[in]	attr attribute name look for
	 * @param[in]	node node pointer to use
	 * @return		value from attribute
	 */
	virtual std::string Attribute(std::string attr, Poco::XML::Node *node);

	std::string Attribute(std::string attr) { return Attribute(std::move(attr), nullptr); }

	/**
	 * @brief		look for node and return pointer to it
	 * @param[in]	newBase path to searched result pointer, separator "/"
	 * @param[in]	base start node, if nullptr start on base.
	 * @return		node pointer
	 */
	Poco::XML::Node *NodePtr(std::string newBase, Poco::XML::Node *base = nullptr);

	/**
	 * @brief		look for nodes and return vector of pointer to it
	 * @param[in]	newBase path to searched result pointer, separator "/"
	 * @param[in]	base start node, if nullptr start on base.
	 * @return		node pointers
	 */
	std::vector<Poco::XML::Node *> NodePtrs(std::string newBase, Poco::XML::Node *base = nullptr);

protected:
	
	/**
	 * @brief		write log message from BoolResult message
	 * @param[in]	result
	 */
	void Log(const BoolResult &result);

	/**
	 * @brief		split string at '/' return elements as map position, name
	 * @param[in] 	input string to split
	 * @return		map position, name
	 */
	static std::map<int, std::string> SplitInput(const std::string &input);
	
private:
	Poco::XML::AutoPtr<Poco::XML::Document> m_document;		//!< hold the whole parsed xml document
	Poco::XML::Node *m_workNode;							//!< pointer to current used work node
	std::map<Poco::XML::Node *, std::unique_ptr<std::map<std::string, std::string>>> m_nodeAttrMap;		//!< map with attributes inside node
};

}

#endif //MERCURYUTILS_XMLCONFIGREADER_H
