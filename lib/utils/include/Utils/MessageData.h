//
// Created by jameson 11.06.18.
//

#pragma once

#include <vector>

#include "Utils/Utils.h"
#include "Utils/FixTypes.h"
#include "Utils/CurrencyPair.h"

namespace UTILS {
namespace MESSAGE {

    /*!
     * \brief This structure holds all relevant data for generating a Trade Capture Repor\t Request Acknowledgement
     */
    struct TradeCaptureReportReqAckData
    {

        using Ptr = std::unique_ptr<TradeCaptureReportReqAckData>;

        /*!
         * Ctor.
         * @param reqType
         * @param tradeReqId
         * @param tradeReqResult
         * @param tradeReqStatus
         */
        TradeCaptureReportReqAckData(int64_t reqType, std::string tradeReqId, int tradeReqResult, int tradeReqStatus)
                : m_reqType(reqType), m_tradeReqId(std::move(tradeReqId)), m_tradeReqResult(tradeReqResult), m_tradeReqStatus(tradeReqStatus)
        {
            m_symbol = "ALL";
            m_tradeReqType = 0;
        }

        /*! CCY - ALL is the only supported value */
        std::string m_symbol;
        /*! Request Type*/
        int64_t m_reqType;
        /*! ID of the Trade report Request */
        std::string m_tradeReqId;
        /*! Tupe of the trade request - 0 only supported value */
        int m_tradeReqType;
        /*! Result of the trade request (successful, not supported, not authorized etc.) */
        int m_tradeReqResult;
        /*! Status - accepted or rejected */
        int m_tradeReqStatus;
    };


    struct TradeCaptureReportData
    {

        using Ptr = std::unique_ptr<TradeCaptureReportData>;

        TradeCaptureReportData(std::string tradeReportId, std::string tradeRequestId, std::string execId, bool prevReported, char ordStatus,
                               UTILS::CurrencyPair sym, double lastQty, double lastPx, std::string tradeDate, int64_t transactTime, std::string settlDate,
                               char side, std::string ordId, std::string orderOriginationFirm, std::string executingFirm, UTILS::Currency currency, int64_t strategy, std::string execRefId, std::string customPbTag,
                               bool amend = false, bool maker = false)
                : m_tradeReportId(std::move(tradeReportId)), m_tradeReqId(std::move(tradeRequestId)), m_execId(std::move(execId)),
                  m_prevReported(prevReported), m_ordStatus(ordStatus), m_symbol(sym), m_lastQty(lastQty), m_lastPx(lastPx),
                  m_tradeDate(std::move(tradeDate)), m_transactTime(transactTime), m_settlDate(std::move(settlDate)), m_side(side),
                  m_ordId(std::move(ordId)), m_orderOriginationFirm(std::move(orderOriginationFirm)), m_executingFirm(std::move(executingFirm)),
                  m_currency(currency), m_strategy(strategy), m_execRefId(std::move(execRefId)), m_customPbTag(std::move(customPbTag)), m_amendment(amend), m_maker(maker)
        {
        }

        /*! Unique ID of the report */
        std::string m_tradeReportId;

        std::string m_tradeReqId;
        /*! ID of the Execution Report */
        std::string m_execId;
        /*! Has the trade been previously reported? */
        bool m_prevReported;
        /*! Order Status */
        char m_ordStatus;
        /*! CCY */
        UTILS::CurrencyPair m_symbol;
        /*! Last Filled Qty */
        double m_lastQty;
        /*! Last Filled Price */
        double m_lastPx;
        /*! Trade Date YYYYMMDD */
        std::string m_tradeDate;
        /*! Transaction time (Epoch time in nanoseconds) */
        int64_t m_transactTime;
        /*! Transaction Value Date/Settlement Date YYYYMMDD*/
        std::string m_settlDate;
        /*! BUY or SELL */
        char m_side;
        /*! ID of the filled order */
        std::string m_ordId;
        /*! Party ID of the firm that sent the order */
        std::string m_orderOriginationFirm;
        /*! Party ID of the firm that executed the order */
        std::string m_executingFirm;
        /*! Traded Currency */
        UTILS::Currency m_currency;
		/*! Execution strategy for external trade reporting (BLC) */
		int64_t m_strategy;
		/*! Execution reference ID - e.g. hit clOrdID for hedging executions (BLC) */
		std::string m_execRefId;
		std::string m_customPbTag;
        /*! Is this report an amendment to an existing one? */
        bool m_amendment;
        /*! Perspective of the report */
        bool m_maker;
    };


    /*!
     * \brief This structure holds all data relevant to generating an Incremental Refresh Message
     */
    struct IncrementalRefreshData
    {

        using Ptr = std::unique_ptr<IncrementalRefreshData>;

        /*!
        *
        * Ctor.
        *
        * @param mdReqId           ID of the Market Data Request
        * @param instrument        Currency Pair Object; Holds all infos on the requested currency pair
        * @param quoteID           Quote ID (-> MDEntryID)
        * @param refID             Quote ID (-> MDEntryRefID)
        * @param price             Price
        * @param volume            Volume
        * @param minQty            Minimum quantity to be ordered
        * @param settlDate         Settlement date
        * @param currency			(opt) Currency to be streamed; default: base currency
       */
		IncrementalRefreshData(std::string mdReqId, UTILS::CurrencyPair instrument, char bidUpdateAction, std::string bidQuoteId,
							   std::string bidRefId, std::string bidChainId, int64_t bidPrice, int64_t bidVolume, int64_t bidMinQty,
							   char askUpdateAction, std::string askQuoteId, std::string askRefId, std::string askChainId, int64_t askPrice,
							   int64_t askVolume, int64_t askMinQty, std::string settlDate, UTILS::Currency currency = UTILS::Currency::NONE)
				: m_mdReqId(std::move(mdReqId)), m_instrument(instrument), m_currency(currency.Valid() ? currency : instrument.BaseCCY()),
				  m_updateAction(bidUpdateAction, askUpdateAction), m_quoteId(std::move(bidQuoteId), std::move(askQuoteId)),
				  m_refId(std::move(bidRefId), std::move(askRefId)), m_chainId(std::move(bidChainId), std::move(askChainId)),
				  m_price(instrument.CpipToDbl(bidPrice), instrument.CpipToDbl(askPrice)),
				  m_volume(instrument.QtyToDouble(bidVolume), instrument.QtyToDouble(askVolume)),
				  m_minQty(instrument.DoubleToQty(bidMinQty), instrument.DoubleToQty(askMinQty)), m_settlDate(settlDate) { }

        std::string m_mdReqId;
        UTILS::CurrencyPair m_instrument;
		UTILS::Currency m_currency;
        UTILS::BidAskPair<char> m_updateAction;
        UTILS::BidAskPair <std::string> m_quoteId;
        UTILS::BidAskPair <std::string> m_refId;
        UTILS::BidAskPair <std::string> m_chainId;
        UTILS::BidAskPair<double> m_price;
        UTILS::BidAskPair<double> m_volume;
        UTILS::BidAskPair<double> m_minQty;
        std::string m_settlDate;
    };

    struct MassQuoteAckData
    {
        using Ptr = std::unique_ptr<MassQuoteAckData>;

        MassQuoteAckData(std::string quoteId)
                : m_quoteId(std::move(quoteId)) { }

        std::string m_quoteId;
    };

    /*!
     * \brief This structure holds all data relevant to generating a Market Data Snapshot/Full Refresh Message
     */
    struct MDSnapshotData
    {

        // using Ptr = std::unique_ptr<MDSnapshotData>;
        using Ptr = std::shared_ptr<MDSnapshotData>;

        struct SingleQuote
        {
            double volume;
            double price;
            double minQty;
        };

        /*!
         *
         * Ctor.
         *
         * @param mdReqId           ID of the Market Data Request
         * @param instrument        Currency Pair Object; Holds all infos on the requested currency pair
         * @param bidQuoteID        Quote ID for the bid part of the Market Data Snapshot
         * @param bidPrice          Price of the current bid
         * @param bidVolume         Amount up for bidding
         * @param bidMinQty         Minimum bid quantity
         * @param offerQuoteID      Quote ID for the offer part of the Market Data Snapshot
         * @param offerPrice        Price of the current offer
         * @param offerVolume       Amount being offered
         * @param offerMinQty       Minimum offer quantity
         * @param settlDate         Settlement date
         * @param currency			(opt) Currency to be streamed; default: base currency
        */
        MDSnapshotData(std::string mdReqId, int64_t depth, UTILS::CurrencyPair instrument, std::string bidQuoteId, std::string offerQuoteId,
                       std::string settlDate = "", std::string bidQuoteCondition = "A", std::string offerQuoteCondition = "A",
					   UTILS::Currency currency = UTILS::Currency::NONE)
                : m_mdReqId(std::move(mdReqId)), m_depth(depth), m_instrument(instrument), m_currency(currency.Valid() ? currency : instrument.BaseCCY()),
				  m_quoteId(std::move(bidQuoteId), std::move(offerQuoteId)),
                  m_settlDate(std::move(settlDate)), m_quotecondition(std::move(bidQuoteCondition), std::move(offerQuoteCondition))
        {
        }

        MDSnapshotData(std::string mdReqId, UTILS::CurrencyPair instrument, std::string bidQuoteId, int64_t bidPrice, int64_t bidVolume, int64_t bidMinQty,
                       std::string offerQuoteId, int64_t offerPrice, int64_t offerVolume, int64_t offerMinQty, std::string settlDate = "",
                       std::string bidQuoteCondition = "A", std::string offerQuoteCondition = "A", UTILS::Currency currency = UTILS::Currency::NONE)
                : m_mdReqId(std::move(mdReqId)), m_depth(1), m_instrument(instrument), m_currency(currency.Valid() ? currency : instrument.BaseCCY()),
				  m_quoteId(std::move(bidQuoteId), std::move(offerQuoteId)),
                  m_settlDate(std::move(settlDate)), m_quotecondition(std::move(bidQuoteCondition), std::move(offerQuoteCondition))
        {
            AddQuote(true, bidVolume, bidPrice, bidMinQty);
            AddQuote(false, offerVolume, offerPrice, offerMinQty);
        }

        void AddQuote(bool bid, int64_t volume, int64_t price, int64_t minQty)
        {
            m_quotes.Get(bid).push_back(
                        {m_instrument.QtyToDouble(volume), m_instrument.CpipToDbl(price), m_instrument.QtyToDouble(minQty)});
        }

        std::string m_mdReqId;
        int64_t m_depth;
        UTILS::CurrencyPair m_instrument;
		UTILS::Currency m_currency;
        UTILS::BidAskPair <std::string> m_quoteId;
        UTILS::BidAskPair <std::vector<SingleQuote>> m_quotes;
        std::string m_settlDate;
        UTILS::BidAskPair <std::string> m_quotecondition;
    };


class MDSnapshotDataPool
{
public:
    static constexpr uint64_t POOL_SIZE = 1L << 20;

    template <typename... Args>
    static MDSnapshotData::Ptr getMDSnapshotData(const char* caller, bool objectPoolDisabled, std::thread::id tid, Args... args)
    {
        if(objectPoolDisabled || op.isExhausted())
        {
            return std::make_unique<MDSnapshotData>(std::forward<Args>(args)...);
        }
        // std::move required
        return MDSnapshotData::Ptr(std::move(op.getObject(caller, tid, std::forward<Args>(args)...)));
    }

private:
    static UTILS::ObjectPool<MDSnapshotData, POOL_SIZE> op;
};

    /*!
     * \brief This structure holds all data relevant to generating a Quote Message
     */
    struct QuoteData
    {

        // using Ptr = std::unique_ptr<QuoteData>;
        using Ptr = std::shared_ptr<QuoteData>;

        struct SingleQuote
        {
            double volume;
            double price;
            double minQty;
        };

        /*!
         *
         * Ctor.
         *
         * @param quoteReqId        ID of the Quote Request
         * @param quoteId        	ID of the Quote
         * @param instrument        Currency Pair Object; Holds all infos on the requested currency pair
         * @param bidQuoteID        Quote ID for the bid part
         * @param bidPrice          Price of the current bid
         * @param bidVolume         Amount up for bidding
         * @param offerQuoteID      Quote ID for the offer part
         * @param offerPrice        Price of the current offer
         * @param offerVolume       Amount being offered
         * @param settlDate         Settlement date
         * @param reqSide			(opt) Side to be streamed; default: both sides
         * @param currency			(opt) Currency to be streamed; default: base currency
        */
		QuoteData(std::string quoteReqId, int64_t depth, std::string quoteId, UTILS::CurrencyPair instrument, std::string bidQuoteId,
				  std::string offerQuoteId, std::string settlDate, UTILS::Side reqSide = UTILS::Side::INVALID,
				  UTILS::Currency currency = UTILS::Currency::NONE)
				: quoteReqId(std::move(quoteReqId)), depth(depth), quoteId(std::move(quoteId)), instrument(instrument),
				  currency(currency.Valid() ? currency : instrument.BaseCCY()), quoteEntryId(std::move(bidQuoteId), std::move(offerQuoteId)),
				  settlDate(std::move(settlDate)), requestedSide(reqSide)
		{
		}
	
		QuoteData(std::string quoteReqId, std::string quoteId, UTILS::CurrencyPair instrument, std::string bidQuoteId, int64_t bidPrice,
				  int64_t bidVolume, std::string offerQuoteId, int64_t offerPrice, int64_t offerVolume, std::string settlDate, int64_t bidMinVolume,
				  int64_t offerMinVolume, UTILS::Side reqSide = UTILS::Side::INVALID, UTILS::Currency currency = UTILS::Currency::NONE)
				: quoteReqId(std::move(quoteReqId)), depth(1), quoteId(std::move(quoteId)), instrument(instrument),
				  currency(currency.Valid() ? currency : instrument.BaseCCY()), quoteEntryId(std::move(bidQuoteId), std::move(offerQuoteId)),
				  settlDate(std::move(settlDate)), requestedSide(reqSide)
		{
			AddQuote(true, bidVolume, bidPrice, bidMinVolume);
			AddQuote(false, offerVolume, offerPrice, offerMinVolume);
		}

        void AddQuote(bool bid, int64_t volume, int64_t price, int64_t minQty)
        {
            quotes.Get(bid).push_back(
                        {instrument.QtyToDouble(volume), instrument.CpipToDbl(price), instrument.QtyToDouble(minQty)});
        }

        std::string quoteReqId;
        int64_t depth;
        std::string quoteId;
    //	int64_t quoteType; indicative vs. tradeable (always tradeable?)
        UTILS::CurrencyPair instrument;
		UTILS::Currency currency;
        UTILS::BidAskPair<std::string> quoteEntryId;
        UTILS::BidAskPair<std::vector<SingleQuote>> quotes;
        std::string settlDate;
        UTILS::Side requestedSide;
    //	BidAskPair<double> price;
    //	BidAskPair<double> volume;
    //	BidAskPair<double> minVolume;
    };

class QuoteDataPool
{
public:
    static constexpr uint64_t POOL_SIZE = 1L << 20;

    template <typename... Args>
    static QuoteData::Ptr getQuoteData(const char* caller, bool objectPoolDisabled, std::thread::id tid, Args... args)
    {
        if(objectPoolDisabled || op.isExhausted())
        {
            return std::make_unique<QuoteData>(std::forward<Args>(args)...);
        }

        return QuoteData::Ptr((op.getObject(caller, tid, std::forward<Args>(args)...)));
    }

private:
    static UTILS::ObjectPool<QuoteData, POOL_SIZE> op;
};


    /*!
     * \brief This structure holds all data relevant to generating a Quote Message
     */
    struct QuoteCancelData
    {

        using Ptr = std::unique_ptr<QuoteCancelData>;

        /*!
         *
         * Ctor.
         *
         * @param quoteReqId        ID of the Quote Request
         * @param quoteCancelType   type of quote cancel
         * @param instrument        Currency Pair Object; Holds all infos on the requested currency pair
         * @param text              Optional free format text string
        */
        QuoteCancelData(std::string quoteId, std::string quoteReqId, int64_t quoteCancelType, UTILS::CurrencyPair instrument, std::string text = "")
                : quoteId(std::move(quoteId)), quoteReqId(std::move(quoteReqId)), quoteCancelType(quoteCancelType), instrument(instrument),
                  text(std::move(text)) { }

        std::string quoteId;
        std::string quoteReqId;
        int64_t quoteCancelType;
        UTILS::CurrencyPair instrument;
        std::string text;
    };

    /*!
     * \brief This structure holds all data relevant to generating a QuoteRequestReject Message
     */
    struct QuoteRequestRejectData
    {

        using Ptr = std::unique_ptr<QuoteRequestRejectData>;

        /*!
         *
         * Ctor.
         *
         * @param quoteReqId        ID of the Quote Request
         * @param rejectReason      Code for RejectReason (tag 658)
         * @param instrument        Currency Pair Object; Holds all infos on the requested currency pair
         * @param text         		Text message
        */
        QuoteRequestRejectData(std::string quoteReqId, int64_t rejectReason, UTILS::CurrencyPair instrument, std::string text = "")
                : quoteReqId(std::move(quoteReqId)), rejectReason(rejectReason), instrument(instrument), text(std::move(text)) { }

        std::string quoteReqId;
        int64_t rejectReason;
        UTILS::CurrencyPair instrument;
        std::string text;
    };

    /*!
     * \brief This structure holds all data relevant to generating an Execution Report Message
     */
    struct ExecutionReportData
    {
        using Ptr = std::shared_ptr<ExecutionReportData>;

        /*!
         * Ctor.
         *
         * @param orderId               ID of the New Order Single
         * @param clOrdID               Unique Client Order ID
         * @param ordType               Order type (market, limit, ...)
         * @param instrument            Currency Pair Object; Holds all infos on the requested currency pair
         * @param currency              Dealt Currency as defined in New Order Single
         * @param execID                ID of the execution Report
         * @param settlDate             Specific date of trade
         * @param execType              Purpose of the execution report
         * @param ordStatus             Status of the order
         * @param side                  Buy/Sell
         * @param orderQty              Traded amount
         * @param orderPx               Price from Order
         * @param lastQty               Last traded amount
         * @param lastPx                Last traded price
         * @param leavesQty             Amount to be processed
         * @param cumQty                Current processed amount
         * @param avgPx                 Average price across all trades for this order
         * @param text                  Optional text
         */
        ExecutionReportData(std::string orderId, std::string clOrdID, char ordType, UTILS::CurrencyPair instrument, UTILS::Currency currency, std::string execID,
                            std::string settlDate, char execType, char ordStatus, char side, int64_t orderQty, int64_t orderPx, int64_t lastQty,
                            int64_t lastPx, int64_t leavesQty, int64_t cumQty, int64_t avgPx, std::string text, std::string account,
                            std::string orderText, std::string username, std::string quoteId, char tif, std::string customPbTag, int64_t transactionTime=0)
                : m_orderId(std::move(orderId)), m_clOrdID(std::move(clOrdID)), m_ordType(ordType), m_instrument(instrument), m_currency(currency),
                  m_execID(std::move(execID)), m_settlDate(std::move(settlDate)), m_execType(execType), m_ordStatus(ordStatus), m_side(side),
                  m_orderQty(instrument.QtyToDouble(orderQty)), m_orderPx(instrument.CpipToDbl(orderPx)), m_lastQty(instrument.QtyToDouble(lastQty)),
                  m_lastPx(instrument.CpipToDbl(lastPx)), m_leavesQty(instrument.QtyToDouble(leavesQty)), m_cumQty(instrument.QtyToDouble(cumQty)),
                  m_avgPx(instrument.CpipToDbl(avgPx)), m_text(std::move(text)), m_account(std::move(account)), m_orderText(std::move(orderText)),
                   m_username(std::move(username)), m_quoteId(std::move(quoteId)), m_tif(tif), m_customPbTag(std::move(customPbTag)), m_transactionTime(transactionTime)
        {
        }

        std::string m_orderId;
        std::string m_clOrdID;
        char m_ordType;
        UTILS::CurrencyPair m_instrument;
        UTILS::Currency m_currency;
        std::string m_execID;
        std::string m_settlDate;
        char m_execType;
        char m_ordStatus;
        char m_side;
        double m_orderQty;
        double m_orderPx;
        double m_lastQty;
        double m_lastPx;
        double m_leavesQty;
        double m_cumQty;
        double m_avgPx;
        std::string m_text;
        std::string m_account;
        std::string m_orderText;
        std::string m_username;
        std::string m_quoteId;
        char m_tif;
		std::string m_customPbTag;
    	int64_t m_transactionTime;
        /*! Map between party roles and their respective IDs */
        //std::map<int, std::string> m_parties;
    };

    struct OrderDataBase
    {
        OrderDataBase(std::string id, char ordType, UTILS::CurrencyPair instrument, UTILS::Currency currency, double qty, double minQty, char side, double price,
                      char handlinst, char timeInForce, std::string account, std::string quoteID, int positionNo, std::string partyID,
                      std::string settlDate, std::string originator, std::string customPb)
                : m_id(std::move(id)), m_ordType(ordType), m_instrument(instrument), m_currency(currency), m_qty(qty), m_minQty(minQty), m_side(side),
                  m_price(price), m_handlinst(handlinst), m_timeInForce(timeInForce), m_account(std::move(account)), m_quoteID(std::move(quoteID)),
                  m_positionNo(positionNo), m_partyID(std::move(partyID)), m_settlDate(std::move(settlDate)), m_originator(std::move(originator)), m_customPb(std::move(customPb)) { }

        std::string m_id;
        char m_ordType;
        UTILS::CurrencyPair m_instrument;
        UTILS::Currency m_currency;
        double m_qty;
        double m_minQty;
        char m_side;
        double m_price;
        char m_handlinst;
        char m_timeInForce;
        std::string m_account;
        std::string m_quoteID;
        int m_positionNo;
        std::string m_partyID;
        std::string m_settlDate;
        std::string m_originator;
		std::string m_customPb;
    };

    struct NewOrderSingleData : OrderDataBase
    {
        using Ptr = std::unique_ptr<NewOrderSingleData>;

        template <typename ...Args>
        explicit NewOrderSingleData(std::string settlDate, Args &&...args)
                : OrderDataBase(std::forward<Args>(args)...)
        {
            m_settlDate = std::move(settlDate);
        }
    };

    struct OrderCancelData
    {
        using Ptr = std::unique_ptr<OrderCancelData>;

        OrderCancelData(std::string prevID, std::string id, UTILS::CurrencyPair instrument, char side, double qty)
                : m_prevID(std::move(prevID)), m_id(std::move(id)), m_instrument(instrument), m_side(side), m_qty(qty) { }

        std::string m_prevID;
        std::string m_id;
        UTILS::CurrencyPair m_instrument;
        char m_side;
        double m_qty;
    };

    struct OrderCancelReplaceData : OrderDataBase
    {
        using Ptr = std::unique_ptr<OrderCancelReplaceData>;

        template <typename ...Args>
        explicit OrderCancelReplaceData(std::string prevID, Args &&...args)
                : OrderDataBase(std::forward<Args>(args)...), m_prevID(std::move(prevID)) { }

        std::string m_prevID;
    };

    struct MarketDataRejectData
    {
        using Ptr = std::unique_ptr<MarketDataRejectData>;

        MarketDataRejectData(std::string reqID, char reason, std::string text)
                : m_reqID(std::move(reqID)), m_reason(reason), m_text(std::move(text)) { }

        std::string m_reqID;
        char m_reason;
        std::string m_text;
    };

    struct SecurityListData
    {
        using Ptr = std::unique_ptr<SecurityListData>;

        SecurityListData(const std::vector<UTILS::CurrencyPair> &instruments, std::string securityReqID, std::string responseID)
                : m_instruments(instruments), m_securityReqID(std::move(securityReqID)), m_responseID(std::move(responseID)) { }

        const std::vector<UTILS::CurrencyPair> &m_instruments;
        std::string m_securityReqID;
        std::string m_responseID;
    };

    struct SecurityListReqData
    {
        using Ptr = std::unique_ptr<SecurityListReqData>;

        explicit SecurityListReqData(std::string id)
                : m_id(std::move(id)) { }

        std::string m_id;
    };

    struct MDRequestData
    {
        using Ptr = std::unique_ptr<MDRequestData>;

        MDRequestData(std::string id, char subscriptionRequestType, int updateType, const std::vector<UTILS::CurrencyPair> *instruments, int depth,
                      const std::vector<int64_t> *priceBands = nullptr, const std::vector<std::string> *providers = nullptr, std::string account={})
                : m_id(std::move(id)), m_subscriptionRequestType(subscriptionRequestType), m_updateType(updateType), m_instruments(instruments),
                  m_depth(depth), m_priceBands(priceBands), m_providers(providers), m_account(account) { }

        std::string m_id;
        char m_subscriptionRequestType;
        int m_updateType;
        const std::vector<UTILS::CurrencyPair> *m_instruments;
        int m_depth;
        const std::vector<int64_t> *m_priceBands;
        const std::vector<std::string> *m_providers;
		std::string m_account;
    };

    /*!
     * \brief This structure holds all data relevant to generating an Order Cancel Reject Message
     */
    struct OrderCancelRejectData
    {

        using Ptr = std::unique_ptr<OrderCancelRejectData>;

        /*!
         *
         * Ctor.
         *
         * @param orderID           Order ID from the Order Cancel Message
         * @param clOrdID           Client Order ID from the Order Cancel Message
         * @param origClOrdID       Client Order ID from the New Order Single to be Cancelled
         * @param ordStatus         Current Status of Order
         * @param account           Account mnemonic as agredd between broker and institution
         * @param cxlRejResponseTo  '1' - Order Cancel Request, '2' - Order Cancel/Replace Request
         * @param cxlRejReason      0 - Too late to cancel, 1 - Unknown order, 2 - Broker option,
         * 3 - Order already in Pending Cancel or Pending Replace status
         * @param text              Optional free format text string
         */
        OrderCancelRejectData(std::string orderID, std::string clOrdID, std::string origClOrdID, char ordStatus, std::string account,
                              char cxlRejResponseTo, int64_t cxlRejReason, std::string text)
                : m_orderID(std::move(orderID)), m_clOrdID(std::move(clOrdID)), m_origClOrdID(std::move(origClOrdID)), m_ordStatus(ordStatus),
                  m_account(std::move(account)), m_cxlRejResponseTo(cxlRejResponseTo), m_cxlRejReason(cxlRejReason), m_text(std::move(text)) { }

        std::string m_orderID;
        std::string m_clOrdID;
        std::string m_origClOrdID;
        char m_ordStatus;
        std::string m_account;
        char m_cxlRejResponseTo;
        int64_t m_cxlRejReason;
        std::string m_text;
    };

    struct RejectData
    {
        using Ptr = std::unique_ptr<RejectData>;

        RejectData(int64_t refSeqNum, int64_t reason, std::string text)
                : m_refSeqNum(refSeqNum), m_reason(reason), m_text(std::move(text)) { }

        int64_t m_refSeqNum;
        int64_t m_reason;
        std::string m_text;
    };

    struct BusinessMessageRejectData
    {
        using Ptr = std::unique_ptr<BusinessMessageRejectData>;

        BusinessMessageRejectData(int64_t refSeqNum, std::string refMsgType, std::string businessRejectRefID, int64_t businessRejectReason,
                                  std::string text)
                : m_refSeqNum(refSeqNum), m_refMsgType(std::move(refMsgType)), m_businessRejectRefID(std::move(businessRejectRefID)),
                  m_businessRejectReason(businessRejectReason), m_text(std::move(text)) { }

        int64_t m_refSeqNum;
        std::string m_refMsgType;
        std::string m_businessRejectRefID;
        int64_t m_businessRejectReason;
        std::string m_text;
    };

    /*!
     * \brief This structure holds all data relevant to generating a "Don't Know Trade" message
     */
    struct OrderTimeoutData
    {

        using Ptr = std::unique_ptr<OrderTimeoutData>;

        /** Constructor
         *
         * @param clOrdID [in]          ClOrdID of the Order message to be rejected
         */
        OrderTimeoutData(std::string clOrdID)
                : m_clOrdID(std::move(clOrdID)) { }

        std::string m_clOrdID;
    };

    /*!
     * \brief This structure holds all data relevant to generating a "Don't Know Trade" message
     */
    struct DontKnowTradeData
    {

        using Ptr = std::unique_ptr<DontKnowTradeData>;

        /*!
         *
         * Ctor.
         *
         * @param clOrdID [in]          ClOrID of the ExecutionReport message to be rejected
         * @param orderID [in]          OrderID of the ExecutionReport message to be rejected
         * @param execID [in]           ExecID of the ExecutionReport message to be rejected
         * @param dkReason [in]         Code for Reason of the rejection
         * @param instrument [in]       Client Order ID of the ExecutionReport message to be rejected
         * @param side [in]             Side of the ExecutionReport message to be rejected
         * @param text [in]             Additional text (optional)
         */
        DontKnowTradeData(std::string clOrdID, std::string orderID, std::string execID, char dkReason, UTILS::CurrencyPair instrument, char side,
                          std::string text = "")
                : m_clOrdID(std::move(clOrdID)), m_orderID(std::move(orderID)), m_execID(std::move(execID)), m_dkReason(dkReason),
                  m_instrument(instrument), m_side(side), m_text(std::move(text)) { }

        std::string m_clOrdID;
        std::string m_orderID;
        std::string m_execID;
        char m_dkReason;
        UTILS::CurrencyPair m_instrument;
        char m_side;
        std::string m_text;
    };

    struct TradingSessionStatusRequestData
    {
        using Ptr = std::unique_ptr<TradingSessionStatusRequestData>;

        TradingSessionStatusRequestData(std::string tradSesReqID, std::string tradingSessionID, char subscriptionRequestType)
                : tradSesReqID(std::move(tradSesReqID)), tradingSessionID(std::move(tradingSessionID)),
                  subscriptionRequestType(subscriptionRequestType) { }

        std::string tradSesReqID;
        std::string tradingSessionID;
        char subscriptionRequestType;
    };

    struct TradingSessionStatusData
    {
        using Ptr = std::unique_ptr<TradingSessionStatusData>;

        TradingSessionStatusData(bool unsolicitedIndicator, std::string tradSesReqID, std::string tradingSessionID, int tradSesStatus)
                : unsolicitedIndicator(unsolicitedIndicator), tradSesReqID(std::move(tradSesReqID)), tradingSessionID(std::move(tradingSessionID)),
                  tradSesStatus(tradSesStatus) { }

        bool unsolicitedIndicator;
        std::string tradSesReqID;
        std::string tradingSessionID;
        int tradSesStatus;
    };

    struct ProtocolReqData
    {
        using Ptr = std::unique_ptr<ProtocolReqData>;

        ProtocolReqData(std::string id, UTILS::CurrencyPair instrument)
                : m_id(std::move(id)), m_instrument(instrument) { }

        std::string m_id;
        UTILS::CurrencyPair m_instrument;
    };


	/** @brief Generic message adapter interface */
    class IMessageAdapter {
    public:
        IMessageAdapter() : m_seqNum(++m_stSeqNumGenerator) {}
        virtual ~IMessageAdapter() = default;

        virtual int GetSeqNum() const
        {
            return m_seqNum;
        }

    private:
        int m_seqNum {};
        static std::atomic<int> m_stSeqNumGenerator;
    };

	/** @brief Generic message adapter holding some message type */
    template <typename MsgType> class CMessageAdapter : public IMessageAdapter {
    public:
        CMessageAdapter(MsgType&& msg) : m_msg(std::move(msg)) {}

        const MsgType& GetMsg() const
        {
            return m_msg;
        }
    private:
        MsgType m_msg;
    };

	/** @brief Creates an adapter from specific message type */
    template <typename MsgType> std::unique_ptr<CMessageAdapter<MsgType>> MakeMessageAdapter(MsgType&& msg)
    {
        return std::make_unique<CMessageAdapter<MsgType>>(std::move(msg));
    }

} // namespace UTILS
} // namespace MESSAGE
