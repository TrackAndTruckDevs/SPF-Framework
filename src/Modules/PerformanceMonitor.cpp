#include "SPF/Modules/PerformanceMonitor.hpp"

#include <numeric>  // For std::accumulate
#include <limits>   // For std::numeric_limits

SPF_NS_BEGIN
namespace Modules {

PerformanceMonitor& PerformanceMonitor::GetInstance() {
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor()
    : m_deltaTime(0.0f),
      m_currentFPS(0.0f),
      m_rollingMinFPS(std::numeric_limits<float>::max()),
      m_rollingMaxFPS(0.0f),
      m_rollingAvgFPS(0.0f),
      m_globalMinFPS(std::numeric_limits<float>::max()),
      m_globalMaxFPS(0.0f),
      m_consecutiveLowFrames(0),
      m_consecutiveHighFrames(0) {
    m_rawFpsHistory.resize(ROLLING_MINMAX_FPS_HISTORY_SIZE, 0.0f);
    // m_smoothedFpsHistory.resize(SMOOTHED_FPS_HISTORY_SIZE, 0.0f); // Not needed as graph is removed
}

void PerformanceMonitor::Update(float deltaTime) {
    m_deltaTime = deltaTime;

    // Calculate current FPS, avoiding division by zero.
    m_currentFPS = (m_deltaTime > 0.00001f) ? (1.0f / m_deltaTime) : 0.0f;

    // --- Update Raw FPS History ---
    m_rawFpsHistory.push_back(m_currentFPS);
    if (m_rawFpsHistory.size() > ROLLING_MINMAX_FPS_HISTORY_SIZE) {
        m_rawFpsHistory.pop_front();
    }

    if (!m_rawFpsHistory.empty()) {
        // --- Recalculate Rolling Average ---
        // Sum only the last ROLLING_AVG_FPS_HISTORY_SIZE elements
        size_t count = std::min(m_rawFpsHistory.size(), ROLLING_AVG_FPS_HISTORY_SIZE);
        auto first = m_rawFpsHistory.end() - count;
        float sum = std::accumulate(first, m_rawFpsHistory.end(), 0.0f);
        m_rollingAvgFPS = sum / count;

        // --- Recalculate Rolling Min/Max (over ROLLING_MINMAX_FPS_HISTORY_SIZE) ---
        m_rollingMinFPS = *std::min_element(m_rawFpsHistory.begin(), m_rawFpsHistory.end());
        m_rollingMaxFPS = *std::max_element(m_rawFpsHistory.begin(), m_rawFpsHistory.end());
    } else {
        m_rollingAvgFPS = 0.0f;
        m_rollingMinFPS = 0.0f;
        m_rollingMaxFPS = 0.0f;
    }

    // --- Update Global Filtered Min/Max FPS ---
    if (m_currentFPS > 0.0f) { // Only consider non-zero FPS for min/max
        // Global Min FPS
        if (m_currentFPS < m_globalMinFPS || m_globalMinFPS == std::numeric_limits<float>::max()) { // Candidate for new global min
            if (m_currentFPS < (m_globalMinFPS - 0.1f) || m_globalMinFPS == std::numeric_limits<float>::max()) { // Significant drop
                m_consecutiveLowFrames++;
                if (m_consecutiveLowFrames >= FILTER_FRAME_COUNT) {
                    m_globalMinFPS = m_currentFPS;
                    m_consecutiveLowFrames = 0; // Reset after updating
                }
            } else { // Not a significant drop, or fluctuating around current min
                m_consecutiveLowFrames = 0; // Reset if not a sustained drop
            }
        } else {
            m_consecutiveLowFrames = 0; // Reset if FPS is no longer low
        }

        // Global Max FPS
        if (m_currentFPS > m_globalMaxFPS) { // Candidate for new global max
            if (m_currentFPS > (m_globalMaxFPS + 0.1f)) { // Significant increase
                m_consecutiveHighFrames++;
                if (m_consecutiveHighFrames >= FILTER_FRAME_COUNT) {
                    m_globalMaxFPS = m_currentFPS;
                    m_consecutiveHighFrames = 0; // Reset after updating
                }
            } else { // Not a significant increase, or fluctuating around current max
                m_consecutiveHighFrames = 0; // Reset if not a sustained increase
            }
        } else {
            m_consecutiveHighFrames = 0; // Reset if FPS is no longer high
        }
    } else { // FPS is 0, reset counters to not falsely trigger min/max
        m_consecutiveLowFrames = 0;
        m_consecutiveHighFrames = 0;
    }


    // --- Update Smoothed FPS History for Graph --- (Commented out as graph is removed)
    // m_smoothedFpsHistory.push_back(m_rollingAvgFPS); // Use rolling average for smoother graph
    // if (m_smoothedFpsHistory.size() > SMOOTHED_FPS_HISTORY_SIZE) {
    //     m_smoothedFpsHistory.pop_front();
    // }
}

float PerformanceMonitor::GetCurrentFPS() const {
    return m_currentFPS;
}

float PerformanceMonitor::GetRollingMinFPS() const {
    return m_rollingMinFPS;
}

float PerformanceMonitor::GetRollingMaxFPS() const {
    return m_rollingMaxFPS;
}

float PerformanceMonitor::GetRollingAvgFPS() const {
    return m_rollingAvgFPS;
}

float PerformanceMonitor::GetGlobalMinFPS() const {
    // Return 0 if no valid frames have been processed yet.
    return (m_globalMinFPS == std::numeric_limits<float>::max()) ? 0.0f : m_globalMinFPS;
}

float PerformanceMonitor::GetGlobalMaxFPS() const {
    return m_globalMaxFPS;
}

// const std::deque<float>& PerformanceMonitor::GetRawFPSHistory() const {
//     return m_rawFpsHistory;
// }

// const std::deque<float>& PerformanceMonitor::GetSmoothedFPSHistory() const {
//     return m_smoothedFpsHistory;
// }

float PerformanceMonitor::GetDeltaTime() const {
    return m_deltaTime;
}

} // namespace Modules
SPF_NS_END

