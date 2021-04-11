#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdbool.h>

#include "radiolink.h"

typedef enum {
    MISSION_STANDBY,
    MISSION_EXPLORING,
    MISSION_RETURNING,
    MISSION_EMERGENCY,
    MISSION_LANDED,
} mission_state_t;

typedef enum {
    EXPLORING_IDLE,
    EXPLORING_LIFTOFF,
    EXPLORING_EXPLORE,
    EXPLORING_ROTATE,
} exploring_state_t;

typedef enum {
    RETURNING_ROTATE_TOWARDS_BASE,
    RETURNING_RETURN,
    RETURNING_ROTATE,
    RETURNING_FORWARD,
    RETURNING_LAND,
    RETURNING_IDLE,
} returning_state_t;

typedef enum {
    EMERGENCY_LAND,
    EMERGENCY_IDLE,
} emergency_state_t;

typedef enum {
    STATUS_STANDBY,
    STATUS_LIFTOFF,
    STATUS_FLYING,
    STATUS_LANDING,
    STATUS_LANDED,
    STATUS_CRASHED,
} drone_status_t;

uint8_t calculateBatteryLevel(void);

void avoidDrones(void);
void avoidObstacles(void);
void explore(void);
void returnToBase(void);
void emergencyLand(void);
bool liftoff(void);
bool forward(void);
bool rotate(void);
bool land(void);
bool isCrashed(void);

void resetInternalStates(void);

void broadcastPosition(void);
void p2pReceivedCallback(P2PPacket* packet);

void updateWaypoint(void);
uint16_t calculateObstacleDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading);

#endif
