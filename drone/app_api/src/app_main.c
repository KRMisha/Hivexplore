/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2020 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * api_app.c - App layer application that calls app API functions to make
 *             sure they are compiled in CI.
 */

#include <math.h>
#include <stdlib.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"

#include "ledseq.h"
#include "crtp_commander_high_level.h"
#include "log.h"
#include "param.h"
#include "pm.h"
#include "app_channel.h"
#include "commander.h"
#include "configblock.h"
#include "sitaw.h"

#include "app_main.h"

#define DEBUG_MODULE "APPAPI"

// Min helper macro
#define MIN(a, b) ((a < b) ? a : b)

// Constants
static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.3f;
static const float CRUISE_VELOCITY = 0.2f;
static const float MAXIMUM_VELOCITY = 0.4f;
static const uint16_t METER_TO_MILLIMETER_FACTOR = 1000;
static const uint16_t MAXIMUM_RETURN_TICKS = 800;
static const uint64_t INITIAL_EXPLORE_TICKS = 600;
static const uint16_t CLEAR_OBSTACLE_TICKS = 100;

// States
static mission_state_t missionState = MISSION_STANDBY;
static exploring_state_t exploringState = EXPLORING_IDLE;
static returning_state_t returningState = RETURNING_ROTATE_TOWARDS_BASE;
static emergency_state_t emergencyState = EMERGENCY_LAND;

// Data
static drone_status_t droneStatus = STATUS_STANDBY;
static bool isLedEnabled = false;
static setpoint_t setPoint;
static point_t initialPosition;
static bool shouldTurnLeft = true;

// Readings
static float rollReading;
static float pitchReading;
static float yawReading;
static point_t positionReading;
static uint16_t frontSensorReading;
static uint16_t leftSensorReading;
static uint16_t backSensorReading;
static uint16_t rightSensorReading;
static uint16_t upSensorReading;
static uint16_t downSensorReading;
static uint8_t rssiReading;

// Targets
static float targetForwardVelocity;
static float targetLeftVelocity;
static float targetHeight;
static float targetYawRate;
static float targetYawToBase;

// Watchdogs (return to base)
static uint16_t returnWatchdog = MAXIMUM_RETURN_TICKS; // Prevent staying stuck in return state by exploring periodically
static uint64_t maximumExploreTicks = INITIAL_EXPLORE_TICKS;
static uint64_t exploreWatchdog = INITIAL_EXPLORE_TICKS; // Prevent staying stuck in forward state by attempting to beeline periodically
static uint16_t clearObstacleCounter = CLEAR_OBSTACLE_TICKS; // Ensure obstacles are sufficiently cleared before resuming

typedef struct {
    float x;
    float y;
    float z;
    uint8_t sourceId;
} P2PPacketContent;

void appMain(void) {
    vTaskDelay(M2T(3000));

    const logVarId_t rollId = logGetVarId("stateEstimate", "roll");
    const logVarId_t pitchId = logGetVarId("stateEstimate", "pitch");
    const logVarId_t yawId = logGetVarId("stateEstimate", "yaw");
    const logVarId_t positionXId = logGetVarId("stateEstimate", "x");
    const logVarId_t positionYId = logGetVarId("stateEstimate", "y");
    const logVarId_t positionZId = logGetVarId("stateEstimate", "z");
    const logVarId_t frontSensorId = logGetVarId("range", "front");
    const logVarId_t leftSensorId = logGetVarId("range", "left");
    const logVarId_t backSensorId = logGetVarId("range", "back");
    const logVarId_t rightSensorId = logGetVarId("range", "right");
    const logVarId_t upSensorId = logGetVarId("range", "up");
    const logVarId_t downSensorId = logGetVarId("range", "zrange");
    const logVarId_t rssiId = logGetVarId("radio", "rssi");

    const paramVarId_t flowDeckModuleId = paramGetVarId("deck", "bcFlow2");
    const paramVarId_t multirangerModuleId = paramGetVarId("deck", "bcMultiranger");

    const bool isFlowDeckInitialized = paramGetUint(flowDeckModuleId);
    const bool isMultirangerInitialized = paramGetUint(multirangerModuleId);
    bool isOutOfService = !isFlowDeckInitialized || !isMultirangerInitialized;
    if (!isFlowDeckInitialized) {
        DEBUG_PRINT("FlowDeckV2 is not connected\n");
    }
    if (!isMultirangerInitialized) {
        DEBUG_PRINT("Multiranger is not connected\n");
    }

    p2pRegisterCB(p2pReceivedCallback);

    while (true) {
        vTaskDelay(M2T(10));

        ledSet(LED_GREEN_R, isLedEnabled);

        if (isOutOfService) {
            ledSet(LED_RED_R, true);
            continue;
        }

        rollReading = logGetFloat(rollId);
        pitchReading = logGetFloat(pitchId);
        yawReading = logGetFloat(yawId);

        positionReading.x = logGetFloat(positionXId);
        positionReading.y = logGetFloat(positionYId);
        positionReading.z = logGetFloat(positionZId);

        frontSensorReading = logGetUint(frontSensorId);
        leftSensorReading = logGetUint(leftSensorId);
        backSensorReading = logGetUint(backSensorId);
        rightSensorReading = logGetUint(rightSensorId);
        upSensorReading = logGetUint(upSensorId);
        downSensorReading = logGetUint(downSensorId);

        rssiReading = logGetUint(rssiId);

        targetForwardVelocity = 0.0;
        targetLeftVelocity = 0.0;
        targetHeight = 0.0;
        targetYawRate = 0.0;
        targetYawToBase = 0.0;

        const bool shouldNotBroadcastPosition =
            missionState == MISSION_STANDBY ||
            (missionState == MISSION_EXPLORING && (exploringState == EXPLORING_IDLE || exploringState == EXPLORING_LIFTOFF)) ||
            (missionState == MISSION_RETURNING && returningState == RETURNING_IDLE) ||
            (missionState == MISSION_EMERGENCY && emergencyState == EMERGENCY_IDLE);

        static const uint8_t broadcastProbabilityPercentage = 5;
        if (!shouldNotBroadcastPosition && (rand() % 100) < broadcastProbabilityPercentage) {
            broadcastPosition();
        }

        switch (missionState) {
        case MISSION_STANDBY:
            droneStatus = STATUS_STANDBY;
            break;
        case MISSION_EXPLORING:
            avoidObstacle();
            explore();
            break;
        case MISSION_RETURNING:
            avoidObstacle();
            returnToBase();
            break;
        case MISSION_EMERGENCY:
            emergencyLand();
            break;
        case MISSION_LANDED:
            droneStatus = STATUS_LANDED;
            break;
        }

        if (isCrashed()) {
            isOutOfService = true;
            droneStatus = STATUS_CRASHED;
            memset(&setPoint, 0, sizeof(setpoint_t));
        }

        static const uint8_t TASK_PRIORITY = 3;
        commanderSetSetpoint(&setPoint, TASK_PRIORITY);
    }
}

void avoidObstacle(void) {
    bool isExploringAvoidanceDisallowed =
        missionState == MISSION_EXPLORING && (exploringState == EXPLORING_IDLE || exploringState == EXPLORING_LIFTOFF);
    bool isReturningAvoidanceDisallowed =
        missionState == MISSION_RETURNING && (returningState == RETURNING_IDLE || returningState == RETURNING_LAND);

    bool isAvoidanceAllowed = !isExploringAvoidanceDisallowed && !isReturningAvoidanceDisallowed;

    if (isAvoidanceAllowed) {
        // Distance correction required to stay out of range of any obstacle
        uint16_t leftDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, leftSensorReading);
        uint16_t rightDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, rightSensorReading);
        uint16_t frontDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, frontSensorReading);
        uint16_t backDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, backSensorReading);

        // Velocity required to apply distance correction
        const float AVOIDANCE_SENSITIVITY = MAXIMUM_VELOCITY / OBSTACLE_DETECTED_THRESHOLD;
        targetLeftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * AVOIDANCE_SENSITIVITY;
        targetForwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * AVOIDANCE_SENSITIVITY;
    }
}

void explore(void) {
    switch (exploringState) {
    case EXPLORING_IDLE: {
        droneStatus = STATUS_STANDBY;

        DEBUG_PRINT("Idle\n");
        memset(&setPoint, 0, sizeof(setpoint_t));

        // Check if any obstacle is in the way before taking off
        if (upSensorReading > EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
            DEBUG_PRINT("Liftoff\n");
            initialPosition = positionReading;
            shouldTurnLeft = true;
            exploringState = EXPLORING_LIFTOFF;
        }
    } break;
    case EXPLORING_LIFTOFF: {
        droneStatus = STATUS_LIFTOFF;

        if (liftoff()) {
            exploringState = EXPLORING_EXPLORE;
        }
    } break;
    case EXPLORING_EXPLORE: {
        droneStatus = STATUS_FLYING;

        if (!forward()) {
            exploringState = EXPLORING_ROTATE;
        }
    } break;
    case EXPLORING_ROTATE: {
        droneStatus = STATUS_FLYING;

        if (rotate()) {
            exploringState = EXPLORING_EXPLORE;
        }
    } break;
    }
}

void returnToBase(void) {
    // If returned to base, land
    static const double distanceToReturnEpsilon = 0.3;
    static const uint8_t rssiLandingThreshold = 60;
    if (returningState != RETURNING_LAND && returningState != RETURNING_IDLE && rssiReading <= rssiLandingThreshold &&
        fabs((double)initialPosition.x - (double)positionReading.x) < distanceToReturnEpsilon &&
        fabs((double)initialPosition.y - (double)positionReading.y) < distanceToReturnEpsilon) {
        DEBUG_PRINT("Found the base\n");
        DEBUG_PRINT("Initial position: %f, %f\n", (double)initialPosition.x, (double)initialPosition.y);
        DEBUG_PRINT("Current position: %f, %f\n", (double)positionReading.x, (double)positionReading.y);
        returningState = RETURNING_LAND;
    }

    switch (returningState) {
    case RETURNING_ROTATE_TOWARDS_BASE: {
        droneStatus = STATUS_FLYING;

        // Calculate rotation angle to turn towards base
        targetYawToBase = atan2(initialPosition.y - positionReading.y, initialPosition.x - positionReading.x) * 360.0 / (2.0 * M_PI);

        // If the drone is towards its base
        static const double yawEpsilon = 5.0;
        double yawDifference = fabs(targetYawToBase - yawReading);
        if (yawDifference < yawEpsilon || yawDifference > (360.0 - yawEpsilon)) {
            DEBUG_PRINT("Return: Finished rotating towards base\n");
            returningState = RETURNING_RETURN;
        } else {
            // Keep turning drone towards its base
            targetHeight += EXPLORATION_HEIGHT;
            updateWaypoint();
            setPoint.mode.yaw = modeAbs;
            setPoint.attitude.yaw = targetYawToBase;
        }
    } break;
    case RETURNING_RETURN: {
        droneStatus = STATUS_FLYING;

        // Go to explore algorithm when a wall is detected in front of the drone or return watchdog is finished
        if (!forward() || returnWatchdog == 0) {
            if (returnWatchdog == 0) {
                DEBUG_PRINT("Return: Return watchdog finished\n");
            } else {
                DEBUG_PRINT("Return: Obstacle detected\n");
            }

            // Reset counter
            returnWatchdog = MAXIMUM_RETURN_TICKS;

            returningState = RETURNING_ROTATE;
        } else {
            returnWatchdog--;
        }
    } break;
    case RETURNING_ROTATE: {
        droneStatus = STATUS_FLYING;

        if (rotate()) {
            returningState = RETURNING_FORWARD;
        }
    } break;
    case RETURNING_FORWARD: {
        droneStatus = STATUS_FLYING;

        // The drone must check its right sensor when it is turning left, and its left sensor when turning right
        uint16_t sensorReadingToCheck = shouldTurnLeft ? rightSensorReading : leftSensorReading;

        // Return to base when obstacle has been passed or explore watchdog is finished
        static const uint16_t OPEN_SPACE_THRESHOLD = 300;
        if ((sensorReadingToCheck > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD && clearObstacleCounter == 0) || exploreWatchdog == 0) {
            if (clearObstacleCounter == 0) {
                DEBUG_PRINT("Explore: Obstacle has been cleared\n");
                maximumExploreTicks = INITIAL_EXPLORE_TICKS;
            }
            if (exploreWatchdog == 0) {
                DEBUG_PRINT("Explore: Explore watchdog finished\n");
            }

            // Reset counters
            maximumExploreTicks *= 2;
            exploreWatchdog = maximumExploreTicks;
            clearObstacleCounter = CLEAR_OBSTACLE_TICKS;

            shouldTurnLeft = !shouldTurnLeft;

            DEBUG_PRINT("Explore: Rotating towards base\n");
            returningState = RETURNING_ROTATE_TOWARDS_BASE;
            break;
        }

        if (!forward()) {
            clearObstacleCounter = CLEAR_OBSTACLE_TICKS;
            returningState = RETURNING_ROTATE;
        } else {
            // Reset sensor reading counter if obstacle is detected
            if (sensorReadingToCheck > EDGE_DETECTED_THRESHOLD) {
                clearObstacleCounter--;
            } else {
                clearObstacleCounter = CLEAR_OBSTACLE_TICKS;
            }
        }
        exploreWatchdog--;
    } break;
    case RETURNING_LAND: {
        droneStatus = STATUS_LANDING;

        if (land()) {
            returningState = RETURNING_IDLE;
        }
    } break;
    case RETURNING_IDLE: {
        droneStatus = STATUS_LANDED;

        memset(&setPoint, 0, sizeof(setpoint_t));
    } break;
    }
}

// TODO: Add a reset internal state function to reaffect all the values (like in the ARGoS controller)

void emergencyLand(void) {
    switch (emergencyState) {
    case EMERGENCY_LAND: {
        droneStatus = STATUS_LANDING;

        if (land()) {
            emergencyState = EMERGENCY_IDLE;
        }
    } break;
    case EMERGENCY_IDLE: {
        droneStatus = STATUS_LANDED;

        memset(&setPoint, 0, sizeof(setpoint_t));
    } break;
    }
}

// Returns true when the action is finished
bool liftoff(void) {
    targetHeight += EXPLORATION_HEIGHT;
    updateWaypoint();
    if (downSensorReading >= EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
        DEBUG_PRINT("Liftoff finished\n");
        return true;
    }

    return false;
}

// Returns true as long as the path forward is clear
// Returns false when the path forward is obstructed by an obstacle
bool forward(void) {
    targetHeight += EXPLORATION_HEIGHT;
    targetForwardVelocity += CRUISE_VELOCITY;
    updateWaypoint();

    return frontSensorReading >= EDGE_DETECTED_THRESHOLD;
}

// Returns true when the action is finished
bool rotate(void) {
    static const uint16_t OPEN_SPACE_THRESHOLD = 300;
    targetHeight += EXPLORATION_HEIGHT;
    targetYawRate = (shouldTurnLeft ? 1 : -1) * 50;
    updateWaypoint();

    return frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD;
}

// Returns true when the action is finished
bool land(void) {
    updateWaypoint();
    static const uint8_t LANDED_HEIGHT = 30;
    if (downSensorReading < LANDED_HEIGHT) {
        droneStatus = STATUS_LANDED;
        return true;
    }

    return false;
}

bool isCrashed(void) {
    bool isCrashed = false;

    if (sitAwTuDetected()) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: firmware tumble detected\n");
    }

    if (sitAwFFDetected()) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: firmware freefall detected\n");
    }

    static const uint8_t maxAngle = 55;
    if (fabs(rollReading) > maxAngle) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: high roll angle detected\n");
    }

    if (fabs(pitchReading) > maxAngle) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: high pitch angle detected\n");
    }

    return isCrashed;
}

void broadcastPosition() {
    // Avoid causing drone reset due to the content size
    if (sizeof(P2PPacketContent) > P2P_MAX_DATA_SIZE) {
        DEBUG_PRINT("P2PPacketContent size too big\n");
        return;
    }

    uint64_t radioAddress = configblockGetRadioAddress();
    uint8_t id = (uint8_t)(radioAddress & 0x00000000ff);

    P2PPacketContent content = {.sourceId = id, .x = positionReading.x, .y = positionReading.y, .z = positionReading.z};

    P2PPacket packet = {.port = 0x00, .size = sizeof(content)};

    if (crtpIsConnected()) {
        memcpy(&packet.data[0], &content, sizeof(content));
        radiolinkSendP2PPacketBroadcast(&packet);
    }
}

void p2pReceivedCallback(P2PPacket* packet) {
    P2PPacketContent content;
    memcpy(&content, &packet->data[0], sizeof(content));
    // TODO: Forward P2P content to methods needing the information
}

void updateWaypoint(void) {
    setPoint.velocity_body = true;
    setPoint.mode.x = modeVelocity;
    setPoint.mode.y = modeVelocity;
    setPoint.velocity.x = targetForwardVelocity;
    setPoint.velocity.y = targetLeftVelocity;

    setPoint.mode.yaw = modeVelocity;
    setPoint.attitudeRate.yaw = targetYawRate;

    setPoint.mode.z = modeAbs;
    setPoint.position.z = targetHeight;
}

uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading) {
    return obstacleThreshold - MIN(sensorReading, obstacleThreshold);
}

LOG_GROUP_START(hivexplore)
LOG_ADD(LOG_UINT8, droneStatus, &droneStatus)
LOG_GROUP_STOP(hivexplore)

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, missionState, &missionState)
PARAM_ADD(PARAM_UINT8, isLedEnabled, &isLedEnabled)
PARAM_GROUP_STOP(hivexplore)
