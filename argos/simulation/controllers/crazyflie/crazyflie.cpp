#include "crazyflie.h"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "experiments/constants.h"

namespace {
    uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading) {
        return obstacleThreshold - std::min(sensorReading, obstacleThreshold);
    }
}
void CCrazyflieController::Init(TConfigurationNode& t_node) {
    try {
        m_pcDistance = GetSensor<CCI_CrazyflieDistanceScannerSensor>("crazyflie_distance_scanner");
        m_pcPropellers = GetActuator<CCI_QuadRotorPositionActuator>("quadrotor_position");
        m_pcRABA = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
        m_pcRABS = GetSensor<CCI_RangeAndBearingSensor>("range_and_bearing");
        m_pcPos = GetSensor<CCI_PositioningSensor>("positioning");
        m_pcBattery = GetSensor<CCI_BatterySensor>("battery");
    } catch (CARGoSException& e) {
        THROW_ARGOSEXCEPTION_NESTED("Error initializing the Crazyflie controller for robot \"" << GetId() << "\"", e);
    }

    Reset();
}

void CCrazyflieController::ControlStep() {
    UpdateCurrentVelocity();

    std::unordered_map<std::string, float> sensorReadings;
    static const std::array<std::string, 4> sensorDirection = {"front", "left", "back", "right"};
    static constexpr std::int8_t sensorSaturated = -1;
    static constexpr std::int8_t sensorEmpty = -2;
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;
    auto distanceReadings = m_pcDistance->GetReadingsMap();
    for (auto it = distanceReadings.begin(); it != distanceReadings.end(); ++it) {
        std::size_t index = std::distance(distanceReadings.begin(), it);
        Real rangeData = it->second;
        if (rangeData == sensorSaturated) {
            rangeData = obstacleTooClose;
        } else if (rangeData == sensorEmpty) {
            rangeData = obstacleTooFar;
        } else {
            rangeData *= 10;
        }
        sensorReadings.emplace(sensorDirection[index], rangeData);
    }

    static constexpr uint16_t obstacleDetectedThreshold = 500;
    static constexpr uint16_t edgeDetectedThreshold = 1000;
    static constexpr float maximumVelocity = 1.0;
    static constexpr float explorationHeight = 1.0;
    static constexpr float cruiseVelocity = 0.2;

    double targetForwardVelocity = 0;
    double targetLeftVelocity = 0;
    double targetHeight = 0;
    double targetYawRate = 0;
    // Global obstacle avoidance
    if (m_currentState == DroneState::Liftoff || m_currentState == DroneState::Explore || m_currentState == DroneState::Rotate ||
        m_currentState == DroneState::Land) {
        // Distance correction required to stay out of range of any obstacle
        uint16_t leftDistanceCorrection = calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["left"]);
        uint16_t rightDistanceCorrection = calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["right"]);
        uint16_t frontDistanceCorrection = calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["front"]);
        uint16_t backDistanceCorrection = calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["back"]);

        // Velocity required to apply distance correction
        const float avoidanceSensitivity = maximumVelocity / obstacleDetectedThreshold;
        targetLeftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * avoidanceSensitivity;
        targetForwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * avoidanceSensitivity;
    }

    switch (m_currentState) {
    case DroneState::Idle: {
        // If start mission received
        m_currentState = DroneState::Liftoff;
    } break;
    case DroneState::Liftoff: {
        targetHeight += explorationHeight;
        static const auto heightEpsilon = 0.005;
        if (std::abs(targetHeight - m_pcPos->GetReading().Position.GetZ()) < heightEpsilon) {
            m_currentState = DroneState::Explore;
        }
    } break;
    case DroneState::Explore: {
        targetHeight += explorationHeight;
        targetForwardVelocity += cruiseVelocity;

        if (sensorReadings["front"] < edgeDetectedThreshold) {
            m_currentState = DroneState::Rotate;
        }
    } break;
    case DroneState::Rotate: {
        static constexpr uint16_t openSpaceTreshold = 300;
        targetHeight += explorationHeight;
        targetYawRate += M_PI / 4;
        if (sensorReadings["front"] > edgeDetectedThreshold + openSpaceTreshold) {
            m_currentState = DroneState::Explore;
            LOG << "Finished rotationg" << std::endl;
        }
    } break;
    case DroneState::Land: {
        static const auto heightEpsilon = 0.005;
        targetHeight += 0.02;
        if (std::abs(targetHeight - m_pcPos->GetReading().Position.GetZ()) < heightEpsilon) {
            m_currentState = DroneState::Liftoff;
        }
    } break;
    }
    m_previousDronePosition = m_pcPos->GetReading().Position;
    CVector3 rotationAxis;
    m_pcPos->GetReading().Orientation.ToAngleAxis(m_previousDroneYaw, rotationAxis);

    // apply target speed
    SetWaypoint(targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
}

void CCrazyflieController::Reset() {
}

void CCrazyflieController::Destroy() {
}

// Returns an unordered_map. The key is the log config name and the value is
// an unordered_map that contains the log variables' names and their values
std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> CCrazyflieController::
    GetLogData() const {
    // Fill map progressively
    std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> logDataMap;

    // BatteryLevel group
    decltype(logDataMap)::mapped_type batteryLevelLog;
    batteryLevelLog.emplace("pm.batteryLevel", static_cast<std::uint8_t>(m_pcBattery->GetReading().AvailableCharge * 100));
    logDataMap.emplace("BatteryLevel", batteryLevelLog);

    // Orientation group
    CRadians angleRadians;
    CVector3 vector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angleRadians, vector);
    Real angleDegrees = ToDegrees(angleRadians.SignedNormalize()).GetValue();
    decltype(logDataMap)::mapped_type orientationLog;
    orientationLog.emplace("stateEstimate.roll", static_cast<float>(angleDegrees * vector.GetX()));
    orientationLog.emplace("stateEstimate.pitch", static_cast<float>(angleDegrees * vector.GetY()));
    orientationLog.emplace("stateEstimate.yaw", static_cast<float>(angleDegrees * vector.GetZ()));
    logDataMap.emplace("Orientation", orientationLog);

    // Position group
    CVector3 position = m_pcPos->GetReading().Position;
    decltype(logDataMap)::mapped_type positionLog;
    positionLog.emplace("stateEstimate.x", static_cast<float>(position.GetX()));
    positionLog.emplace("stateEstimate.y", static_cast<float>(position.GetY()));
    positionLog.emplace("stateEstimate.z", static_cast<float>(position.GetZ()));
    logDataMap.emplace("Position", positionLog);

    // TODO: Add velocity

    // Range group
    CCI_CrazyflieDistanceScannerSensor::TReadingsMap distanceReadings = m_pcDistance->GetReadingsMap();
    static const std::array<std::string, 4> rangeLogNames = {"range.front", "range.left", "range.back", "range.right"};
    decltype(logDataMap)::mapped_type rangeLog;
    static constexpr std::int8_t sensorSaturated = -1;
    static constexpr std::int8_t sensorEmpty = -2;
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;

    for (auto it = distanceReadings.begin(); it != distanceReadings.end(); ++it) {
        std::size_t index = std::distance(distanceReadings.begin(), it);
        Real rangeData = it->second;
        if (rangeData == sensorSaturated) {
            rangeData = obstacleTooClose;
        } else if (rangeData == sensorEmpty) {
            rangeData = obstacleTooFar;
        } else {
            rangeData *= 10;
        }
        rangeLog.emplace(rangeLogNames[index], static_cast<std::uint16_t>(rangeData));
    }
    // TODO: Find sensor to get range.up value
    rangeLog.emplace("range.up", static_cast<std::uint16_t>(0));
    // TODO: Find sensor to get range.zrange value
    rangeLog.emplace("range.zrange", static_cast<std::uint16_t>(position.GetZ() * 1000));
    logDataMap.emplace("Range", rangeLog);

    return logDataMap;
}

void CCrazyflieController::SetParamData(const std::string& param, std::variant<bool> value) {
    if (param == "hivexplore.isM1LedOn") {
        // Print LED state since simulated Crazyflie doesn't have LEDs
        RLOG << "LED changed: " << std::get<bool>(value) << '\n';
    }
}

void CCrazyflieController::UpdateCurrentVelocity() {
    m_currentVelocity = (m_pcPos->GetReading().Position - m_previousDronePosition) / Constants::secondsPerTick;

    CRadians currentYaw;
    CVector3 rotationAxis;
    m_pcPos->GetReading().Orientation.ToAngleAxis(currentYaw, rotationAxis);
    m_currentYawRate = (currentYaw - m_previousDroneYaw) / Constants::secondsPerTick;
}

void CCrazyflieController::SetWaypoint(double targetForwardVelocity, double targetLeftVelocity, double targetHeight, double targetYawRate) const {
    //  ---------------- Y
    //  |
    //  |
    //  |
    //  |
    //  |
    //  X
    // Convert target relative velocity to absolute velocity
    // -Y -> Front
    // -X -> Right
    CRadians yaw;
    CVector3 rotationAxis;
    m_pcPos->GetReading().Orientation.ToAngleAxis(yaw, rotationAxis);
    // const auto xAbsoluteVelocity = std::cos(yaw.GetValue()) * targetVelocity.GetY() + std::sin(yaw.GetValue()) * targetVelocity.GetX();
    // const auto yAbsoluteVelocity = std::cos(yaw.GetValue()) * targetVelocity.GetX() + std::sin(yaw.GetValue()) * targetVelocity.GetY();
    LOG << "Yaw: " << yaw << std::endl;
    const auto xAbsoluteVelocity = targetLeftVelocity * std::cos(yaw.GetValue()) - targetForwardVelocity * std::sin(yaw.GetValue());
    const auto yAbsoluteVelocity = targetLeftVelocity * std::sin(yaw.GetValue()) - targetForwardVelocity * std::cos(yaw.GetValue());

    // Calculate next absolute position using absolute target velocity and current position
    const auto currentPosition = m_pcPos->GetReading().Position;
    CVector3 targetAbsolutePosition;
    targetAbsolutePosition.SetX(currentPosition.GetX() + (xAbsoluteVelocity - m_currentVelocity.GetX()) * Constants::secondsPerTick);
    targetAbsolutePosition.SetY(currentPosition.GetY() + (yAbsoluteVelocity - m_currentVelocity.GetY()) * Constants::secondsPerTick);
    targetAbsolutePosition.SetZ(targetHeight);

    LOG << "Correction x:" << xAbsoluteVelocity - m_currentVelocity.GetX() << std::endl;
    LOG << "Correction y:" << yAbsoluteVelocity - m_currentVelocity.GetY() << std::endl;
    m_pcPropellers->SetAbsolutePosition(targetAbsolutePosition);
    m_pcPropellers->SetAbsoluteYaw(CRadians(yaw.GetValue() + (targetYawRate - m_currentYawRate.GetValue()) * Constants::secondsPerTick));
}

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
