<template>
    <Drone :battery="batteryLevel" @changeLedStatus="changeLedStatus" />
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import Drone from './components/Drone.vue';

const ipAddress = 'ws://127.0.0.1';
const port      = '5678'
const serverUrl = ipAddress + ':' + port;

export default defineComponent({
    name: 'App',
    components: {
        Drone
    },
    setup() {
        const batteryLevel = ref(0);
        const connection = new WebSocket(serverUrl);

        connection.onopen = (event: Event) => {
            console.log('Connection sucessful');
        }

        connection.onmessage = (event: MessageEvent) => {
            console.log(event.data);
            batteryLevel.value = +event.data;
        }

        function changeLedStatus(isLedOn: boolean) {
            connection.send(`${isLedOn}`);
        }

        return {
            batteryLevel,
            connection,
            changeLedStatus
        };
    },
});

</script>
