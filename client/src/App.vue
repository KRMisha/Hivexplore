<template>
    <Toast />
    <ConfirmDialog />
    <ConfirmDialog group="positionDialog"></ConfirmDialog>

    <div class="container">
        <Map class="map-container" />
        <div class="button-container">
            <Button
                label="Start mission"
                class="left-button"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Standby"
                @click="startMissionButtonClick()"
            />
            <Button
                label="Return to base"
                :disabled="droneIds.length === 0 || missionState !== MissionState.Exploring"
                @click="returnToBaseButtonClick()"
            />
            <Button
                :label="endMissionButtonLabel"
                class="right-button"
                :style="{ 'background-color': endMissionButtonColor }"
                :disabled="droneIds.length === 0 || missionState === MissionState.Standby"
                @click="endMissionButtonClick()"
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
import { useConfirm } from "primevue/useconfirm";
import { useToast } from "primevue/usetoast";

export default defineComponent({
    name: 'App',
    components: {
        Drone,
        Map,
    },
    setup() {
        const confirm = useConfirm();
        const toast = useToast();
        const socketClient = new SocketClient();
        const droneIds = ref<string[]>([]);
        let emergencyLandWasCalled = false;

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

        function startMissionButtonClick() {
            // Emergency land was called use case
            if (emergencyLandWasCalled) {
                confirm.require({
                    message: 'Are you sure you want to START mission since an EMERGENCY land was called?',
                    header: 'Confirmation',
                    icon: 'pi pi-exclamation-triangle',
                    accept: () => {
                        toast.add({severity:'success', summary:'Initiated', detail:'Start mission initiated', life: 3000});
                        toast.add({severity:'success', summary:'ALATAK', detail:'ALATAK', life: 3000});
                        setMissionState(MissionState.Exploring);
                        emergencyLandWasCalled = false; // Reset
                    },
                    reject: () => {
                        toast.add({severity:'warn', summary:'Rejected', detail:'Start mission rejected', life: 3000});
                    }
                });
            // Normal use case
            } else {
                toast.add({severity:'success', summary:'Initiated', detail:'Start mission initiated', life: 3000});
                toast.add({severity:'success', summary:'ALATAK', detail:'ALATAK', life: 3000});
                setMissionState(MissionState.Exploring);
            }
        }

        function returnToBaseButtonClick() {
            toast.add({severity:'success', summary:'Initiated', detail:'Return to base initiated', life: 3000});
            setMissionState(MissionState.Returning);
        }

        const endMissionButtonLabel = computed((): string => {
            return (missionState.value === MissionState.Landed ? 'End mission' : 'Emergency land');
        });

        const endMissionButtonColor = computed((): string => {
            return (missionState.value === MissionState.Landed ? 'var(--primary-color)' : 'red');
        });

        function endMissionButtonClick() {
            // Emergency land
            if (missionState.value !== MissionState.Landed) {
                confirm.require({
                    message: 'Are you sure you want to initiate an EMERGENCY land?',
                    header: 'Confirmation',
                    icon: 'pi pi-exclamation-triangle',
                    accept: () => {
                        toast.add({severity:'success', summary:'Initiated', detail:'Emergency land initiated', life: 3000});
                        setMissionState(MissionState.Emergency);
                        emergencyLandWasCalled = true;
                    },
                    reject: () => {
                        toast.add({severity:'warn', summary:'Rejected', detail:'Emergency land rejected', life: 3000});
                    }
                });
            // End mission
            } else {
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
            startMissionButtonClick,
            returnToBaseButtonClick,
            endMissionButtonLabel,
            endMissionButtonColor,
            endMissionButtonClick,
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
