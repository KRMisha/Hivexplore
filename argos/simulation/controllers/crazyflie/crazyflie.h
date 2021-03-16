#ifndef CRAZYFLIE_H
#define CRAZYFLIE_H

#include <unordered_map>
#include <utility>
#include <variant>
#include <string>
#include <argos3/core/control_interface/ci_controller.h>
#include <argos3/plugins/robots/crazyflie/control_interface/ci_crazyflie_distance_scanner_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_quadrotor_position_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_positioning_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_actuator.h>
#include <argos3/plugins/robots/generic/control_interface/ci_range_and_bearing_sensor.h>
#include <argos3/plugins/robots/generic/control_interface/ci_battery_sensor.h>
#include "libs/json.hpp"

using namespace argos;
using json = nlohmann::json;

enum class MissionState {
    Standby,
    Exploring,
    Returning,
};

enum class ExploringState {
    Idle,
    Liftoff,
    Explore,
    Brake,
    Rotate,
    Land,
};

enum class ReturningState {
    Return,
    Land,
    Idle,
};

enum class DroneStatus {
    Standby,
    Flying,
    Crashed,
};

class CCrazyflieController : public CCI_Controller {
public:
    // Use vector of pairs to preserve insertion order (required to receive orientation and position data before range data for mapping)
    using LogVariableMap = std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>;
    using LogConfigs = std::vector<std::pair<std::string, LogVariableMap>>;

    virtual void Init(TConfigurationNode& t_node) override;
    virtual void ControlStep() override;
    virtual void Reset() override;
    virtual void Destroy() override;

    LogConfigs GetLogData() const;
    const std::vector<std::string>& GetConsoleLogs() const;
    void SetParamData(const std::string& param, json value);

private:
    bool AvoidObstacle();
    void Explore();
    void Return();

    void UpdateSensorReadings();
    void UpdateVelocity();
    void UpdateRssi();
    void UpdateDroneStatus();

    void DebugPrint(const std::string& text);

    template<typename T, typename U = T>
    std::unordered_map<std::string, U> GetSensorReadings(const std::array<std::string, 6>& sensorNames) const;

    // Sensors and actuators
    CCI_CrazyflieDistanceScannerSensor* m_pcDistance = nullptr;
    CCI_QuadRotorPositionActuator* m_pcPropellers = nullptr;
    CCI_RangeAndBearingActuator* m_pcRABA = nullptr;
    CCI_RangeAndBearingSensor* m_pcRABS = nullptr;
    CCI_PositioningSensor* m_pcPos = nullptr;
    CCI_BatterySensor* m_pcBattery = nullptr;

    // States
    MissionState m_missionState = MissionState::Standby;
    ExploringState m_exploringState = ExploringState::Idle;
    ReturningState m_returningState = ReturningState::Return;

    // Data
    CVector3 m_initialPosition;
    CVector3 m_previousPosition;
    CVector3 m_velocity;
    std::unordered_map<std::string, float> m_sensorReadings;
    std::uint8_t m_rssiReading = 0;
    DroneStatus m_droneStatus = DroneStatus::Standby;
    std::vector<std::string> m_consoleLogs;

    // Obstacle avoidance variables
    bool m_isAvoidingObstacle = false;
    ExploringState m_exploringStateOnHold = ExploringState::Idle;
    ReturningState m_returningStateOnHold = ReturningState::Return;
    CVector3 m_obstacleDetectedPosition;
    double m_correctionDistance = 0.0;

    // To avoid having multiple states to simulate drone control, we use bools within the
    // states to wait for movement commands to finish before executing a new command

    // Liftoff variables
    bool m_isLiftoffCommandFinished = true;

    // Exploration variables
    bool m_isForwardCommandFinished = true;
    CVector3 m_forwardCommandReferencePosition;

    // Braking variables
    bool m_isBrakeCommandFinished = true;
    CVector3 m_brakingReferencePosition;

    // Rotation variables
    bool m_isRotateCommandFinished = true;
    CRadians m_lastReferenceYaw;

    // Emergency landing variables
    bool m_isEmergencyLandingFinished = true;
    CVector3 m_emergencyLandingPosition;
};

#endif
