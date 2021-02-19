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
            const message = JSON.parse(event.data);
            switch (message.event) {
                case 'battery-level':
                    batteryLevel.value = message.data;
                    break;
                default:
                    console.warn(`Unknown socket event received: ${message.event}`);
                    break;
            }
        };

        socket.onclose = (event: CloseEvent) => {
            console.log('Connection closed');
        };

        function changeLedStatus(isLedOn: boolean) {
            // Convert date to local timezone by stripping the timezone offset
            const timestampUtc = new Date();
            const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

            const message = JSON.stringify({
                event: 'set-led',
                data: isLedOn,
                timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
            });
            socket.send(message);
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
