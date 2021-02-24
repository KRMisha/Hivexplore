#ifndef CRAZYFLIE_SENSING_H
#define CRAZYFLIE_SENSING_H

#include <argos3/core/control_interface/ci_controller.h>
#include <argos3/plugins/robots/crazyflie/control_interface/ci_crazyflie_distance_scanner_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_quadrotor_position_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_positioning_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_battery_sensor.h>

using namespace argos;

enum class DroneState {
    OnGround,
    Takeoff,
    WaitTakeoff,
    ForwardMovement,
    WaitForwardMovement,
    BrakeMovement,
    WaitBrakeMovement,
    Rotate,
    WaitRotation,
    StopRotation,
    WaitStopRotation
};

class CCrazyflieController : public CCI_Controller {
public:
    virtual void Init(TConfigurationNode& t_node);
    virtual void ControlStep();
    virtual void Reset();
    virtual void Destroy();

private:
    void LogData();

    CCI_CrazyflieDistanceScannerSensor* m_pcDistance = nullptr;
    CCI_QuadRotorPositionActuator* m_pcPropellers = nullptr;
    CCI_RangeAndBearingActuator* m_pcRABA = nullptr;
    CCI_RangeAndBearingSensor* m_pcRABS = nullptr;
    CCI_PositioningSensor* m_pcPos = nullptr;
    CCI_BatterySensor* m_pcBattery = nullptr;

    CVector3 m_initialPosition;
    CVector3 m_lastReferencePosition;
    CRadians m_lastReferenceYaw;
    DroneState m_currentState = DroneState::OnGround;
};

#endif
