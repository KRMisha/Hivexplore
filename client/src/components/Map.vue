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
interface DronePosition {
    droneId: string,
    position: Point,
};

interface DroneSensorLine {
    droneId: string,
    lines: [Point, Point][],
};

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const maxMapPoints = 1_000_000;
        const maxDronePositions = 1_000;

        const containerRef = ref<HTMLElement | undefined>(undefined);

        let scene: THREE.Scene;
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        let droneIds = new Map<string, number>();

        let mapPoints: THREE.Points;
        let pointCount = 0;
        let dronePoints: THREE.Points;
        let droneIndexes = 0;
        let droneSensorLinePoints: THREE.Line;
        let droneSensorLinesCount = 0;

        function onWindowResize() {
            camera.aspect = containerRef.value!.clientWidth / containerRef.value!.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(containerRef.value!.clientWidth, containerRef.value!.clientHeight);
        }

        function init() {
            // Scene
            scene = new THREE.Scene();

            // Camera
            const fov = 70;
            camera = new THREE.PerspectiveCamera(fov, containerRef.value!.clientWidth / containerRef.value!.clientHeight);
            camera.position.set(0, 10, -6);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            // Map points variables
            // Geometry - buffer to hold all point positions
            const mapGeometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxMapPoints * 3);
            mapGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            mapGeometry.setDrawRange(0, 0);

            // Material
            const mapMaterial = new THREE.PointsMaterial({ size: 0.2, color: 0xfbc02d });

            // Points
            mapPoints = new THREE.Points(mapGeometry, mapMaterial);
            scene.add(mapPoints);

            // Drone related variables
            // Geometry - buffer to hold all point positions
            const droneGeometry = new THREE.BufferGeometry();
            const dronePositions = new Float32Array(maxDronePositions * 3);
            droneGeometry.setAttribute('position', new THREE.BufferAttribute(dronePositions, 3));
            droneGeometry.setDrawRange(0, 0);
            const droneSensorLinesGeometry = new THREE.BufferGeometry();
            const droneSensorLines = new Float32Array(maxDronePositions * 4 * 3);
            droneSensorLinesGeometry.setAttribute('position', new THREE.BufferAttribute(droneSensorLines, 3));
            droneSensorLinesGeometry.setDrawRange(0, 0);

            // Material
            const droneMaterial = new THREE.PointsMaterial({ size: 0.4, color: 0xfb4c0d });
            const droneSensorLineMaterial = new THREE.PointsMaterial({ size: 0.4, color: 0x00ff00 });

            // Points
            dronePoints = new THREE.Points(droneGeometry, droneMaterial);
            droneSensorLinePoints = new THREE.Line(droneSensorLinesGeometry, droneSensorLineMaterial);
            scene.add(dronePoints);
            scene.add(droneSensorLinePoints);

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

        function setDroneSensorLines(droneId: string, allDroneSensorLines: [Point, Point][]) {
            // for (const droneSensorLine of allDroneSensorLines) {
            //     for (const point of droneSensorLine) {
            //         droneSensorLinePoints.geometry.attributes.position.setXYZ(0, ...point);
            //         droneSensorLinePoints.geometry.attributes.position.setXYZ(1, ...point);
            //     }
            // }
            // droneSensorLinePoints.geometry.setDrawRange(0, 2);
            // droneSensorLinePoints.geometry.attributes.position.needsUpdate = true;
        }

        function addPoint(point: Point) {
            mapPoints.geometry.attributes.position.setXYZ(pointCount, ...point);
            pointCount++;

            mapPoints.geometry.setDrawRange(0, pointCount);
            mapPoints.geometry.attributes.position.needsUpdate = true;
        }

        function setDronePosition(droneId: string, position: Point) {
            if (droneIds.get(droneId) === undefined) {
                droneIds.set(droneId, droneIndexes);
                dronePoints.geometry.attributes.position.setXYZ(droneIds.get(droneId)!, ...position);
                droneIndexes++;
                dronePoints.geometry.setDrawRange(0, droneIndexes);
            } else {
                dronePoints.geometry.attributes.position.setXYZ(droneIds.get(droneId)!, ...position);
            }
            dronePoints.geometry.attributes.position.needsUpdate = true;
        }

        socketClient.bindMessage('drone-sensor-lines', (allDroneSensorLines: DroneSensorLine) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            let newDroneSensorLines: [Point, Point][] = [];
            for (const droneSensorLine of allDroneSensorLines.lines) {
                const firstPoint = droneSensorLine[0]
                const secondPoint = droneSensorLine[1]
                const newDroneSensorLine: [Point, Point] = [[firstPoint[1], firstPoint[2], firstPoint[0]], [secondPoint[1], secondPoint[2], secondPoint[0]]];
                newDroneSensorLines.push(newDroneSensorLine);
            }

            console.log(allDroneSensorLines.droneId, newDroneSensorLines);
            setDroneSensorLines(allDroneSensorLines.droneId, newDroneSensorLines);
        });

        socketClient.bindMessage('map-points', (mapPoints: Point[]) => {
            for (const point of mapPoints) {
                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addPoint([point[1], point[2], point[0]]);
            }
        });

        socketClient.bindMessage('drone-position', (dronePosition: DronePosition) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            setDronePosition(dronePosition.droneId, [dronePosition.position[1], dronePosition.position[2], dronePosition.position[0]]);
        });

        socketClient.bindMessage('clear-map', () => {
            pointCount = 0;
            mapPoints.geometry.setDrawRange(0, pointCount);
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
