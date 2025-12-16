#pragma once

#include <deque>
// #include <vector> // Not needed anymore
#include <chrono>
#include <limits> // Required for std::numeric_limits

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules {

/**
 * @class PerformanceMonitor
 * @brief A singleton service for calculating and storing framework performance metrics.
 *
 * This service is driven by the renderer and provides performance data (like FPS)
 * to any part of the framework that needs it, such as the UI.
 */
class PerformanceMonitor {
public:
    /**
     * @brief Gets the singleton instance of the PerformanceMonitor.
     */
    static PerformanceMonitor& GetInstance();

    // Deleted copy/move constructors and assignment operators to enforce singleton pattern.
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    void operator=(const PerformanceMonitor&) = delete;

    /**
     * @brief Updates the performance metrics with the latest frame time.
     * @param deltaTime The time elapsed since the last visual frame, in seconds.
     */
    void Update(float deltaTime);

    // --- Public Accessors ---
    float GetCurrentFPS() const; // Raw FPS, calculated and displayed
    float GetRollingMinFPS() const; // Rolling min over ROLLING_MINMAX_FPS_HISTORY_SIZE (calculated and displayed)
    float GetRollingMaxFPS() const; // Rolling max over ROLLING_MINMAX_FPS_HISTORY_SIZE (calculated and displayed)
    float GetRollingAvgFPS() const; // Rolling average over ROLLING_AVG_FPS_HISTORY_SIZE (calculated and displayed)
    float GetGlobalMinFPS() const; // Min FPS for the whole session, filtered (calculated and displayed)
    float GetGlobalMaxFPS() const; // Max FPS for the whole session, filtered (calculated and displayed)
    // const std::deque<float>& GetRawFPSHistory() const; // Raw FPS history (calculated, but not directly exposed as deque)
    // const std::deque<float>& GetSmoothedFPSHistory() const; // Smoothed FPS history (calculation commented out)
    float GetDeltaTime() const;

private:
    // Private constructor for the singleton pattern.
    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    // --- Performance Data ---
    static constexpr size_t ROLLING_AVG_FPS_HISTORY_SIZE = 60; // For rolling average calculation
    static constexpr size_t ROLLING_MINMAX_FPS_HISTORY_SIZE = 600; // For rolling min/max calculation
    static constexpr int FILTER_FRAME_COUNT = 15; // Number of consecutive frames for global min/max filtering (0.25s at 60 FPS)

    float m_deltaTime = 0.0f;
    float m_currentFPS = 0.0f;

    // Rolling stats
    float m_rollingMinFPS;
    float m_rollingMaxFPS;
    float m_rollingAvgFPS = 0.0f;

    // Global stats for the whole session, filtered for "sustained" values
    float m_globalMinFPS;
    float m_globalMaxFPS;
    int m_consecutiveLowFrames = 0;
    int m_consecutiveHighFrames = 0;
    
    std::deque<float> m_rawFpsHistory; // Raw FPS history (max size ROLLING_MINMAX_FPS_HISTORY_SIZE)
    // std::deque<float> m_smoothedFpsHistory; // Smoothed FPS history (calculation commented out)
};

} // namespace Modules
SPF_NS_END
