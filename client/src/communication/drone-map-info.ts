import * as THREE from 'three';

export type Point = [number, number, number];
export type Line = [Point, Point];

export interface DroneInfo {
    position: THREE.Points;
    sensorLines: [THREE.Line, THREE.Line, THREE.Line, THREE.Line];
}

export interface DronePosition {
    droneId: string;
    position: Point;
}

export interface DroneSensorLine {
    droneId: string;
    sensorLines: Line[];
}
