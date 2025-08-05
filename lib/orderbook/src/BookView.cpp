//
// Created by helmut on 13.03.18.
//

#include "OrderBook/BookView.h"

using namespace UTILS;

namespace CORE {
namespace BOOK {

/*! \brief Fills a vector of quote groups
 *
 * This function takes a reference to a vector of quote groups, and
 * fills it with one quote group per level.
 *
 * @param vec       Reference to a vector of quote groups
 * @param n         Maximum number of levels
 * @param quotePred (optional) predicate to be fulfilled by a quote to be added to a quote group
 */
void BookView::GetLevels(QuoteGroupVec &vec, unsigned int n, const QuotePred &quotePred) const
{
	IterateQuoteGroups([&vec, &n](int level, QuoteGroup::Ptr &qg, bool &cont)
					   {
						   vec.push_back(qg);
						   cont = n == 0 || --n > 0; // n == 0 => unlimited levels; n > 0 => n levels (break when --n reaches 0)
					   }, quotePred);
}

/*! \brief Returns a vector of quote groups
 *
 * This function creates a new vector of quote groups, and fills it with one
 * quote group per level.
 *
 * @param n         Maximum number of levels
 *
 * @return Vector of quote groups
 */
BookView::QuoteGroupVec BookView::GetLevels(unsigned int n, const QuotePred &quotePred) const
{
	QuoteGroupVec out;
	GetLevels(out, n, quotePred);
	return out;
}

/*! \brief Return the top level quote group
 *
 * This function returns the current top level quote group
 *
 * @return Current top level quote group
 */
QuoteGroup::Ptr BookView::AggregateLevel(int64_t volume, const QuotePred &quotePred) const
{
	QuoteGroup::QuoteVector quoteVec;
	IterateQuoteGroups([&quoteVec, &volume](int level, QuoteGroup::Ptr &qg, bool &cont)
					   {
						   qg->ForEachQuote([&volume, &quoteVec](const Quote::Ptr &q)
											{
												if (volume > 0 && !q->Used())
												{
													quoteVec.push_back(q);
													volume -= q->Volume();
												}
											});
						   cont = (volume > 0);
					   }, quotePred);
	return std::make_shared<QuoteGroup>(quoteVec);
}

void BookView::AggregateLevels(int64_t minVolumePerLevel, const QuoteGroupFunc &action, const QuotePred &quotePred) const
{
	QuoteGroup::QuoteVector quoteVec;
	int64_t aggVolume { 0 };
	int aggLevel { 0 };
	IterateQuoteGroups([&quoteVec, &aggVolume, &aggLevel, minVolumePerLevel, &action](int level, QuoteGroup::Ptr &qg, bool &cont)
					   {
						   qg->ForEachQuote([&aggVolume, &aggLevel, &quoteVec, minVolumePerLevel, &action, &cont](const Quote::Ptr &q)
											{
												if (cont && !q->Used())
												{
													quoteVec.push_back(q);
													aggVolume += q->Volume();
													if (aggVolume >= minVolumePerLevel)
													{
														QuoteGroup::Ptr aggQuoteGroup { std::make_shared<QuoteGroup>(quoteVec) };
														action(aggLevel, aggQuoteGroup, cont);
														++aggLevel;
														aggVolume = 0;
														quoteVec.clear();
													}
												}
											});
					   }, quotePred);
	if (!quoteVec.empty())
	{
		bool cont { true };
		QuoteGroup::Ptr aggQuoteGroup { std::make_shared<QuoteGroup>(quoteVec) };
		action(aggLevel, aggQuoteGroup, cont);
	}
}

/*! \brief Pointer to the sort view that provides the quotes groups for all
 * downstream filter views.
 *
 * This functions recursively traces all source view until it reaches the
 * base sort view.
 *
 * @return Raw pointer to the sort view providing the quote groups for
 * downstream filter views
 * */
BookView::RawPtr BookView::SortViewPtr()
{
	BookView::Ptr src { SourceViewPtr() };
	return (src ? src->SortViewPtr() : this);
}

/*! \brief Const pointer to the sort view that provides the quotes groups for
 * all downstream filter views.
 *
 * This functions recursively traces all source view until it reaches the
 * base sort view.Delimiter
 *
 * @return Const raw pointer to the sort view providing the quote groups for
 * downstream filter views
 * */
BookView::RawPtrToConst BookView::SortViewPtr() const
{
	BookView::PtrToConst src { SourceViewPtr() };
	return (src ? src->SortViewPtr() : this);
}

/*! \brief Returns the sequence of filter conditions used by this view
 *
 * @parameter delimiter String used to delimit the filter conditions
 * (default: " -> ")
 *
 * @return Delimited string representing the sequence of filter conditions
 * used by this view
 * */
std::string BookView::GetFilterSequence(bool skipSortView, const std::string &delimiter) const
{
	std::stringstream ss;
	AppendFilter(ss, skipSortView, delimiter);
	std::string result { ss.str() };
	if (result.empty())
	{
		result = "no filter";
	}
	return result;
}

/*! \brief Checks whether the given instrument and side matches instrument
 * and side of this view.
 *
 * @return \a true if this view matches the given instrument and side
 * */
bool BookView::Matches(CurrencyPair instrument, Side side) const
{
	return Instrument() == instrument && Bid() == side.Sell();
}

} // namespace BOOK
} // namespace CORE
