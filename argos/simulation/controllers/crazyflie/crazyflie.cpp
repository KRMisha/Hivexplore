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

    constexpr double calculateObstacleDistanceCorrection(double threshold, double reading) {
        return reading == obstacleTooFar ? 0.0 : threshold - std::min(threshold, reading);
    }

    template<typename T>
    constexpr std::int8_t getSign(T value) {
        return value < 0 ? -1 : 1;
    }

    constexpr double calculateDroneDistanceCorrection(double threshold, double distance) {
        return getSign(distance) * (threshold - std::abs(distance));
    }

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
    // Clear the debug print only if it has been flushed (with '\n') in the previous step
    if (m_debugPrint.find('\n') != std::string::npos) {
        m_debugPrint.clear();
    }

    UpdateSensorReadings();
    UpdateVelocity();
    UpdateRssi();
    PingOtherDrones();

    switch (m_missionState) {
    case MissionState::Standby:
        m_droneStatus = DroneStatus::Standby;
        break;
    case MissionState::Exploring:
        if (!AvoidObstacle()) {
            Explore();
        }
        break;
    case MissionState::Returning:
        if (!AvoidObstacle()) {
            ReturnToBase();
        }
        break;
    case MissionState::Emergency:
        EmergencyLand();
        break;
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
    velocityLog.emplace("stateEstimate.vx", static_cast<float>(m_velocity.GetX()));
    velocityLog.emplace("stateEstimate.vy", static_cast<float>(m_velocity.GetY()));
    velocityLog.emplace("stateEstimate.vz", static_cast<float>(m_velocity.GetZ()));
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

    // DroneStatus group
    LogVariableMap droneStatusLog;
    droneStatusLog.emplace("hivexplore.droneStatus", static_cast<std::uint8_t>(m_droneStatus));
    logDataMap.emplace_back("DroneStatus", droneStatusLog);

    return logDataMap;
}

const std::string& CCrazyflieController::GetDebugPrint() const {
    return m_debugPrint;
}

void CCrazyflieController::SetParamData(const std::string& param, json value) {
    if (param == "hivexplore.missionState") {
        m_missionState = static_cast<MissionState>(value.get<std::uint8_t>());
        RLOG << "Set mission state: " << static_cast<std::uint8_t>(m_missionState) << '\n';
    } else if (param == "hivexplore.isM1LedOn") {
        // Print LED state since simulated Crazyflie doesn't have LEDs
        RLOG << "Set LED state: " << value.get<bool>() << '\n';
    } else {
        RLOG << "Unknown param: " << param << '\n';
    }
}

bool CCrazyflieController::AvoidObstacle() {
    // The obstacle detection threshold (similar to the logic found in the drone firmware) is smaller than the map edge rotation
    // detection threshold to avoid conflicts between the obstacle/drone collision avoidance and the exploration logic
    static constexpr std::uint16_t obstacleDetectedThreshold = 600;

    bool isObstacleDetected = std::any_of(m_sensorReadings.begin(), m_sensorReadings.end(), [](const auto& reading) {
        return reading.second <= obstacleDetectedThreshold;
    });

    bool isOtherDroneDetected = std::any_of(m_pcRABS->GetReadings().begin(), m_pcRABS->GetReadings().end(), [](const auto& packet) {
        return packet.Range * 10 <= obstacleDetectedThreshold; // Convert range from cm to mm
    });

    bool isExploringAvoidanceDisallowed = m_missionState == MissionState::Exploring &&
                                          (m_exploringState == ExploringState::Idle || m_exploringState == ExploringState::Liftoff);
    bool isReturningAvoidanceDisallowed =
        m_missionState == MissionState::Returning && (m_returningState == ReturningState::Idle || m_returningState == ReturningState::Land);

    bool shouldStartAvoidance = !m_isAvoidingObstacle && (isObstacleDetected || isOtherDroneDetected) && !isExploringAvoidanceDisallowed &&
                                !isReturningAvoidanceDisallowed;
    // Calculate necessary correction to avoid obstacles
    if (shouldStartAvoidance) {
        static constexpr double maximumVelocity = 1.0;
        static constexpr double avoidanceSensitivity = maximumVelocity / meterToMillimeterFactor;
        double leftDistanceCorrection = 0.0;
        double rightDistanceCorrection = 0.0;
        double frontDistanceCorrection = 0.0;
        double backDistanceCorrection = 0.0;

        // Obstacle collision avoidance
        if (isObstacleDetected) {
            leftDistanceCorrection +=
                calculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["left"]) * avoidanceSensitivity;
            rightDistanceCorrection +=
                calculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["right"]) * avoidanceSensitivity;
            frontDistanceCorrection +=
                calculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["front"]) * avoidanceSensitivity;
            backDistanceCorrection +=
                calculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["back"]) * avoidanceSensitivity;
        }

        // Drone collision avoidance
        if (isOtherDroneDetected) {
            for (const auto& packet : m_pcRABS->GetReadings()) {
                const double horizontalAngle = packet.HorizontalBearing.GetValue();
                // Convert packet range from cm to mm
                const auto vectorToDrone = packet.Range * 10 * CVector3(std::cos(horizontalAngle), std::sin(horizontalAngle), 0.0);
                static const double droneAvoidanceSensitivity = 1.0 / 3000.0;
                leftDistanceCorrection +=
                    calculateDroneDistanceCorrection(obstacleDetectedThreshold, vectorToDrone.GetX()) * droneAvoidanceSensitivity;
                backDistanceCorrection +=
                    calculateDroneDistanceCorrection(obstacleDetectedThreshold, vectorToDrone.GetY()) * droneAvoidanceSensitivity;
            }
        }

        // Y: Back, -Y: Forward, X: Left, -X: Right
        auto positionCorrection =
            CVector3(rightDistanceCorrection - leftDistanceCorrection, frontDistanceCorrection - backDistanceCorrection, 0.0);

        // Start obstacle avoidance if correction is not negligible
        static constexpr double correctionEpsilon = 0.02;
        m_correctionDistance = positionCorrection.Length() <= correctionEpsilon ? 0.0 : positionCorrection.Length();
        if (m_correctionDistance != 0.0) {
            m_isAvoidingObstacle = true;
            m_exploringStateOnHold = m_exploringState;
            m_returningStateOnHold = m_returningState;
            m_obstacleDetectedPosition = m_pcPos->GetReading().Position;
            m_pcPropellers->SetRelativePosition(positionCorrection);
        }
    }

    // Check if obstacle avoidance is finished
    if (m_isAvoidingObstacle) {
        static constexpr double distanceCorrectionEpsilon = 0.015;
        if ((m_pcPos->GetReading().Position - m_obstacleDetectedPosition).Length() >= m_correctionDistance - distanceCorrectionEpsilon) {
            m_isAvoidingObstacle = false;

            // Reset all states to avoid problems when going back to a state
            m_isLiftoffCommandFinished = true;
            m_isForwardCommandFinished = true;
            m_isBrakeCommandFinished = true;
            m_isRotateCommandFinished = true;
            m_isEmergencyLandingFinished = true;
            m_exploringState = m_exploringStateOnHold;
            m_returningState = m_returningStateOnHold;
        }
    }

    return m_isAvoidingObstacle;
}

void CCrazyflieController::Explore() {
    static constexpr std::uint16_t edgeDetectedThreshold = 1200;

    switch (m_exploringState) {
    case ExploringState::Idle: {
        m_droneStatus = DroneStatus::Standby;

        m_initialPosition = m_pcPos->GetReading().Position;
        m_exploringState = ExploringState::Liftoff;
    } break;
    case ExploringState::Liftoff: {
        m_droneStatus = DroneStatus::Flying;

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
            m_exploringState = ExploringState::Explore;
            m_isLiftoffCommandFinished = true;
        }
    } break;
    case ExploringState::Explore: {
        m_droneStatus = DroneStatus::Flying;

        static constexpr double distanceToTravel = 0.07;

        // Order exploration movement
        if (m_isForwardCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, -distanceToTravel, 0.0));
            m_forwardCommandReferencePosition = m_pcPos->GetReading().Position;
            m_isForwardCommandFinished = false;
        }

        // Change state when a wall is detected in front of the drone
        static constexpr double distanceToTravelEpsilon = 0.005;
        if (m_sensorReadings["front"] <= edgeDetectedThreshold) {
            m_exploringState = ExploringState::Brake;
            m_isForwardCommandFinished = true;
        }
        // If we finished traveling the exploration step
        else if ((m_pcPos->GetReading().Position - m_forwardCommandReferencePosition).Length() >=
                 distanceToTravel - distanceToTravelEpsilon) {
            m_isForwardCommandFinished = true;
        }
    } break;
    case ExploringState::Brake: {
        m_droneStatus = DroneStatus::Flying;

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
            m_exploringState = ExploringState::Rotate;
            m_isBrakeCommandFinished = true;
        }
        m_brakingReferencePosition = m_pcPos->GetReading().Position;
    } break;
    case ExploringState::Rotate: {
        m_droneStatus = DroneStatus::Flying;

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
            if (m_sensorReadings["front"] > edgeDetectedThreshold) {
                m_exploringState = ExploringState::Explore;
            }
            m_isRotateCommandFinished = true;
        }
    } break;
    }
}

void CCrazyflieController::ReturnToBase() {
    switch (m_returningState) {
    case ReturningState::Return: {
        m_droneStatus = DroneStatus::Flying;

        static constexpr double distanceToReturnEpsilon = 0.05;

        m_pcPropellers->SetAbsolutePosition(
            CVector3(m_initialPosition.GetX(), m_initialPosition.GetY(), m_pcPos->GetReading().Position.GetZ()));

        if (std::abs(m_pcPos->GetReading().Position.GetX() - m_initialPosition.GetX()) <= distanceToReturnEpsilon &&
            std::abs(m_pcPos->GetReading().Position.GetY() - m_initialPosition.GetY()) <= distanceToReturnEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_returningState = ReturningState::Land;
        }
    } break;
    case ReturningState::Land: {
        m_droneStatus = DroneStatus::Flying;

        if (Land()) {
            m_returningState = ReturningState::Idle;
        }
    } break;
    case ReturningState::Idle:
        m_droneStatus = DroneStatus::Landed;
        break;
    }
}

void CCrazyflieController::EmergencyLand() {
    switch (m_emergencyState) {
    case EmergencyState::Land: {
        m_droneStatus = DroneStatus::Flying;

        if(Land()) {
            m_emergencyState = EmergencyState::Idle;
        }
    } break;
    case EmergencyState::Idle:
        m_droneStatus = DroneStatus::Landed;
        break;
    }
}

bool CCrazyflieController::Land() {
    static constexpr double targetDroneLandHeight = 0.05;
    static constexpr double targetDroneHeightEpsilon = 0.05;

    m_pcPropellers->SetAbsolutePosition(m_initialPosition + CVector3(0.0, 0.0, targetDroneLandHeight));
    if (m_pcPos->GetReading().Position.GetZ() <= targetDroneLandHeight + targetDroneHeightEpsilon) {
        m_droneStatus = DroneStatus::Landed;
        return true;
    }
    return false;

    // TODO: Move emergency landing to MissionState switch
    // case ExploringState::Land: {
    //     if (m_isEmergencyLandingFinished) {
    //         m_emergencyLandingPosition = m_pcPos->GetReading().Position;
    //         m_isEmergencyLandingFinished = false;
    //     }

    //     static constexpr double landingAltitude = 0.015;
    //     static constexpr double landingAltitudeEpsilon = 0.0001;

    //     // Wait for landing to finish
    //     if (m_pcPos->GetReading().Position.GetZ() >= landingAltitude - landingAltitudeEpsilon) {
    //         m_pcPropellers->SetAbsolutePosition(
    //             CVector3(m_emergencyLandingPosition.GetX(), m_emergencyLandingPosition.GetY(), landingAltitude));
    //         m_exploringState = ExploringState::Idle;
    //         m_isEmergencyLandingFinished = true;
    //     }
    // } break;
}

void CCrazyflieController::UpdateSensorReadings() {
    static const std::array<std::string, 6> sensorDirections = {"front", "left", "back", "right", "up", "down"};
    m_sensorReadings = GetSensorReadings<float>(sensorDirections);
}

void CCrazyflieController::UpdateVelocity() {
    m_velocity = (m_pcPos->GetReading().Position - m_previousPosition) / Constants::secondsPerTick;
}

void CCrazyflieController::UpdateRssi() {
    // Simulate RSSI, considering (0, 0, 0) as the base
    static constexpr double distanceToRssiMultiplier = 5.0;
    CVector3 dronePosition = m_pcPos->GetReading().Position;
    double distanceToBase =
        std::sqrt(std::pow(dronePosition.GetX(), 2) + std::pow(dronePosition.GetY(), 2) + std::pow(dronePosition.GetZ(), 2));
    m_rssiReading = static_cast<std::uint8_t>(distanceToBase * distanceToRssiMultiplier);
}

void CCrazyflieController::PingOtherDrones() {
    static constexpr std::uint8_t pingData = 0;
    m_pcRABA->SetData(sizeof(pingData), pingData);
}

void CCrazyflieController::DebugPrint(const std::string& text) {
    RLOG << text;
    m_debugPrint += text;
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
