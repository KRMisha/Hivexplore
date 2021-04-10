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
            <Logs />
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted, provide } from 'vue';
import { WebSocketClient } from '@/communication/web-socket-client';
import Control from '@/components/Control.vue';
import DroneList from '@/components/DroneList.vue';
import Logs from '@/components/Logs.vue';
import Map from '@/components/Map.vue';

export default defineComponent({
    name: 'App',
    components: {
        Control,
        DroneList,
        Logs,
        Map,
    },
    setup() {
        const webSocketClient = new WebSocketClient();
        provide('webSocketClient', webSocketClient);

        onUnmounted(() => {
            webSocketClient.close();
        });
    },
});
</script>

<style lang="scss" scoped>
.topbar {
    height: 3.5rem;
    color: var(--primary-color-text);
    background-color: var(--primary-color);
    font-size: 1.375rem;
    font-weight: 500;
}

::v-deep(.p-panel) .p-panel-header .p-panel-title {
    font-weight: 600;
}

::v-deep(.p-card) .p-card-title {
    font-weight: 500;
}
</style>
