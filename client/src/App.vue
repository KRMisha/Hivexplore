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
                :disabled="droneIds.length === 0 || missionState !== MissionState.Exploring"
                @click="setMissionState(MissionState.Returning)"
            />
            <Button
                :label="endMissionButtonLabel"
                class="right-button"
                :style="{ 'background-color': endMissionButtonColor }"
                :disabled="droneIds.length === 0 || missionState === MissionState.Standby"
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
    </div>
</template>

<script lang="ts">
import { computed, defineComponent, onUnmounted, provide, ref } from 'vue';
import Drone from '@/components/Drone.vue';
import Map from '@/components/Map.vue';
import { SocketClient } from '@/classes/socket-client';
import { MissionState } from '@/enums/mission-state';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
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

        const endMissionButtonLabel = computed((): string => {
            return (missionState.value === MissionState.Landed ? 'End mission' : 'Emergency land');
        });

        const endMissionButtonColor = computed((): string => {
            return (missionState.value === MissionState.Landed ? 'var(--primary-color)' : 'red');
        });

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
            endMissionButtonLabel,
            endMissionButtonColor,
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

.right-button {
    margin-left: 16px;
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
