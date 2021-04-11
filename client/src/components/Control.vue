<template>
    <div class="stretched">
        <ConfirmPopup />
        <Panel header="Mission control" class="stretched">
            <div class="p-grid p-m-0 p-ai-stretch stretched">
                <div v-if="warningMessage !== ''" class="p-col-12 p-mb-4 p-p-0">
                    <InlineMessage severity="info" class="p-p-2 warning-message">{{ warningMessage }}</InlineMessage>
                </div>

                <div class="p-col p-p-0 p-d-flex p-flex-column p-jc-evenly">
                    <div class="p-d-flex p-flex-column">
                        <Button
                            label="Start mission"
                            icon="pi pi-send"
                            class="p-my-1"
                            :disabled="isStartMissionButtonDisabled"
                            @click="onStartMissionButtonClick($event)"
                        />
                        <Button
                            label="Return to base"
                            icon="pi pi-home"
                            class="p-my-1"
                            :disabled="isReturnToBaseButtonDisabled"
                            @click="setMissionState(MissionState.Returning)"
                        />
                        <Button
                            :label="endMissionButtonLabel"
                            :icon="endMissionButtonIcon"
                            class="p-my-1"
                            :class="endMissionButtonClass"
                            :disabled="isEndMissionButtonDisabled"
                            @click="onEndMissionButtonClick($event)"
                        />
                    </div>
                    <div class="p-d-flex p-jc-between p-ai-center p-mt-4">
                        <span class="label">Drone count</span>
                        <Chip :label="droneCount.toString()" :class="droneCountChipClass" />
                    </div>
                </div>
                <div class="p-col p-p-0 p-d-flex p-flex-column p-jc-center">
                    <Timeline :value="missionStates">
                        <template #marker="slotProps">
                            <div class="p-timeline-event-marker" :class="timelineMarkerClass(slotProps.item)"></div>
                        </template>
                        <template #content="slotProps">
                            <div :class="timelineContentClass(slotProps.item)">{{ slotProps.item }}</div>
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
import { WebSocketClient } from '@/communication/web-socket-client';
import { WebSocketEvent } from '@/communication/web-socket-event';
import { MissionState } from '@/enums/mission-state';

export default defineComponent({
    name: 'Control',
    setup() {
        const confirm = useConfirm();
        const webSocketClient = inject('webSocketClient') as WebSocketClient;

        const missionState = ref(MissionState.Standby);

        webSocketClient.bindMessage(WebSocketEvent.MissionState, (newMissionState: MissionState) => {
            missionState.value = newMissionState;
        });

        function setMissionState(missionState: MissionState) {
            webSocketClient.sendMessage(WebSocketEvent.MissionState, missionState);
        }

        const missionStates = computed(() => {
            const states = Object.values(MissionState);
            const stateToRemove = missionState.value !== MissionState.Emergency ? MissionState.Emergency : MissionState.Returning;
            const index = states.indexOf(stateToRemove);
            if (index !== -1) {
                states.splice(index, 1);
            }
            return states;
        });

        const droneCount = ref(0);
        webSocketClient.bindMessage(WebSocketEvent.DroneIds, (newDroneIds: string[]) => {
            droneCount.value = newDroneIds.length;
        });

        const areAllDronesCharged = ref(false);
        webSocketClient.bindMessage(WebSocketEvent.AreAllDronesCharged, (newAreAllDronesCharged: boolean) => {
            areAllDronesCharged.value = newAreAllDronesCharged;
        });

        const warningMessage = computed(() => {
            if (!webSocketClient.isConnected) {
                return 'The server is not connected';
            }

            if (missionState.value !== MissionState.Standby) {
                return '';
            }

            if (droneCount.value === 0) {
                return 'Connect at least one drone before the mission can start';
            }

            if (!areAllDronesCharged.value) {
                return 'All drones must have at least 30% battery before the mission can start';
            }

            return '';
        });

        const isStartMissionButtonDisabled = computed(() => {
            return (
                !webSocketClient.isConnected ||
                droneCount.value === 0 ||
                !areAllDronesCharged.value ||
                missionState.value !== MissionState.Standby
            );
        });

        let wasEmergencyLandingCalled = false;

        function onStartMissionButtonClick(event: Event) {
            if (wasEmergencyLandingCalled) {
                // If last mission ended with an emergency landing
                confirm.require({
                    target: event.currentTarget!,
                    message: 'The last mission was forcefully ended. Are you sure you want to start a new mission?',
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

        const isReturnToBaseButtonDisabled = computed(() => {
            return !webSocketClient.isConnected || droneCount.value === 0 || missionState.value !== MissionState.Exploring;
        });

        const endMissionButtonLabel = computed(() => {
            return missionState.value === MissionState.Landed ? 'End mission' : 'Emergency land';
        });

        const endMissionButtonIcon = computed(() => {
            return missionState.value === MissionState.Landed ? 'pi pi-replay' : 'pi pi-exclamation-circle';
        });

        const endMissionButtonClass = computed(() => {
            return {
                'p-button-danger': missionState.value !== MissionState.Landed,
            };
        });

        const isEndMissionButtonDisabled = computed(() => {
            return (
                !webSocketClient.isConnected ||
                droneCount.value === 0 ||
                missionState.value === MissionState.Standby ||
                missionState.value === MissionState.Emergency
            );
        });

        function onEndMissionButtonClick(event: Event) {
            if (missionState.value !== MissionState.Landed) {
                // Emergency land
                confirm.require({
                    target: event.currentTarget!,
                    message: 'Are you sure you want to initiate an emergency landing?',
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

        const droneCountChipClass = computed(() => {
            return { 'colored-chip': droneCount.value > 0 };
        });

        function timelineMarkerClass(timelineMissionState: MissionState) {
            return { 'selected-marker': timelineMissionState === missionState.value };
        }

        function timelineContentClass(timelineMissionState: MissionState) {
            return {
                'selected-content': timelineMissionState === missionState.value,
                'p-error': timelineMissionState === MissionState.Emergency,
            };
        }

        return {
            MissionState,
            missionState,
            missionStates,
            setMissionState,
            droneCount,
            warningMessage,
            isStartMissionButtonDisabled,
            onStartMissionButtonClick,
            isReturnToBaseButtonDisabled,
            endMissionButtonLabel,
            endMissionButtonIcon,
            endMissionButtonClass,
            isEndMissionButtonDisabled,
            onEndMissionButtonClick,
            droneCountChipClass,
            timelineMarkerClass,
            timelineContentClass,
        };
    },
});
</script>

<style lang="scss" scoped>
.stretched {
    height: 100%;
}

::v-deep(.p-panel) {
    display: flex;
    flex-direction: column;

    .p-panel-header {
        min-height: 72px;
    }

    .p-toggleable-content {
        height: 100%;
        .p-panel-content {
            height: 100%;
        }
    }
}

div::v-deep(.p-inline-message) {
    color: var(--primary-color-text);
    background-color: var(--primary-color);

    .p-inline-message-icon {
        color: var(--primary-color-text);
    }
}

.warning-message {
    width: 100%;
    margin-top: -0.5rem;
    border-radius: 0.875rem;
}

.label {
    font-weight: 500;
}

.colored-chip {
    color: var(--primary-color-text);
    background-color: var(--primary-color);
}

.p-timeline {
    flex-grow: 0;
}

@media (max-width: 575px), (min-width: 992px) and (max-width: 1280px) {
    .p-timeline::v-deep(.p-timeline-event) {
        .p-timeline-event-opposite {
            flex-grow: 2;
            padding-right: 0;
        }

        .p-timeline-event-content {
            flex-grow: 5;
            padding-right: 0;
        }
    }
}

@media (max-width: 400px) {
    .p-component {
        font-size: 0.875rem;
    }
}

.p-timeline-event-marker.selected-marker {
    background-color: var(--primary-color);
}

.selected-content {
    font-weight: bold;
}
</style>
