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
#include "timers.h"
#define DEBUG_MODULE "APPAPI"

uint16_t min(uint16_t a, uint16_t b) {
    return (a < b) ? a : b;
}

uint16_t max(uint16_t a, uint16_t b) {
    return (a > b) ? a : b;
}

enum state { IDLE = 0, STARTUP, LIFTOFF, EXPLORE, ROTATE, LAND, OUT_OF_SERVICE } typedef state;

static bool isM1LedOn = true;
static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.5f;
static const float CRUISE_VELOCITY = 0.2f;
static const uint16_t meterToMillimeterFactor = 1000;

static const float MAXIMUM_VELOCITY = 1.0f;

static void setWaypoint(setpoint_t* setPoint, float forwardVelocity, float leftVelocity, float height, float yaw) {
    setPoint->velocity_body = true;
    setPoint->mode.x = modeVelocity;
    setPoint->mode.y = modeVelocity;
    setPoint->velocity.x = forwardVelocity;
    setPoint->velocity.y = leftVelocity;

    setPoint->mode.yaw = modeVelocity;
    setPoint->attitudeRate.yaw = yaw;

    setPoint->mode.z = modeAbs;
    setPoint->position.z = height;
}

void appMain() {
    setpoint_t setPoint;
    vTaskDelay(M2T(3000));

    logVarId_t upSensorId = logGetVarId("range", "up");
    logVarId_t downSensorId = logGetVarId("range", "zrange");
    logVarId_t leftSensorId = logGetVarId("range", "left");
    logVarId_t rightSensorId = logGetVarId("range", "right");
    logVarId_t frontSensorId = logGetVarId("range", "front");
    logVarId_t backSensorId = logGetVarId("range", "back");

    paramVarId_t flowDeckModuleId = paramGetVarId("deck", "bcFlow2");
    paramVarId_t multirangerModuleId = paramGetVarId("deck", "bcMultiranger");

    state currentState = IDLE;
    // Named as bool since the returned value is actually a bool placed in a uint8_t (see multiranger.c and flowdeck_v1v2.c call to
    // PARAM_ADD())
    const uint8_t isFlowDeckconnected = paramGetUint(flowDeckModuleId);
    const uint8_t isMultirangerConnected = paramGetUint(multirangerModuleId);
    if (!isFlowDeckconnected) {
        DEBUG_PRINT("FlowdeckV2 is not connected\n");
        currentState = OUT_OF_SERVICE;
    }
    if (!isMultirangerConnected) {
        DEBUG_PRINT("Multiranger is not connected\n");
        currentState = OUT_OF_SERVICE;
    }

    while (true) {
        vTaskDelay(M2T(10));

        uint16_t upSensorReading = logGetUint(upSensorId);
        uint16_t downSensorReading = logGetUint(downSensorId);
        uint16_t leftSensorreading = logGetUint(leftSensorId);
        uint16_t rightSensorReading = logGetUint(rightSensorId);
        uint16_t frontSensorReading = logGetUint(frontSensorId);
        uint16_t backSensorReading = logGetUint(backSensorId);

        float forwardVelocity = 0;
        float leftVelocity = 0;
        float height = 0;
        float yawRate = 0;

        // Global obstacle avoidance
        if (currentState == LIFTOFF || currentState == EXPLORE || currentState == ROTATE || currentState == LAND) {
            // Distance correction required to stay out of range of any obstacle
            uint16_t leftDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - min(leftSensorreading, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t rightDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - min(rightSensorReading, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t frontDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - min(frontSensorReading, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t backDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - min(backSensorReading, OBSTACLE_DETECTED_THRESHOLD);

            // Velocity required to apply distance correction
            const float AVOIDANCE_SENSITIVITY = MAXIMUM_VELOCITY / OBSTACLE_DETECTED_THRESHOLD;
            leftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * AVOIDANCE_SENSITIVITY;
            forwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * AVOIDANCE_SENSITIVITY;
        }

        switch (currentState) {
        case IDLE: {
            if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Startup\n");
                currentState = STARTUP;
            }
            memset(&setPoint, 0, sizeof(setpoint_t));
        } break;
        case STARTUP: {
            // Check if any obstacle is in the way before taking off
            if (upSensorReading > EXPLORATION_HEIGHT * meterToMillimeterFactor) {
                DEBUG_PRINT("Liftoff\n");
                currentState = LIFTOFF;
            }
        } break;
        case LIFTOFF: {
            height += EXPLORATION_HEIGHT;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            if (downSensorReading >= EXPLORATION_HEIGHT * meterToMillimeterFactor) {
                DEBUG_PRINT("Liftoff finished\n");
                currentState = EXPLORE;
            }

            // TODO: Remove in final algorithm, currently used to make drone land
            if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                currentState = LAND;
            }
        } break;
        case EXPLORE: {
            height += EXPLORATION_HEIGHT;
            forwardVelocity += CRUISE_VELOCITY;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);

            if (frontSensorReading < EDGE_DETECTED_THRESHOLD) {
                currentState = ROTATE;
            }

            // TODO: Remove in final algorithm, currently used to make drone land
            if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                currentState = LAND;
            }
        } break;
        case ROTATE: {
            const uint16_t OPEN_SPACE_THRESHOLD = 300;
            if (frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD) {
                currentState = EXPLORE;
            }
            height += EXPLORATION_HEIGHT;
            yawRate = 50;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);

            // TODO: Remove in final algorithm, currently used to make drone land
            if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                currentState = LAND;
            }
        } break;
        case LAND: {
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            const uint16_t LANDED_HEIGHT = 50;
            if (downSensorReading < LANDED_HEIGHT) {
                DEBUG_PRINT("Landed\n");
                currentState = IDLE;
            }
        } break;
        case OUT_OF_SERVICE: {
            isM1LedOn = true;
            ledSet(LED_RED_R, isM1LedOn);
            memset(&setPoint, 0, sizeof(setpoint_t));
        } break;
        }
        const uint8_t taskPriority = 3;
        commanderSetSetpoint(&setPoint, taskPriority);
    }
}

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
