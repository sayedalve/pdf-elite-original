#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

namespace utils {

class PerfLog {
public:
    static PerfLog& Instance() {
        static PerfLog instance;
        return instance;
    }

    void Log(const std::string& eventName, double durationMs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_logFile.is_open()) {
            m_logFile.open("perf_baseline.log", std::ios::app);
        }
        m_logFile << eventName << ": " << durationMs << " ms\n";
        m_logFile.flush();
    }

private:
    PerfLog() {}
    ~PerfLog() {
        if (m_logFile.is_open()) m_logFile.close();
    }
    std::ofstream m_logFile;
    std::mutex m_mutex;
};

class PerfTimer {
public:
    PerfTimer(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    ~PerfTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - m_start;
        PerfLog::Instance().Log(m_name, diff.count());
    }
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

} // namespace utils

#define PERF_SCOPE(name) utils::PerfTimer __timer_##__LINE__(name)
