/**
* @file BgwMonitor.cpp
* @author Helmut <james.doll@bbf-it.at>
* @date 12/1/21
* @version * @copyright (c) 2021 BBF-IT
*/

#include "Utils/Utils.h"

#include "Utils/BgwMonitor.h"

namespace UTILS
{

bool BgwMonitor::Result::Valid() const
{
	return totalDuration > 0 && durations.size() > 1;
}

double BgwMonitor::Result::BusyRate() const
{
	if (!Valid())
	{
		return 0.0;
	}
	return 1.0 - double(durations.front()) / double(totalDuration);
}

double BgwMonitor::Result::FullyOccupiedRate() const
{
	if (!Valid())
	{
		return 0.0;
	}
	return double(durations.back()) / double(totalDuration);
}

double BgwMonitor::Result::AverageUtilization() const
{
	if (!Valid() || totalDuration <= durations.front())
	{
		return 0.0;
	}
	int64_t weightedDurationSum { 0 };
	for (size_t ix { 1 }; ix < durations.size(); ++ix)
	{
		weightedDurationSum += int64_t(ix) * durations[ix];
	}
	return double(weightedDurationSum) / double((durations.size() - 1) * (totalDuration - durations.front()));
}

size_t BgwMonitor::Result::MinQueueSize() const
{
	return queueSizeMap.empty() ? 0 : queueSizeMap.begin()->first;
}

size_t BgwMonitor::Result::MaxQueueSize() const
{
	return queueSizeMap.empty() ? 0 : queueSizeMap.rbegin()->first;
}

double BgwMonitor::Result::AvgQueueSize() const
{
	double result { 0.0 };
	if (!queueSizeMap.empty())
	{
		if (double dblTotalDuration { double(totalDuration) }; dblTotalDuration > 0.0)
		{
			for (const auto &it: queueSizeMap)
			{
				result += double(it.first) * (double(it.second) / dblTotalDuration);
			}
		}
	}
	return result;
}

BgwMonitor::Task::Task(size_t maxThreads, size_t busyThreads, std::optional<size_t> queueSize, int64_t timestamp)
		: m_active(maxThreads > 0 && busyThreads <= maxThreads), m_firstStart(m_active ? (timestamp != 0 ? timestamp : CurrentTimestamp()) : 0),
		  m_latestStart(m_firstStart), m_currentBusy(m_active ? busyThreads : 0), m_monitorQueueSize(queueSize.has_value()),
		  m_latestQueueSizeUpdate(m_monitorQueueSize ? m_firstStart : 0), m_currentQueueSize(m_monitorQueueSize ? queueSize.value() : 0)
{
	if (m_active)
	{
		m_durations.resize(maxThreads + 1);
	}
}

bool BgwMonitor::Task::IsActive() const
{
	std::lock_guard lock { m_mtx };
	return m_active;
}

void BgwMonitor::Task::Stop()
{
	std::lock_guard lock { m_mtx };
	m_active = false;
}

bool BgwMonitor::Task::UpdateBusyThreads(size_t busyThreads, int64_t timestamp)
{
	std::lock_guard lock { m_mtx };
	if (!m_active || busyThreads >= m_durations.size())
	{
		return false;
	}
	if (busyThreads == m_currentBusy)
	{
		return true;
	}
	const int64_t ts { timestamp != 0 ? timestamp : CurrentTimestamp() };
	m_durations[m_currentBusy] += ts - m_latestStart;
	m_currentBusy = busyThreads;
	m_latestStart = ts;
	return true;
}

bool BgwMonitor::Task::UpdateQueueSize(size_t queueSize, int64_t timestamp)
{
	if (!m_monitorQueueSize)
	{
		return false;
	}
	std::lock_guard lock { m_mtx };
	if (!m_active)
	{
		return false;
	}
	if (queueSize == m_currentQueueSize)
	{
		return true;
	}
	const int64_t ts { timestamp != 0 ? timestamp : CurrentTimestamp() };
	UpdateQueueSizeMap(m_currentQueueSize, ts - m_latestQueueSizeUpdate);
	m_currentQueueSize = queueSize;
	m_latestQueueSizeUpdate = ts;
	return true;
}

void BgwMonitor::Task::UpdateQueueSizeMap(size_t queueSize, int64_t duration)
{
	const auto &it { m_queueSizeMap.find(queueSize) };
	if (it == m_queueSizeMap.end())
	{
		m_queueSizeMap.emplace(queueSize, duration);
	}
	else
	{
		it->second += duration;
	}
}

BgwMonitor::Result BgwMonitor::Task::GetResultAndReset(int64_t timestamp)
{
	std::lock_guard lock { m_mtx };
	if (!m_active)
	{
		return { };
	}
	const int64_t ts { timestamp != 0 ? timestamp : CurrentTimestamp() };
	const size_t durationSize { m_durations.size() };
	m_durations[m_currentBusy] += ts - m_latestStart;
	if (m_monitorQueueSize)
	{
		UpdateQueueSizeMap(m_currentQueueSize, ts - m_latestQueueSizeUpdate);
	}
	Result result { ts - m_firstStart, std::move(m_durations), (m_monitorQueueSize ? std::move(m_queueSizeMap) : QueueSizeMap {})};
	m_latestStart = m_firstStart = ts;
	m_durations.clear();
	m_durations.resize(durationSize);
	if (m_monitorQueueSize)
	{
		m_latestQueueSizeUpdate = ts;
		m_queueSizeMap.clear();
	}
	return result;
}

void BgwMonitor::Task::TestOnlyGetMembers(bool &active, int64_t &firstStart, int64_t &latestStart, size_t &currentBusy, std::vector<int64_t> &durations,
										  QueueSizeMap &queueSizeMap) const
{
	active = m_active;
	firstStart = m_firstStart;
	latestStart = m_latestStart;
	currentBusy = m_currentBusy;
	durations = m_durations;
	queueSizeMap = m_queueSizeMap;
}

bool BgwMonitor::Start(size_t cntThreads)
{
	std::lock_guard lock { m_mtx };
	if (cntThreads == 0 || m_cntThreads != 0)
	{
		return false;
	}
	m_cntThreads = cntThreads;
	return true;
}

bool BgwMonitor::Stop()
{
	std::lock_guard lock { m_mtx };
	if (m_cntThreads == 0)
	{
		return false;
	}
	for (const auto &task: m_tasks)
	{
		task->Stop();
	}
	m_tasks.clear();
	m_cntThreads = 0;
	return true;
}

BgwMonitor::Task::Ptr BgwMonitor::StartTask(size_t busyThreads, std::optional<size_t> queueSize, int64_t timestamp)
{
	std::lock_guard lock { m_mtx };
	if (m_cntThreads == 0 || busyThreads > m_cntThreads)
	{
		return nullptr;
	}
	Task::Ptr result { std::make_shared<Task>(m_cntThreads, busyThreads, queueSize, timestamp) };
	m_tasks.push_back(result);
	return result;
}

bool BgwMonitor::IsActive() const
{
	std::lock_guard lock { m_mtx };
	return m_cntThreads > 0;
}

bool BgwMonitor::UpdateIdleThreads(size_t idleThreads, int64_t timestamp)
{
	std::lock_guard lock { m_mtx };
	if (m_cntThreads == 0 || idleThreads > m_cntThreads)
	{
		return false;
	}
	
	ForEachTaskDo([this, idleThreads, timestamp](Task &t) { t.UpdateBusyThreads(m_cntThreads - idleThreads, timestamp); });
	return true;
}

bool BgwMonitor::UpdateQueueSize(size_t queueSize, int64_t timestamp)
{
	std::lock_guard lock { m_mtx };
	ForEachTaskDo([queueSize, timestamp](Task &t) { t.UpdateQueueSize(queueSize, timestamp); });
	return true;
}

} // namespace UTILS
