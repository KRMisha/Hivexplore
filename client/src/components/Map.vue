<template>
    <Panel header="Map">
        <div id="map-container"></div>
    </Panel>
</template>

<script lang="ts">
import { defineComponent, onMounted, onUnmounted } from 'vue';
import * as THREE from 'three';

// Source for three.js setup: https://stackoverflow.com/questions/47849626/import-and-use-three-js-library-in-vue-component

export default defineComponent({
    name: 'Map',
    setup() {
        let camera: THREE.PerspectiveCamera;
        let scene: THREE.Scene;
        let renderer: THREE.WebGLRenderer;
        let mesh: THREE.Mesh;

        let container: HTMLDivElement;

        function init() {
            container = document.getElementById('map-container')! as HTMLDivElement;

            camera = new THREE.PerspectiveCamera(70, container.clientWidth / container.clientHeight, 0.01, 10);
            camera.position.z = 1;

            scene = new THREE.Scene();

            const geometry = new THREE.BoxGeometry(0.2, 0.2, 0.2);
            const material = new THREE.MeshNormalMaterial();

            mesh = new THREE.Mesh(geometry, material);
            scene.add(mesh);

            renderer = new THREE.WebGLRenderer({ antialias: true });
            renderer.setSize(container.clientWidth, container.clientHeight);
            container.append(renderer.domElement);
        }

        function animate() {
            requestAnimationFrame(animate);
            mesh.rotation.x += 0.01;
            mesh.rotation.y += 0.02;
            renderer.render(scene, camera);
        }

        function onWindowResize() {
            camera.aspect = container.clientWidth / container.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize(container.clientWidth, container.clientHeight);
        }

        window.addEventListener('resize', onWindowResize);

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
}
</style>
