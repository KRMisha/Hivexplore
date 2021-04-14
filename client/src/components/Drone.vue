<template>
    <Card class="card">
        <template #title>
            <div class="p-d-flex p-jc-center title">Drone {{ droneId }}</div>
        </template>
        <template #content>
            <div class="p-grid">
                <div class="p-col p-d-flex p-flex-column p-ai-center">
                    <span class="p-my-2 label">Velocity <span class="units">(m/s)</span></span>
                    <Knob v-model="velocity" readonly :max="5" />
                </div>
                <div class="p-col-fixed p-d-flex p-flex-column p-px-0 p-px-sm-2 middle-container">
                    <div class="p-d-flex p-flex-column p-ai-center">
                        <span class="p-my-2 label">Status</span>
                        <Chip :label="droneStatus" :style="droneStatusStyle" />
                    </div>
                    <div class="p-d-flex p-flex-column p-ai-center p-mt-sm-2">
                        <span class="p-my-2 label">LED</span>
                        <InputSwitch v-model="isLedEnabled" @change="setLedEnabled" />
                    </div>
                </div>
                <div class="p-col p-d-flex p-flex-column p-ai-center">
                    <span class="p-my-2 label">Battery <span class="units">(%)</span></span>
                    <Knob v-model="batteryLevel" readonly />
                </div>
            </div>
        </template>
    </Card>
</template>

<script lang="ts">
import { computed, defineComponent, inject, onUnmounted, ref } from 'vue';
import { WebSocketClient } from '@/communication/web-socket-client';
import { WebSocketEvent } from '@/communication/web-socket-event';
import { DroneStatus } from '@/enums/drone-status';

export default defineComponent({
    name: 'Drone',
    props: {
        droneId: {
            type: String,
            required: true,
        },
    },
    setup(props) {
        const webSocketClient = inject('webSocketClient') as WebSocketClient;

        const velocity = ref(0);
        const newVelocityCallback = (newVelocity: number) => {
            velocity.value = newVelocity;
        };
        webSocketClient.bindDroneMessage(WebSocketEvent.Velocity, props.droneId, newVelocityCallback);

        const batteryLevel = ref(0);
        const newBatteryLevelCallback = (newBatteryLevel: number) => {
            batteryLevel.value = newBatteryLevel;
        };
        webSocketClient.bindDroneMessage(WebSocketEvent.BatteryLevel, props.droneId, newBatteryLevelCallback);

        const droneStatus = ref(DroneStatus.Standby);
        const newDroneStatusCallback = (newDroneStatus: DroneStatus) => {
            droneStatus.value = newDroneStatus;
        };
        webSocketClient.bindDroneMessage(WebSocketEvent.DroneStatus, props.droneId, newDroneStatusCallback);

        const isLedEnabled = ref(false);
        const newIsLedEnabledCallback = (newIsLedEnabled: boolean) => {
            isLedEnabled.value = newIsLedEnabled;
        };
        webSocketClient.bindDroneMessage(WebSocketEvent.Led, props.droneId, newIsLedEnabledCallback);

        function setLedEnabled() {
            webSocketClient.sendDroneMessage(WebSocketEvent.Led, props.droneId, isLedEnabled.value);
        }

        const droneStatusStyle = computed(() => {
            let color: string | undefined = undefined; // Default text color
            let backgroundColor: string | undefined = undefined; // Default background color

            switch (droneStatus.value) {
                case DroneStatus.Liftoff:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--blue-300)';
                    break;
                case DroneStatus.Flying:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--primary-color)';
                    break;
                case DroneStatus.Returning:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--purple-200)';
                    break;
                case DroneStatus.Landing:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--teal-300)';
                    break;
                case DroneStatus.Landed:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--green-400)';
                    break;
                case DroneStatus.Crashed:
                    color = 'var(--primary-color-text)';
                    backgroundColor = 'var(--pink-200)';
                    break;
            }

            return {
                color: color,
                'background-color': backgroundColor,
            };
        });

        onUnmounted(() => {
            webSocketClient.unbindDroneMessage(WebSocketEvent.Velocity, props.droneId, newVelocityCallback);
            webSocketClient.unbindDroneMessage(WebSocketEvent.BatteryLevel, props.droneId, newBatteryLevelCallback);
            webSocketClient.unbindDroneMessage(WebSocketEvent.DroneStatus, props.droneId, newDroneStatusCallback);
            webSocketClient.unbindDroneMessage(WebSocketEvent.Led, props.droneId, newIsLedEnabledCallback);
        });

        return {
            velocity,
            batteryLevel,
            droneStatus,
            isLedEnabled,
            setLedEnabled,
            droneStatusStyle,
        };
    },
});
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

.p-card::v-deep(.p-card-content) {
    padding: 0;
}

.label {
    font-weight: 500;
}

.units {
    font-size: 0.75em;
}

.middle-container {
    min-width: 6rem;
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
