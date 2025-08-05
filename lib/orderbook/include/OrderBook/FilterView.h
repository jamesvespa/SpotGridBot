//
// Created by helmut on 16.03.18.
//

#ifndef COROUT_FILTEREDBOOKVIEW_H
#define COROUT_FILTEREDBOOKVIEW_H

#include "Filter.h"
#include "Quote.h"
#include "OrderBook/BookView.h"

namespace CORE {
namespace BOOK {

/*! \brief This class represents a view that is defined by a filter */
class FilterView : public BookView
{
public:
	FilterView(std::string name, BookView::Ptr source, Filter::Ptr filter, bool active = true);
	
	bool Valid() const override; //!< Is this FilterView valid?
	
	void IterateQuoteGroups(const QuoteGroupFunc &action, const QuotePred &quotePred) const override;
	
	/*! \brief Pointer to the direct source view providing the quotes for this view */
	BookView::Ptr SourceViewPtr() override { return m_source; }
	
	/*! \brief Const Pointer to the direct source view providing the quotes for this view */
	BookView::PtrToConst SourceViewPtr() const override { return m_source; }
	
	/*! \brief Pointer to the filter defining the condition for this view */
	Filter::Ptr FilterPtr() const { return m_filter; }
	
	void AppendFilter(std::stringstream &ss, bool skipSortView, const std::string &delimiter) const override;
	
	/*! \brief Instrument
	 *
	 * Overridden virtual function.
	 *
	 * @return Instrument supported by this view
	 * */
	UTILS::CurrencyPair Instrument() const override { return SortViewPtr()->Instrument(); }
	
	/*! \brief Quote type (bid/ask)
	 *
	 * Overridden virtual function.
	 *
	 * @return Quote type supported by this view
	 * */
	UTILS::QuoteType Type() const override { return SortViewPtr()->Type(); }
	
	/** Returns the filter that defines this view
	 *
	 * @return Filter
	 */
	const Filter *GetFilter() const override { return m_filter.get(); }

private:
	/*! \brief Direct source view providing the quotes for this view */
	const BookView::Ptr m_source;
	
	/*! \brief Filter defining the condition for this view */
	const Filter::Ptr m_filter;
};

} // namespace BOOK
} // namespace CORE


#endif //COROUT_FILTEREDBOOKVIEW_H
