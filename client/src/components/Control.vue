<template>
    <div class="stretched">
        <ConfirmPopup />
        <Panel header="Mission control" class="stretched">
            <div class="p-grid p-m-0 p-ai-stretch stretched">
                <div class="p-col p-p-0 p-d-flex p-flex-column p-jc-evenly">
                    <div class="p-d-flex p-flex-column">
                        <Button
                            label="Start mission"
                            icon="pi pi-send"
                            class="p-my-1"
                            :disabled="droneCount === 0 || missionState !== MissionState.Standby"
                            @click="onStartMissionButtonClick($event)"
                        />
                        <Button
                            label="Return to base"
                            icon="pi pi-home"
                            class="p-my-1"
                            :disabled="droneCount === 0 || missionState !== MissionState.Exploring"
                            @click="setMissionState(MissionState.Returning)"
                        />
                        <Button
                            :label="endMissionButtonLabel"
                            :icon="endMissionButtonIcon"
                            class="p-my-1"
                            :class="endMissionButtonClass"
                            :disabled="droneCount === 0 || missionState === MissionState.Standby || missionState === MissionState.Emergency"
                            @click="onEndMissionButtonClick($event)"
                        />
                    </div>
                    <div class="p-d-flex p-jc-between p-ai-center">
                        <span class="label">Drone count</span>
                        <Chip :label="droneCount.toString()" :class="{ 'colored-chip': droneCount > 0 }" />
                    </div>
                </div>
                <div class="p-col p-p-0 p-d-flex p-flex-column p-jc-center">
                    <Timeline :value="missionStates" class="timeline">
                        <template #marker="stateProps">
                            <div class="p-timeline-event-marker" :class="{ 'selected-marker': stateProps.item === missionState }"></div>
                        </template>
                        <template #content="stateProps">
                            <div
                                :class="{
                                    'selected-content': stateProps.item === missionState,
                                    'p-error': stateProps.item === MissionState.Emergency,
                                }"
                            >
                                {{ stateProps.item }}
                            </div>
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
    setup() {
        const confirm = useConfirm();
        const socketClient: SocketClient | undefined = inject('socketClient');

        const missionState = ref(MissionState.Standby);

        socketClient!.bindMessage('mission-state', (newMissionState: MissionState) => {
            missionState.value = newMissionState;
        });

        function setMissionState(missionState: MissionState) {
            socketClient!.sendMessage('mission-state', missionState);
        }

        const missionStates = computed((): MissionState[] => {
            const states = Object.values(MissionState);
            const stateToRemove = missionState.value !== MissionState.Emergency ? MissionState.Emergency : MissionState.Returning;
            const index = states.indexOf(stateToRemove);
            if (index !== -1) {
                states.splice(index, 1);
            }
            return states;
        });

        const droneCount = ref(0);
        socketClient!.bindMessage('drone-ids', (newDroneIds: string[]) => {
            droneCount.value = newDroneIds.length;
        });

        let wasEmergencyLandingCalled = false;

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
            MissionState,
            missionState,
            missionStates,
            setMissionState,
            droneCount,
            onStartMissionButtonClick,
            endMissionButtonLabel,
            endMissionButtonIcon,
            endMissionButtonClass,
            onEndMissionButtonClick,
        };
    },
});
// TODO: Simplify logic
// TODO: Fix colors
// TODO: Remove explicit type annotations for computed()
// TODO: Remove explicit return for computed
// TODO: Rename stateProps to slotProps
</script>

<style lang="scss" scoped>
.stretched {
    height: 100%;
}

div::v-deep(.p-panel) {
    display: flex;
    flex-direction: column;

    .p-panel-header {
        height: 72px;
    }

    .p-toggleable-content {
        height: 100%;
        .p-panel-content {
            height: 100%;
        }
    }
}

.label {
    font-weight: 500;
}

.colored-chip {
    color: var(--primary-color-text);
    background-color: var(--primary-color);
}

.timeline {
    flex-grow: 0;
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
