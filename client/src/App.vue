<template>
    <div class="p-component p-grid p-jc-center p-ai-center p-mb-2 p-shadow-2 topbar">Hivexplore</div>
    <div class="p-grid p-flex-row-reverse">
        <div class="p-col-12 p-lg-6 p-xl-7">
            <Map />
        </div>
        <div class="p-col-12 p-lg-6 p-xl-5">
            <Control />
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
// TODO: Semantic grouping inside all setup() functions
// TODO: Nicer UI when no drones are connected
// TODO: Improve map background color
// TODO: Check tab order potential issue caused by p-flex-row-reverse
</script>

<style lang="scss" scoped>
.p-component.topbar {
    height: 64px;
    color: var(--primary-color-text);
    background-color: var(--primary-color);
    font-size: 1.5rem;
    font-weight: 500;
}

ul {
    width: 100%;
    list-style-type: none;
}
</style>
