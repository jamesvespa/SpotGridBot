//
// Created by helmut on 20.02.18.
//

#ifndef COROUT_FILTER_H
#define COROUT_FILTER_H

#include <string>
#include <vector>
#include <functional>
#include <regex>

#include "Utils/Utils.h"
#include "Utils/Logging.h"
#include "Utils/ErrorHandler.h"

#include "Quote.h"

namespace CORE {
namespace BOOK {

/*! \brief Tag of the filter element inside the XML config */
#define TAG_FILTER  "Filter"

/*! \brief This class represents the result of a filter application to a quote group.
 * It is defined by a combination of three result flags: "accept", "open", and "last"
 * */
class FilterResult
{
/*! \brief Is the filter condition fulfilled by the quote group? */
#define F_ACCEPT    (1u<<0u)
/*! \brief May the resulting quote group be supplemented with quotes from the following groups (aggregation of quote groups)? */
#define F_OPEN      (1u<<1u)
/*! \brief Is the quote group guaranteed to be the last one to fulfil the filter condition? */
#define F_LAST      (1u<<2u)

public:
	
	/*! \brief Constructor
	 *
	 *  @param accept \a true -> filter condition is fulfilled by trhe quote group
	 *  @param open \a true -> the resulting quote group may be supplemented with
	 *  quotes from the following groups (aggregation of quote groups), default: \a false
	 *  @param last  \a true -> the quote group is guaranteed to be the last one to fulfil
	 *  the filter condition, default: \a false
	 * */
	explicit FilterResult(bool accept, bool open = false, bool last = false)
			: m_result(0)
	{
		if (accept)
		{
			m_result |= F_ACCEPT;
		}
		if (open)
		{
			m_result |= F_OPEN;
		}
		if (last)
		{
			m_result |= F_LAST;
		}
	}
	
	/*! \brief Is the filter condition fulfilled by the quote group? */
	bool Accept() const { return (m_result & F_ACCEPT) != 0; }
	
	/*! \brief May the resulting quote group be supplemented with quotes from the following groups (aggregation of quote groups)? */
	bool Open() const { return (m_result & F_OPEN) != 0; }
	
	/*! \brief Is the quote group guaranteed to be the last one to fulfil the filter condition? */
	bool Last() const { return (m_result & F_LAST) != 0; }

private:
	
	/*! \brief Integer representation of the filter result (combination of three result flags) */
	unsigned int m_result;
};

/*! \brief This class represents a single filter used by filter views.
 *
 * */
class Filter : protected UTILS::Logging, public UTILS::ErrorHandler
{
public:
	/*! \brief Type alias for shared pointer to Filter */
	using Ptr = std::shared_ptr<Filter>;
	
	Filter() = delete; //<! no default constructor
	
	Filter(std::string name, std::string definition, std::string description = "");
	
	bool Valid() const { return m_predicate.Valid(); }  //!< Is this Filter valid?
	
	/*! \brief Is the filter active */
	bool Active() const { return m_active; }
	
	/*! \brief Name of the filter */
	const std::string &Name() const { return m_name; }
	
	/*! \brief Definition of the filter predicate */
	const std::string &Definition() const { return m_definition; }
	
	/*! \brief Human-readable description of the filter */
	const std::string &Description() const { return m_description; }
	
	bool Accept(const Quote &quote) const;
	
	FilterResult Apply(int level, const QuoteGroup::Ptr &srcGroup, QuoteGroup::Ptr &dstGroup) const;
	
	std::string OperatorAsString() const { return m_predicate.OperatorAsString(); }
	
	std::string FieldAsString() const { return m_predicate.FieldAsString(); }
	
	std::string ValueAsString() const { return m_predicate.ValueAsString(); }

	/*! \brief This class represents a filter predicate
	 * */
	class Predicate
	{
	public:
		/*! \brief Enum type representing an operator */
		enum Operator
		{
			opNone, opEQ, opNE, opGT, opGE, opLT, opLE, opIN, opNI
		};
		
		/*! \brief Type alias for a unique pointer to a predicate */
		using Ptr = std::unique_ptr<Predicate>;
		
		Predicate() = delete; //!< no default constructor
		
		explicit Predicate(const std::string &definition);
		
		bool Valid() const { return Op() != opNone && FieldType() != Quote::fldNone; }  //!< Is this Predicate valid?
		
//		Predicate(Operator op, Quote::Field fieldType, std::string arg);
//
//		Predicate(Operator op, Quote::Field fieldType, int64_t arg);
//
//		Predicate(Operator op, Quote::Field fieldType, std::set<std::string> arg);
//
//		Predicate(Operator op, Quote::Field fieldType, std::set<int64_t> arg);
		
		bool Evaluate(const Quote &q) const;
		
		/*! \brief Operator (enum value) */
		Operator Op() const { return m_op; }
		
		static bool IsListOp(Operator op) { return op == opIN || op == opNI; }

		/*! \brief Field of the quote to be compared with the argument */
		Quote::Field FieldType() const { return m_fieldType; }
		
		/*! \brief Base type of the field (string/integer) */
		Quote::FieldBaseType FieldBaseType() const { return m_fieldBaseType; }
		
		/*! \brief String argument */
		const std::string &StrArg() const { return m_strArg; }
		
		/*! \brief Integer argument */
		int64_t IntArg() const { return m_intArg; }
		
		/*! \brief String set argument */
		const std::set<std::string> &StrSet() const { return m_strSet; }
		
		/*! \brief Integer set argument */
		const std::set<int64_t> &IntSet() const { return m_intSet; }
		
		static bool pred(Operator op, const std::string &val1, const std::string &val2);
		
		static bool pred(Operator op, int64_t val1, int64_t val2);
		
		static bool pred(Operator op, const std::string &val1, const std::set<std::string> &val2);
		
		static bool pred(Operator op, int64_t val1, const std::set<int64_t> & val2);
		
		std::string OperatorAsString() const;
		
		std::string FieldAsString() const;
		
		std::string ValueAsString() const;
	
	private:
		/*! \brief Operator (enum value) */
		Operator m_op;
		
		/*! \brief Field of the quote to be compared with the argument */
		Quote::Field m_fieldType;
		
		/*! \brief Base type of the field (string/integer) */
		Quote::FieldBaseType m_fieldBaseType;
		
		/*! \brief String argument */
		std::string m_strArg;
		
		/*! \brief Integer argument */
		int64_t m_intArg;
		
		/*! \brief String set argument */
		std::set<std::string> m_strSet;
		
		/*! \brief Integer set argument */
		std::set<int64_t> m_intSet;
		
		static Operator GetOp(const std::string &opStr);
	};

	const Predicate &GetPredicate() const { return m_predicate; }
private:
	
	/*! \brief Name of the filter */
	std::string m_name;
	/*! \brief Definition of the filter predicate */
	std::string m_definition;
	/*! \brief Human-readable description of the filter */
	std::string m_description;
	/*! \brief Is the filter active */
	bool m_active;
	/*! \brief Filter predicate */
	Predicate m_predicate;
};

} // namespace BOOK
} // namespace CORE


#endif //COROUT_FILTER_H
