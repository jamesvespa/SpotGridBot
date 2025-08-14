#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace UTILS
{

#define DEFAULT_TRADER_ID                "TOKKYO"

#define TRADEREQRESULT_SUCCESSFUL       0
#define TRADEREQRESULT_NOT_SUPPORTED    8

#define TRADEREQSTATUS_ACCEPTED         0
#define TRADEREQSTATUS_REJETED          2

#define ORDTYPE_MARKET                '1'
#define ORDTYPE_LIMIT                '2'

#define ORDSTATUS_NOTSENT        '\000' // internal
#define ORDSTATUS_SENT            '\001' // internal
#define ORDSTATUS_TIMED_OUT        '\002' // internal

#define ORDSTATUS_NEW               '0'
#define ORDSTATUS_PARTIALLY_FILLED  '1'
#define ORDSTATUS_FILLED            '2'
#define ORDSTATUS_DONE_FOR_DAY      '3'
#define ORDSTATUS_CANCELED          '4'
#define ORDSTATUS_REPLACED          '5'
#define ORDSTATUS_PENDING_CANCEL    '6'
#define ORDSTATUS_STOPPED           '7'
#define ORDSTATUS_REJECTED          '8'
#define ORDSTATUS_SUSPENDED         '9'
#define ORDSTATUS_PENDING_NEW       'A'
#define ORDSTATUS_CALCULATED        'B'
#define ORDSTATUS_EXPIRED           'C'
#define ORDSTATUS_ACCEPTED_FOR_BIDDING 'D'
#define ORDSTATUS_PENDING_REPLACE   'E'

#define ORDSTATUS_VALID(_s_) ((_s_) >= ORDSTATUS_NEW && (_s_) <= ORDSTATUS_PENDING_REPLACE)
#define ORDSTATUS_PENDING(_s_) ((_s_) == ORDSTATUS_NEW || (_s_) == ORDSTATUS_PARTIALLY_FILLED || (_s_) == ORDSTATUS_PENDING_CANCEL || (_s_) == ORDSTATUS_PENDING_NEW || (_s_) == ORDSTATUS_ACCEPTED_FOR_BIDDING || (_s_) == ORDSTATUS_PENDING_REPLACE)

#define EXECTYPE_NONE                '\000' // internal
#define EXECTYPE_CANCEL_REJECT        '\001' // internal
#define EXECTYPE_TIMED_OUT            '\002' // internal

#define EXECTYPE_NEW               '0'
#define EXECTYPE_PARTIAL_FILL       '1'
#define EXECTYPE_FILL              '2'
#define EXECTYPE_DONE_FOR_DAY      '3'
#define EXECTYPE_CANCELED          '4'
#define EXECTYPE_REPLACE           '5'
#define EXECTYPE_PENDING_CANCEL    '6'
#define EXECTYPE_STOPPED           '7'
#define EXECTYPE_REJECTED          '8'
#define EXECTYPE_SUSPENDED         '9'
#define EXECTYPE_PENDING_NEW       'A'
#define EXECTYPE_CALCULATED        'B'
#define EXECTYPE_EXPIRED           'C'
#define EXECTYPE_RESTATED           'D'
#define EXECTYPE_PENDING_REPLACE   'E'
#define EXECTYPE_TRADE               'F'
#define EXECTYPE_TRADE_CORRECT       'G'
#define EXECTYPE_TRADE_CANCEL       'H'
#define EXECTYPE_ORDER_STATUS       'I'

#define EXECTRANSTYPE_NONE         '\000'
#define EXECTRANSTYPE_NEW          '0'
#define EXECTRANSTYPE_CANCEL       '1'
#define EXECTRANSTYPE_CORRECT      '2'
#define EXECTRANSTYPE_STATUS       '3'

#define UPDATEACTION_NEW               '0'
#define UPDATEACTION_CHANGE            '1'
#define UPDATEACTION_DELETE            '2'

#define CXLREJRESPONSETO_ORDERCANCELREQUEST         '1'
#define CXLREJRESPONSETO_ORDERCANCELREPLACEREQUEST  '2'

#define CXLREJREASON_TOO_LATE_TO_CANCEL 0
#define CXLREJREASON_UNKNOWN_ORDER      2
#define CXLREJREASON_BROKER_OPTION      3
#define CXLREJREASON_ALREADY_PENDING    4

#define EXECSTRATEGY_NONE           -1
#define EXECSTRATEGY_ORDER           0
#define EXECSTRATEGY_INTERNAL_MATCH  1
#define EXECSTRATEGY_EXTERNAL_ORDER  2
#define EXECSTRATEGY_EXTERNAL_MATCH  3

#define TSS_HALTED        1 //!< TradSesStatus <340>: Halted
#define TSS_OPEN        2 //!< TradSesStatus <340>: Open
#define TSS_CLOSED        3 //!< TradSesStatus <340>: Closed
#define TSS_PRE_OPEN    4 //!< TradSesStatus <340>: Pre-Open
#define TSS_PRE_CLOSE    5 //!< TradSesStatus <340>: Pre-Close

/*! \brief Quote type constant: Snapshot/Full refresh */
#define QT_SNAPSHOT     -1
/*! \brief Quote type constant: Incremental refresh - NEW */
#define QT_NEW           0
/*! \brief Quote type constant: Incremental refresh - UPDATE */
#define QT_UPDATE        1
/*! \brief Quote type constant: Incremental refresh - DELETE */
#define QT_DELETE        2

#define TIF_MIN     TimeInForce::DAY
#define TIF_MAX     TimeInForce::CLS

#define TIF_VALID_COUNT   8
#define TIF_IDX_INVALID  TIF_VALID_COUNT
#define TIF_DELIMITER   ","

enum class SubscriptionRequestType : char {
    Snapshot = '0',
	Subscribe = '1',
	Unsubscribe = '2'
};

enum class MDUpdateType : int {
	FullRefresh = 0,
	IncrementalRefresh = 1
};

}

