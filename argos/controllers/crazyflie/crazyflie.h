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
#include "utils/log_name.h"

using namespace argos;
using json = nlohmann::json;

enum class MissionState {
    Standby,
    Exploring,
    Returning,
    Emergency,
    Landed,
};

enum class ExploringState {
    Idle,
    Liftoff,
    Explore,
    Brake,
    Rotate,
};

enum class ReturningState {
    BrakeTowardsBase,
    RotateTowardsBase,
    Return,
    Brake,
    Rotate,
    Forward,
    Land,
    Idle,
};

enum class EmergencyState {
    Land,
    Idle,
};

enum class DroneStatus {
    Standby,
    Liftoff,
    Flying,
    Returning,
    Landing,
    Landed,
    Crashed,
};

class CCrazyflieController : public CCI_Controller {
public:
    // Use vector of pairs to preserve insertion order (required to receive orientation and position data before range data for mapping)
    using LogVariableMap = std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>;
    using LogConfigs = std::vector<std::pair<LogName, LogVariableMap>>;

    virtual void Init(TConfigurationNode& t_node) override;
    virtual void ControlStep() override;
    virtual void Reset() override;
    virtual void Destroy() override;

    LogConfigs GetLogData() const;
    const std::string& GetDebugPrint() const;
    void SetParamData(const std::string& param, json value);

private:
    bool AvoidObstaclesAndDrones();
    void Explore();
    void ReturnToBase();
    void EmergencyLand();
    bool Liftoff();
    bool Forward();
    bool Brake();
    bool Rotate();
    bool Land();
    bool IsCrashed();

    void ResetInternalStates();

    void UpdateVelocity();
    void UpdateSensorReadings();
    void UpdateRssi();

    void PingOtherDrones();

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
    ReturningState m_returningState = ReturningState::BrakeTowardsBase;
    EmergencyState m_emergencyState = EmergencyState::Land;

    // Data
    CVector3 m_initialPosition;
    CVector3 m_previousPosition;
    bool m_isBatteryBelowMinimumThreshold = false;
    DroneStatus m_droneStatus = DroneStatus::Standby;
    std::string m_debugPrint;

    // Readings
    CVector3 m_velocityReading;
    std::unordered_map<std::string, float> m_sensorReadings;
    std::uint8_t m_rssiReading = 0;

    // Obstacle avoidance variables
    bool m_isAvoidingObstacle = false;
    ExploringState m_exploringStateOnHold = ExploringState::Idle;
    ReturningState m_returningStateOnHold = ReturningState::BrakeTowardsBase;
    CVector3 m_obstacleDetectedPosition;
    double m_correctionDistance = 0.0;

    // To avoid having multiple states to simulate drone control, we use bools within the
    // states to wait for movement commands to finish before executing a new command

    // Exploration variables
    bool m_isForwardCommandFinished = true;
    CVector3 m_forwardCommandReferencePosition;

    // Braking variables
    bool m_isBrakeCommandFinished = true;
    CVector3 m_brakingReferencePosition;

    // Rotation variables
    bool m_isRotateCommandFinished = true;
    bool m_shouldTurnLeft = true;
    std::uint8_t m_rotationChangeWatchdog;
    CRadians m_lastReferenceYaw;
    CRadians m_rotationAngle;

    // Return to base variables
    bool m_isRotateToBaseCommandFinished = true;
    CRadians m_targetYawToBase;
    std::uint16_t m_stabilizeRotationCounter; // Ensure drone is oriented towards the base before resuming
    std::uint16_t m_returnWatchdog; // Prevent staying stuck in return state by exploring periodically
    std::uint64_t m_maximumExploreTicks;
    std::uint64_t m_exploreWatchdog; // Prevent staying stuck in forward state by attempting to beeline periodically
    std::uint16_t m_clearObstacleCounter; // Ensure obstacles are sufficiently cleared before resuming

    // Crash detection variables
    CVector3 m_lastActivePosition;
    std::uint32_t m_watchdogCounter = 0;
};

#endif
