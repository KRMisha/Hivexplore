#ifndef CRAZYFLIE_H
#define CRAZYFLIE_H

#include <unordered_map>
#include <variant>
#include <string>
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
    WaitStopRotation,
};

class CCrazyflieController : public CCI_Controller {
public:
    virtual void Init(TConfigurationNode& t_node) override;
    virtual void ControlStep() override;
    virtual void Reset() override;
    virtual void Destroy() override;

    // TODO: Add more types to the std::variant (check which types are used in the Crazyflie firmware for each log/param)
    std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> GetLogData() const;
    void SetParamData(const std::string& param, std::variant<bool> value);

private:
    void LogData(); // TODO: Remove

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
