#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>

class PrometheusMetrics
{
public:
    explicit PrometheusMetrics(std::string_view addr = "0.0.0.0:8888")
        : m_exposer(std::make_shared<prometheus::Exposer>(std::string(addr))), m_registry(std::make_shared<prometheus::Registry>())
    {
        m_exposer->RegisterCollectable(m_registry);
    }

    void counter_inc(std::string_view name, double val, const prometheus::Labels& labels = {})
    {
        const std::scoped_lock<std::mutex> lock(m_mutex);
        std::string key(name);
        const auto it = m_counters.find(key);
        if (it == m_counters.end())
        {
            auto& fam = prometheus::BuildCounter().Name(key).Register(*m_registry);
            fam.Add(labels).Increment(val);
            m_counters.emplace(std::move(key), &fam);
        }
        else
        {
            it->second->Add(labels).Increment(val);
        }
    }

    // Fast path: cache (name+labelKey+labelValue) -> Counter* to avoid std::map allocation
    [[nodiscard]] auto counter_get(std::string_view name, std::string_view labelKey, std::string_view labelValue) -> prometheus::Counter*
    {
        std::string cacheKey(name);
        cacheKey.push_back('\0');
        cacheKey.append(labelKey);
        cacheKey.push_back('\0');
        cacheKey.append(labelValue);

        const std::scoped_lock<std::mutex> lock(m_mutex);
        const auto it = m_cachedCounters.find(cacheKey);
        if (it != m_cachedCounters.end())
            return it->second;

        const auto nameIt = m_counters.find(std::string(name));
        if (nameIt == m_counters.end())
            return nullptr;

        auto* counter = &nameIt->second->Add({{std::string(labelKey), std::string(labelValue)}});
        m_cachedCounters.emplace(std::move(cacheKey), counter);
        return counter;
    }

    void gauge_set(std::string_view name, double val, const prometheus::Labels& labels = {})
    {
        const std::scoped_lock<std::mutex> lock(m_mutex);
        std::string key(name);
        const auto it = m_gauges.find(key);
        if (it == m_gauges.end())
        {
            auto& fam = prometheus::BuildGauge().Name(key).Register(*m_registry);
            fam.Add(labels).Set(val);
            m_gauges.emplace(std::move(key), &fam);
        }
        else
        {
            it->second->Add(labels).Set(val);
        }
    }

    [[nodiscard]] auto gauge_get(std::string_view name, std::string_view labelKey, std::string_view labelValue) -> prometheus::Gauge*
    {
        std::string cacheKey(name);
        cacheKey.push_back('\0');
        cacheKey.append(labelKey);
        cacheKey.push_back('\0');
        cacheKey.append(labelValue);

        const std::scoped_lock<std::mutex> lock(m_mutex);
        const auto it = m_cachedGauges.find(cacheKey);
        if (it != m_cachedGauges.end())
            return it->second;

        const auto nameIt = m_gauges.find(std::string(name));
        if (nameIt == m_gauges.end())
            return nullptr;

        auto* gauge = &nameIt->second->Add({{std::string(labelKey), std::string(labelValue)}});
        m_cachedGauges.emplace(std::move(cacheKey), gauge);
        return gauge;
    }

    void gauge_inc(std::string_view name, double val, const prometheus::Labels& labels = {})
    {
        const std::scoped_lock<std::mutex> lock(m_mutex);
        std::string key(name);
        const auto it = m_gauges.find(key);
        if (it == m_gauges.end())
        {
            auto& fam = prometheus::BuildGauge().Name(key).Register(*m_registry);
            fam.Add(labels).Increment(val);
            m_gauges.emplace(std::move(key), &fam);
        }
        else
        {
            it->second->Add(labels).Increment(val);
        }
    }

private:
    std::mutex m_mutex;
    std::shared_ptr<prometheus::Exposer> m_exposer;
    std::shared_ptr<prometheus::Registry> m_registry;
    std::unordered_map<std::string, prometheus::Family<prometheus::Counter>*> m_counters;
    std::unordered_map<std::string, prometheus::Family<prometheus::Gauge>*> m_gauges;
    std::unordered_map<std::string, prometheus::Counter*> m_cachedCounters;
    std::unordered_map<std::string, prometheus::Gauge*> m_cachedGauges;
};
