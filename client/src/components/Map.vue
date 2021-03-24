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
import { getCurrentTimestamp } from '@/utils/format-date';
import { SocketClient } from '@/classes/socket-client';

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');

        const maxPoints = 1_000_000;

        let container: HTMLDivElement;

        let scene: THREE.Scene;
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        let stats: Stats;

        let points: THREE.Points;
        let pointCount = 0;

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

            // Geometry - buffer to hold all point positions
            const geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxPoints * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.setDrawRange(0, 0);

            // Material
            const material = new THREE.PointsMaterial({ size: 0.5, color: 0x00ff00 });

            // Points
            points = new THREE.Points(geometry, material);
            scene.add(points);

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
            points.geometry.attributes.position.setXYZ(pointCount, ...point);
            pointCount++;

            points.geometry.setDrawRange(0, pointCount);
            points.geometry.attributes.position.needsUpdate = true;
        }

        socketClient!.bindMessage('map-points', (points: [number, number, number][]) => {
            for (const point of points) {
                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addPoint([point[1], point[2], point[0]]);
            }
        });

        function saveAsImage() {
            // Convert date to local timezone by stripping the timezone offset
            const timestampUnfiltered = getCurrentTimestamp();
            const timestamp = timestampUnfiltered.toISOString().replace('Z', '').replaceAll(':', ''); // Remove the trailing Z since the timestamp is not in UTC
            const filename = `hivexplore_map_${timestamp}.png`;

            const url = renderer.domElement.toDataURL('image/png;base64');
            let link = document.createElement('a');

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
