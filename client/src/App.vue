<template>
    <div class="container">
        <Map class="map-container" />
        <div class="button-container">
            <Button
                label="Start mission"
                class="left-button"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Standby"
                @click="setMissionState(MissionState.Exploring)"
            />
            <Button
                label="Return to base"
                class="p-button-info"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Exploring"
                @click="setMissionState(MissionState.Returning)"
            />
        </div>
        <Timeline :value="missionStates" layout="horizontal" align="bottom" class="timeline">
            <template #marker="stateProps">
                <div class="p-timeline-event-marker" :class="{ 'selected-marker': stateProps.item === missionState }"></div>
            </template>
            <template #content="stateProps">
                <div :class="{ 'selected-content': stateProps.item === missionState }">{{ stateProps.item }}</div>
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
        <Log />
    </div>
</template>

<script lang="ts">
import { defineComponent, onUnmounted, provide, ref } from 'vue';
import Drone from '@/components/Drone.vue';
import Log from '@/components/Log.vue';
import Map from '@/components/Map.vue';
import { SocketClient } from '@/classes/socket-client';
import { MissionState } from '@/enums/mission-state';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
        Log,
        Map,
    },
    setup() {
        const socketClient = new SocketClient();
        const droneIds = ref<string[]>([]);

        socketClient.bindMessage('drone-ids', (newDroneIds: string[]) => {
            droneIds.value = newDroneIds;
        });

        const missionState = ref(MissionState.Standby);
        socketClient.bindMessage('mission-state', (newMissionState: MissionState) => {
            missionState.value = newMissionState;
        });

        function setMissionState(missionState: MissionState) {
            socketClient.sendMessage('mission-state', missionState);
        }

        provide('socketClient', socketClient);

        onUnmounted(() => {
            socketClient.close();
        });

        return {
            droneIds,
            missionState,
            missionStates: Object.values(MissionState),
            MissionState,
            setMissionState,
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

.button-container {
    padding-top: 16px;
    padding-bottom: 16px;
}

.left-button {
    margin-right: 16px;
}

.timeline {
    width: 50%;
    margin-left: 42px;
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
