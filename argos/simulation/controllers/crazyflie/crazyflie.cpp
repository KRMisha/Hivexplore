/* Include the controller definition */
#include "crazyflie.h"
/* Function definitions for XML parsing */
#include <argos3/core/utility/configuration/argos_configuration.h>
/* 2D vector definition */
#include <argos3/core/utility/math/vector2.h>
/* Logging */
#include <type_traits>
#include <unordered_map>
#include <argos3/core/utility/logging/argos_log.h>
/****************************************/
/****************************************/

/****************************************/
/****************************************/

void CCrazyflieController::Init(TConfigurationNode& t_node) {
    try {
        /*
         * Initialize sensors/actuators
         */
        m_pcDistance = GetSensor<CCI_CrazyflieDistanceScannerSensor>("crazyflie_distance_scanner");
        m_pcPropellers = GetActuator<CCI_QuadRotorPositionActuator>("quadrotor_position");
        /* Get pointers to devices */
        m_pcRABA = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
        m_pcRABS = GetSensor<CCI_RangeAndBearingSensor>("range_and_bearing");
        try {
            m_pcPos = GetSensor<CCI_PositioningSensor>("positioning");
        } catch (CARGoSException& ex) {
        }
        try {
            m_pcBattery = GetSensor<CCI_BatterySensor>("battery");
        } catch (CARGoSException& ex) {
        }
    } catch (CARGoSException& ex) {
        THROW_ARGOSEXCEPTION_NESTED("Error initializing the crazyflie sensing controller for robot \"" << GetId() << "\"", ex);
    }
    /*
     * Initialize other stuff
     */
    /* Create a random number generator. We use the 'argos' category so
       that creation, reset, seeding and cleanup are managed by ARGoS. */
    m_pcRNG = CRandom::CreateRNG("argos");

    m_uiCurrentStep = 0;
    Reset();
}

/****************************************/
/****************************************/

void CCrazyflieController::ControlStep() {
    LogData();

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

    static std::unordered_map<DroneState, std::string> droneStateNames = {{DroneState::OnGround, "OnGround"},
                                                                          {DroneState::Takeoff, "Takeoff"},
                                                                          {DroneState::WaitTakeoff, "WaitTakeoff"},
                                                                          {DroneState::ForwardMovement, "ForwardMovement"},
                                                                          {DroneState::WaitForwardMovement, "WaitForwardMovement"},
                                                                          {DroneState::BrakeMovement, "BrakeMovement"},
                                                                          {DroneState::WaitBrakeMovement, "WaitBrakeMovement"},
                                                                          {DroneState::Rotate, "Rotate"},
                                                                          {DroneState::WaitRotation, "WaitRotation"},
                                                                          {DroneState::StopRotation, "StopRotation"},
                                                                          {DroneState::WaitStopRotation, "WaitStopRotation"}};
    LOG << "Current state: " << droneStateNames[m_currentState] << std::endl;

    m_uiCurrentStep++;
}

/****************************************/
/****************************************/

void CCrazyflieController::Reset() {
}

/****************************************/
/****************************************/

void CCrazyflieController::LogData() {
    // Battery sensor
    LOG << "Battery level: " << m_pcBattery->GetReading().AvailableCharge << std::endl;

    // Distance sensors
    auto iterDistRead = m_pcDistance->GetReadingsMap().begin();
    if (m_pcDistance->GetReadingsMap().size() == 4) {
        LOG << "Front dist: " << (iterDistRead++)->second << std::endl;
        LOG << "Left dist: " << (iterDistRead++)->second << std::endl;
        LOG << "Back dist: " << (iterDistRead++)->second << std::endl;
        LOG << "Right dist: " << (iterDistRead)->second << std::endl;
    }

    // Position sensor
    LOG << "PosSens Pos X: " << m_pcPos->GetReading().Position.GetX() << std::endl;
    LOG << "PosSens Pos Y: " << m_pcPos->GetReading().Position.GetY() << std::endl;
    LOG << "PosSens Pos Z: " << m_pcPos->GetReading().Position.GetZ() << std::endl;

    CRadians angle;
    CVector3 vector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);
    LOG << "PosSens Angle X: " << vector.GetX() << std::endl;
    LOG << "PosSens Angle Y: " << vector.GetY() << std::endl;
    LOG << "PosSens Angle Z: " << vector.GetZ() << std::endl;
    LOG << "PosSens Angle Angle: " << angle << std::endl;

    // Range & bearing sensors
    auto rangeAndBearingReadings = m_pcRABS->GetReadings();
    std::uint16_t i = 0;
    for (auto it = rangeAndBearingReadings.begin(); it != rangeAndBearingReadings.end(); ++it) {
        i++;
        LOG << "RABS #" << i << " HorizBearing: " << it->HorizontalBearing << std::endl;
        LOG << "RABS #" << i << " VertBearing: " << it->VerticalBearing << std::endl;
        LOG << "RABS #" << i << " Range: " << it->Range << std::endl;
    }
}

/*
 * This statement notifies ARGoS of the existence of the controller.
 * It binds the class passed as first argument to the string passed as
 * second argument.
 * The string is then usable in the XML configuration file to refer to
 * this controller.
 * When ARGoS reads that string in the XML file, it knows which controller
 * class to instantiate.
 * See also the XML configuration files for an example of how this is used.
 */
REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
