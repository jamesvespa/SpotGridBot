//
// Created by james on 23/12/22.
// Contains common classes to support REST API
//
#include <Poco/URI.h>
#include "RestConnectionBase.h"

using namespace UTILS;

namespace CORE {
namespace RESTAPI {

//------------------------------------------------------------------------------
RestConnectionBase::RestConnectionBase(const CRYPTO::Settings &settings, const std::string& loggingPropsPath, const std::string &loggerName)
		: RestBase(loggerName), m_settings(settings), m_logger(settings, loggingPropsPath)
{

}


//------------------------------------------------------------------------------
RestConnectionBase::~RestConnectionBase()
{
	Disconnect();
}


//------------------------------------------------------------------------------
/*! \brief returns a copy of existing order transactions */
RestConnectionBase::TOrderTransactions RestConnectionBase::GetOrderTransactions() const
{
	return m_orderTransactions;
}


//------------------------------------------------------------------------------
void RestConnectionBase::StartOrderTransaction(const std::string &senderCompId, const UTILS::Currency::Value currency,
											   const TExecutionReports &execReports)
{
//	if (execReports.empty())
//	{
//		return;
//	}
//	auto execReport { execReports.back() };
//	switch (execReport.m_ordStatus)
//	{
//		case ORDSTATUS_NEW:
//		case ORDSTATUS_PARTIALLY_FILLED:
//		{
//			if (!m_settings.m_orderMonitoringIntervalMs)
//			{
//				poco_warning_f1(logger(), "Order transaction cannot be started because the '%s' is zero", ATTR_ORDER_MONITORING_INTERVAL);
//				return;
//			}
//
//			m_orderTransactions.emplace(zx::application::GetNextSequence(), OrderTransaction { senderCompId, currency, execReport });
//			poco_information_f4(logger(), "Started order transaction for senderCompId='%s', instrument='%s', order='%s', status='%s'", senderCompId,
//								execReport.m_instrument.ToString(), execReport.m_orderId, UTILS::OrdStatusToString(execReport.m_ordStatus));
//			break;
//		}
//		default:
//			// All other statuses are ignored - we don't need transactions for them
//			break;
//	} // switch
}


//------------------------------------------------------------------------------
void RestConnectionBase::UpdateOrderTransactions(const TOpenPositionUpdates &updates)
{
//	std::unique_lock lock(m_orderTransactions);
//	// Add new transactions from updates
//	for (const auto &iter: updates)
//	{
//		// Check if we have already transaction for this order
//		if (std::find_if(m_orderTransactions.Get().cbegin(), m_orderTransactions.Get().cend(), [&iter](const auto &pair)
//		{
//			return pair.second.ExecReport.m_clOrdID == iter.first;
//		}) == m_orderTransactions.Get().cend())
//		{
//			// This order is not monitored - add
//			const auto &senderCompId = iter.second.first;
//			const auto &instrument = iter.second.second;
//
//			auto execReport = TOOLS::CreateEmptyExecutionReportData();
//			execReport.m_instrument = iter.second.second;
//			execReport.m_clOrdID = iter.first;
//
//			m_orderTransactions.Get().emplace(zx::application::GetNextSequence(), OrderTransaction { senderCompId, execReport.m_instrument.BaseCCY(), execReport });
//			poco_information_f2(logger(), "Added transaction monitoring: senderCompId='%s', instrument='%s'", senderCompId, instrument.ToString());
//		}
//	} // for
}

//------------------------------------------------------------------------------
/*! \brief processing existing order transactions each order_monitoring_interval */
bool RestConnectionBase::ProcessOrderTransactions()
{
//	auto ae = static_cast<CORE::ADAPTERS::AdapterExecutor*>(m_ctx.Controller().GetExecutor().get());
//	std::unique_lock lock(m_orderTransactions);
//	auto &transactions = m_orderTransactions.Get();
//	for (auto iter = transactions.begin(); iter != transactions.end();)
//	{
//		auto &tr = iter->second;
//		const auto jsonResponse = QueryOrder(tr.ExecReport.m_instrument, tr.ExecReport.m_orderId, tr.ExecReport.m_clOrdID);
//		if (!jsonResponse.empty()) // ignore empty responses
//		{
//			const auto execReports = TranslateOrderResult(std::make_shared<JSONDocument>(jsonResponse));
//			if (!execReports.empty())
//			{
//				const auto &execReport = execReports.back();
//				if (execReport.m_ordStatus != tr.ExecReport.m_ordStatus || execReport.m_leavesQty != tr.ExecReport.m_leavesQty)
//				{
//					// Status or qty changed
//					// Update transaction
//					tr.ExecReport = execReport;
//					// Send update to subscribers
//					const auto res = ae->SendExecutionReport(*this, tr.SenderCompId, tr.Currency, execReport);
//					if (!res)
//					{
//						poco_error_f1(logger(), "Failed to send execution report: %s", res.ErrorMessage());
//					}
//					else
//					{
//						// Check if transaction can be finished
//						if (tr.ExecReport.m_ordStatus != ORDSTATUS_NEW && tr.ExecReport.m_ordStatus != ORDSTATUS_PARTIALLY_FILLED)
//						{
//							poco_information_f3(logger(), "Transaction for '%s' (%s) has been finished. Order Status is '%s'", tr.SenderCompId,
//												tr.ExecReport.m_instrument.ToString(), UTILS::OrdStatusToString(tr.ExecReport.m_ordStatus));
//							iter = transactions.erase(iter);
//							continue;
//						}
//					}
//				}
//			}
//		}
//
//		++iter;
//	}
	
	return true;
}

//------------------------------------------------------------------------------
/*! \brief Translates order result from json string
* @return: ExecutionReportData
* */
RestConnectionBase::TExecutionReports RestConnectionBase::TranslateOrderResult(const std::string &jsonStr) const
{
	try
	{
		return TranslateOrderResult(std::make_shared<CRYPTO::JSONDocument>(jsonStr));
	}
	catch (std::exception &e)
	{
		auto emptyResult = CRYPTO::TOOLS::CreateEmptyExecutionReportData();
		emptyResult.m_text = UTILS::Format("Failed to parse order result from '%s': %s", jsonStr, std::string(e.what()));
		return { emptyResult };
	}
}

}
}