<template>
    <div class="stretched">
        <ConfirmPopup />
        <Panel header="Mission control" class="stretched">
            <div class="p-grid p-ai-stretch">
                <div class="p-col p-d-flex p-flex-column p-jc-around">
                    <div class="p-d-flex p-flex-column">
                        <Button
                            label="Start mission"
                            class="p-my-1"
                            :disabled="droneCount === 0 || missionState !== MissionState.Standby"
                            @click="onStartMissionButtonClick($event)"
                        />
                        <Button
                            label="Return to base"
                            class="p-my-1"
                            :disabled="droneCount === 0 || missionState !== MissionState.Exploring"
                            @click="setMissionState(MissionState.Returning)"
                        />
                        <Button
                            :label="endMissionButtonLabel"
                            class="p-my-1"
                            :style="{ 'background-color': endMissionButtonColor }"
                            :disabled="droneCount === 0 || missionState === MissionState.Standby || missionState === MissionState.Emergency"
                            @click="onEndMissionButtonClick($event)"
                        />
                    </div>
                    <div class="p-d-flex p-jc-between p-ai-center">
                        <span class="label">Drone count</span>
                        <Chip :label="droneCount.toString()" :class="{ 'colored-chip': droneCount > 0 }" />
                    </div>
                </div>
                <div class="p-col">
                    <Timeline :value="missionStates" class="timeline">
                        <template #marker="stateProps">
                            <div class="p-timeline-event-marker" :class="{ 'selected-marker': stateProps.item === missionState }"></div>
                        </template>
                        <template #content="stateProps">
                            <div :class="{ 'selected-content': stateProps.item === missionState }">{{ stateProps.item }}</div>
                        </template>
                    </Timeline>
                </div>
            </div>
        </Panel>
    </div>
</template>

<script lang="ts">
import { computed, defineComponent, inject, ref } from 'vue';
import { useConfirm } from 'primevue/useconfirm';
import { SocketClient } from '@/classes/socket-client';
import { MissionState } from '@/enums/mission-state';

export default defineComponent({
    name: 'Control',
    props: {
        droneCount: Number,
    },
    setup() {
        const confirm = useConfirm();
        const socketClient: SocketClient | undefined = inject('socketClient');

        let wasEmergencyLandingCalled = false;

        const missionState = ref(MissionState.Standby);
        socketClient!.bindMessage('mission-state', (newMissionState: MissionState) => {
            missionState.value = newMissionState;
        });

        function setMissionState(missionState: MissionState) {
            socketClient!.sendMessage('mission-state', missionState);
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
                        setMissionState(MissionState.Exploring);
                        wasEmergencyLandingCalled = false; // Reset
                    },
                });
            } else {
                // If last mission ended normally
                setMissionState(MissionState.Exploring);
            }
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
                        setMissionState(MissionState.Emergency);
                        wasEmergencyLandingCalled = true;
                    },
                });
            } else {
                // End mission
                setMissionState(MissionState.Standby);
            }
        }

        return {
            missionState,
            missionStates: Object.values(MissionState), // TODO: Use computed here to dynamically add/remove emergency
            MissionState,
            setMissionState,
            onStartMissionButtonClick,
            endMissionButtonLabel,
            endMissionButtonColor,
            onEndMissionButtonClick,
        };
    },
});
// TODO: Simplify logic
// TODO: Fix colors
</script>

<style lang="scss" scoped>
.stretched {
    height: 100%;
}

div::v-deep(.p-panel) {
    display: flex;
    flex-direction: column;

    .p-toggleable-content {
        height: 100%;
    }
}

.label {
    font-weight: 500;
}

.colored-chip {
    color: #ffffff;
    background-color: var(--primary-color);
}

@media (max-width: 576px), (min-width: 992px) and (max-width: 1280px) {
    .timeline {
        margin-left: -4.5rem;
    }
}

@media (max-width: 400px) {
    .timeline {
        font-size: 0.875rem;
    }
}

@media (max-width: 360px) {
    .p-component {
        font-size: 0.875rem;
    }

    .timeline {
        margin-left: -5rem;
        margin-right: -1rem;
    }
}

.p-timeline-event-marker.selected-marker {
    background-color: var(--primary-color);
}

.selected-content {
    font-weight: bold;
}
</style>
