import * as THREE from 'three';

export interface DroneInfo {
    position: THREE.Points;
    sensorLines: [THREE.Line, THREE.Line, THREE.Line, THREE.Line, THREE.Line, THREE.Line];
}
