<template>
    <Panel header="Map">
        <div id="map-container"></div>
    </Panel>
</template>

<script lang="ts">
import { defineComponent, inject, onMounted, onUnmounted } from 'vue';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import Stats from 'three/examples/jsm/libs/stats.module';
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
            camera.position.set(0, 100, 60);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            // Geometry - buffer to hold all point positions
            const geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxPoints * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.setDrawRange(0, 0);

            // Material
            const material = new THREE.PointsMaterial({ size: 5, color: 0x00ff00 });

            // Points
            points = new THREE.Points(geometry, material);
            scene.add(points);

            // Renderer
            renderer = new THREE.WebGLRenderer({ antialias: true });
            renderer.setSize(container.clientWidth, container.clientHeight);
            container.append(renderer.domElement);

            // Controls
            controls = new OrbitControls(camera, renderer.domElement);
            controls.enableDamping = true;

            // Helpers
            const axesHelper = new THREE.AxesHelper(25);
            scene.add(axesHelper);
            const gridHelper = new THREE.GridHelper(125, 10);
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
                const coordinateScaleFactor = 10;
                const scaledPoint = point.map(x => x * coordinateScaleFactor);

                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addPoint([scaledPoint[0], scaledPoint[2], scaledPoint[1]]);
            }
        });

        onMounted(() => {
            init();
            animate();
        });

        onUnmounted(() => {
            window.removeEventListener('resize', onWindowResize);
        });
    },
});
</script>

<style scoped lang="scss">
#map-container {
    height: 300px;
    position: relative;
}
</style>
