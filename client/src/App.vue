<template>
    <Drone :battery="batteryLevel" @changeLedStatus="changeLedStatus" />
</template>

<script lang="ts">
import { defineComponent, onUnmounted, ref } from 'vue';
import Drone from './components/Drone.vue';

const ipAddress = 'ws:localhost';
const port      = '5678';
const serverUrl = ipAddress + ':' + port;

export default defineComponent({
    name: 'App',
    components: {
        Drone
    },
    setup() {
        const batteryLevel = ref(0);
        const socket = new WebSocket(serverUrl);

        socket.onopen = (event: Event) => {
            console.log('Connection successful');
        }

        socket.onmessage = (event: MessageEvent) => {
            batteryLevel.value = +event.data;
        }

        socket.onclose = (event: CloseEvent) => {
            console.log('Connection closed');
        }

        function changeLedStatus(isLedOn: boolean) {
            socket.send(`${isLedOn}`);
        }

        onUnmounted(() => {
            socket.close();
        });

        return {
            batteryLevel,
            socket,
            changeLedStatus
        };
    },
});

</script>
