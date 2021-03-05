<template>
    <div class="container">
        <Map class="map-container" />
        <Timeline :value="missionStates" layout="horizontal" align="bottom" class="timeline">
            <template #marker="stateProps">
                <div class="p-timeline-event-marker" :class="{ 'selected-marker': stateProps.item.name === missionState }"></div>
            </template>
            <template #content="stateProps">
                <div :class="{ 'selected-content': stateProps.item.name === missionState }">{{ stateProps.item.name }}</div>
            </template>
        </Timeline>
        <ul v-if="droneIds.length > 0">
            <li v-for="droneId in droneIds" :key="droneId">
                <Drone :droneId="droneId" />
            </li>
        </ul>
        <div v-else>
            ✂ No drones connected ✂
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted } from 'vue';
import Drone from './components/Drone.vue';
import Map from './components/Map.vue';
import SocketClient from './classes/socket-client';
import { provide, ref } from 'vue';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
        Map,
    },
    setup() {
        const socket = new SocketClient();
        const droneIds = ref<Array<string>>([]);

        socket.bindMessage('drone-ids', (newDroneIds: Array<string>) => {
            droneIds.value = newDroneIds;
        });

        const missionState = ref('Standby');
        socket!.bindMessage('state', (newMissionState: string) => {
            missionState.value = newMissionState;
        });

        provide('socket', socket);

        onUnmounted(() => {
            socket.close();
        });

        return {
            droneIds,
            missionState,
            missionStates: [{ name: 'Standby' }, { name: 'Mission' }, { name: 'Returned' }],
        };
    },
});
</script>

<style scoped lang="scss">
.container {
    display: flex;
    flex-direction: column;
    align-items: center;
    width: 100%;
    padding-left: 32px;
    padding-right: 32px;
}

.map-container {
    width: 100%;
    max-width: 1200px;
}

.timeline {
    width: 50%;
    margin-left: 80px;
}

.p-timeline-event-marker.selected-marker {
    background-color: var(--primary-color);
}

.selected-content {
    font-weight: bold;
}

ul {
    width: 100%;
    list-style-type: none;
}
</style>
