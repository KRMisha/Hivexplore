<template>
    <Drone :battery="batteryLevel" @changeLedStatus="changeLedStatus" />
</template>

<script lang="ts">
import { defineComponent, onUnmounted, ref } from 'vue';
import Drone from './components/Drone.vue';
import SocketClient from './classes/socket-client';

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

        socket.bind('battery-level', (message: any) => {
            batteryLevel.value = message.data;
        });

        function changeLedStatus(isLedOn: boolean) {
            // Convert date to local timezone by stripping the timezone offset
            const timestampUtc = new Date();
            const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

            const message = {
                data: isLedOn,
                timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
            };
            socket.send('set-led', message);
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
