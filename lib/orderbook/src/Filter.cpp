//
// Created by helmut on 20.02.18.
//

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>

#include "OrderBook/Filter.h"

#define ATTR_FLT_ACTIVE     "active"
#define ATTR_FLT_DEFINITION "definition"

using namespace UTILS;

namespace CORE {
namespace BOOK {


/*! \brief Constructor
 *
 * @param name  Name for the filter
 * @param definition Predicate expression defining the filter condition
 * @param description Human-readable description of the filter
 */
Filter::Filter(std::string name, std::string definition, std::string description)
		: Logging("Filter"), ErrorHandler(pLogger()), m_name(std::move(name)), m_definition(std::move(definition)),
		  m_description(description.empty() ? m_definition : std::move(description)), m_active(true),
		  m_predicate(m_definition)
{
}

/*! \brief Checks whether a quote fulfils the filter condition
 *
 * @param quote Quote to be checked
 * @return \a true if the quote fulfils the condition
 */
bool Filter::Accept(const Quote &quote) const
{
	bool result { true };
	if (Active())
	{
		result = m_predicate.Evaluate(quote);
	}
	return result;
}


/*! \brief Applies the filter to a quote group
 *
 * @param level Current level of the source quote group
 * @param srcGroup Source quote group
 * @param dstGroup Reference to the resulting quote group
 * @return Filter result value
 */
FilterResult Filter::Apply(int level, const QuoteGroup::Ptr &srcGroup, QuoteGroup::Ptr &dstGroup) const
{
	FilterResult result { false };
	// clear the destination quote group unless the filter defines an aggregation of source quote groups
	if (m_predicate.FieldType() != Quote::fldAggregateVolume)
	{
		dstGroup.reset();
	}
	switch (m_predicate.FieldType())
	{
		case Quote::fldLevel:
		{
			bool success { false }, last { false };
			if (Predicate::IsListOp(m_predicate.Op()))
			{
				success = Predicate::pred(m_predicate.Op(), level, m_predicate.IntSet());
			}
			else
			{
				success = Predicate::pred(m_predicate.Op(), level, m_predicate.IntArg());
			}
			switch (m_predicate.Op())
			{
				case Predicate::opEQ:
					last = success;
					break;
				case Predicate::opLE:
				case Predicate::opLT:
					last = !success;
					break;
				default:
					break;
			}
			result = FilterResult(success, false, last);
			break;
		}
		case Quote::fldLevelVolume:
			result = FilterResult(Predicate::pred(m_predicate.Op(), srcGroup->TotalVolume(), m_predicate.IntArg()));
			break;
		case Quote::fldAggregateVolume:
		{
			int64_t aggVolume { (dstGroup ? dstGroup->TotalVolume() : 0) };
			bool success { Predicate::pred(m_predicate.Op(), aggVolume + srcGroup->TotalVolume(), m_predicate.IntArg()) };
			//std::cout << aggVolume << " + " << srcGroup->TotalVolume() << " >= " << m_predicate.IntArg() << " ==> " << success << std::endl;
			if (success && !dstGroup)
			{
				dstGroup = srcGroup;
			}
			else
			{
				if (!dstGroup)
				{
					dstGroup = QuoteGroup::Create();
				}
				dstGroup->AddQuotes(srcGroup);
			}
			result = FilterResult(true, !success, false); // continue creating aggregate quote groups
			//result = FilterResult(true, !success, success); // return only one aggregate
			break;
		}
		default:
		{
			if (m_predicate.FieldType() == Quote::fldPrice && srcGroup->SinglePrice())
			{
				result = FilterResult(Predicate::pred(m_predicate.Op(), srcGroup->AvgPrice(), m_predicate.IntArg()));
			}
			else
			{
				dstGroup = QuoteGroup::Create();
				srcGroup->ForEachQuote([this, &dstGroup](const Quote::Ptr &q)
									   {
										   if (Accept(*q))
										   {
											   dstGroup->AddQuote(q);
										   }
									   });
				result = FilterResult(dstGroup->HasQuotes());
			}
			break;
		}
	}
	if (result.Accept())
	{
		if (!dstGroup)
		{
			dstGroup = srcGroup;
		}
	}
	else
	{
		dstGroup.reset();
	}
	return result;
}

/*! \brief Constructor
 *
 * @param definition Predicate expression
 */
Filter::Predicate::Predicate(const std::string &definition)
		: m_op(opNone), m_fieldType(Quote::fldNone), m_fieldBaseType(Quote::fbtNone), m_strArg(""), m_intArg(0)
{
	static const std::regex base_regex(R"( *([A-Za-z]+) *(=|==| eq |!=|<>| ne |<| lt |<=| le |>| gt |>=| ge | in | ni ) *(\{[^}]*\}|[0-9]+|"[^"]*"|'[^']*') *)");
	std::cmatch base_match;
	
	if (std::regex_match(definition.c_str(), base_match, base_regex))
	{
		if (base_match.size() == 4)
		{
			std::string field, op, value;
			std::csub_match base_sub_match1 { base_match[1] };
			field = base_sub_match1.str();
			std::csub_match base_sub_match2 { base_match[2] };
			op = base_sub_match2.str();
			std::csub_match base_sub_match3 { base_match[3] };
			value = base_sub_match3.str();
			if (!value.empty() && (value[0] == '"' || value[0] == '\''))
			{
				value = value.substr(1, value.length() - 2);
			}
			m_op = GetOp(op);
			if (!value.empty() && IsListOp(m_op) && value[0] == '{')
			{
				value = value.substr(1, value.length() - 2);
			}
			m_fieldType = Quote::GetField(field);
			m_fieldBaseType = Quote::GetBaseType(m_fieldType);
			switch (m_fieldBaseType)
			{
				case Quote::fbtInt64:
					if (IsListOp(m_op))
					{
						Poco::StringTokenizer tokens { value, ",", Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM };
						for (const std::string &s : tokens)
						{
							m_intSet.emplace(UTILS::parse<int64_t>(s, 0));
						}
					}
					else
					{
						m_intArg = UTILS::parse<int64_t>(value, 0);
					}
					break;
				case Quote::fbtString:
					if (IsListOp(m_op))
					{
						Poco::StringTokenizer tokens { value, ",", Poco::StringTokenizer::TOK_TRIM };
						for (const std::string &s : tokens)
						{
							m_strSet.emplace(s);
						}
					}
					else
					{
						m_strArg = value;
					}
					break;
				default:
					break;
			}
		}
	}
}

/*! \brief Constructor
 *
 * @param op Operator as enum value
 * @param fieldType Field of the quote to be checked (enum value)
 * @param arg String argument
 * */
//Filter::Predicate::Predicate(Operator op, Quote::Field fieldType, std::string arg)
//		: m_op(op), m_fieldType(fieldType), m_fieldBaseType(Quote::GetBaseType(m_fieldType)), m_strArg(std::move(arg)), m_intArg(0) { }

/*! \brief Constructor
 *
 * @param op Operator as enum value
 * @param fieldType Field of the quote to be checked (enum value)
 * @param arg Integer argument
 * */
//Filter::Predicate::Predicate(Operator op, Quote::Field fieldType, int64_t arg)
//		: m_op(op), m_fieldType(fieldType), m_fieldBaseType(Quote::GetBaseType(m_fieldType)), m_strArg(""), m_intArg(arg) { }
//
//Filter::Predicate::Predicate(Operator op, Quote::Field fieldType, std::set<std::string> arg)
//		: m_op(op), m_fieldType(fieldType), m_fieldBaseType(Quote::GetBaseType(m_fieldType)), m_strArg(""), m_intArg(0), m_strSet(std::move(arg)) { }
//
//Filter::Predicate::Predicate(Operator op, Quote::Field fieldType, std::set<int64_t> arg)
//		: m_op(op), m_fieldType(fieldType), m_fieldBaseType(Quote::GetBaseType(m_fieldType)), m_strArg(""), m_intArg(0), m_intSet(std::move(arg)) { }


/*! \brief Evaluate the predicate for a quote
 *
 * @param quote Quote for which the predicate is to be evaluated
 * @return \a true if the quote fulfils the predicate
 */
bool Filter::Predicate::Evaluate(const Quote &q) const
{
	bool result { false };
	if (m_fieldType != Quote::fldNone)
	{
		if (m_fieldBaseType == Quote::fbtInt64)
		{
			if (IsListOp(m_op))
			{
				result = pred(m_op, q.GetInt(m_fieldType), m_intSet);
			}
			else
			{
				result = pred(m_op, q.GetInt(m_fieldType), m_intArg);
			}
		}
		else
		{
			if (IsListOp(m_op))
			{
				result = pred(m_op, q.GetString(m_fieldType), m_strSet);
			}
			else
			{
				result = pred(m_op, q.GetString(m_fieldType), m_strArg);
			}
		}
	}
	return result;
}

/*! \brief String comparison
 *
 * Static function comparing two strings.
 *
 * @param op Relational operator
 * @param val1 Left-hand operand
 * @param val2 Right-hand operand
 * @return Result of the comparison
 */
bool Filter::Predicate::pred(Operator op, const std::string &val1, const std::string &val2)
{
	switch (op)
	{
		case opEQ:
			return val1 == val2;
		case opNE:
			return val1 != val2;
		case opGT:
			return val1 > val2;
		case opGE:
			return val1 >= val2;
		case opLT:
			return val1 < val2;
		case opLE:
			return val1 <= val2;
		default:
			return false;
	}
}

/*! \brief Integer comparison
 *
 * Static function comparing two 64bit integers.
 *
 * @param op Relational operator
 * @param val1 Left-hand operand
 * @param val2 Right-hand operand
 * @return Result of the comparison
 */
bool Filter::Predicate::pred(Operator op, int64_t val1, int64_t val2)
{
	switch (op)
	{
		case opEQ:
			return val1 == val2;
		case opNE:
			return val1 != val2;
		case opGT:
			return val1 > val2;
		case opGE:
			return val1 >= val2;
		case opLT:
			return val1 < val2;
		case opLE:
			return val1 <= val2;
		default:
			return false;
	}
}

//static
bool Filter::Predicate::pred(Operator op, const std::string &val1, const std::set<std::string> &val2)
{
	switch (op)
	{
		case opIN:
			return val2.find(val1) != val2.end();
		case opNI:
			return val2.find(val1) == val2.end();
		default:
			return false;
	}
}

//static
bool Filter::Predicate::pred(Operator op, int64_t val1, const std::set<int64_t> &val2)
{
	switch (op)
	{
		case opIN:
			return val2.find(val1) != val2.end();
		case opNI:
			return val2.find(val1) == val2.end();
		default:
			return false;
	}
}


/*! \brief Convert a string representation of an operator to an enum value
 *
 * Static function comparing.
 *
 * @param opStr String representaion of an operator
 * @return Enum value representing an operator
 */
Filter::Predicate::Operator Filter::Predicate::GetOp(const std::string &opStr)
{
	if (opStr == "=" || opStr == "==" || opStr == " eq ")
	{
		return opEQ;
	}
	else if (opStr == "!=" || opStr == "<>" || opStr == " ne ")
	{
		return opNE;
	}
	else if (opStr == ">=" || opStr == " ge ")
	{
		return opGE;
	}
	else if (opStr == ">" || opStr == " gt ")
	{
		return opGT;
	}
	else if (opStr == "<=" || opStr == " le ")
	{
		return opLE;
	}
	else if (opStr == "<" || opStr == " lt ")
	{
		return opLT;
	}
	else if (opStr == " in ")
	{
		return opIN;
	}
	else if (opStr == " ni ")
	{
		return opNI;
	}
	else
	{
		return opNone;
	}
}

std::string Filter::Predicate::OperatorAsString() const
{
	std::string result { "" };
	switch (m_op)
	{
		case opEQ:
			result = "EQ";
			break;
		case opNE:
			result = "NE";
			break;
		case opGT:
			result = "GT";
			break;
		case opGE:
			result = "GE";
			break;
		case opLT:
			result = "LT";
			break;
		case opLE:
			result = "LE";
			break;
		case opIN:
			result = "IN";
			break;
		case opNI:
			result = "NI";
			break;
		case opNone:
		default:
			break;
	}
	return result;
}

std::string Filter::Predicate::FieldAsString() const
{
	std::string result { "" };
	switch (m_fieldType)
	{
		case Quote::fldLevel:
			result = "Level";
			break;
		case Quote::fldLevelVolume:
			result = "LevelVolume";
			break;
		case Quote::fldAggregateVolume:
			result = "AggregateVolume";
			break;
		case Quote::fldQuoteID:
			result = "QuoteID";
			break;
		case Quote::fldCompID:
			result = "CompID";
			break;
		case Quote::fldPb:
			result = "PB";
			break;
		case Quote::fldSession:
			result = "Session";
			break;
		case Quote::fldSeqNum:
			result = "SeqNum";
			break;
		case Quote::fldPrice:
			result = "Price";
			break;
		case Quote::fldVolume:
			result = "Volume";
			break;
		case Quote::fldMinQuantity:
			result = "MinQuantity";
			break;
		case Quote::fldKey:
			result = "Key";
			break;
		case Quote::fldRefKey:
			result = "RefKey";
			break;
		case Quote::fldSendingTime:
			result = "SendingTime";
			break;
		case Quote::fldReceiptTime:
			result = "ReceiptTime";
			break;
		case Quote::fldQuoteType:
			result = "QuoteType";
			break;
		case Quote::fldNone:
		default:
			break;
	}
	return result;
}

std::string Filter::Predicate::ValueAsString() const
{
	std::string result { "" };
	switch (m_fieldBaseType)
	{
		case Quote::fbtInt64:
			if (IsListOp(m_op))
			{
				std::stringstream ss;
				bool first { true };
				for (int64_t n : m_intSet)
				{
					if (first)
					{
						first = false;
					}
					else
					{
						ss << ",";
					}
					ss << n;
				}
				result = ss.str();
			}
			else
			{
				result = std::to_string(IntArg());
			}
			break;
		case Quote::fbtString:
			if (IsListOp(m_op))
			{
				std::stringstream ss;
				bool first { true };
				for (const std::string &s : m_strSet)
				{
					if (first)
					{
						first = false;
					}
					else
					{
						ss << ",";
					}
					ss << s;
				}
				result = ss.str();
			}
			else
			{
				result = StrArg();
			}
			break;
		default:
			break;
	}
	return result;
}

} // namespace BOOK
} // namespace CORE
