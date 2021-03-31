<template>
    <ul v-if="droneIds.length > 0">
        <li v-for="droneId in droneIds" :key="droneId">
            <Drone :droneId="droneId" />
        </li>
    </ul>
    <div v-else>
        ✂ No drones connected ✂
    </div>
</template>

<script lang="ts">
import { defineComponent, inject, ref } from 'vue';
import Drone from '@/components/Drone.vue';
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'DroneList',
    components: {
        Drone,
    },
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');

        const droneIds = ref<string[]>([]);
        socketClient!.bindMessage('drone-ids', (newDroneIds: string[]) => {
            droneIds.value = newDroneIds;
        });

        return {
            droneIds,
        };
    },
});
// TODO: Remove ul for for loop
// TODO: Nicer UI when no drones are connected
</script>

<style lang="scss" scoped>
ul {
    width: 100%;
    list-style-type: none;
}
</style>
