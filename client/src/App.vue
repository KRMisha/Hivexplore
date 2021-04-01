<template>
    <div class="p-grid">
        <div class="p-col-12 p-d-flex p-jc-center p-ai-center p-shadow-2 topbar">Hivexplore</div>
        <div class="p-col-12 p-lg-6 p-xl-5 p-order-2 p-order-lg-1">
            <Control />
        </div>
        <div class="p-col-12 p-lg-6 p-xl-7 p-order-1 p-order-lg-2">
            <Map />
        </div>
        <div class="p-col-12 p-order-3">
            <DroneList />
        </div>
        <div class="p-col-12 p-order-4">
            <Log />
        </div>
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
// TODO: Investigate issue with non-null assertions for injections
</script>

<style lang="scss" scoped>
.topbar {
    height: 64px;
    color: var(--primary-color-text);
    background-color: var(--primary-color);
    font-size: 1.5rem;
    font-weight: 500;
}
</style>
