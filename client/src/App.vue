<template>
    <Drone :battery="batteryLevel" />
</template>

<script lang="ts">
import { defineComponent, onUnmounted, ref } from 'vue';
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
        const batteryLevel = ref(0);
        const socket = new SocketClient(serverIpAddress, serverPort);

        provide('socket', socket);

        socket.bind('battery-level', (message: any) => {
            batteryLevel.value = message.data;
        });

        onUnmounted(() => {
            socket.close();
        });

        return {
            batteryLevel,
            socket,
        };
    },
});
</script>
