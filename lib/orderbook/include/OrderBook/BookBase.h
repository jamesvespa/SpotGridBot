#ifndef COROUT_BOOKBASE_H
#define COROUT_BOOKBASE_H
#include "Utils/ErrorHandler.h"
#include "Utils/Logging.h"

namespace CORE {
namespace BOOK {

/*! Base class for all classes represeting order books */
class BookBase : public UTILS::Logging, public UTILS::ErrorHandler
{
public:
	/*! Constructor */
	explicit BookBase(const std::string &loggerName)
		:Logging(loggerName){}
	
	~BookBase() = default;
	
	/*! \brief Default copy constructor. */
	BookBase(const BookBase & /* other */) = default;
	
	/*! \brief Default copy assignment operator. */
	BookBase &operator=(const BookBase & /* other */) = default;
	
	/*! \brief Default move constructor. */
	BookBase(BookBase && /* other */) = default;
	
	/*! \brief Default move assignment operator. */
	BookBase &operator=(BookBase && /* other */) = default;

protected:
	const char *typeString() const { return "BookBase"; }
	
	void connect() { }
	
	std::string propDefaultValue(const std::string &name) const
	{
		if (name == "batchsize")
		{
			return "1";
		}

		return "0";
	}

};

} // namespace BOOK
} // namespace CORE

#endif //COROUT_BOOKBASE_H
