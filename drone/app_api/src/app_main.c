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
static setpoint_t setPoint;
enum state {IDLE = 0 , STARTUP, TAKEOFF, FORWARD, LANDING, LANDED} typedef state;
static state currentState = IDLE;

float cruiseHeight = 1.f;
float cruiseVelocity = 0.2f;

static void setWaypoint(setpoint_t* setpoint, float vx, float vy, float z, float yawrate) {
    setpoint->mode.z = modeAbs;
    setpoint->position.z = z;

    setpoint->mode.yaw = modeVelocity;
    setpoint->attitudeRate.yaw = yawrate;

    setpoint->mode.x = modeVelocity;
    setpoint->mode.y = modeVelocity;
    setpoint->velocity.x = vx;
    setpoint->velocity.y = vy;

    setpoint->velocity_body = true;
}


void appMain() {
    logVarId_t idUp = logGetVarId("range", "up");
    logVarId_t idDown = logGetVarId("range", "zrange");
    // logVarId_t idLeft = logGetVarId("range", "left");
    // logVarId_t idRight = logGetVarId("range", "right");
    logVarId_t idFront = logGetVarId("range", "front");
    // logVarId_t idBack = logGetVarId("range", "back");

    // paramVarId_t idPositioningDeck = paramGetVarId("deck", "bcFlow2");
    // paramVarId_t idMultiranger = paramGetVarId("deck", "bcMultiranger");

    vTaskDelay(M2T(1000));
    currentState = IDLE;
    while (true) {
        vTaskDelay(M2T(10));
        // uint8_t positioningInit = paramGetUint(idPositioningDeck);
        // uint8_t multirangerInit = paramGetUint(idMultiranger);
        uint16_t up = logGetUint(idUp);
        // DEBUG_PRINT("Up: %i\n", up);
        uint16_t down = logGetUint(idDown);
        // uint16_t left = logGetUint(idLeft);
        // uint16_t right = logGetUint(idRight);
        uint16_t front = logGetUint(idFront);
        // uint16_t back = logGetUint(idBack);

        switch (currentState) {
            case IDLE: {
                    if (up < 200) {
                        DEBUG_PRINT("Startup\n");
                        currentState = STARTUP;
                    }
                    memset(&setPoint, 0, sizeof(setpoint_t));
                    commanderSetSetpoint(&setPoint, 3);
                }
                break;
            case STARTUP: {
                    if (up > 300) {
                        DEBUG_PRINT("Takeoff\n");
                        currentState = TAKEOFF;
                    }
                }
                break;
            case TAKEOFF: {
                    // TODO: Check obstacle above
                    float vx = 0.0f;
                    float vy = 0.0f;
                    setWaypoint(&setPoint, vx, vy, cruiseHeight, 0);
                    commanderSetSetpoint(&setPoint, 3);
                    if(down >= cruiseHeight * 1000) {
                        DEBUG_PRINT("Take off started\n");
                        currentState = FORWARD;
                        DEBUG_PRINT("Take off finished\n");
                    }
                }
                break;
            case FORWARD: {
                    float vy = 0.f;
                    setWaypoint(&setPoint, cruiseVelocity, vy, cruiseHeight, 0);
                    commanderSetSetpoint(&setPoint, 3);
                    if (front < 200) {
                        DEBUG_PRINT("Wow minute madame, on se calme le ponpon\n");
                        currentState = LANDING;
                    }
                }
                break;
            case LANDING: {
                    float vx = 0.0f;
                    float vy = 0.0f;
                    float height = 0.0f;
                    setWaypoint(&setPoint, vx, vy, height, 0);
                    commanderSetSetpoint(&setPoint, 3);
                    if (down < 50) {
                        DEBUG_PRINT("Land started\n");
                        currentState = LANDED;
                        DEBUG_PRINT("Land finished\n");
                    }
                }
                break;
            case LANDED: {
                    DEBUG_PRINT("4->0\n");
                    currentState = IDLE;
                }
                break;
        }
    }
}

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
