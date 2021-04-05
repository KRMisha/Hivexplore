<template>
    <Panel header="Map">
        <template #icons>
            <Button
                class="p-panel-header-icon"
                icon="pi pi-download"
                v-tooltip.left="'Download map'"
                aria-label="Download map"
                @click="saveAsImage"
            />
        </template>
        <div ref="containerRef" class="container"></div>
    </Panel>
</template>

<script lang="ts">
import { defineComponent, inject, onMounted, onUnmounted, ref } from 'vue';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import { SocketClient } from '@/classes/socket-client';
import { getLocalTimestamp } from '@/utils/local-timestamp';

type Point = [number, number, number];
type Line = [Point, Point];

interface DroneInfo {
    dronePosition: THREE.Points;
    droneLines: [THREE.Line, THREE.Line, THREE.Line, THREE.Line];
}

// TODO: Move this to communication folder
interface DronePosition {
    droneId: string;
    position: Point;
}

interface DroneSensorLine {
    droneId: string;
    lines: Line[];
}

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const containerRef = ref<HTMLElement | undefined>(undefined);

        const scene = new THREE.Scene();
        const droneGroups = new THREE.Group();
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        // Map-related variables
        const maxMapPoints = 1_000_000;
        let mapPoints: THREE.Points;
        let mapPointCount = 0;

        // Drone-related variables
        const droneInfos = new Map<string, DroneInfo>();

        function onWindowResize() {
            camera.aspect = containerRef.value!.clientWidth / containerRef.value!.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(containerRef.value!.clientWidth, containerRef.value!.clientHeight);
        }

        function init() {
            // Camera
            const fov = 70;
            camera = new THREE.PerspectiveCamera(fov, containerRef.value!.clientWidth / containerRef.value!.clientHeight);
            camera.position.set(0, 10, -6);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            // Map point geometry - buffer to hold all map point positions
            const mapPointGeometry = new THREE.BufferGeometry();
            const mapPointPositions = new Float32Array(maxMapPoints * 3);
            mapPointGeometry.setAttribute('position', new THREE.BufferAttribute(mapPointPositions, 3));
            mapPointGeometry.setDrawRange(0, 0);

            // Map point material
            const mapPointMaterial = new THREE.PointsMaterial({ size: 0.2, color: 0xfbc02d });

            // Map points
            mapPoints = new THREE.Points(mapPointGeometry, mapPointMaterial);
            scene.add(mapPoints);

            // Renderer
            renderer = new THREE.WebGLRenderer({ alpha: true, antialias: true, preserveDrawingBuffer: true });
            renderer.setSize(containerRef.value!.clientWidth, containerRef.value!.clientHeight);
            renderer.setClearColor(0xffffff, 0);
            containerRef.value!.append(renderer.domElement);

            // Controls
            controls = new OrbitControls(camera, renderer.domElement);
            controls.enableDamping = true;

            // Helpers
            const axesHelper = new THREE.AxesHelper(2);
            scene.add(axesHelper);
            const gridHelper = new THREE.GridHelper(16, 16);
            scene.add(gridHelper);

            // Resize canvas on window resize
            window.addEventListener('resize', onWindowResize);
        }

        function animate() {
            requestAnimationFrame(animate);
            controls.update();
            renderer.render(scene, camera);
        }

        const socketClient = inject('socketClient') as SocketClient;

        socketClient.bindMessage('drone-ids', (newDroneIds: string[]) => {
            scene.remove(droneGroups);

            for (const newDroneId of newDroneIds) {
                // Drone
                const droneGeometry = new THREE.BufferGeometry();
                const dronePositions = new Float32Array(3);
                droneGeometry.setAttribute('position', new THREE.BufferAttribute(dronePositions, 3));
                const droneMaterial = new THREE.PointsMaterial({ size: 0.4, color: 0xfb4c0d });
                const dronePositionPoint = new THREE.Points(droneGeometry, droneMaterial);

                // Drone sensor lines
                const droneSensorLineGeometry = new THREE.BufferGeometry();
                const droneSensorLines = new Float32Array(2 * 4 * 3);
                droneSensorLineGeometry.setAttribute('position', new THREE.BufferAttribute(droneSensorLines, 3));
                const droneSensorLineMaterial = new THREE.PointsMaterial({ size: 0.1, color: 0xffffff });
                const lineGroup = new THREE.Group();
                const lines: THREE.Line[] = [];
                for (let i = 0; i < 4; i++) {
                    const line = new THREE.Line(droneSensorLineGeometry, droneSensorLineMaterial);
                    lines.push(line);
                    lineGroup.add(line);
                }

                const droneGroup = new THREE.Group();
                droneGroup.add(dronePositionPoint);
                droneGroup.add(lineGroup);
                droneInfos.set(newDroneId, {
                    dronePosition: dronePositionPoint,
                    droneLines: lines as [THREE.Line, THREE.Line, THREE.Line, THREE.Line],
                });

                scene.add(droneGroup);
            }
        });

        function convertServerPointCoords(point: Point): Point {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            return [point[1], point[2], point[0]];
        }

        function addMapPoint(point: Point) {
            mapPoints.geometry.attributes.position.setXYZ(mapPointCount, ...point);
            mapPointCount++;

            mapPoints.geometry.setDrawRange(0, mapPointCount);
            mapPoints.geometry.attributes.position.needsUpdate = true;
        }

        socketClient.bindMessage('map-points', (mapPoints: Point[]) => {
            for (const point of mapPoints) {
                addMapPoint(convertServerPointCoords(point));
            }
        });

        socketClient.bindMessage('clear-map', () => {
            mapPointCount = 0;
            mapPoints.geometry.setDrawRange(0, mapPointCount);
        });

        function setDronePosition(droneId: string, position: Point) {
            const droneInfo = droneInfos.get(droneId);
            if (droneInfo === undefined) {
                return;
            }
            droneInfo.dronePosition.geometry.attributes.position.setXYZ(0, ...position);
            droneInfo.dronePosition.geometry.attributes.position.needsUpdate = true;
        }

        socketClient.bindMessage('drone-position', (dronePosition: DronePosition) => {
            setDronePosition(dronePosition.droneId, convertServerPointCoords(dronePosition.position));
        });

        function setDroneSensorLines(droneId: string, newDroneSensorLines: Line[]) {
            const droneInfo = droneInfos.get(droneId);
            if (droneInfo === undefined) {
                return;
            }

            let index = 0;
            for (let i = 0; i < newDroneSensorLines.length; i++) {
                droneInfo.droneLines[i].geometry.attributes.position.setXYZ(index++, ...newDroneSensorLines[i][0]);
                droneInfo.droneLines[i].geometry.attributes.position.setXYZ(index++, ...newDroneSensorLines[i][1]);
                droneInfo.droneLines[i].geometry.attributes.position.needsUpdate = true;
            }
        }

        socketClient.bindMessage('drone-sensor-lines', (newDroneSensorLines: DroneSensorLine) => {
            const droneSensorLines: Line[] = newDroneSensorLines.lines.map((line: Line) => {
                return [convertServerPointCoords(line[0]), convertServerPointCoords(line[1])];
            });
            setDroneSensorLines(newDroneSensorLines.droneId, droneSensorLines);
        });

        function saveAsImage() {
            const filename = `hivexplore_map_${getLocalTimestamp().replaceAll(':', '')}.png`;
            const url = renderer.domElement.toDataURL('image/png;base64');
            const link = document.createElement('a');
            link.download = filename;
            link.href = url;
            link.click();
            window.URL.revokeObjectURL(url);
        }

        onMounted(() => {
            init();
            animate();
        });

        onUnmounted(() => {
            window.removeEventListener('resize', onWindowResize);
        });

        return {
            containerRef,
            saveAsImage,
        };
    },
});
</script>

<style lang="scss" scoped>
.container {
    height: 300px;

    @media (max-width: 575px) {
        height: 225px;
    }
}
</style>
