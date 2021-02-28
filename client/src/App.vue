<template>
    <li v-for="droneId in droneIds" :key="droneId">
        <Drone :droneId="droneId" />
    </li>
    <div v-if="droneIds.length === 0">
        ✂✂ No drones connected ✂✂
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted } from 'vue';
import Drone from './components/Drone.vue';
import SocketClient from './classes/socket-client';
import { provide, ref } from 'vue';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
    },
    setup() {
        const socket = new SocketClient();
        const droneIds = ref<Array<string>>([]);

        socket.bind('drone-ids', undefined, (newDroneIds: Array<string>) => {
            droneIds.value = newDroneIds;
        });

        provide('socket', socket);

        onUnmounted(() => {
            socket.close();
        });

        return {
            droneIds,
        };
    },
});
</script>
