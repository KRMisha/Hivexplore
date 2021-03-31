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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"

#include "ledseq.h"
#include "crtp_commander_high_level.h"
#include "locodeck.h"
#include "mem.h"
#include "log.h"
#include "param.h"
#include "pm.h"
#include "app_channel.h"
#include "commander.h"
#include "configblock.h"
#include "sitaw.h"

#include "app_main.h"

#define DEBUG_MODULE "APPAPI"

// Min max helper macros
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

// Constants
static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.5f;
static const float CRUISE_VELOCITY = 0.2f;
static const float MAXIMUM_VELOCITY = 1.0f;
static const uint16_t METER_TO_MILLIMETER_FACTOR = 1000;

// States
static mission_state_t missionState = MISSION_STANDBY;
static exploring_state_t exploringState = EXPLORING_IDLE;
static returning_state_t returningState = RETURNING_RETURN;
static emergency_state_t emergencyState = EMERGENCY_LAND;

// Data
static drone_status_t droneStatus = STATUS_STANDBY;
static bool isM1LedOn = false;
static setpoint_t setPoint;

// Readings
static uint16_t frontSensorReading;
static uint16_t leftSensorReading;
static uint16_t backSensorReading;
static uint16_t rightSensorReading;
static uint16_t upSensorReading;
static uint16_t downSensorReading;
static position_t currentPosition;
static float rollReading;
static float pitchReading;
static uint8_t rssiReading;

// Targets
static float targetForwardVelocity;
static float targetLeftVelocity;
static float targetHeight;
static float targetYawRate;

void appMain(void) {
    vTaskDelay(M2T(3000));

    const logVarId_t frontSensorId = logGetVarId("range", "front");
    const logVarId_t leftSensorId = logGetVarId("range", "left");
    const logVarId_t backSensorId = logGetVarId("range", "back");
    const logVarId_t rightSensorId = logGetVarId("range", "right");
    const logVarId_t upSensorId = logGetVarId("range", "up");
    const logVarId_t downSensorId = logGetVarId("range", "zrange");
    const logVarId_t rollId = logGetVarId("stateEstimate", "roll");
    const logVarId_t pitchId = logGetVarId("stateEstimate", "pitch");
    const logVarId_t rssiId = logGetVarId("radio", "rssi");
    const logVarId_t positionXId = logGetVarId("stateEstimate", "x");
    const logVarId_t positionYId = logGetVarId("stateEstimate", "y");
    const logVarId_t positionZId = logGetVarId("stateEstimate", "z");

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

    // Initialize random function
    time_t t;
    srand((unsigned) time(&t));

    p2pRegisterCB(p2pCallbackHandler);

    while (true) {
        vTaskDelay(M2T(10));

        ledSet(LED_GREEN_R, isM1LedOn);

        static const uint8_t broadcastPercentage = 20;
        if ((rand() % 100) < broadcastPercentage) {
            broadcastPosition();
        }

        if (isOutOfService) {
            ledSet(LED_RED_R, true);
            continue;
        }

        frontSensorReading = logGetUint(frontSensorId);
        leftSensorReading = logGetUint(leftSensorId);
        backSensorReading = logGetUint(backSensorId);
        rightSensorReading = logGetUint(rightSensorId);
        upSensorReading = logGetUint(upSensorId);
        downSensorReading = logGetUint(downSensorId);
        currentPosition.x = logGetFloat(positionXId);
        currentPosition.y = logGetFloat(positionYId);
        currentPosition.z = logGetFloat(positionZId);

        rollReading = logGetFloat(rollId);
        pitchReading = logGetFloat(pitchId);

        rssiReading = logGetUint(rssiId);
        (void)rssiReading; // TODO: Remove (this silences the unused variable compiler warning which is treated as an error)

        targetForwardVelocity = 0.0;
        targetLeftVelocity = 0.0;
        targetHeight = 0.0;
        targetYawRate = 0.0;

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
        missionState == MISSION_RETURNING && (returningState == RETURNING_LAND || returningState == RETURNING_LAND);

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

        targetHeight += EXPLORATION_HEIGHT;
        targetForwardVelocity += CRUISE_VELOCITY;
        updateWaypoint();

        if (frontSensorReading < EDGE_DETECTED_THRESHOLD) {
            exploringState = EXPLORING_ROTATE;
        }
    } break;
    case EXPLORING_ROTATE: {
        droneStatus = STATUS_FLYING;

        static const uint16_t OPEN_SPACE_THRESHOLD = 300;
        if (frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD) {
            exploringState = EXPLORING_EXPLORE;
        }
        targetHeight += EXPLORATION_HEIGHT;
        targetYawRate = 50;
        updateWaypoint();
    } break;
    }
}

void returnToBase(void) {
    switch (returningState) {
    case RETURNING_RETURN: {
        droneStatus = STATUS_FLYING;

        // TODO: Add return logic
        vTaskDelay(M2T(5000));
        returningState = RETURNING_LAND;
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

void emergencyLand(void) {
    switch (emergencyState) {
    case EMERGENCY_LAND:
        droneStatus = STATUS_LANDING;

        if (land()) {
            emergencyState = EMERGENCY_IDLE;
        }
        break;
    case EMERGENCY_IDLE:
        droneStatus = STATUS_LANDED;

        memset(&setPoint, 0, sizeof(setpoint_t));
        break;
    }
}

bool liftoff(void) {
    targetHeight += EXPLORATION_HEIGHT;
    updateWaypoint();
    if (downSensorReading >= EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
        DEBUG_PRINT("Liftoff finished\n");
        return true;
    }
    return false;
}

bool land(void) {
    updateWaypoint();
    static const uint16_t LANDED_HEIGHT = 50;
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

void broadcastPosition() {
    uint64_t radioAddress = configblockGetRadioAddress();
    uint8_t myId = (uint8_t)((radioAddress)&0x00000000ff);
    P2PPacket packet;
    packet.port = 0x00;
    packet.data[0] = myId;
    memcpy(&packet.data[1], &currentPosition, sizeof(currentPosition));
    radiolinkSendP2PPacketBroadcast(&packet);
}

void p2pCallbackHandler(P2PPacket* packet) {
    // Get source Id
    uint8_t other_id = packet->data[0];

    position_t sourcePosition;
    memcpy(&sourcePosition, &packet->data[1], sizeof(sourcePosition));

    uint8_t rssi = packet->rssi;
    DEBUG_PRINT("[RSSI: -%d dBm] %d Position: X(%d), Y(%d), Z(%d) \n", rssi, other_id, sourcePosition.x, sourcePosition.y, sourcePosition.z);
}

LOG_GROUP_START(hivexplore)
LOG_ADD(LOG_UINT8, droneStatus, &droneStatus)
LOG_GROUP_STOP(hivexplore)

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, missionState, &missionState)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
