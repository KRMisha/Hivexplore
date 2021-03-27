#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdbool.h>
#include <stdint.h>

// Enums
typedef enum { MISSION_STANDBY, MISSION_EXPLORING, MISSION_RETURNING, MISSION_EMERGENCY, MISSION_LANDED } mission_state_t;
typedef enum { EXPLORING_IDLE, EXPLORING_LIFTOFF, EXPLORING_EXPLORE, EXPLORING_ROTATE } exploring_state_t;
typedef enum { RETURNING_RETURN, RETURNING_LAND, RETURNING_IDLE } returning_state_t;
typedef enum { EMERGENCY_LAND, EMERGENCY_IDLE } emergency_state_t;
typedef enum { STATUS_STANDBY, STATUS_LIFTOFF, STATUS_FLYING, STATUS_LANDING, STATUS_LANDED, STATUS_CRASHED } drone_status_t;

void avoidObstacle(void);
void explore(void);
void returnToBase(void);
void emergencyLand(void);
bool liftoff(void);
bool land(void);

void updateWaypoint(void);
uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading);

#endif