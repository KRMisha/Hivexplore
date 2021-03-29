<template>
    <Toast />
    <ConfirmPopup />

    <div class="container">
        <Map class="map-container" />
        <div class="button-container">
            <Button
                label="Start mission"
                class="left-button"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Standby"
                @click="onStartMissionButtonClick($event)"
            />
            <Button
                label="Return to base"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Exploring"
                @click="onReturnToBaseButtonClick()"
            />
            <Button
                :label="endMissionButtonLabel"
                class="right-button"
                :style="{ 'background-color': endMissionButtonColor }"
                :disabled="droneIds.length === 0 || missionState === MissionState.Standby || missionState === MissionState.Emergency"
                @click="onEndMissionButtonClick($event)"
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
import { computed, defineComponent, onUnmounted, provide, ref } from 'vue';
import { useConfirm } from 'primevue/useconfirm';
import { useToast } from 'primevue/usetoast';
import Drone from '@/components/Drone.vue';
import Log from '@/components/Log.vue';
import Map from '@/components/Map.vue';
import { SocketClient } from '@/classes/socket-client';
import { MissionState } from '@/enums/mission-state';
import { SocketEvent } from '@/enums/socket-event';

export default defineComponent({
    name: 'App',
    components: {
        Drone,
        Log,
        Map,
    },
    setup() {
        const confirm = useConfirm();
        const toast = useToast();
        const socketClient = new SocketClient();
        const droneIds = ref<string[]>([]);
        let wasEmergencyLandingCalled = false;

        socketClient.bindMessage(SocketEvent.DroneIds, (newDroneIds: string[]) => {
            droneIds.value = newDroneIds;
        });

        const missionState = ref(MissionState.Standby);
        socketClient.bindMessage(SocketEvent.MissionState, (newMissionState: MissionState) => {
            missionState.value = newMissionState;
        });

        function setMissionState(missionState: MissionState) {
            socketClient.sendMessage(SocketEvent.MissionState, missionState);
        }

        function onStartMissionButtonClick(event: Event) {
            if (wasEmergencyLandingCalled) {
                // If last mission ended with an emergency landing
                confirm.require({
                    target: event.currentTarget!,
                    message: 'The last mission was forcefully ended. Are you sure you want to start a new mission?',
                    header: 'Confirmation',
                    icon: 'pi pi-exclamation-triangle',
                    accept: () => {
                        toast.add({ severity: 'success', summary: 'Initiated', detail: 'Start mission initiated', life: 3000 });
                        toast.add({ severity: 'success', summary: 'ALATAK', detail: 'ALATAK🚀🚀', life: 3000 });
                        setMissionState(MissionState.Exploring);
                        wasEmergencyLandingCalled = false; // Reset
                    },
                });
            } else {
                // If last mission ended normally
                toast.add({ severity: 'success', summary: 'Initiated', detail: 'Start mission initiated', life: 3000 });
                toast.add({ severity: 'success', summary: 'ALATAK', detail: 'ALATAK', life: 3000 });
                setMissionState(MissionState.Exploring);
            }
        }

        function onReturnToBaseButtonClick() {
            toast.add({ severity: 'success', summary: 'Initiated', detail: 'Return to base initiated', life: 3000 });
            setMissionState(MissionState.Returning);
        }

        const endMissionButtonLabel = computed((): string => {
            return missionState.value === MissionState.Landed ? 'End mission' : 'Emergency land';
        });

        const endMissionButtonColor = computed((): string => {
            return missionState.value === MissionState.Landed ? 'var(--primary-color)' : 'red';
        });

        function onEndMissionButtonClick(event: Event) {
            if (missionState.value !== MissionState.Landed) {
                // Emergency land
                confirm.require({
                    target: event.currentTarget!,
                    message: 'Are you sure you want to initiate an *emergency* landing?',
                    header: 'Confirmation',
                    icon: 'pi pi-exclamation-triangle',
                    acceptClass: 'p-button-danger',
                    accept: () => {
                        toast.add({ severity: 'success', summary: 'Initiated', detail: 'Emergency landing initiated', life: 3000 });
                        setMissionState(MissionState.Emergency);
                        wasEmergencyLandingCalled = true;
                    },
                });
            } else {
                // End mission
                setMissionState(MissionState.Standby);
            }
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
            onStartMissionButtonClick,
            onReturnToBaseButtonClick,
            endMissionButtonLabel,
            endMissionButtonColor,
            onEndMissionButtonClick,
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
