<template>
    <Panel header="Map" class="map">
        <div id="map-container"></div>
    </Panel>
    <Button class="button" label="Download map" @click="saveAsImage()" />
</template>

<script lang="ts">
import { defineComponent, inject, onMounted, onUnmounted } from 'vue';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import Stats from 'three/examples/jsm/libs/stats.module';
import { SocketClient } from '@/classes/socket-client';
import { getLocalTimestamp } from '@/utils/local-timestamp';

// TODO: Move this to comm folder
interface DronePosition {
    droneId: string,
    position: [number, number, number],
};

interface DroneOrientation {
    droneId: string,
    orientation: [number, number, number],
};

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');

        const maxMapPoints = 1_000_000;
        const maxDronePositions = 1_000;

        let container: HTMLDivElement;

        let scene: THREE.Scene;
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        let stats: Stats;

        let droneIds: Map<string, number> = new Map();

        let mapPoints: THREE.Points;
        let pointCount = 0;
        let dronePoints: THREE.Points;
        let droneIndexes = 0;

        function onWindowResize() {
            camera.aspect = container.clientWidth / container.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(container.clientWidth, container.clientHeight);
        }

        function init() {
            container = document.getElementById('map-container')! as HTMLDivElement;

            // Scene
            scene = new THREE.Scene();

            // Camera
            const fov = 70;
            camera = new THREE.PerspectiveCamera(fov, container.clientWidth / container.clientHeight);
            camera.position.set(0, 10, -6);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            // Map points variables
            // Geometry - buffer to hold all point positions
            const mapGeometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxMapPoints * 3);
            mapGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            mapGeometry.setDrawRange(0, 0);

            // Material
            const mapMaterial = new THREE.PointsMaterial({ size: 0.5, color: 0x00ff00 });

            // Points
            mapPoints = new THREE.Points(mapGeometry, mapMaterial);
            scene.add(mapPoints);

            // Drone positions variables
            // Geometry - buffer to hold all point positions
            const droneGeometry = new THREE.BufferGeometry();
            const dronePositions = new Float32Array(maxDronePositions * 3);
            droneGeometry.setAttribute('position', new THREE.BufferAttribute(dronePositions, 3));
            droneGeometry.setDrawRange(0, 0);

            // Material
            const droneMaterial = new THREE.PointsMaterial({ size: 0.5, color: 0xff0000 });

            // Points
            dronePoints = new THREE.Points(droneGeometry, droneMaterial);
            scene.add(dronePoints);

            // Renderer
            renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
            renderer.setSize(container.clientWidth, container.clientHeight);
            container.append(renderer.domElement);

            // Controls
            controls = new OrbitControls(camera, renderer.domElement);
            controls.enableDamping = true;

            // Helpers
            const axesHelper = new THREE.AxesHelper(2);
            scene.add(axesHelper);
            const gridHelper = new THREE.GridHelper(16, 16);
            scene.add(gridHelper);

            // Stats
            stats = Stats();
            stats.dom.style.position = 'absolute';
            container.append(stats.dom);

            // Resize canvas on window resize
            window.addEventListener('resize', onWindowResize);
        }

        function animate() {
            requestAnimationFrame(animate);
            controls.update();
            renderer.render(scene, camera);

            stats.update();
        }

        function addPoint(point: [number, number, number]) {
            mapPoints.geometry.attributes.position.setXYZ(pointCount, ...point);
            pointCount++;

            mapPoints.geometry.setDrawRange(0, pointCount);
            mapPoints.geometry.attributes.position.needsUpdate = true;
        }

        function setDronePosition(droneId: string, position: [number, number, number]) {
            if (!droneIds.get(droneId)) {
                droneIds.set(droneId, droneIndexes);
                dronePoints.geometry.attributes.position.setXYZ(droneIds.get(droneId)!, ...position);
                droneIndexes++;
                dronePoints.geometry.setDrawRange(0, droneIndexes);
            } else {
                dronePoints.geometry.attributes.position.setXYZ(droneIds.get(droneId)!, ...position);
            }
            dronePoints.geometry.attributes.position.needsUpdate = true;
        }

        socketClient!.bindMessage('map-points', (mapPoints: [number, number, number][]) => {
            for (const point of mapPoints) {
                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addPoint([point[1], point[2], point[0]]);
            }
        });

        socketClient!.bindMessage('drone-orientation', (droneOrientation: DroneOrientation) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            // TODO
        });

        socketClient!.bindMessage('drone-position', (dronePosition: DronePosition) => {
            // Change point coordinates to match three.js coordinate system
            // X: Right, Y: Up, Z: Out (towards user)
            setDronePosition(dronePosition.droneId, [dronePosition.position[1], dronePosition.position[2], dronePosition.position[0]]);
        });

        socketClient!.bindMessage('clear-map', () => {
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
            saveAsImage,
        };
    },
});
</script>

<style scoped lang="scss">
.map {
    width: 100%;
}

#map-container {
    height: 300px;
    position: relative;
}

.button {
    margin-top: 16px;
}
</style>
