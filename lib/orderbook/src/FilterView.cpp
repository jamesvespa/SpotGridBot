//
// Created by helmut on 16.03.18.
//

#include "OrderBook/FilterView.h"

namespace CORE {
namespace BOOK {

/*! Constructor
 *
 * @param name  Name of the view
 * @param source    Book view (either SortView or FilterView) that is the
 * direct source of quote groups for this view
 * @param filter Filter to be used for filtering quote groups from the source,
 * or generating new quote groups
 * @param active \a true -> view may be used directly, \a false -> view
 * is only to be used as intermediary for other views
 *
 * */
FilterView::FilterView(std::string name, BookView::Ptr source, Filter::Ptr filter, bool active)
		: BookView(std::move(name), active), m_source(std::move(source)), m_filter(std::move(filter))
{
}

/*! \brief Dynamically creates quote groups and passes them to the provided
 * callback action.
 *
 * This function creates quote groups representing one market level or
 * synthetic level defined by the filter condition, and sequentially
 * passes each group to the provided callback action.
 *
 * @param action    void-function to be called for every generated quote group.
 * @param quotePred (optional) predicate to be fulfilled by a quote to be added to a quote group
 */
void FilterView::IterateQuoteGroups(const QuoteGroupFunc &action, const QuotePred &quotePred) const
{
	QuoteGroup::Ptr qg;
	int lvl { 0 };
	m_source->IterateQuoteGroups([this, &action, &qg, &lvl](int level, QuoteGroup::Ptr &quoteGroup, bool &cont)
								 {
									 FilterResult result { m_filter->Apply(level, quoteGroup, qg) };
									 if (result.Accept() && !result.Open())
									 {
										 action(++lvl, qg, cont);
										 qg.reset();
									 }
									 cont = cont && !result.Last();
								 }, quotePred);
}

/*! \brief Append filter condition as string.
 *
 * Overridden virtual function.
 *
 * @param ss   Reference to a stringstream to receive the sequence or filter conditions
 * @param delimiter String to delimit the filter conditions
 *
 * */
void FilterView::AppendFilter(std::stringstream &ss, bool skipSortView, const std::string &delimiter) const
{
	SourceViewPtr()->AppendFilter(ss, skipSortView, delimiter);
	if (ss.rdbuf()->in_avail() > 0) // same as "if (!ss.str().empty())", but probably faster
	{
		ss << delimiter;
	}
	ss << FilterPtr()->Definition();
}

bool FilterView::Valid() const
{
	return FilterPtr() && FilterPtr()->Valid() && SourceViewPtr() && SourceViewPtr()->Valid();
}

} // namespace BOOK
} // namespace CORE
