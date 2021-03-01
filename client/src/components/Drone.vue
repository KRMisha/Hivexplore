<template>
    <Card class="drone-card">
        <template #title> Drone {{ droneId }} </template>
        <template #content>
            <div class="p-text-center">Drone Battery:</div>
            <Knob v-model="batteryLevel" readonly :size="64" />
            <Divider />
            <div class="p-text-center">Drone Led:</div>
            <InputSwitch v-model="isLedOn" @change="changeLedStatus" />
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
        socket!.bind_drone_message('battery-level', props.droneId!, (updatedBatteryLevel: number) => {
            batteryLevel.value = updatedBatteryLevel;
        });

        const isLedOn = ref(false);
        socket!.bind_drone_message('set-led', props.droneId!, (updatedIsLedOn: boolean) => {
            isLedOn.value = updatedIsLedOn;
            console.log('a');
        });


        function changeLedStatus() {
            socket!.send_drone_message('set-led', props.droneId!, isLedOn.value);
        }

        return {
            batteryLevel,
            isLedOn,
            changeLedStatus,
        };
    },
});
</script>

<style scoped lang="scss">
.drone-card {
    width: 25rem;
    margin: auto;
    margin-bottom: 2em;
}
</style>
