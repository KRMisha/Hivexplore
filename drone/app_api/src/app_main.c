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

typedef enum { STANDBY, EXPLORING, RETURNING } mission_state_t;
typedef enum { IDLE, STARTUP, LIFTOFF, EXPLORE, ROTATE, LAND, OUT_OF_SERVICE } explore_state_t;

static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.5f;
static const float CRUISE_VELOCITY = 0.2f;
static const float MAXIMUM_VELOCITY = 1.0f;
static const uint16_t METER_TO_MILLIMETER_FACTOR = 1000;

static mission_state_t missionState = STANDBY;
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

static uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading) {
    return obstacleThreshold - MIN(sensorReading, obstacleThreshold);
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

    paramVarId_t flowDeckModuleId = paramGetVarId("deck", "bcFlow2");
    paramVarId_t multirangerModuleId = paramGetVarId("deck", "bcMultiranger");

    explore_state_t exploreState = IDLE;
    const bool isFlowDeckInitialized = paramGetUint(flowDeckModuleId);
    const bool isMultirangerInitialized = paramGetUint(multirangerModuleId);
    if (!isFlowDeckInitialized) {
        DEBUG_PRINT("FlowDeckV2 is not connected\n");
        exploreState = OUT_OF_SERVICE;
    }
    if (!isMultirangerInitialized) {
        DEBUG_PRINT("Multiranger is not connected\n");
        exploreState = OUT_OF_SERVICE;
    }

    while (true) {
        vTaskDelay(M2T(10));

        ledSet(LED_GREEN_R, isM1LedOn);

        switch (missionState) {
        case STANDBY:
            break;
        case EXPLORING: {
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
            if (exploreState == LIFTOFF || exploreState == EXPLORE || exploreState == ROTATE || exploreState == LAND) {
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

            switch (exploreState) {
            case IDLE: {
                DEBUG_PRINT("Startup\n");
                exploreState = STARTUP;
                memset(&setPoint, 0, sizeof(setpoint_t));
            } break;
            case STARTUP: {
                // Check if any obstacle is in the way before taking off
                if (upSensorReading > EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
                    DEBUG_PRINT("Liftoff\n");
                    exploreState = LIFTOFF;
                }
            } break;
            case LIFTOFF: {
                targetHeight += EXPLORATION_HEIGHT;
                setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
                if (downSensorReading >= EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
                    DEBUG_PRINT("Liftoff finished\n");
                    exploreState = EXPLORE;
                }

                // TODO: Remove in final algorithm, currently used to make drone land
                if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                    exploreState = LAND;
                }
            } break;
            case EXPLORE: {
                targetHeight += EXPLORATION_HEIGHT;
                targetForwardVelocity += CRUISE_VELOCITY;
                setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);

                if (frontSensorReading < EDGE_DETECTED_THRESHOLD) {
                    exploreState = ROTATE;
                }

                // TODO: Remove in final algorithm, currently used to make drone land
                if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                    exploreState = LAND;
                }
            } break;
            case ROTATE: {
                static const uint16_t OPEN_SPACE_THRESHOLD = 300;
                if (frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD) {
                    exploreState = EXPLORE;
                }
                targetHeight += EXPLORATION_HEIGHT;
                targetYawRate = 50;
                setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);

                // TODO: Remove in final algorithm, currently used to make drone land
                if (upSensorReading < OBSTACLE_DETECTED_THRESHOLD) {
                    exploreState = LAND;
                }
            } break;
            case LAND: {
                setWaypoint(&setPoint, targetForwardVelocity, targetLeftVelocity, targetHeight, targetYawRate);
                static const uint16_t LANDED_HEIGHT = 50;
                if (downSensorReading < LANDED_HEIGHT) {
                    DEBUG_PRINT("Landed\n");
                    exploreState = IDLE;
                }
            } break;
            case OUT_OF_SERVICE: {
                ledSet(LED_RED_R, true);
                memset(&setPoint, 0, sizeof(setpoint_t));
            } break;
            }

            static const uint8_t TASK_PRIORITY = 3;
            commanderSetSetpoint(&setPoint, TASK_PRIORITY);
        } break;
        case RETURNING:
            break;
        }
    }
}

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, missionState, &missionState)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
