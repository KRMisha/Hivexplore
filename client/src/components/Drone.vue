<template>
    <Card class="drone-card">
        <template #title>
            Drone
        </template>
        <template #content>
            <div class="p-text-center">Drone Battery:</div>
            <Knob v-model="battery" readonly :size="64" />
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
    props: {
        battery: Number,
    },
    setup() {
        const isLedOn = ref(false);

        const socket: SocketClient | undefined = inject('socket');

        function changeLedStatus() {
            // Convert date to local timezone by stripping the timezone offset
            const timestampUtc = new Date();
            const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

            const message = {
                data: isLedOn,
                timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
            };
            socket!.send('set-led', message);
        }

        return {
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
