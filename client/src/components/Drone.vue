<template>
    <Card class="drone-card">
        <template #title>
            Drone
        </template>
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
import { inject } from 'vue'
import SocketClient from './../classes/socket-client';

export default defineComponent({
    name: 'Drone',
    setup() {
        const socket: SocketClient | undefined = inject('socket');

        const batteryLevel = ref(0);
        socket!.bind('battery-level', (message: number) => {
            batteryLevel.value = message;
        });

        const isLedOn = ref(false);

        function changeLedStatus() {
            socket!.send('set-led', isLedOn.value);
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
