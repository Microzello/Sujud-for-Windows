#pragma once

#include <cmath>
#include <numbers>

namespace sujud::qibla {

inline constexpr double kaabaLatitude = 21.422487;
inline constexpr double kaabaLongitude = 39.826206;

// Initial great-circle bearing to the Kaaba, in degrees clockwise from true north (0..<360).
inline double bearing(double latitude, double longitude) noexcept {
    constexpr double toRadians = std::numbers::pi / 180;
    const double phi1 = latitude * toRadians;
    const double phi2 = kaabaLatitude * toRadians;
    const double deltaLambda = (kaabaLongitude - longitude) * toRadians;
    const double y = std::sin(deltaLambda);
    const double x = std::cos(phi1) * std::tan(phi2) - std::sin(phi1) * std::cos(deltaLambda);
    return std::fmod(std::atan2(y, x) / toRadians + 360, 360);
}

// Signed smallest difference `to - from`, in -180...180 degrees.
inline double angleDifference(double from, double to) noexcept {
    const double diff = std::fmod(to - from, 360);
    if (diff > 180) return diff - 360;
    if (diff < -180) return diff + 360;
    return diff;
}

}
