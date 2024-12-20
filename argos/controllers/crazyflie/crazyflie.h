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
    BrakeAway,
    RotateAway,
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
    bool RotateToTargetYaw();
    bool Land();
    bool IsCrashed();

    void ResetInternalStates();

    void UpdateBatteryLevel();
    void UpdateVelocity();
    void UpdateSensorReadings();
    void UpdateRssi();

    void PingOtherDrones();

    CRadians CalculateAngleAwayFromCenterOfMass();

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
    std::uint8_t m_batteryLevel;
    bool m_isBatteryBelowMinimumThreshold = false;
    bool m_isOutOfService = false;
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
    std::uint8_t m_lowBatteryIgnoredCounter; // To protect from low voltage spikes triggering return
    std::uint16_t m_reorientationWatchdog; // To reorient away from the swarm's center of mass

    // Braking variables
    bool m_isBrakeCommandFinished = true;
    CVector3 m_brakingReferencePosition;

    // Rotation variables
    bool m_isRotateCommandFinished = true;
    bool m_shouldTurnLeft = true;
    std::uint8_t m_rotationChangeWatchdog; // To randomly change exploration rotation direction
    CRadians m_lastReferenceYaw;
    CRadians m_rotationAngle;
    bool m_isRotateToTargetYawCommandFinished = true;
    CRadians m_targetYaw;
    std::uint16_t m_stabilizeRotationCounter; // Ensure drone has reached the target yaw before resuming

    // Return to base variables
    std::uint16_t m_returnWatchdog; // Prevent staying stuck in return state by exploring periodically
    std::uint64_t m_maximumExploreTicks;
    std::uint64_t m_exploreWatchdog; // Prevent staying stuck in forward state by attempting to beeline periodically
    std::uint16_t m_clearObstacleCounter; // Ensure obstacles are sufficiently cleared before resuming

    // Crash detection variables
    CVector3 m_lastActivePosition;
    std::uint32_t m_watchdogCounter = 0;
};

#endif
