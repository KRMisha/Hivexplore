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
#include <stdint.h>
#include <stdbool.h>

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

#define DEBUG_MODULE "APPAPI"

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

typedef enum { MISSION_STANDBY, MISSION_EXPLORING, MISSION_RETURNING, MISSION_EMERGENCY } mission_state_t;
typedef enum { EXPLORING_IDLE, EXPLORING_LIFTOFF, EXPLORING_EXPLORE, EXPLORING_ROTATE } exploring_state_t;
typedef enum { RETURNING_RETURN, RETURNING_LAND, RETURNING_IDLE } returning_state_t;
typedef enum { STATUS_STANDBY, STATUS_FLYING, STATUS_LANDED, STATUS_CRASHED } drone_status_t;

static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.5f;
static const float CRUISE_VELOCITY = 0.2f;
static const float MAXIMUM_VELOCITY = 1.0f;
static const uint16_t METER_TO_MILLIMETER_FACTOR = 1000;

static mission_state_t missionState = MISSION_STANDBY;
static exploring_state_t exploringState = EXPLORING_IDLE;
static returning_state_t returningState = RETURNING_RETURN;
static bool droneIsLanding = false;

static drone_status_t droneStatus = STATUS_STANDBY;
static bool isM1LedOn = false;

static void setWaypoint(setpoint_t* setPoint, float targetForwardVelocity, float targetLeftVelocity, float targetHeight, float yaw) {
    setPoint->velocity_body = true;
    setPoint->mode.x = modeVelocity;
    setPoint->mode.y = modeVelocity;
    setPoint->velocity.x = targetForwardVelocity;
    setPoint->velocity.y = targetLeftVelocity;

    setPoint->mode.yaw = modeVelocity;
    setPoint->attitudeRate.yaw = yaw;

    setPoint->mode.z = modeAbs;
    setPoint->position.z = targetHeight;
}

static void updateDroneStatus() {
    // TODO: Handle crash state
    if (missionState == MISSION_STANDBY) {
        droneStatus = STATUS_STANDBY;
    } else if (droneIsLanding && returningState == RETURNING_IDLE) {
        droneIsLanding = false;
        droneStatus = STATUS_LANDED;
    } else if (missionState == MISSION_EMERGENCY || returningState == RETURNING_LAND) {
        droneIsLanding = true;
    } else if (missionState == MISSION_STANDBY && exploringState == EXPLORING_IDLE && returningState == RETURNING_IDLE) {
        droneStatus = STATUS_FLYING;
    }
}

static uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading) {
    return obstacleThreshold - MIN(sensorReading, obstacleThreshold);
}

static void Explore() {
    // TODO: many variables will need to be hoisted out globally to be accessible to both explore() and return() (ex: readings)
    uint16_t frontSensorReading = logGetUint(frontSensorId);
    uint16_t leftSensorReading = logGetUint(leftSensorId);
    uint16_t backSensorReading = logGetUint(backSensorId);
    uint16_t rightSensorReading = logGetUint(rightSensorId);
    uint16_t upSensorReading = logGetUint(upSensorId);
    uint16_t downSensorReading = logGetUint(downSensorId);

    uint8_t rssiReading = logGetUint(rssiId);
    (void)rssiReading; // TODO: Remove (this silences the unused variable compiler warning which is treated as an error)

    float targetForwardVelocity = 0.0;
    float targetLeftVelocity = 0.0;
    float targetHeight = 0.0;
    float targetYawRate = 0.0;

    // Global obstacle avoidance
    if (exploringState == EXPLORING_LIFTOFF || exploringState == EXPLORING_EXPLORE || exploringState == EXPLORING_ROTATE) {
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

    switch (exploringState) {
    case EXPLORING_IDLE: {
        DEBUG_PRINT("Idle\n");
        memset(&setPoint, 0, sizeof(setpoint_t));

        // Check if any obstacle is in the way before taking off
        if (upSensorReading > EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
            DEBUG_PRINT("Liftoff\n");
            exploringState = EXPLORING_LIFTOFF;
        }
    } break;
    case EXPLORING_LIFTOFF: {
        targetHeight += EXPLORATION_HEIGHT;
        setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
        if (downSensorReading >= EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
            DEBUG_PRINT("Liftoff finished\n");
            exploringState = EXPLORING_EXPLORE;
        }
    } break;
    case EXPLORING_EXPLORE: {
        targetHeight += EXPLORATION_HEIGHT;
        targetForwardVelocity += CRUISE_VELOCITY;
        setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);

        if (frontSensorReading < EDGE_DETECTED_THRESHOLD) {
            exploringState = EXPLORING_ROTATE;
        }
    } break;
    case EXPLORING_ROTATE: {
        static const uint16_t OPEN_SPACE_THRESHOLD = 300;
        if (frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD) {
            exploringState = EXPLORING_EXPLORE;
        }
        targetHeight += EXPLORATION_HEIGHT;
        targetYawRate = 50;
        setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
    } break;
    }

    static const uint8_t TASK_PRIORITY = 3;
    commanderSetSetpoint(&setPoint, TASK_PRIORITY);
}

static void Land() {
    setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
    static const uint16_t LANDED_HEIGHT = 50;
    if (downSensorReading < LANDED_HEIGHT) {
        DEBUG_PRINT("Landed\n");
    }
    returningState = RETURNING_IDLE
}

static void Return() {
    switch (returningState) {
    case RETURNING_RETURN: {
        // TODO: Add return logic
        vTaskDelay(M2T(5000));
        returningState = RETURNING_LAND
    } break;
    case RETURNING_LAND: {
        Land();
    } break;
    case RETURNING_IDLE:
        break;
    }
}

void appMain(void) {
    setpoint_t setPoint;
    vTaskDelay(M2T(3000));

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
    const bool isOutOfService = !isFlowDeckInitialized || !isMultirangerInitialized;
    if (!isFlowDeckInitialized) {
        DEBUG_PRINT("FlowDeckV2 is not connected\n");
    }
    if (!isMultirangerInitialized) {
        DEBUG_PRINT("Multiranger is not connected\n");
    }

    while (true) {
        vTaskDelay(M2T(10));

        updateDroneStatus();
        ledSet(LED_GREEN_R, isM1LedOn);

        if (isOutOfService) {
            ledSet(LED_RED_R, true);
            continue;
        }

        switch (missionState) {
        case MISSION_STANDBY:
            break;
        case MISSION_EXPLORING: {
            Explore();
        } break;
        case MISSION_RETURNING: {
            Return();
        } break;
        case MISSION_EMERGENCY: {
            Land();
        } break;
        }
    }
}

LOG_GROUP_START(hivexplore)
LOG_ADD(LOG_UINT8, droneStatus, &droneStatus)
LOG_GROUP_STOP(hivexplore)

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, missionState, &missionState)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
