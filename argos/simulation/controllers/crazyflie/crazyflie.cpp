#include "crazyflie.h"
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
        m_pcPropellers->SetRelativePosition(CVector3(distanceToTravel, 0.0, 0.0));
        m_lastReferencePosition = m_pcPos->GetReading().Position;
        m_currentState = DroneState::WaitForwardMovement;
        break;
    case DroneState::WaitForwardMovement:
        // If we detect a wall in front of us
        if ((++m_pcDistance->GetReadingsMap().begin())->second >= 0) {
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
        if ((++m_pcDistance->GetReadingsMap().begin())->second < 0) {
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

std::unordered_map<std::string, std::variant<std::uint8_t>> CCrazyflieController::GetLogData() const {
    // TODO: Add more log data to the map (match the names for keys from cflib) with code from LogData
    return {{"pm.batteryLevel", static_cast<std::uint8_t>(m_pcBattery->GetReading().AvailableCharge * 100)}};
}

void CCrazyflieController::SetParamData(const std::string& param, std::variant<bool> value) {
    if (param == "hivexplore.isM1LedOn") {
        // TODO: Toggle LED (see CCI_LEDsActuator in footbot_foraging)
        RLOG << "LED changed: " << std::get<bool>(value) << '\n';
    }
}

// TODO: Remove to integrate in GetLogData
void CCrazyflieController::LogData() {
    // Battery sensor
    RLOG << "Battery level: " << m_pcBattery->GetReading().AvailableCharge << std::endl;

    // Distance sensors
    auto iterDistRead = m_pcDistance->GetReadingsMap().begin();
    if (m_pcDistance->GetReadingsMap().size() == 4) {
        RLOG << "Front dist: " << (iterDistRead++)->second << std::endl;
        RLOG << "Left dist: " << (iterDistRead++)->second << std::endl;
        RLOG << "Back dist: " << (iterDistRead++)->second << std::endl;
        RLOG << "Right dist: " << (iterDistRead)->second << std::endl;
    }

    // Position sensor
    RLOG << "PosSens Pos X: " << m_pcPos->GetReading().Position.GetX() << std::endl;
    RLOG << "PosSens Pos Y: " << m_pcPos->GetReading().Position.GetY() << std::endl;
    RLOG << "PosSens Pos Z: " << m_pcPos->GetReading().Position.GetZ() << std::endl;

    CRadians angle;
    CVector3 vector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);
    RLOG << "PosSens Angle X: " << vector.GetX() << std::endl;
    RLOG << "PosSens Angle Y: " << vector.GetY() << std::endl;
    RLOG << "PosSens Angle Z: " << vector.GetZ() << std::endl;
    RLOG << "PosSens Angle Angle: " << angle << std::endl;
}

REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
