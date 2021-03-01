#include "crazyflie.h"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>

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
    // Takeoff constants
    static constexpr double targetDroneHeight = 0.5;
    static constexpr double targetDroneHeightEpsilon = 0.005;

    // Forward movement constants
    static constexpr double distanceToTravel = 0.07;
    static constexpr double distanceToTravelEpsilon = 0.005;
    static constexpr double breakingAcuracyEpsilon = 0.002;

    // Rotation constants
    static const CRadians rotationAngle = CRadians::PI / 8;
    static const CRadians rotationAngleEpsilon = CRadians::PI / 128;

    switch (m_currentState) {
    case DroneState::OnGround:
        m_initialPosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::Takeoff;
        break;
    case DroneState::Takeoff:
        m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, targetDroneHeight));
        m_currentState = DroneState::WaitTakeoff;
        break;
    case DroneState::WaitTakeoff:
        if (m_pcPos->GetReading().Position.GetZ() >= targetDroneHeight - targetDroneHeightEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::ForwardMovement;
        }
        break;
    case DroneState::ForwardMovement:
        m_pcPropellers->SetRelativePosition(CVector3(0.0, -distanceToTravel, 0.0));
        m_lastReferencePosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::WaitForwardMovement;
        break;
    case DroneState::WaitForwardMovement:
        // If we detect a wall in front of us
        if (m_pcDistance->GetReadingsMap().begin()->second >= 0) {
            m_currentState = DroneState::BrakeMovement;
        }
        // If we finished traveling
        else if ((m_pcPos->GetReading().Position - m_lastReferencePosition).Length() >= distanceToTravel - distanceToTravelEpsilon) {
            m_currentState = DroneState::ForwardMovement;
        }
        break;
    case DroneState::BrakeMovement:
        m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
        m_lastReferencePosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::WaitBrakeMovement;
        break;
    case DroneState::WaitBrakeMovement:
        if ((m_pcPos->GetReading().Position - m_lastReferencePosition).Length() <= breakingAcuracyEpsilon) {
            m_pcPropellers->SetRelativePosition(CVector3(0.0, 0.0, 0.0));
            m_currentState = DroneState::Rotate;
        }
        m_lastReferencePosition = m_pcPos->GetReading().Position;
        break;
    case DroneState::Rotate:
        if (m_pcDistance->GetReadingsMap().begin()->second < 0) {
            m_currentState = DroneState::ForwardMovement;
        } else {
            CRadians angle;
            CVector3 vector;
            m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);
            m_lastReferenceYaw = angle;
            m_pcPropellers->SetRelativeYaw(rotationAngle);
            m_currentState = DroneState::WaitRotation;
        }
        break;
    case DroneState::WaitRotation: {
        CRadians angle;
        CVector3 vector;
        m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);

        if (std::abs((angle - m_lastReferenceYaw).GetValue()) >= rotationAngle.GetValue()) {
            m_currentState = DroneState::Rotate;
        }
        break;
    }
    case DroneState::StopRotation:
        m_pcPropellers->SetRelativeYaw(CRadians(0));
        m_currentState = DroneState::WaitStopRotation;
        break;
    case DroneState::WaitStopRotation: {
        CRadians angle;
        CVector3 vector;
        m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);

        if (std::abs((angle - m_lastReferenceYaw).GetValue()) <= rotationAngleEpsilon.GetValue()) {
            m_currentState = DroneState::ForwardMovement;
        }
        break;
    }
    }
}

void CCrazyflieController::Reset() {
}

void CCrazyflieController::Destroy() {
}

std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> CCrazyflieController::GetLogData()
    const {
    // TODO: Add more log data to the map and crazyflie manager (match the names for keys from cflib) (velocity)

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
    float angleDegrees = static_cast<float>(ToDegrees(angleRadians.SignedNormalize()).GetValue());
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

    // Range group
    CCI_CrazyflieDistanceScannerSensor::TReadingsMap distanceReadings = m_pcDistance->GetReadingsMap();
    static const std::array<std::string, 4> rangeLogNames = {"range.front", "range.left", "range.back", "range.right"};
    decltype(logDataMap)::mapped_type rangeLog;
    for (auto it = distanceReadings.begin(); it != distanceReadings.end(); ++it) {
        std::size_t index = std::distance(distanceReadings.begin(), it);
        Real rangeData = (it->second == -1)? 0 : ((it->second == -2)? 4000 : it->second * 10);
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

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
