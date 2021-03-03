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
                        <Chip :label="currentDroneState" :color="droneStateColor" />
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
import { inject } from 'vue';
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

        const velocity = ref(0);
        socket!.bindDroneMessage('velocity', props.droneId!, (newVelocity: number) => {
            velocity.value = newVelocity;
        });

        const currentDroneState = ref('');
        socket!.bindDroneMessage('drone-state', props.droneId!, (newCurrentMissionState: string) => {
            currentDroneState.value = newCurrentMissionState;
        });
        currentDroneState.value = 'Flying'; // TODO: Set state dynamically

        const isLedOn = ref(false);
        socket!.bindDroneMessage('set-led', props.droneId!, (newIsLedOn: boolean) => {
            isLedOn.value = newIsLedOn;
        });

        function changeLedStatus() {
            socket!.sendDroneMessage('set-led', props.droneId!, isLedOn.value);
        }

        function droneStateColor() {
            if (currentDroneState.value === 'Flying') {
                return 'blue';
            }
            if (currentDroneState.value === 'Crashed') {
                return 'red';
            }
            return 'gray';
        }

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

.blue-chip {
    color: blue;
}

.red-chip {
    color: red;
}
</style>
