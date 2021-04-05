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

// TODO: Move this to comm folder
interface DroneInfo {
    dronePosition: THREE.Points;
    droneLines: [THREE.Line, THREE.Line, THREE.Line, THREE.Line];
}

interface DronePosition {
    droneId: string;
    position: Point;
}

interface DroneSensorLine {
    droneId: string;
    lines: [Point, Point][];
}

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const maxMapPoints = 1_000_000;

        const containerRef = ref<HTMLElement | undefined>(undefined);

        const scene = new THREE.Scene();
        const droneGroups = new THREE.Group();
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        // Map related variables
        let mapPoints: THREE.Points;
        let mapPointCount = 0;

        // Drone related variables
        const allDronesInfo = new Map<string, DroneInfo>();

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

            // Map points variables
            /// Geometry - buffer to hold all point positions
            const mapGeometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxMapPoints * 3);
            mapGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            mapGeometry.setDrawRange(0, 0);

            /// Material
            const mapMaterial = new THREE.PointsMaterial({ size: 0.2, color: 0xfbc02d });

            /// Points
            mapPoints = new THREE.Points(mapGeometry, mapMaterial);
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
                const droneGeometry = new THREE.BufferGeometry();
                const dronePositions = new Float32Array(3);
                droneGeometry.setAttribute('position', new THREE.BufferAttribute(dronePositions, 3));
                droneGeometry.setDrawRange(0, 1);
                const droneMaterial = new THREE.PointsMaterial({ size: 0.4, color: 0xfb4c0d });
                const dronePositionPoint = new THREE.Points(droneGeometry, droneMaterial);

                const droneSensorLineGeometry = new THREE.BufferGeometry();
                const droneSensorLines = new Float32Array(2 * 4 * 3);
                droneSensorLineGeometry.setAttribute('position', new THREE.BufferAttribute(droneSensorLines, 3));
                droneSensorLineGeometry.setDrawRange(0, 8);
                const droneSensorLineMaterial = new THREE.PointsMaterial({ size: 0.1, color: 0xffffff });
                const lineFront = new THREE.Line(droneSensorLineGeometry, droneSensorLineMaterial);
                const lineLeft = new THREE.Line(droneSensorLineGeometry, droneSensorLineMaterial);
                const lineBack = new THREE.Line(droneSensorLineGeometry, droneSensorLineMaterial);
                const lineRight = new THREE.Line(droneSensorLineGeometry, droneSensorLineMaterial);
                const lineGroup = new THREE.Group();
                lineGroup.add(lineFront);
                lineGroup.add(lineLeft);
                lineGroup.add(lineBack);
                lineGroup.add(lineRight);

                const droneGroup = new THREE.Group();
                droneGroup.add(dronePositionPoint);
                droneGroup.add(lineGroup);
                allDronesInfo.set(newDroneId, {
                    dronePosition: dronePositionPoint,
                    droneLines: [lineFront, lineLeft, lineBack, lineRight],
                });

                scene.add(droneGroup);
            }
        });

        function setDroneSensorLines(droneId: string, newDroneSensorLines: [Point, Point][]) {
            const droneInfo = allDronesInfo.get(droneId);
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

        function addMapPoint(point: Point) {
            mapPoints.geometry.attributes.position.setXYZ(mapPointCount, ...point);
            mapPointCount++;

            mapPoints.geometry.setDrawRange(0, mapPointCount);
            mapPoints.geometry.attributes.position.needsUpdate = true;
        }

        function setDronePosition(droneId: string, position: Point) {
            const droneInfo = allDronesInfo.get(droneId);
            if (droneInfo === undefined) {
                return;
            }
            droneInfo.dronePosition.geometry.attributes.position.setXYZ(0, ...position);
            droneInfo.dronePosition.geometry.attributes.position.needsUpdate = true;
        }

        socketClient.bindMessage('drone-sensor-lines', (droneSensorLines: DroneSensorLine) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            const newDroneSensorLines: [Point, Point][] = [];
            for (const droneSensorLine of droneSensorLines.lines) {
                const firstPoint = droneSensorLine[0];
                const secondPoint = droneSensorLine[1];
                const newDroneSensorLine: [Point, Point] = [
                    [firstPoint[1], firstPoint[2], firstPoint[0]],
                    [secondPoint[1], secondPoint[2], secondPoint[0]],
                ];
                newDroneSensorLines.push(newDroneSensorLine);
            }

            setDroneSensorLines(droneSensorLines.droneId, newDroneSensorLines);
        });

        socketClient.bindMessage('map-points', (mapPoints: Point[]) => {
            for (const point of mapPoints) {
                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addMapPoint([point[1], point[2], point[0]]);
            }
        });

        socketClient.bindMessage('drone-position', (dronePosition: DronePosition) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            setDronePosition(dronePosition.droneId, [dronePosition.position[1], dronePosition.position[2], dronePosition.position[0]]);
        });

        socketClient.bindMessage('clear-map', () => {
            mapPointCount = 0;
            mapPoints.geometry.setDrawRange(0, mapPointCount);
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
