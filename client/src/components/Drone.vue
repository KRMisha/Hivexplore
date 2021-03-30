<template>
    <Card class="card">
        <template #title>
            <div class="center-title">Drone {{ droneId }}</div>
        </template>
        <template #content>
            <div class="card-container">
                <div class="item-container">
                    <h4>Velocity (m/s) 🐝</h4>
                    <Knob v-model="velocity" readonly :size="128" :max="5" />
                </div>
                <div class="item-container middle-container">
                    <div class="item-container">
                        <h4>Status 🍯</h4>
                        <Chip :label="droneStatus" :style="{ 'background-color': droneStatusColor }" />
                    </div>
                    <div class="item-container">
                        <h4>LED 💡</h4>
                        <InputSwitch v-model="isLedEnabled" @change="setLedEnabled" />
                    </div>
                </div>
                <div class="item-container">
                    <h4>Battery 🔋</h4>
                    <Knob v-model="batteryLevel" readonly :size="128" />
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
</script>

<style scoped lang="scss">
.card {
    margin-bottom: 16px;
}

.center-title {
    text-align: center;
}

.card-container {
    display: flex;
    justify-content: space-evenly;
}

.item-container {
    display: flex;
    flex-direction: column;
    align-items: center;
}

.middle-container {
    min-width: 100px;
}
</style>
