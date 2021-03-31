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
        <DroneList />
        <Log />
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted, provide } from 'vue';
import Control from '@/components/Control.vue';
import DroneList from '@/components/DroneList.vue';
import Log from '@/components/Log.vue';
import Map from '@/components/Map.vue';
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'App',
    components: {
        Control,
        DroneList,
        Log,
        Map,
    },
    setup() {
        const socketClient = new SocketClient();
        provide('socketClient', socketClient);

        onUnmounted(() => {
            socketClient.close();
        });
    },
});
// TODO: Semantic grouping inside all setup() functions
// TODO: Improve map background color
// TODO: Check tab order potential issue caused by p-flex-row-reverse
// TODO: Investigate issue with non-null assertions for injections
// TODO: Investigate using only one grid for main UI layout
</script>

<style lang="scss" scoped>
.p-component.topbar {
    height: 64px;
    color: var(--primary-color-text);
    background-color: var(--primary-color);
    font-size: 1.5rem;
    font-weight: 500;
}
</style>
