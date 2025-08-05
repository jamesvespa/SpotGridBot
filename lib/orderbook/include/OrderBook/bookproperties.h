//
// Created by james on 25/09/2020.
//

#ifndef _BOOKPROPERTIES_H
#define _BOOKPROPERTIES_H

#define DEFAULT_BATCH_SIZE 250

#include <functional>

#include <thread>
#include <mutex>
#include <map>

#include "Utils/Utils.h"
#include "Utils/Logging.h"

#include "Poco/DOM/NodeIterator.h"

    namespace ORDERBOOK {

        #define TYPE_NOTSET             -1
        #define TYPE_ALL                0
        #define TYPE_QUOTES             (1u<<0u)
        #define TVAL_QUOTES             "quotes"
        #define TYPE_MONITORING         (1u<<1u)
        #define TVAL_MONITORING         "monitoring"
        #define TYPE_CONFIGURATION      (1u<<2u)
        #define TVAL_CONFIGURATION      "configuration"
        #define TYPE_PERSISTENCE        (1u<<3u)
        #define TVAL_PERSISTENCE        "persistence"
        #define TYPE_ARCHIVE            (1u<<4u)
        #define TVAL_ARCHIVE            "archive"
        #define TYPE_PROFILE            (1u<<5u)
        #define TVAL_PROFILE            "profile"
        
        class BookProperties : protected UTILS::Logging
        {
            using PropMap = std::map<std::string, std::string>;
        
        protected:
            BookProperties(const std::string &loggerName)
                    : Logging(loggerName)
            {
            
            }
            
            /*! \brief Checks the 'active' property.
             *
             * Checks if property 'active' is set for the connection.
             *
             * @return true if the property is set
             */
            bool Active() const { return BoolProp("active"); }
            
            /*! \brief Returns the value of 'type' property.
             *
             * Returns the value of the connection property 'type'.
             *
             * @return Type of the connection according to the property settings.
             */
            int Type() const
            {
                if (m_type == TYPE_NOTSET)
                {
                    m_type = GetType(Prop("type"));
                }
                return m_type;
            }
            
            /*! \brief Returns the value of 'type' property.
             *
             * Returns the value of the connection property 'type'.
             *
             * @return Type of the connection according to the property settings.
             */
            bool HasType(int type) const
            {
                int tp = Type();
                return (tp == TYPE_ALL || (tp & type) != 0);
            }
            
            std::string TypeString() const { return typeString(); }
            
            std::string typeString() const { return "BookProperites"; }
            
            std::string Name() const { return Prop("name"); }
            
            unsigned long BatchSize() const { return ULongProp("batchsize"); }
            
            unsigned long MaxDelaySec() const { return ULongProp("max_delay_sec"); }
            
            virtual void Configure(const Poco::XML::Node *pNode)
            {
                m_properties.clear();
                if (pNode && pNode->hasAttributes())
                {
                    Poco::XML::NamedNodeMap *attrs { pNode->attributes() };
                    if (attrs)
                    {
                        Poco::XML::Node *attr;
                        for (unsigned int i = 0; i < attrs->length(); i++)
                        {
                            attr = attrs->item(i);
                            m_properties[UTILS::tolower(attr->nodeName())] = attr->nodeValue();
                        }
                        attrs->release();
                    }
                }
                else
                {
                    // No attributes defined...
                }
            }
            
            const std::string getProp(const std::string &propName) const
            {
                std::string pn { UTILS::tolower(propName) };
                auto it { m_properties.find(pn) };
                if (it != m_properties.end())
                {
                    return it->second;
                }
                else
                {
                    return propDefaultValue(pn);
                }
            }
            
            std::string Prop(const std::string &propName) const
            {
                return getProp(propName);
            }
            
            int IntProp(const std::string &propName) const
            {
                return std::stoi(getProp(propName));
            }
            
            int64_t Int64Prop(const std::string &propName) const
            {
                return std::stol(getProp(propName));
            }
            
            unsigned long ULongProp(const std::string &propName) const
            {
                return std::stoul(getProp(propName));
            }
            
            double DblProp(const std::string &propName) const
            {
                return std::stod(getProp(propName));
            }
            
            bool BoolProp(const std::string &propName) const
            {
                std::string val { getProp(propName) };
                if (!val.empty())
                {
                    char c { (char) ::tolower(val[0]) };
                    return (c == 't' || c == 'y' || c == '1');
                }
                else
                {
                    return false;
                }
            }
        
        protected:
            bool IsValidProp(const std::string &prop) const
            {
                return isValidProp(UTILS::tolower(prop));
            }
            
            bool isValidProp(const std::string &lowerprop) const
            {
                return (lowerprop == "name" || lowerprop == "active" || lowerprop == "batchsize" || lowerprop == "max_delay_sec");
            }
            
            virtual std::string propDefaultValue(const std::string &name) const
            {
                if (name == "active")
                {
                    return "true";
                }
                if (name == "batchsize")
                {
                    return "1";
                }
                if (name == "max_delay_sec")
                {
                    return "3600"; // 1 hour
                }
                if (name == "type")
                {
                    return "";
                }
                return "";
            }
    
            virtual void connect()=0;
            
        private:
            PropMap m_properties;
            mutable int m_type;
            
            static int GetType(const std::string &str);
            
        };
    }
#endif //_BOOKPROPERTIES_H
