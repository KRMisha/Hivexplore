<template>
    <Card class="card">
        <template #title>
            <div class="center-title">Drone {{ droneId }}</div>
        </template>
        <template #content>
            <div class="card-container">
                <div class="item-container">
                    <h4 class="center-title">Velocity 🐝</h4>
                    <Knob v-model="velocity" readonly :size="128" />
                </div>
                <div class="center-container">
                    <div class="item-container">
                        <h4 class="center-title">Status 🍯</h4>
                        <Chip :label="currentDroneState" :style="{'background-color': droneStateColor}" />
                    </div>
                    <div class="item-container">
                        <h4 class="center-title">LED 💡</h4>
                        <InputSwitch v-model="isLedOn" @change="changeLedStatus" />
                    </div>
                </div>
                <div class="item-container">
                    <h4 class="center-title">Battery 🔋</h4>
                    <Knob v-model="batteryLevel" readonly :size="128" />
                </div>
            </div>
        </template>
    </Card>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import { inject, computed } from 'vue';
import SocketClient from './../classes/socket-client';

export default defineComponent({
    name: 'Drone',
    props: {
        droneId: String,
    },
    setup(props) {
        const socket: SocketClient | undefined = inject('socket');

        const batteryLevel = ref(0);
        socket!.bindDroneMessage('battery-level', props.droneId!, (newBatteryLevel: number) => {
            batteryLevel.value = newBatteryLevel;
        });

        const velocity = ref(0); // TODO: Send message on server
        socket!.bindDroneMessage('velocity', props.droneId!, (newVelocity: number) => {
            velocity.value = newVelocity;
        });

        const currentDroneState = ref('Standby'); // TODO: Send message on server
        socket!.bindDroneMessage('drone-state', props.droneId!, (newCurrentMissionState: string) => {
            currentDroneState.value = newCurrentMissionState;
        });

        const isLedOn = ref(false);
        socket!.bindDroneMessage('set-led', props.droneId!, (newIsLedOn: boolean) => {
            isLedOn.value = newIsLedOn;
        });

        function changeLedStatus() {
            socket!.sendDroneMessage('set-led', props.droneId!, isLedOn.value);
        }

        const droneStateColor = computed(() => {
            switch (currentDroneState.value) {
                case 'Standby':
                    return 'light-gray';  // TODO: use var(--color) colors
                case 'Flying':
                    return 'var(--primary-color)';
                case 'Crashed':
                    return 'red'; // TODO: use var(--color) colors
            }
        });

        return {
            batteryLevel,
            velocity,
            isLedOn,
            changeLedStatus,
            currentDroneState,
            droneStateColor,
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

.item-container,
.center-container {
    display: flex;
    flex-direction: column;
}

.center-container {
    align-items: center;
}
</style>
