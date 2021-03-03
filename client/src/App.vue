<template>
    <div class="container">
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

        const missionState = ref('Standby'); // TODO: Send message on server
        socket!.bindMessage('state', (newCurrentMissionState: string) => {
            missionState.value = newCurrentMissionState;
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
    padding-left: 32px;
    padding-right: 32px;
}

ul {
    width: 100%;
    list-style-type: none;
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
</style>
