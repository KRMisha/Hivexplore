<template>
    <Panel header="Drones">
        <Carousel v-if="droneIds.length > 0" :value="droneIds" :numVisible="4" :numScroll="4" :responsiveOptions="responsiveOptions">
            <template #item="slotProps">
                <Drone :droneId="slotProps.data" />
            </template>
        </Carousel>
        <div v-else>
            ✂️ No drones connected ✂️
        </div>
    </Panel>
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
            droneIds,
            responsiveOptions,
        };
    },
});
// TODO: Nicer UI when no drones are connected
// TODO: Order drones
</script>

<style lang="scss" scoped>
div::v-deep(.p-carousel-container) {
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
</style>
