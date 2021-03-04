#include "crazyflie.h"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "experiments/constants.h"

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

    // Rotation constants
    static const CRadians rotationAngle = CRadians::PI / 8;
    static const CRadians rotationAngleEpsilon = CRadians::PI / 128;

    static constexpr uint16_t obstacleDetectedThreshold = 500;
    static constexpr uint16_t edgeDetectedThreshold = 1000;

    // Sensor reading constants
    static constexpr std::int8_t sensorSaturated = -1;
    static constexpr std::int8_t sensorEmpty = -2;
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;

    // Get sensor readings to avoid duplicate accessing logic
    // TODO: extract this logic from GetLogData() and ControlStep()
    std::unordered_map<std::string, float> sensorReadings;
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
        static const std::array<std::string, 4> sensorDirection = {"front", "left", "back", "right"};
        sensorReadings.emplace(sensorDirection[index], rangeData);
    }

    switch (m_currentState) {
    case DroneState::OnGround: {
        m_initialPosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::Takeoff;
    } break;
    case DroneState::Takeoff: {
        // Takeoff constants
        static constexpr double targetDroneHeight = 0.5;
        static constexpr double targetDroneHeightEpsilon = 0.005;

        // Order takeoff
        if (m_isLiftoffCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, targetDroneHeight));
            m_isLiftoffCommandFinished = false;
        }

        // Change state when takeoff finished
        if (m_pcPos->GetReading().Position.GetZ() >= targetDroneHeight - targetDroneHeightEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::ForwardMovement;
            m_isLiftoffCommandFinished = true;
        }
    } break;
    case DroneState::ForwardMovement: {
        static constexpr double distanceToTravel = 0.07;
        // Order forward movement
        if (m_isForwardCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, -distanceToTravel, 0.0));
            m_lastReferencePosition = m_pcPos->GetReading().Position;
            m_isForwardCommandFinished = false;
        }

        // If we detect a wall in front of us
        if (sensorReadings["front"] <= edgeDetectedThreshold) {
            m_currentState = DroneState::BrakeMovement;
            m_isForwardCommandFinished = true;
        }
        // If we finished traveling
        static constexpr double distanceToTravelEpsilon = 0.005;
        else if ((m_pcPos->GetReading().Position - m_lastReferencePosition).Length() >= distanceToTravel - distanceToTravelEpsilon) {
            m_isForwardCommandFinished = true;
        }
    } break;
    case DroneState::BrakeMovement: {
        // Order brake
        if (m_isBrakeCommandFinished) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_lastReferencePosition = m_pcPos->GetReading().Position;
            m_isBrakeCommandFinished = false;
        }

        // If position variation negligible, brake command finished
        static constexpr double breakingAcuracyEpsilon = 0.002;
        if ((m_pcPos->GetReading().Position - m_lastReferencePosition).Length() <= breakingAcuracyEpsilon) {
            m_isBrakeCommandFinished = true;
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::Rotate;
        }
        m_lastReferencePosition = m_pcPos->GetReading().Position;
    } break;
    case DroneState::Rotate: {
        if (sensorReadings["front"] > edgeDetectedThreshold) {
            m_currentState = DroneState::ForwardMovement;
        } else {
            CRadians angle;
            CVector3 vector;
            m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);
            m_lastReferenceYaw = angle;
            m_pcPropellers->SetRelativeYaw(rotationAngle);
            m_currentState = DroneState::WaitRotation;
        }
    } break;
    case DroneState::WaitRotation: {
        CRadians angle;
        CVector3 vector;
        m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);

        if (std::abs((angle - m_lastReferenceYaw).GetValue()) >= rotationAngle.GetValue()) {
            m_currentState = DroneState::Rotate;
        }
    } break;
    case DroneState::StopRotation: {
        m_pcPropellers->SetRelativeYaw(CRadians(0));
        m_currentState = DroneState::WaitStopRotation;
    } break;
    case DroneState::WaitStopRotation: {
        CRadians angle;
        CVector3 vector;
        m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);

        if (std::abs((angle - m_lastReferenceYaw).GetValue()) <= rotationAngleEpsilon.GetValue()) {
            m_currentState = DroneState::ForwardMovement;
        }
    } break;
    }
    m_previousDronePosition = m_pcPos->GetReading().Position;
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
}

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
