#ifndef CRAZYFLIE_SENSING_H
#define CRAZYFLIE_SENSING_H

#include <argos3/core/control_interface/ci_controller.h>
#include <argos3/plugins/robots/crazyflie/control_interface/ci_crazyflie_distance_scanner_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_quadrotor_position_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_positioning_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_battery_sensor.h>
#include <argos3/core/utility/math/rng.h> // TODO: Remove

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
    /*
     * This function initializes the controller.
     * The 't_node' variable points to the <parameters> section in the XML
     * file in the <controllers><footbot_foraging_controller> section.
     */
    virtual void Init(TConfigurationNode& t_node);

    /*
     * This function is called once every time step.
     * The length of the time step is set in the XML file.
     */
    virtual void ControlStep();

    /*
     * This function resets the controller to its state right after the
     * Init().
     * It is called when you press the reset button in the GUI.
     */
    virtual void Reset();

    /*
     * Called to cleanup what done by Init() when the experiment finishes.
     * In this example controller there is no need for clean anything up,
     * so the function could have been omitted. It's here just for
     * completeness.
     */
    virtual void Destroy() {}

    /*
     * This function logs all the sensors's data
     */
    void LogData();

private:
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
