<template>
    <Panel :header="sortedDroneIds.length > 0 ? 'Drones' : ''">
        <Carousel
            v-if="sortedDroneIds.length > 0"
            :value="sortedDroneIds"
            :numVisible="4"
            :numScroll="4"
            :responsiveOptions="responsiveOptions"
        >
            <template #item="slotProps">
                <Drone :droneId="slotProps.data" :key="slotProps.data" />
            </template>
        </Carousel>
        <div v-else class="p-text-center p-pb-3 no-drones-text">
            No drones connected
        </div>
    </Panel>
</template>

<script lang="ts">
import { computed, defineComponent, inject, ref } from 'vue';
import Drone from '@/components/Drone.vue';
import { WebSocketClient } from '@/communication/web-socket-client';
import { WebSocketEvent } from '@/communication/web-socket-event';

export default defineComponent({
    name: 'DroneList',
    components: {
        Drone,
    },
    setup() {
        const webSocketClient = inject('webSocketClient') as WebSocketClient;

        const droneIds = ref<string[]>([]);
        webSocketClient.bindMessage(WebSocketEvent.DroneIds, (newDroneIds: string[]) => {
            droneIds.value = newDroneIds;
        });

        const sortedDroneIds = computed(() => {
            return [...droneIds.value].sort((first: string, second: string): number => first.localeCompare(second));
        });

        const responsiveOptions = [
            {
                breakpoint: '1900px',
                numVisible: 3,
                numScroll: 3,
            },
            {
                breakpoint: '1400px',
                numVisible: 2,
                numScroll: 2,
            },
            {
                breakpoint: '900px',
                numVisible: 1,
                numScroll: 1,
            },
        ];

        return {
            sortedDroneIds,
            responsiveOptions,
        };
    },
});
</script>

<style lang="scss" scoped>
.p-carousel::v-deep(.p-carousel-container) {
    @media (max-width: 575px) {
        .p-carousel-prev,
        .p-carousel-next {
            display: none;
        }
    }

    .p-carousel-item {
        display: flex;
        justify-content: center;
    }
}

.no-drones-text {
    font-size: 1.25rem;
    font-weight: 500;
}
</style>
