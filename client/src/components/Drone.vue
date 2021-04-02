<template>
    <Card class="card">
        <template #title>
            <div class="p-d-flex p-jc-center title">Drone {{ droneId }}</div>
        </template>
        <template #content>
            <div class="p-grid">
                <div class="p-col p-d-flex p-flex-column p-ai-center">
                    <span class="p-my-2 label">Velocity <span class="units">(m/s)</span></span>
                    <Knob v-model="velocity" readonly :size="0" :max="5" />
                </div>
                <div class="p-col-fixed p-d-flex p-flex-column p-px-0 p-px-sm-2 middle-container">
                    <div class="p-d-flex p-flex-column p-ai-center">
                        <span class="p-my-2 label">Status</span>
                        <Chip :label="droneStatus" :style="{ 'background-color': droneStatusColor }" />
                    </div>
                    <div class="p-d-flex p-flex-column p-ai-center p-mt-sm-2">
                        <span class="p-my-2 label">LED</span>
                        <InputSwitch v-model="isLedEnabled" @change="setLedEnabled" />
                    </div>
                </div>
                <div class="p-col p-d-flex p-flex-column p-ai-center">
                    <span class="p-my-2 label">Battery</span>
                    <Knob v-model="batteryLevel" readonly :size="0" />
                </div>
            </div>
        </template>
    </Card>
</template>

<script lang="ts">
import { computed, defineComponent, inject, ref } from 'vue';
import { SocketClient } from '@/classes/socket-client';
import { DroneStatus } from '@/enums/drone-status';

export default defineComponent({
    name: 'Drone',
    props: {
        droneId: String,
    },
    setup(props) {
        const socketClient: SocketClient | undefined = inject('socketClient');

        const batteryLevel = ref(0);
        socketClient!.bindDroneMessage('battery-level', props.droneId!, (newBatteryLevel: number) => {
            batteryLevel.value = newBatteryLevel;
        });

        const velocity = ref(0);
        socketClient!.bindDroneMessage('velocity', props.droneId!, (newVelocity: number) => {
            velocity.value = newVelocity;
        });

        const droneStatus = ref(DroneStatus.Standby);
        socketClient!.bindDroneMessage('drone-status', props.droneId!, (newDroneStatus: DroneStatus) => {
            droneStatus.value = newDroneStatus;
        });

        const isLedEnabled = ref(false);
        socketClient!.bindDroneMessage('set-led', props.droneId!, (newIsLedEnabled: boolean) => {
            isLedEnabled.value = newIsLedEnabled;
        });

        function setLedEnabled() {
            socketClient!.sendDroneMessage('set-led', props.droneId!, isLedEnabled.value);
        }

        const droneStatusColor = computed((): string | undefined => {
            // TODO: Handle color for DroneStatus.Landing, DroneStatus.Landed, DroneStatus.Liftoff
            switch (droneStatus.value) {
                case DroneStatus.Standby:
                    return undefined; // Default background color
                case DroneStatus.Flying:
                    return 'var(--primary-color)';
                case DroneStatus.Crashed:
                    return 'var(--orange-400)';
                default:
                    return undefined;
            }
        });

        return {
            batteryLevel,
            velocity,
            droneStatus,
            isLedEnabled,
            setLedEnabled,
            droneStatusColor,
        };
    },
});
// TODO: Fix status colors and set the text color dynamically when needed
// TODO: Investigate using watch
</script>

<style lang="scss" scoped>
.card {
    flex-basis: 100%;
    border: 1px solid var(--surface-d);
    margin: 0 0.25rem;
    max-width: 32rem;
}

.title {
    font-size: 1.125rem;
}

div::v-deep(.p-card-content) {
    padding: 0;
}

.label {
    font-weight: 500;
}

.units {
    font-size: 0.75em;
}

.middle-container {
    min-width: 5.75rem;
}

div::v-deep(.p-knob) svg {
    width: 7rem;
    height: 7rem;
}

@media (max-width: 575px), (min-width: 900px) and (max-width: 991px) {
    div::v-deep(.p-knob) svg {
        width: 6rem;
        height: 6rem;
    }
}

@media (max-width: 400px) {
    .p-component {
        font-size: 0.875rem;
    }

    .middle-container {
        min-width: 4.25rem;
    }

    .p-chip {
        padding: 0 0.5rem;
    }

    div::v-deep(.p-knob) svg {
        width: 5rem;
        height: 5rem;
    }
}
</style>
