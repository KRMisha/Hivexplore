<template>
    <!-- TODO: Loop through available drones and give them their proper IDs -->
    <Drone droneId="test" />
</template>

<script lang="ts">
import { defineComponent, onUnmounted } from 'vue';
import Drone from './components/Drone.vue';
import SocketClient from './classes/socket-client';
import { provide } from 'vue';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
    },
    setup() {
        const socket = new SocketClient();

        provide('socket', socket);

        onUnmounted(() => {
            socket.close();
        });
    },
});
</script>
