<template>
    <ul v-if="droneIds.length > 0">
        <li v-for="droneId in droneIds" :key="droneId">
            <Drone :droneId="droneId" />
        </li>
    </ul>
    <div v-else>
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

        socket.bindMessage('drone-ids', (newDroneIds: Array<string>) => {
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
<style scoped lang="scss">
ul {
    list-style-type: none;
}
</style>
