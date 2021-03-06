#include "crazyflie.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "experiments/constants.h"

namespace {
    // Sensor reading constants
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;

    static constexpr std::uint16_t meterToMillimeterFactor = 1000;

    constexpr double calculateDistanceCorrection(double threshold, double reading) { return threshold - std::min(threshold, reading); }
} // namespace

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

    static const std::array<std::string, 6> sensorDirections = {"front", "left", "back", "right", "up", "down"};
    std::unordered_map<std::string, float> sensorReadings = GetSensorReadings<float>(sensorDirections);

    // The obstacle detection threshold (similar to the logic found in the drone firmware) is smaller than the map edge rotation
    // detection threshold to avoid conflicts between the obstacle/drone collision avoidance and the exploration logic
    static constexpr std::uint16_t obstacleDetectedThreshold = 600;
    static constexpr std::uint16_t edgeDetectedThreshold = 1200;
    bool shouldAvoid = std::any_of(sensorReadings.begin(), sensorReadings.end(), [](const auto& reading) {
        return reading.second <= obstacleDetectedThreshold;
    });

    // Calculate necessary correction to avoid obstacles
    if (shouldAvoid && m_currentState != DroneState::AvoidObstacle && m_currentState != DroneState::Liftoff &&
        m_currentState != DroneState::Idle) {
        static constexpr double maximumVelocity = 1.0;
        static constexpr double avoidanceSensitivity = maximumVelocity / meterToMillimeterFactor;
        double leftDistanceCorrection =
            sensorReadings["left"] == obstacleTooFar
                ? 0.0
                : calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["left"]) * avoidanceSensitivity;
        double rightDistanceCorrection =
            sensorReadings["right"] == obstacleTooFar
                ? 0.0
                : calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["right"]) * avoidanceSensitivity;
        double frontDistanceCorrection =
            sensorReadings["front"] == obstacleTooFar
                ? 0.0
                : calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["front"]) * avoidanceSensitivity;
        double backDistanceCorrection =
            sensorReadings["back"] == obstacleTooFar
                ? 0.0
                : calculateDistanceCorrection(obstacleDetectedThreshold, sensorReadings["back"]) * avoidanceSensitivity;

        // Y: Back, -Y: Forward, X: Left, -X: Right
        auto positionCorrection =
            CVector3(rightDistanceCorrection - leftDistanceCorrection, frontDistanceCorrection - backDistanceCorrection, 0.0);

        // Avoid negligible corrections
        static constexpr double correctionEpsilon = 0.02;
        m_correctionDistance = positionCorrection.Length() <= correctionEpsilon ? 0.0 : positionCorrection.Length();
        if (m_correctionDistance != 0.0) {
            m_stateOnHold = m_currentState;
            m_currentState = DroneState::AvoidObstacle;
            m_obstacleDetectedPosition = m_pcPos->GetReading().Position;
            m_pcPropellers->SetRelativePosition(positionCorrection);
            m_isAvoidObstacleCommandFinished = false;
        }
    }

    // Simulate RSSI, considering (0, 0, 0) as the base
    static constexpr double distanceToRssiMultiplier = 5.0;
    CVector3 dronePosition = m_pcPos->GetReading().Position;
    double distanceToBase =
        std::sqrt(std::pow(dronePosition.GetX(), 2) + std::pow(dronePosition.GetY(), 2) + std::pow(dronePosition.GetZ(), 2));
    m_rssiReading = static_cast<std::uint8_t>(distanceToBase * distanceToRssiMultiplier);

    switch (m_currentState) {
    case DroneState::Idle: {
        m_initialPosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::Liftoff;
    } break;
    case DroneState::AvoidObstacle: {
        static constexpr double distanceCorrectionEpsilon = 0.015;
        if ((m_pcPos->GetReading().Position - m_obstacleDetectedPosition).Length() >= m_correctionDistance - distanceCorrectionEpsilon) {
            m_isAvoidObstacleCommandFinished = true;

            // Reset all states to avoid problems when going back to a state
            m_isLiftoffCommandFinished = true;
            m_isForwardCommandFinished = true;
            m_isBrakeCommandFinished = true;
            m_isRotateCommandFinished = true;
            m_isEmergencyLandingFinished = true;
            m_currentState = m_stateOnHold;
        }
    } break;
    case DroneState::Liftoff: {
        static constexpr double targetDroneHeight = 0.5;
        static constexpr double targetDroneHeightEpsilon = 0.005;

        // Order liftoff
        if (m_isLiftoffCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, targetDroneHeight));
            m_isLiftoffCommandFinished = false;
        }

        // Wait for liftoff to finish
        if (m_pcPos->GetReading().Position.GetZ() >= targetDroneHeight - targetDroneHeightEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::Explore;
            m_isLiftoffCommandFinished = true;
        }
    } break;
    case DroneState::Explore: {
        static constexpr double distanceToTravel = 0.07;

        // Order exploration movement
        if (m_isForwardCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, -distanceToTravel, 0.0));
            m_forwardCommandReferencePosition = m_pcPos->GetReading().Position;
            m_isForwardCommandFinished = false;
        }

        // Change state when a wall is detected in front of the drone
        static constexpr double distanceToTravelEpsilon = 0.005;
        if (sensorReadings["front"] <= edgeDetectedThreshold) {
            m_currentState = DroneState::Brake;
            m_isForwardCommandFinished = true;
        }
        // If we finished traveling the exploration step
        else if ((m_pcPos->GetReading().Position - m_forwardCommandReferencePosition).Length() >=
                 distanceToTravel - distanceToTravelEpsilon) {
            m_isForwardCommandFinished = true;
        }
    } break;
    case DroneState::Brake: {
        // Order brake
        if (m_isBrakeCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_brakingReferencePosition = m_pcPos->GetReading().Position;
            m_isBrakeCommandFinished = false;
        }

        // If position variation is negligible, end the brake command
        static constexpr double brakingAcuracyEpsilon = 0.002;
        if ((m_pcPos->GetReading().Position - m_brakingReferencePosition).Length() <= brakingAcuracyEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::Rotate;
            m_isBrakeCommandFinished = true;
        }
        m_brakingReferencePosition = m_pcPos->GetReading().Position;
    } break;
    case DroneState::Rotate: {
        // Get current yaw
        CRadians currentYaw;
        CVector3 rotationAxis;
        m_pcPos->GetReading().Orientation.ToAngleAxis(currentYaw, rotationAxis);

        static const CRadians rotationAngle = CRadians::PI / 8;

        // Order rotation
        if (m_isRotateCommandFinished) {
            m_lastReferenceYaw = currentYaw;
            m_pcPropellers->SetRelativeYaw(rotationAngle);
            m_isRotateCommandFinished = false;
        }

        // Wait for rotation to finish
        if (std::abs((currentYaw - m_lastReferenceYaw).GetValue()) >= rotationAngle.GetValue()) {
            if (sensorReadings["front"] > edgeDetectedThreshold) {
                m_currentState = DroneState::Explore;
            }
            m_isRotateCommandFinished = true;
        }
    } break;
    case DroneState::Land: {
        // Emergency land
        if (m_isEmergencyLandingFinished) {
            m_emergencyLandingPosition = m_pcPos->GetReading().Position;
            m_isEmergencyLandingFinished = false;
        }

        static constexpr double landingAltitude = 0.015;
        static constexpr double landingAltitudeEpsilon = 0.0001;

        // Wait for landing to finish
        if (m_pcPos->GetReading().Position.GetZ() >= landingAltitude - landingAltitudeEpsilon) {
            m_pcPropellers->SetAbsolutePosition(
                CVector3(m_emergencyLandingPosition.GetX(), m_emergencyLandingPosition.GetY(), landingAltitude));
            m_currentState = DroneState::Idle;
            m_isEmergencyLandingFinished = true;
        }
    } break;
    }

    m_previousPosition = m_pcPos->GetReading().Position;
}

void CCrazyflieController::Reset() {
}

void CCrazyflieController::Destroy() {
}

// Returns an unordered_map. The key is the log config name and the value is
// an unordered_map that contains the log variables' names and their values
CCrazyflieController::LogConfigs CCrazyflieController::GetLogData() const {
    // Fill map progressively
    LogConfigs logDataMap;

    // BatteryLevel group
    LogVariableMap batteryLevelLog;
    batteryLevelLog.emplace("pm.batteryLevel", static_cast<std::uint8_t>(m_pcBattery->GetReading().AvailableCharge * 100));
    logDataMap.emplace_back("BatteryLevel", batteryLevelLog);

    // Orientation group
    CRadians angleRadians;
    CVector3 vector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angleRadians, vector);
    Real angleDegrees = ToDegrees(angleRadians.SignedNormalize()).GetValue();
    LogVariableMap orientationLog;
    orientationLog.emplace("stateEstimate.roll", static_cast<float>(angleDegrees * vector.GetX()));
    orientationLog.emplace("stateEstimate.pitch", static_cast<float>(angleDegrees * vector.GetY()));
    orientationLog.emplace("stateEstimate.yaw", static_cast<float>(angleDegrees * vector.GetZ()));
    logDataMap.emplace_back("Orientation", orientationLog);

    // Position group
    CVector3 position = m_pcPos->GetReading().Position;
    LogVariableMap positionLog;
    positionLog.emplace("stateEstimate.x", static_cast<float>(position.GetX()));
    positionLog.emplace("stateEstimate.y", static_cast<float>(position.GetY()));
    positionLog.emplace("stateEstimate.z", static_cast<float>(position.GetZ()));
    logDataMap.emplace_back("Position", positionLog);

    // Velocity group
    LogVariableMap velocityLog;
    velocityLog.emplace("stateEstimate.vx", static_cast<float>(m_currentVelocity.GetX()));
    velocityLog.emplace("stateEstimate.vy", static_cast<float>(m_currentVelocity.GetY()));
    velocityLog.emplace("stateEstimate.vz", static_cast<float>(m_currentVelocity.GetZ()));
    logDataMap.emplace_back("Velocity", velocityLog);

    // Range group - must be added after orientation and position
    static const std::array<std::string, 6> rangeLogNames =
        {"range.front", "range.left", "range.back", "range.right", "range.up", "range.zrange"};
    LogVariableMap rangeLog = GetSensorReadings<std::uint16_t, LogVariableMap::mapped_type>(rangeLogNames);
    logDataMap.emplace_back("Range", rangeLog);

    // RSSI group
    LogVariableMap rssiLog;
    rssiLog.emplace("radio.rssi", m_rssiReading);
    logDataMap.emplace_back("Rssi", rssiLog);

    return logDataMap;
}

void CCrazyflieController::SetParamData(const std::string& param, std::variant<bool> value) {
    if (param == "hivexplore.isM1LedOn") {
        // Print LED state since simulated Crazyflie doesn't have LEDs
        RLOG << "LED changed: " << std::get<bool>(value) << '\n';
    }
}

void CCrazyflieController::UpdateCurrentVelocity() {
    m_currentVelocity = (m_pcPos->GetReading().Position - m_previousPosition) / Constants::secondsPerTick;
}

template<typename T, typename U = T>
std::unordered_map<std::string, U> CCrazyflieController::GetSensorReadings(const std::array<std::string, 6>& sensorNames) const {
    std::unordered_map<std::string, U> sensorReadings;
    CCI_CrazyflieDistanceScannerSensor::TReadingsMap distanceReadings = m_pcDistance->GetReadingsMap();
    for (auto it = distanceReadings.begin(); it != distanceReadings.end(); ++it) {
        std::size_t index = std::distance(distanceReadings.begin(), it);
        Real rangeData = it->second;
        static constexpr std::int8_t sensorSaturated = -1;
        static constexpr std::int8_t sensorEmpty = -2;
        if (rangeData == sensorSaturated) {
            rangeData = obstacleTooClose;
        } else if (rangeData == sensorEmpty) {
            rangeData = obstacleTooFar;
        } else {
            rangeData *= 10; // Convert cm to mm to reflect multiranger deck
        }

        sensorReadings.emplace(sensorNames[index], static_cast<T>(rangeData));
    }

    // TODO: Find sensor to get range.up value
    sensorReadings.emplace(sensorNames[4], static_cast<T>(obstacleTooFar));
    // TODO: Find sensor to get range.zrange value
    sensorReadings.emplace(sensorNames[5], static_cast<T>(m_pcPos->GetReading().Position.GetZ() * meterToMillimeterFactor));

    return sensorReadings;
}

template std::unordered_map<std::string, float> CCrazyflieController::GetSensorReadings<float>(
    const std::array<std::string, 6>& sensorNames) const;

template CCrazyflieController::LogVariableMap CCrazyflieController::GetSensorReadings<std::uint16_t>(
    const std::array<std::string, 6>& sensorNames) const;

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
