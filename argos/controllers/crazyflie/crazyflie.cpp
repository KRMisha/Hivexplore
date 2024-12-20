#include "crazyflie.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "utils/constants.h"
#include "utils/param_name.h"

namespace {
    // Sensor reading constants
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;
    static constexpr std::uint16_t edgeDetectedThreshold = 1200;
    static constexpr std::uint16_t openSpaceThreshold = 600;
    static constexpr std::uint16_t meterToMillimeterFactor = 1000;

    // Explore constants
    static constexpr std::uint8_t initialLowBatteryIgnoredTicks = 40;
    static constexpr std::uint16_t initialReorientationTicks = 20;
    static constexpr std::uint16_t maximumReorientationTicks = 600;
    static constexpr std::uint16_t stabilizeRotationTicks = 40;

    // Return to base constants
    static constexpr std::uint16_t maximumReturnTicks = 800;
    static constexpr std::uint64_t initialExploreTicks = 600;
    static constexpr std::uint16_t clearObstacleTicks = 120;

    constexpr double CalculateObstacleDistanceCorrection(double threshold, double reading) {
        return reading == obstacleTooFar ? 0.0 : threshold - std::min(threshold, reading);
    }

    std::uint8_t GetRandomRotationChangeCount() {
        static std::random_device randomDevice;
        static std::default_random_engine randomEngine(randomDevice());

        static constexpr std::uint8_t minRotationCount = 3;
        static constexpr std::uint8_t maxRotationCount = 6;
        static std::uniform_int_distribution<std::uint8_t> randomDistribution(minRotationCount, maxRotationCount);

        return randomDistribution(randomEngine);
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

    m_initialPosition = m_pcPos->GetReading().Position;

    Reset();
}

void CCrazyflieController::ControlStep() {
    // Clear the debug print only if it has been flushed (with '\n') in the previous step
    if (m_debugPrint.find('\n') != std::string::npos) {
        m_debugPrint.clear();
    }

    UpdateBatteryLevel();
    UpdateVelocity();
    UpdateSensorReadings();
    UpdateRssi();

    if (m_isOutOfService) {
        return;
    }

    const bool shouldNotBroadcastPosition = m_missionState == MissionState::Standby ||
                                            (m_missionState == MissionState::Exploring &&
                                             (m_exploringState == ExploringState::Idle || m_exploringState == ExploringState::Liftoff)) ||
                                            (m_missionState == MissionState::Returning && m_returningState == ReturningState::Idle) ||
                                            (m_missionState == MissionState::Emergency && m_emergencyState == EmergencyState::Idle);
    if (!shouldNotBroadcastPosition) {
        PingOtherDrones();
    }

    switch (m_missionState) {
    case MissionState::Standby:
        m_droneStatus = DroneStatus::Standby;
        ResetInternalStates();
        break;
    case MissionState::Exploring:
        if (!AvoidObstaclesAndDrones()) {
            if (m_isBatteryBelowMinimumThreshold) {
                ReturnToBase();
            } else {
                Explore();
            }
        }
        break;
    case MissionState::Returning:
        if (!AvoidObstaclesAndDrones()) {
            ReturnToBase();
        }
        break;
    case MissionState::Emergency:
        EmergencyLand();
        break;
    case MissionState::Landed:
        m_droneStatus = DroneStatus::Landed;
        break;
    }

    if (IsCrashed()) {
        m_isOutOfService = true;
        m_droneStatus = DroneStatus::Crashed;
        m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
        m_pcPropellers->SetRelativeYaw(CRadians(0.0));
    }

    m_previousPosition = m_pcPos->GetReading().Position;
}

void CCrazyflieController::Reset() {
    ResetInternalStates();
}

void CCrazyflieController::Destroy() {
}

// Returns an unordered_map. The key is the log config name and the value is
// an unordered_map that contains the log variables' names and their values
CCrazyflieController::LogConfigs CCrazyflieController::GetLogData() const {
    // Fill map progressively
    LogConfigs logDataMap;

    // Battery level group
    LogVariableMap batteryLevelLog;
    batteryLevelLog.emplace("hivexplore.batteryLevel", m_batteryLevel);
    logDataMap.emplace_back(LogName::BatteryLevel, batteryLevelLog);

    // Orientation group
    CRadians angleRadians;
    CVector3 angleUnitVector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angleRadians, angleUnitVector);
    Real angleDegrees = ToDegrees(angleRadians.SignedNormalize()).GetValue();
    LogVariableMap orientationLog;
    orientationLog.emplace("stateEstimate.roll", static_cast<float>(angleDegrees * angleUnitVector.GetX()));
    orientationLog.emplace("stateEstimate.pitch", static_cast<float>(angleDegrees * angleUnitVector.GetY()));
    // Rotate 90 degrees clockwise to make a yaw of 0 face forward
    orientationLog.emplace("stateEstimate.yaw", static_cast<float>(angleDegrees * angleUnitVector.GetZ() - 90.0));
    logDataMap.emplace_back(LogName::Orientation, orientationLog);

    // Position group
    CVector3 position = m_pcPos->GetReading().Position;
    LogVariableMap positionLog;
    positionLog.emplace("stateEstimate.x", static_cast<float>(position.GetX()));
    positionLog.emplace("stateEstimate.y", static_cast<float>(position.GetY()));
    positionLog.emplace("stateEstimate.z", static_cast<float>(position.GetZ()));
    logDataMap.emplace_back(LogName::Position, positionLog);

    // Velocity group
    LogVariableMap velocityLog;
    velocityLog.emplace("stateEstimate.vx", static_cast<float>(m_velocityReading.GetX()));
    velocityLog.emplace("stateEstimate.vy", static_cast<float>(m_velocityReading.GetY()));
    velocityLog.emplace("stateEstimate.vz", static_cast<float>(m_velocityReading.GetZ()));
    logDataMap.emplace_back(LogName::Velocity, velocityLog);

    // Range group - must be added after orientation and position
    static const std::array<std::string, 6> rangeLogNames =
        {"range.front", "range.left", "range.back", "range.right", "range.up", "range.zrange"};
    LogVariableMap rangeLog = GetSensorReadings<std::uint16_t, LogVariableMap::mapped_type>(rangeLogNames);
    logDataMap.emplace_back(LogName::Range, rangeLog);

    // RSSI group
    LogVariableMap rssiLog;
    rssiLog.emplace("radio.rssi", m_rssiReading);
    logDataMap.emplace_back(LogName::Rssi, rssiLog);

    // Drone status group
    LogVariableMap droneStatusLog;
    droneStatusLog.emplace("hivexplore.droneStatus", static_cast<std::uint8_t>(m_droneStatus));
    logDataMap.emplace_back(LogName::DroneStatus, droneStatusLog);

    return logDataMap;
}

const std::string& CCrazyflieController::GetDebugPrint() const {
    return m_debugPrint;
}

void CCrazyflieController::SetParamData(const std::string& param, json value) {
    if (param == "hivexplore." + paramNameToString(ParamName::MissionState)) {
        m_missionState = static_cast<MissionState>(value.get<std::uint8_t>());
        RLOG << "Set mission state: " << static_cast<std::uint8_t>(m_missionState) << '\n';
    } else if (param == "hivexplore." + paramNameToString(ParamName::IsLedEnabled)) {
        // Print LED state since simulated Crazyflie doesn't have LEDs
        RLOG << "Set LED state: " << value.get<bool>() << '\n';
    } else {
        RLOG << "Unknown param: " << param << '\n';
    }
}

bool CCrazyflieController::AvoidObstaclesAndDrones() {
    // The obstacle detection threshold (similar to the logic found in the drone firmware) is smaller than the map edge rotation
    // detection threshold to avoid conflicts between the obstacle/drone collision avoidance and the exploration logic
    static constexpr std::uint16_t obstacleDetectedThreshold = 300;

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
                CalculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["left"]) * avoidanceSensitivity;
            rightDistanceCorrection +=
                CalculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["right"]) * avoidanceSensitivity;
            frontDistanceCorrection +=
                CalculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["front"]) * avoidanceSensitivity;
            backDistanceCorrection +=
                CalculateObstacleDistanceCorrection(obstacleDetectedThreshold, m_sensorReadings["back"]) * avoidanceSensitivity;
        }

        // Drone collision avoidance
        if (isOtherDroneDetected) {
            for (const auto& packet : m_pcRABS->GetReadings()) {
                const double horizontalAngle = packet.HorizontalBearing.GetValue();
                // Convert packet range from cm to mm
                const auto vectorToDrone = packet.Range * 10 * CVector3(std::cos(horizontalAngle), std::sin(horizontalAngle), 0.0);
                static const double droneAvoidanceSensitivity = 1.0 / 3000.0;
                leftDistanceCorrection += vectorToDrone.GetX() * droneAvoidanceSensitivity;
                backDistanceCorrection += vectorToDrone.GetY() * droneAvoidanceSensitivity;
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
            m_isForwardCommandFinished = true;
            m_isBrakeCommandFinished = true;
            m_isRotateCommandFinished = true;
            m_exploringState = m_exploringStateOnHold;
            m_returningState = m_returningStateOnHold;
        }
    }

    return m_isAvoidingObstacle;
}

void CCrazyflieController::Explore() {
    static constexpr std::uint8_t lowBatteryThreshold = 30;
    if (m_batteryLevel < lowBatteryThreshold) {
        if (m_lowBatteryIgnoredCounter == 0) {
            m_isBatteryBelowMinimumThreshold = true;
            DebugPrint("Low battery\n");
        } else {
            m_lowBatteryIgnoredCounter--;
        }
    }

    switch (m_exploringState) {
    case ExploringState::Idle: {
        m_droneStatus = DroneStatus::Standby;

        m_exploringState = ExploringState::Liftoff;
    } break;
    case ExploringState::Liftoff: {
        m_droneStatus = DroneStatus::Liftoff;

        if (Liftoff()) {
            m_exploringState = ExploringState::Explore;
        }
    } break;
    case ExploringState::Explore: {
        m_droneStatus = DroneStatus::Flying;

        // Only reorient away from the center of mass when other drones are detected
        const std::uint8_t activeP2PIdsCount = m_pcRABS->GetReadings().size();
        if (activeP2PIdsCount > 0) {
            if (m_reorientationWatchdog == 0) {
                m_exploringState = ExploringState::BrakeAway;
                break;
            }
            m_reorientationWatchdog--;
        }

        if (!Forward()) {
            m_exploringState = ExploringState::Brake;
        }
    } break;
    case ExploringState::BrakeAway: {
        m_droneStatus = DroneStatus::Flying;

        if (Brake()) {
            DebugPrint("Reorienting\n");
            m_targetYaw = CalculateAngleAwayFromCenterOfMass();
            m_exploringState = ExploringState::RotateAway;
        }
    } break;
    case ExploringState::RotateAway: {
        m_droneStatus = DroneStatus::Flying;

        if (RotateToTargetYaw()) {
            DebugPrint("Finished reorienting\n");
            m_reorientationWatchdog = maximumReorientationTicks;
            m_exploringState = ExploringState::Explore;
        }
    } break;
    case ExploringState::Brake: {
        m_droneStatus = DroneStatus::Flying;

        if (Brake()) {
            m_exploringState = ExploringState::Rotate;
        }
    } break;
    case ExploringState::Rotate: {
        m_droneStatus = DroneStatus::Flying;

        if (Rotate()) {
            m_exploringState = ExploringState::Explore;

            m_rotationChangeWatchdog--;
            if (m_rotationChangeWatchdog == 0) {
                m_shouldTurnLeft = !m_shouldTurnLeft;
                m_rotationChangeWatchdog = GetRandomRotationChangeCount();
            }
        }
    } break;
    }
}

void CCrazyflieController::ReturnToBase() {
    // If returned to base, land
    static constexpr double distanceToReturnEpsilon = 0.3;
    static constexpr uint8_t rssiLandingThreshold = 8;
    if (m_returningState != ReturningState::Land && m_returningState != ReturningState::Idle && m_rssiReading <= rssiLandingThreshold &&
        std::abs(m_pcPos->GetReading().Position.GetX() - m_initialPosition.GetX()) <= distanceToReturnEpsilon &&
        std::abs(m_pcPos->GetReading().Position.GetY() - m_initialPosition.GetY()) <= distanceToReturnEpsilon) {
        DebugPrint("Found the base\n");
        m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
        m_returningState = ReturningState::Land;
    }

    switch (m_returningState) {
    case ReturningState::BrakeTowardsBase: {
        m_droneStatus = DroneStatus::Returning;

        // Brake before rotation towards base
        if (Brake()) {
            DebugPrint("Return: Braking towards base finished\n");

            // Calculate rotation angle to turn towards base
            m_targetYaw = CRadians(std::atan2(m_initialPosition.GetY() - m_pcPos->GetReading().Position.GetY(),
                                              m_initialPosition.GetX() - m_pcPos->GetReading().Position.GetX())) +
                          CRadians::PI / 2; // Add PI / 2 because a zero degree yaw is along negative Y

            m_returningState = ReturningState::RotateTowardsBase;
        }
    } break;
    case ReturningState::RotateTowardsBase: {
        m_droneStatus = DroneStatus::Returning;

        if (RotateToTargetYaw()) {
            m_returningState = ReturningState::Return;
        }
    } break;
    case ReturningState::Return: {
        m_droneStatus = DroneStatus::Returning;

        // Go to explore algorithm when a wall is detected in front or return watchdog is finished
        if (!Forward() || m_returnWatchdog == 0) {
            if (m_returnWatchdog == 0) {
                DebugPrint("Return: Return watchdog finished\n");
            } else {
                DebugPrint("Return: Obstacle detected\n");
            }

            // Reset counter
            m_returnWatchdog = maximumReturnTicks;

            m_returningState = ReturningState::Brake;
        } else {
            m_returnWatchdog--;
        }
    } break;
    case ReturningState::Brake: {
        m_droneStatus = DroneStatus::Returning;

        if (Brake()) {
            m_returningState = ReturningState::Rotate;
        }
    } break;
    case ReturningState::Rotate: {
        m_droneStatus = DroneStatus::Returning;

        if (Rotate()) {
            m_returningState = ReturningState::Forward;
        }
    } break;
    case ReturningState::Forward: {
        m_droneStatus = DroneStatus::Returning;

        // Check right sensor when turning left, and left sensor when turning right
        static float sensorReadingToCheck = m_shouldTurnLeft ? m_sensorReadings["right"] : m_sensorReadings["left"];

        // Return to base when obstacle has been passed or explore watchdog is finished
        if ((sensorReadingToCheck > edgeDetectedThreshold + openSpaceThreshold && m_clearObstacleCounter == 0) || m_exploreWatchdog == 0) {
            if (m_clearObstacleCounter == 0) {
                DebugPrint("Explore: Obstacle has been passed\n");
                m_maximumExploreTicks = initialExploreTicks;
            }
            if (m_exploreWatchdog == 0) {
                DebugPrint("Explore: Explore watchdog finished\n");
            }

            // Reset counters
            m_maximumExploreTicks *= 2;
            m_exploreWatchdog = m_maximumExploreTicks;
            m_clearObstacleCounter = clearObstacleTicks;

            m_shouldTurnLeft = !m_shouldTurnLeft;

            DebugPrint("Explore: Rotating towards base\n");
            m_returningState = ReturningState::BrakeTowardsBase;
            break;
        }

        if (!Forward()) {
            m_clearObstacleCounter = clearObstacleTicks;
            m_returningState = ReturningState::Brake;
        } else {
            // Reset sensor reading counter if obstacle is detected
            if (sensorReadingToCheck > edgeDetectedThreshold + openSpaceThreshold) {
                m_clearObstacleCounter--;
            } else {
                m_clearObstacleCounter = clearObstacleTicks;
            }
        }
        m_exploreWatchdog--;
    } break;
    case ReturningState::Land: {
        m_droneStatus = DroneStatus::Landing;

        if (Land()) {
            m_returningState = ReturningState::Idle;
        }
    } break;
    case ReturningState::Idle: {
        m_droneStatus = DroneStatus::Landed;
    } break;
    }
}

void CCrazyflieController::EmergencyLand() {
    switch (m_emergencyState) {
    case EmergencyState::Land: {
        m_droneStatus = DroneStatus::Landing;

        if (Land()) {
            m_emergencyState = EmergencyState::Idle;
        }
    } break;
    case EmergencyState::Idle: {
        m_droneStatus = DroneStatus::Landed;
    } break;
    }
}

// Returns true when the action is finished
bool CCrazyflieController::Liftoff() {
    static constexpr double targetDroneHeight = 0.2;
    static constexpr double targetDroneHeightEpsilon = 0.005;

    CVector3 targetPosition = CVector3(m_pcPos->GetReading().Position.GetX(), m_pcPos->GetReading().Position.GetY(), targetDroneHeight);
    m_pcPropellers->SetAbsolutePosition(targetPosition);

    // Wait for liftoff to finish
    if (m_pcPos->GetReading().Position.GetZ() >= targetDroneHeight - targetDroneHeightEpsilon) {
        m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
        return true;
    }

    return false;
}

// Returns true as long as the path forward is clear
// Returns false when the path forward is obstructed by an obstacle
bool CCrazyflieController::Forward() {
    // Change state when a wall is detected in front
    static constexpr double distanceToTravelEpsilon = 0.005;
    if (m_sensorReadings["front"] <= edgeDetectedThreshold) {
        m_isForwardCommandFinished = true;
        return false;
    }

    // Order exploration movement
    static constexpr double distanceToTravel = 0.07;
    if (m_isForwardCommandFinished) {
        m_pcPropellers->SetRelativePosition(CVector3(0.0, -distanceToTravel, 0.0));
        m_forwardCommandReferencePosition = m_pcPos->GetReading().Position;
        m_isForwardCommandFinished = false;
    }

    // If we finished traveling the exploration step
    else if ((m_pcPos->GetReading().Position - m_forwardCommandReferencePosition).Length() >= distanceToTravel - distanceToTravelEpsilon) {
        m_isForwardCommandFinished = true;
    }

    return true;
}

// Returns true when the action is finished
bool CCrazyflieController::Brake() {
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
        m_isBrakeCommandFinished = true;
        m_isForwardCommandFinished = true;
        m_brakingReferencePosition = m_pcPos->GetReading().Position;
        return true;
    }

    m_brakingReferencePosition = m_pcPos->GetReading().Position;
    return false;
}

// Returns true when the action is finished
bool CCrazyflieController::Rotate() {
    // Get current yaw
    CRadians currentYaw;
    CVector3 angleUnitVector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(currentYaw, angleUnitVector);

    // Order rotation
    CRadians rotationAngle = (m_shouldTurnLeft ? 1 : -1) * CRadians::PI / 8;
    if (m_isRotateCommandFinished) {
        m_rotationAngle = rotationAngle;
        m_lastReferenceYaw = currentYaw;
        m_pcPropellers->SetRelativeYaw(rotationAngle);
        m_isRotateCommandFinished = false;
    }

    // Wait for rotation to finish
    if (std::abs((currentYaw - m_lastReferenceYaw).GetValue()) >= m_rotationAngle.GetValue()) {
        if (m_sensorReadings["front"] > edgeDetectedThreshold + openSpaceThreshold) {
            m_isRotateCommandFinished = true;
            return true;
        }
        m_isRotateCommandFinished = true;
    }

    return false;
}

bool CCrazyflieController::RotateToTargetYaw() {
    // Rotate to target yaw
    if (m_isRotateToTargetYawCommandFinished) {
        DebugPrint("Rotating to target yaw\n");
        m_pcPropellers->SetAbsoluteYaw(m_targetYaw);
        m_isRotateToTargetYawCommandFinished = false;
    }

    // Get current absolute yaw
    CRadians angleRadians;
    CVector3 angleUnitVector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angleRadians, angleUnitVector);
    CRadians currentAbsoluteYaw = angleRadians * angleUnitVector.GetZ();

    // If target yaw has been reached, decrease stabilize rotation counter
    static const CRadians yawEpsilon = CRadians::PI / 64;
    CRadians yawDifference = currentAbsoluteYaw - m_targetYaw;

    bool isTargetYawReached = (((yawDifference <= yawEpsilon) && (yawDifference >= -yawEpsilon)) ||
                               ((yawDifference <= (CRadians::PI * 2 + yawEpsilon)) && (yawDifference >= (CRadians::PI * 2 - yawEpsilon))));

    if (!m_isRotateToTargetYawCommandFinished && m_stabilizeRotationCounter != 0 && isTargetYawReached) {
        m_stabilizeRotationCounter--;
    }

    // If target yaw been reached and the drone is stable
    if (m_stabilizeRotationCounter == 0) {
        m_isRotateToTargetYawCommandFinished = true;

        // Reset counter
        m_stabilizeRotationCounter = stabilizeRotationTicks;

        DebugPrint("Finished rotating to target yaw\n");
        return true;
    }
    return false;
}

// Returns true when the action is finished
bool CCrazyflieController::Land() {
    static constexpr double targetDroneLandHeight = 0.09;
    static constexpr double targetDroneHeightEpsilon = 0.05;

    CVector3 targetPosition = CVector3(m_pcPos->GetReading().Position.GetX(), m_pcPos->GetReading().Position.GetY(), targetDroneLandHeight);
    m_pcPropellers->SetAbsolutePosition(targetPosition);

    // Wait for land to finish
    if (m_pcPos->GetReading().Position.GetZ() <= targetDroneLandHeight + targetDroneHeightEpsilon) {
        m_droneStatus = DroneStatus::Landed;
        return true;
    }

    return false;
}

bool CCrazyflieController::IsCrashed() {
    if (m_droneStatus != DroneStatus::Crashed && (m_droneStatus == DroneStatus::Standby || m_droneStatus == DroneStatus::Landed)) {
        return false;
    }

    static constexpr double positionEpsilon = 0.005;
    if ((m_lastActivePosition - m_pcPos->GetReading().Position).Length() <= positionEpsilon) {
        m_watchdogCounter++;
    } else {
        m_watchdogCounter = 0;
        m_lastActivePosition = m_pcPos->GetReading().Position;
    }

    static constexpr std::uint8_t watchdogTimeout = 250;
    return m_watchdogCounter == watchdogTimeout;
}

void CCrazyflieController::ResetInternalStates() {
    m_exploringState = ExploringState::Idle;
    m_returningState = ReturningState::BrakeTowardsBase;
    m_emergencyState = EmergencyState::Land;

    m_isBatteryBelowMinimumThreshold = false;
    m_lowBatteryIgnoredCounter = initialLowBatteryIgnoredTicks;

    m_isAvoidingObstacle = false;
    m_exploringStateOnHold = ExploringState::Idle;
    m_returningStateOnHold = ReturningState::Return;

    m_isForwardCommandFinished = true;
    m_reorientationWatchdog = initialReorientationTicks;
    m_isBrakeCommandFinished = true;

    m_isRotateCommandFinished = true;
    m_shouldTurnLeft = true;
    m_rotationChangeWatchdog = GetRandomRotationChangeCount();
    m_isRotateToTargetYawCommandFinished = true;
    m_stabilizeRotationCounter = stabilizeRotationTicks;

    m_returnWatchdog = maximumReturnTicks;
    m_maximumExploreTicks = initialExploreTicks;
    m_exploreWatchdog = initialExploreTicks;
    m_clearObstacleCounter = clearObstacleTicks;
}

void CCrazyflieController::UpdateBatteryLevel() {
    m_batteryLevel = static_cast<std::uint8_t>(m_pcBattery->GetReading().AvailableCharge * 100);
}

void CCrazyflieController::UpdateVelocity() {
    m_velocityReading = (m_pcPos->GetReading().Position - m_previousPosition) / Constants::secondsPerTick;
}

void CCrazyflieController::UpdateSensorReadings() {
    static const std::array<std::string, 6> sensorDirections = {"front", "left", "back", "right", "up", "down"};
    m_sensorReadings = GetSensorReadings<float>(sensorDirections);
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

CRadians CCrazyflieController::CalculateAngleAwayFromCenterOfMass() {
    // Current yaw
    CRadians angleRadians;
    CVector3 angleUnitVector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angleRadians, angleUnitVector);
    CRadians currentAbsoluteYaw = angleRadians * angleUnitVector.GetZ();
    CRadians currentYawAdjustment = currentAbsoluteYaw - CRadians::PI / 2;

    // Current position
    CVector2 currentPosition = CVector2(m_pcPos->GetReading().Position.GetX(), m_pcPos->GetReading().Position.GetY());
    CVector2 centerOfMass = currentPosition;

    // Sum of other drones' received positions
    for (const auto& packet : m_pcRABS->GetReadings()) {
        const double horizontalAngle = packet.HorizontalBearing.GetValue() + currentYawAdjustment.GetValue();
        // Convert packet range from cm to m
        const auto vectorToDrone = packet.Range * 0.01 * CVector2(std::cos(horizontalAngle), std::sin(horizontalAngle));

        CVector2 otherDronePosition =
            CVector2(currentPosition.GetX() - vectorToDrone.GetY(), currentPosition.GetY() + vectorToDrone.GetX());
        centerOfMass = CVector2(centerOfMass.GetX() + otherDronePosition.GetX(), centerOfMass.GetY() + otherDronePosition.GetY());
    }

    const std::uint8_t activeP2PIdsCount = m_pcRABS->GetReadings().size();
    centerOfMass = CVector2(centerOfMass.GetX() / (activeP2PIdsCount + 1), centerOfMass.GetY() / (activeP2PIdsCount + 1));
    const auto vectorAway = CVector2(currentPosition.GetX() - centerOfMass.GetX(), currentPosition.GetY() - centerOfMass.GetY());

    return (CRadians(std::atan2(vectorAway.GetY(), vectorAway.GetX())) +
            CRadians::PI / 2); // Add PI / 2 because a zero degree yaw is along negative Y
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

    // Future work: Write sensor to get range.up value
    sensorReadings.emplace(sensorNames[4], static_cast<T>(obstacleTooFar));
    // Future work: Write sensor to get range.zrange value
    sensorReadings.emplace(sensorNames[5], static_cast<T>(m_pcPos->GetReading().Position.GetZ() * meterToMillimeterFactor));

    return sensorReadings;
}

template std::unordered_map<std::string, float> CCrazyflieController::GetSensorReadings<float>(
    const std::array<std::string, 6>& sensorNames) const;

template CCrazyflieController::LogVariableMap CCrazyflieController::GetSensorReadings<std::uint16_t>(
    const std::array<std::string, 6>& sensorNames) const;

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
