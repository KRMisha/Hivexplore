<template>
    <Toast />
    <div class="p-grid p-flex-row-reverse">
        <div class="p-col-12 p-lg-6 p-xl-8">
            <Map />
        </div>
        <div class="p-col-12 p-lg-6 p-xl-4">
            <Control :hasDrones="droneIds.length > 0" />
        </div>
    </div>
    <div class="p-grid">
        <ul v-if="droneIds.length > 0">
            <li v-for="droneId in droneIds" :key="droneId">
                <Drone :droneId="droneId" />
            </li>
        </ul>
        <div v-else>
            ✂ No drones connected ✂
        </div>
        <Log />
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted, provide, ref } from 'vue';
import Control from '@/components/Control.vue';
import Drone from '@/components/Drone.vue';
import Log from '@/components/Log.vue';
import Map from '@/components/Map.vue';
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'App',
    components: {
        Control,
        Drone,
        Log,
        Map,
    },
    setup() {
        const socketClient = new SocketClient();
        provide('socketClient', socketClient);

        const droneIds = ref<string[]>([]);
        socketClient.bindMessage('drone-ids', (newDroneIds: string[]) => {
            droneIds.value = newDroneIds;
        });

        onUnmounted(() => {
            socketClient.close();
        });

        return {
            droneIds,
        };
    },
});
// TODO: Top bar
// TODO: Semantic grouping inside all setup() functions
// TODO: Nicer UI when no drones are connected
// TODO: Improve map background color
</script>

<style lang="scss" scoped>
ul {
    width: 100%;
    list-style-type: none;
}
</style>
