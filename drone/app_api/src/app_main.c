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

static bool isM1LedOn = true;
enum state { IDLE = 0, STARTUP, TAKEOFF, HOVER, DANCE, DANCING, LANDING, LANDED } typedef state;

static const uint16_t OBTACLE_DETECTED_THRESHOLD = 300;
const float EXPLORATION_HEIGHT = 0.5f;
const float CRUISE_VELOCITY = 0.2f;

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

    // TODO: Check module status and switch to Out-Of-Service state if unavailale
    // paramVarId_t idPositioningDeck = paramGetVarId("deck", "bcFlow2");
    // paramVarId_t idMultiranger = paramGetVarId("deck", "bcMultiranger");

    float forwardVelocity = 0;
    float leftVelocity = 0;
    float height = 0;
    float yawRate = 0;
    static state currentState = IDLE;
    while (true) {
        vTaskDelay(M2T(10));
        // uint8_t positioningInit = paramGetUint(idPositioningDeck);
        // uint8_t multirangerInit = paramGetUint(idMultiranger);
        uint16_t up = logGetUint(idUp);
        uint16_t down = logGetUint(idDown);
        uint16_t left = logGetUint(idLeft);
        uint16_t right = logGetUint(idRight);
        uint16_t front = logGetUint(idFront);
        uint16_t back = logGetUint(idBack);

        switch (currentState) {
        case IDLE: {
            if (up < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Startup\n");
                currentState = STARTUP;
            }
            memset(&setPoint, 0, sizeof(setpoint_t));
        } break;
        case STARTUP: {
            if (up > OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Takeoff\n");
                currentState = TAKEOFF;
            }
        } break;
        case TAKEOFF: {
            // TODO: Check obstacle above
            forwardVelocity = 0;
            leftVelocity = 0;
            height = EXPLORATION_HEIGHT;
            yawRate = 0;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            const uint16_t millimeterToMeterFactor = 1000;
            if (down >= EXPLORATION_HEIGHT * millimeterToMeterFactor) {
                DEBUG_PRINT("Take off started\n");
                currentState = HOVER;
                DEBUG_PRINT("Take off finished\n");
            }
        } break;
        case HOVER: {
            forwardVelocity = 0;
            leftVelocity = 0;
            height = EXPLORATION_HEIGHT;
            yawRate = 0;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);

            if (up < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Obstacle above detected\n");
                currentState = LANDING;
            }
            if (left < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Obstacle on the left detected\n");
            }
            if (right < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Obstacle on the right detected\n");
            }
            if (front < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("Obstacle in front detected\n");
            }
            if (back < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("EVERYBODY DANCE NOW!\n");
                currentState = DANCE;
            }
        } break;
        case DANCE: {
            if (back > OBTACLE_DETECTED_THRESHOLD) {
                currentState = DANCING;
            }
        } break;
        case DANCING: {
            if (front < OBTACLE_DETECTED_THRESHOLD) {
                DEBUG_PRINT("BOOOOOOOOOOO!\n");
                currentState = HOVER;
            }
            forwardVelocity = 0;
            leftVelocity = 0;
            height = EXPLORATION_HEIGHT;
            yawRate = 20;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
        } break;
        case LANDING: {
            forwardVelocity = 0;
            leftVelocity = 0;
            height = 0;
            yawRate = 0;
            setWaypoint(&setPoint, forwardVelocity, leftVelocity, height, yawRate);
            const uint16_t LANDED_HEIGHT = 50;
            if (down < LANDED_HEIGHT) {
                DEBUG_PRINT("Land started\n");
                currentState = LANDED;
                DEBUG_PRINT("Land finished\n");
            }
        } break;
        case LANDED: {
            DEBUG_PRINT("4->0\n");
            currentState = IDLE;
        } break;
        }
        const uint8_t taskPriority = 3;
        commanderSetSetpoint(&setPoint, taskPriority);
    }
}

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
