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

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        const containerRef = ref<HTMLElement | undefined>(undefined);

        let scene: THREE.Scene;
        let camera: THREE.PerspectiveCamera;
        let renderer: THREE.WebGLRenderer;
        let controls: OrbitControls;

        const maxPoints = 1_000_000;
        let points: THREE.Points;
        let pointCount = 0;

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

            // Geometry - buffer to hold all point positions
            const geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(maxPoints * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.setDrawRange(0, 0);

            // Material
            const material = new THREE.PointsMaterial({ size: 0.5, color: 0xfbc02d });

            // Points
            points = new THREE.Points(geometry, material);
            scene.add(points);

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

        function addPoint(point: [number, number, number]) {
            points.geometry.attributes.position.setXYZ(pointCount, ...point);
            pointCount++;

            points.geometry.setDrawRange(0, pointCount);
            points.geometry.attributes.position.needsUpdate = true;
        }

        socketClient.bindMessage('map-points', (points: [number, number, number][]) => {
            for (const point of points) {
                // Change point coordinates to match three.js coordinate system
                // X: Right, Y: Up, Z: Out (towards user)
                addPoint([point[1], point[2], point[0]]);
            }
        });

        socketClient.bindMessage('clear-map', () => {
            pointCount = 0;
            points.geometry.setDrawRange(0, pointCount);
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
