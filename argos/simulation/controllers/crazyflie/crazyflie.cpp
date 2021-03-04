#include "crazyflie.h"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "experiments/constants.h"

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
    UpdateCurrentVelocity();
    m_previousDronePosition = m_pcPos->GetReading().Position;

    CVector3 targetVelocity;
    double targetHeight = 0;
    double targetYawRate = 0;

    // if (m_currentState == DroneState::Takeoff || m_currentState == DroneState::Land) {
    //     // Distance correction required to stay out of range of any obstacle
    //     uint16_t leftDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, leftSensorReading);
    //     uint16_t rightDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, rightSensorReading);
    //     uint16_t frontDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, frontSensorReading);
    //     uint16_t backDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, backSensorReading);

    //     // Velocity required to apply distance correction
    //     const float AVOIDANCE_SENSITIVITY = MAXIMUM_VELOCITY / OBSTACLE_DETECTED_THRESHOLD;
    //     targetLeftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * AVOIDANCE_SENSITIVITY;
    //     targetForwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * AVOIDANCE_SENSITIVITY;
    // }

    UpdateCurrentVelocity();
    switch (m_currentState) {
        case DroneState::OnGround: {
            // if (startMission)
            //  get initial position
            //  m_current_state = Drone::Takeoff;
            m_currentState = DroneState::Takeoff;
        } break;

        case DroneState::Takeoff: {
            targetHeight += 1;
            // targetVelocity.SetX(targetVelocity.GetX() + 1);
            static const auto heightEpsilon = 0.005;
            if (std::abs(targetHeight - m_pcPos->GetReading().Position.GetZ()) < heightEpsilon) {
                m_currentState = DroneState::Foward;
            }
        } break;
        case DroneState::Foward: {
            targetVelocity.SetX(targetVelocity.GetX() + 1);
        } break;
        case DroneState::Land: {
            static const auto heightEpsilon = 0.005;
            targetHeight += 0.02;
            if (std::abs(targetHeight - m_pcPos->GetReading().Position.GetZ()) < heightEpsilon) {
                m_currentState = DroneState::Takeoff;
            }
        } break;
    }
    m_previousDronePosition = m_pcPos->GetReading().Position;

    // apply target speed
    SetWaypoint(targetVelocity, targetHeight, targetYawRate);
}

void CCrazyflieController::Reset() {
}

void CCrazyflieController::Destroy() {
}

// Returns an unordered_map. The key is the log config name and the value is
// an unordered_map that contains the log variables' names and their values
std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::uint8_t, std::uint16_t, float>>> CCrazyflieController::
    GetLogData() const {
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
    Real angleDegrees = ToDegrees(angleRadians.SignedNormalize()).GetValue();
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

    // TODO: Add velocity

    // Range group
    CCI_CrazyflieDistanceScannerSensor::TReadingsMap distanceReadings = m_pcDistance->GetReadingsMap();
    static const std::array<std::string, 4> rangeLogNames = {"range.front", "range.left", "range.back", "range.right"};
    decltype(logDataMap)::mapped_type rangeLog;
    static constexpr std::int8_t sensorSaturated = -1;
    static constexpr std::int8_t sensorEmpty = -2;
    static constexpr std::uint8_t obstacleTooClose = 0;
    static constexpr std::uint16_t obstacleTooFar = 4000;

    for (auto it = distanceReadings.begin(); it != distanceReadings.end(); ++it) {
        std::size_t index = std::distance(distanceReadings.begin(), it);
        Real rangeData = it->second;
        if (rangeData == sensorSaturated) {
            rangeData = obstacleTooClose;
        } else if (rangeData == sensorEmpty) {
            rangeData = obstacleTooFar;
        } else {
            rangeData *= 10;
        }
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

void CCrazyflieController::UpdateCurrentVelocity() {
    static constexpr double secondsPerTick = 1.0 / Constants::ticksPerSeconds;
    m_currentVelocity = (m_pcPos->GetReading().Position - m_previousDronePosition) / secondsPerTick;
}

void CCrazyflieController::SetWaypoint(CVector3 targetVelocity, double targetHeight, double targetYawRate) const {
    // const auto currentVelocityCorrection = targetVelocity - GetVelocity();

    // CVector3 targetRelativePosition;
    // static constexpr double positionToVelocityFactor = 0.01;
    // targetRelativePosition.SetX(currentVelocityCorrection.GetX() * positionToVelocityFactor);
    // targetRelativePosition.SetY(currentVelocityCorrection.GetY() * positionToVelocityFactor);
    // LOG << m_pcPos->GetReading().Position.GetZ() << std::endl;
    // static constexpr double zPositionEpsilon = 0.001;
    // const double targetHeightVelocity = (targetHeight - m_pcPos->GetReading().Position.GetZ()) < zPositionEpsilon
    //                                     && (targetHeight - m_pcPos->GetReading().Position.GetZ()) > -zPositionEpsilon
    //                                     ? 0
    //                                     : targetHeight - m_pcPos->GetReading().Position.GetZ();
    // targetRelativePosition.SetZ((currentVelocityCorrection.GetZ() + targetHeightVelocity) * positionToVelocityFactor);
    // m_pcPropellers->SetRelativePosition(CVector3(targetRelativePosition.GetX(), targetRelativePosition.GetY(), 0));
    CVector3 targetAbsolutePosition;
    const auto currentPosition = m_pcPos->GetReading().Position;
    static constexpr double secondsPerTick = 1.0 / Constants::ticksPerSeconds;
    // Split wanted speed using yaw (for both)
    CRadians angle;
    CVector3 vector;
    m_pcPos->GetReading().Orientation.ToAngleAxis(angle, vector);
    LOG << "Angle: " << angle.GetValue() * 180 / M_PI << std::endl;
    LOG << "Position x: " << m_pcPos->GetReading().Position.GetX() << std::endl;
    LOG << "Position y: " << m_pcPos->GetReading().Position.GetY() << std::endl;
    LOG << "Position z: " << m_pcPos->GetReading().Position.GetZ() << std::endl;
    const auto xVelocity = std::cos(angle.GetValue()) * targetVelocity.GetX() + std::sin(angle.GetValue()) * targetVelocity.GetY();
    const auto yVelocity = std::cos(angle.GetValue()) * targetVelocity.GetY() + std::sin(angle.GetValue()) * targetVelocity.GetX();
    LOG << "xVelocity: " << xVelocity << std::endl;
    LOG << "yVelocity: " << yVelocity << std::endl;
    targetAbsolutePosition.SetX(xVelocity * secondsPerTick + currentPosition.GetX());
    targetAbsolutePosition.SetY(yVelocity * secondsPerTick + currentPosition.GetY());
    targetAbsolutePosition.SetZ(targetVelocity.GetZ() * secondsPerTick + targetHeight);
    m_pcPropellers->SetAbsolutePosition(targetAbsolutePosition);
    // if (m_currentState == DroneState::Foward) {
    //     m_pcPropellers->SetRelativePosition(CVector3(targetVelocity.GetX(), targetVelocity.GetY(), targetVelocity.GetZ()) * secondsPerTick);
    // } else {
    // }
}


REGISTER_CONTROLLER(CCrazyflieController, "crazyflie_controller")
