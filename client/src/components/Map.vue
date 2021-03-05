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
import SocketClient from '@/classes/socket-client';

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');

        const maxPoints = 1_000_000;

        let camera: THREE.PerspectiveCamera;
        let scene: THREE.Scene;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        let stats: Stats;

        let container: HTMLDivElement;

        let points: THREE.Points;
        let pointCount = 0;

        function onWindowResize() {
            camera.aspect = container.clientWidth / container.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(container.clientWidth, container.clientHeight);
        }

        function init() {
            container = document.getElementById('map-container')! as HTMLDivElement;

            const fov = 70;
            camera = new THREE.PerspectiveCamera(fov, container.clientWidth / container.clientHeight);
            camera.position.set(0, 200, 400);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            scene = new THREE.Scene();

            const geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxPoints * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.setDrawRange(0, 0);

            const material = new THREE.PointsMaterial({ size: 5, color: 0x00ff00 });

            points = new THREE.Points(geometry, material);
            scene.add(points);

            renderer = new THREE.WebGLRenderer({ antialias: true });
            renderer.setSize(container.clientWidth, container.clientHeight);
            container.append(renderer.domElement);

            controls = new OrbitControls(camera, renderer.domElement);
            controls.enableDamping = true;

            stats = Stats();
            stats.dom.style.position = 'absolute';
            container.append(stats.dom);

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
                addPoint(point.map(x => x * coordinateScaleFactor) as [number, number, number]);
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
