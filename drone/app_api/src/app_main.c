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

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

static bool isM1LedOn = true;
enum state { IDLE = 0, STARTUP, TAKEOFF, EXPLORE, ROTATE, LAND, LANDED, OUT_OF_SERVICE } typedef state;

static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
const float EXPLORATION_HEIGHT = 0.5f;
const float CRUISE_VELOCITY = 0.2f;
const uint16_t meterToMillimeterFactor = 1000;

const float MAXIMUM_VELOCITY = 1.0f;
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
    static setpoint_t setPoint;
    vTaskDelay(M2T(3000));

    logVarId_t idUp = logGetVarId("range", "up");
    logVarId_t idDown = logGetVarId("range", "zrange");
    logVarId_t idLeft = logGetVarId("range", "left");
    logVarId_t idRight = logGetVarId("range", "right");
    logVarId_t idFront = logGetVarId("range", "front");
    logVarId_t idBack = logGetVarId("range", "back");

    paramVarId_t idPositioningDeck = paramGetVarId("deck", "bcFlow2");
    paramVarId_t idMultiranger = paramGetVarId("deck", "bcMultiranger");

    static state currentState = IDLE;
    while (true) {
        vTaskDelay(M2T(10));
        uint8_t positioningInit = paramGetUint(idPositioningDeck);
        uint8_t multirangerInit = paramGetUint(idMultiranger);
        uint16_t upSensor = logGetUint(idUp);
        uint16_t downSensor = logGetUint(idDown);
        uint16_t leftSensor = logGetUint(idLeft);
        uint16_t rightSensor = logGetUint(idRight);
        uint16_t frontSensor = logGetUint(idFront);
        uint16_t backSensor = logGetUint(idBack);

        float forwardVelocity = 0;
        float leftVelocity = 0;
        float height = 0;
        float yawRate = 0;

        // Obstacle avoidance
        if (currentState != IDLE && currentState != STARTUP
            && currentState != OUT_OF_SERVICE) {

            // Distance correction required to stay out of range of any obstacle
            uint16_t leftDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - MIN(leftSensor, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t rightDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - MIN(rightSensor, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t frontDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - MIN(frontSensor, OBSTACLE_DETECTED_THRESHOLD);
            uint16_t backDistanceCorrection = OBSTACLE_DETECTED_THRESHOLD - MIN(backSensor, OBSTACLE_DETECTED_THRESHOLD);

            // float leftVelocityCompensation = -1 * leftDistanceCorrection * AVOIDANCE_SENSITIVITY;
            // float rightVelocityCompensation = rightDistanceCorrection * AVOIDANCE_SENSITIVITY;
            // float frontVelocityCompensation = -1 * frontDistanceCorrection * AVOIDANCE_SENSITIVITY;
            // float backVelocityCompensation = backDistanceCorrection * AVOIDANCE_SENSITIVITY;

            // Velocity required to apply distance correction
            const float AVOIDANCE_SENSITIVITY = MAXIMUM_VELOCITY / OBSTACLE_DETECTED_THRESHOLD;
            leftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * AVOIDANCE_SENSITIVITY;
            forwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * AVOIDANCE_SENSITIVITY;

            // leftVelocity += rightVelocityCompensation + leftVelocityCompensation;
            // forwardVelocity += frontVelocityCompensation + belowVelocityCompensation;
        }

        switch (currentState) {
        case IDLE: {
            if (upSensor < OBSTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Startup\n");
                currentState = STARTUP;
            }
            memset(&setPoint, 0, sizeof(setpoint_t));
        } break;
        case STARTUP: {
            // Check if any obstacle if present before taking off
            if (upSensor > EXPLORATION_HEIGHT * meterToMillimeterFactor) {
                DEBUG_PRINT("Takeoff\n");
                currentState = TAKEOFF;
            }

            if (!positioningInit || !multirangerInit) {
                currentState = OUT_OF_SERVICE;
            }
        } break;
        case TAKEOFF: {
            height += EXPLORATION_HEIGHT;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            if (downSensor >= EXPLORATION_HEIGHT * meterToMillimeterFactor) {
                currentState = EXPLORE;
                DEBUG_PRINT("Take off finished\n");
            }
        } break;
        case EXPLORE: {
            height += EXPLORATION_HEIGHT;
            forwardVelocity += CRUISE_VELOCITY;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);

            if (frontSensor < EDGE_DETECTED_THRESHOLD) {
                currentState = ROTATE;
            }

            if (upSensor < OBSTACLE_DETECTED_THRESHOLD) {
                currentState = LAND;
            }
        } break;
        case ROTATE: {
            const uint16_t OPEN_SPACE_THRESHOLD = 300;
            if (frontSensor > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD) {
                currentState = EXPLORE;
            }

            height += EXPLORATION_HEIGHT;
            yawRate = 50;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
        } break;
        case LAND: {
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            const uint16_t LANDED_HEIGHT = 50;
            if (downSensor < LANDED_HEIGHT) {
                DEBUG_PRINT("Land started\n");
                currentState = LANDED;
                DEBUG_PRINT("Land finished\n");
            }
        } break;
        case LANDED: {
            DEBUG_PRINT("4->0\n");
            currentState = IDLE;
        } break;
        case OUT_OF_SERVICE: {
            if (!positioningInit) {
                DEBUG_PRINT("FlowdeckV2 is not connected\n");
            }

            if (!multirangerInit) {
                DEBUG_PRINT("Multiranger is not connected\n");
            }
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
