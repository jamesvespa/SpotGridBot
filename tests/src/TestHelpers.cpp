#include "unittest.h"
#include "TestHelpers.h"

namespace {
// One Time Run poco initializer for tests
class PocoInitializer
{
public:
	PocoInitializer()
	{
		Poco::Net::initializeSSL();
		Poco::Net::HTTPSStreamFactory::registerFactory();
	}
};

static PocoInitializer pi;
}


//------------------------------------------------------------------------------
/*! \brief Registers crypto currencies used in the tests */
void MERCURY::TEST::RegisterTestCurrencies()
{
	UTILS::CurrencyPair::InitializeCurrencyConfigs(PATH_TEST_CURRENCYCONFIG);
}


//------------------------------------------------------------------------------
/*! \brief Returns true if fld can be found in fields. If exact == true, full string is searched */
bool MERCURY::TEST::CheckRequestField(const Poco::StringTokenizer &fields, const std::string &fld, bool exact)
{
	return std::find_if(fields.begin(), fields.end(), [exact, &fld](const auto &elem)
	{
		return exact ? elem == fld : elem.find(fld) == 0;
	}) != fields.end();
};

