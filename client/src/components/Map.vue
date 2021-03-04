<template>
    <Panel header="Map">
        <div id="map-container"></div>
    </Panel>
</template>

<script lang="ts">
import { defineComponent, onMounted, onUnmounted } from 'vue';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import Stats from 'three/examples/jsm/libs/stats.module';

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const maxPoints = 1_000_000;

        let camera: THREE.PerspectiveCamera;
        let scene: THREE.Scene;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        let stats: Stats;

        let container: HTMLDivElement;

        let points: THREE.Points;
        let positionCount = 0;

        let intervalId: number | undefined; // TODO: Remove

        function init() {
            container = document.getElementById('map-container')! as HTMLDivElement;

            camera = new THREE.PerspectiveCamera(70, container.clientWidth / container.clientHeight, 0.1, 2000);
            camera.position.set(0, 200, 400);
            camera.lookAt(new THREE.Vector3(0, 0, 0));

            scene = new THREE.Scene();

            const geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxPoints * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

            const material = new THREE.PointsMaterial({ size: 5, color: 0x00FF00 });

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

            // TODO: Remove
            intervalId = setInterval(() => {
                addPoint();
            }, 5);
        }

        function animate() {
            requestAnimationFrame(animate);
            controls.update();
            renderer.render(scene, camera);

            stats.update();
        }

        function onWindowResize() {
            camera.aspect = container.clientWidth / container.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(container.clientWidth, container.clientHeight);
        }

        // TODO: Make this take a 3D point fed from the server
        function addPoint() {
            // TODO: Remove
            if (positionCount >= maxPoints) {
                clearInterval(intervalId);
                intervalId = undefined;
            }

            // TODO: Generate more complex shape
            const x = (Math.random() - 0.5) * 300;
            const y = (Math.random() - 0.5) * 300;
            const z = (Math.random() - 0.5) * 300;

            points.geometry.attributes.position.setXYZ(positionCount, x, y, z);
            positionCount += 3;

            points.geometry.setDrawRange(0, positionCount)
            points.geometry.attributes.position.needsUpdate = true;
        }

        onMounted(() => {
            init();
            animate();
        });

        onUnmounted(() => {
            window.removeEventListener('resize', onWindowResize);
        })
    },
});
</script>

<style scoped lang="scss">
#map-container {
    height: 200px;
    position: relative;
}
</style>
