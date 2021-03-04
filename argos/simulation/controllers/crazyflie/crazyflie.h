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
    Foward,
    Land,
};

class CCrazyflieController : public CCI_Controller {
public:
    virtual void Init(TConfigurationNode& t_node) override;
    virtual void ControlStep() override;
    virtual void Reset() override;
    virtual void Destroy() override;

    std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> GetLogData() const;
    void SetParamData(const std::string& param, std::variant<bool> value);

private:
    void UpdateCurrentVelocity();
    void SetWaypoint(CVector3 targetVelocity, double targetHeight, double targetYawRate) const;

    CCI_CrazyflieDistanceScannerSensor* m_pcDistance = nullptr;
    CCI_QuadRotorPositionActuator* m_pcPropellers = nullptr;
    CCI_RangeAndBearingActuator* m_pcRABA = nullptr;
    CCI_RangeAndBearingSensor* m_pcRABS = nullptr;
    CCI_PositioningSensor* m_pcPos = nullptr;
    CCI_BatterySensor* m_pcBattery = nullptr;

    CVector3 m_currentVelocity;
    CVector3 m_previousDronePosition;
    DroneState m_currentState = DroneState::OnGround;
};

#endif
