<template>
    <Drone />
</template>

<script lang="ts">
import { defineComponent, onUnmounted } from 'vue';
import Drone from './components/Drone.vue';
import SocketClient from './classes/socket-client';
import { provide } from 'vue'

const serverIpAddress = 'ws:localhost';
const serverPort = '5678';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
    },
    setup() {
        const socket = new SocketClient(serverIpAddress, serverPort);

        provide('socket', socket);

        onUnmounted(() => {
            socket.close();
        });
    },
});
</script>
