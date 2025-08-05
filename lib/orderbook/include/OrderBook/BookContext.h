//
// Created by helmut on 20.03.18.
//

#ifndef COROUT_BOOKCONTEXT_H
#define COROUT_BOOKCONTEXT_H

#include <memory>

namespace CORE {

namespace BOOK {

class SortBook;

}
/*! \brief Abstract derived class that acts as a base class of the concrete
 * class that stores the global context.
 *
 * This class is abstract as it does not implement the pure virtual functions
 * declared in the base class \a ContextBase.
 *
 * It adds the context components provided by the crbook library.
 */
class BookContext
{
public:
	void SetSortBook(BOOK::SortBook *value) { m_sortbook = value; }
	
	bool HasSortBook() const { return m_sortbook != nullptr; }
	
	BOOK::SortBook &SortBook() { return *m_sortbook; }
	
	const BOOK::SortBook &SortBook() const { return *m_sortbook; }

private:
	BOOK::SortBook *m_sortbook; //!< standard sortbook
};

} // namespace CORE

#endif //COROUT_BOOKCONTEXT_H
