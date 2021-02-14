<template>
    <Drone :battery="batteryLevel" @changeLedStatus="changeLedStatus" />
</template>

<script lang="ts">
import { defineComponent, onUnmounted, ref } from 'vue';
import Drone from './components/Drone.vue';

const serverIpAddress = 'ws:localhost';
const serverPort = '5678';
const serverUrl = serverIpAddress + ':' + serverPort;

export default defineComponent({
    name: 'App',
    components: {
        Drone,
    },
    setup() {
        const batteryLevel = ref(0);
        const socket = new WebSocket(serverUrl);

        socket.onopen = (event: Event) => {
            console.log('Connection successful');
        };

        socket.onmessage = (event: MessageEvent) => {
            batteryLevel.value = +event.data;
        };

        socket.onclose = (event: CloseEvent) => {
            console.log('Connection closed');
        };

        function changeLedStatus(isLedOn: boolean) {
            socket.send(`${isLedOn}`);
        }

        onUnmounted(() => {
            socket.close();
        });

        return {
            batteryLevel,
            socket,
            changeLedStatus,
        };
    },
});
</script>
