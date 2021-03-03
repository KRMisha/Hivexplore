<template>
    <Card class="drone-card">
        <template #title>
            <div style="text-align:center"> Drone {{ droneId }} </div>
        </template>
        <template #content>
            <div class="flexbox1">
                <div>
                    <div class="p-text-center">Battery</div>
                    <Knob v-model="batteryLevel" readonly :size="64" />
                    <div class="p-text-center">Velocity</div>
                    <Knob v-model="velocity" readonly :size="64" />
                    <Divider />
                    <div class="p-text-center">LED</div>
                    <InputSwitch v-model="isLedOn" @change="changeLedStatus" />
                    <Divider />
                </div>
                <div class="flexbox2">
                    <Timeline :value="states">
                        <template #marker="stateProps">
                            <span>
                                <i class="dot" v-if="stateProps.item.name === currentState" style="background-color:blue"></i>
                                <i class="dot" v-else></i>
                            </span>
                        </template>
                        <template #content="stateProps">
                            <div v-if="stateProps.item.name === currentState" style="font-weight:bold">{{stateProps.item.name}}</div>
                            <div v-else>{{stateProps.item.name}}</div>
                        </template>
                    </Timeline>
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
        velocity: String,
        state: String,
    },
    setup(props) {
        const socket: SocketClient | undefined = inject('socket');

        const batteryLevel = ref(0);
        socket!.bindDroneMessage('battery-level', props.droneId!, (newBatteryLevel: number) => {
            batteryLevel.value = newBatteryLevel;
        });

        const velocity = ref(0);
        socket!.bindDroneMessage('velocity', props.velocity!, (newVelocity: number) => {
            velocity.value = newVelocity;
        });

        const currentState = ref('');
        socket!.bindDroneMessage('state', props.state!, (newState: string) => {
            currentState.value = newState;
        });
        // TODO: Set state dynamically
        currentState.value = 'Standby'

        const isLedOn = ref(false);
        socket!.bindDroneMessage('set-led', props.droneId!, (newIsLedOn: boolean) => {
            isLedOn.value = newIsLedOn;
        });

        function changeLedStatus() {
            socket!.sendDroneMessage('set-led', props.droneId!, isLedOn.value);
        }

        return {
            batteryLevel,
            velocity,
            currentState,
            states: [
                {name: 'Standby'},
                {name: 'In-mission'},
                {name: 'Crashed'},
                {name: 'Returned'}
            ],
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
.dot {
    height: 10px;
    width: 10px;
    background-color: #bbb;
    border-radius: 50%;
    display: inline-block;
}
.flexbox1 {
    display: flex;
    justify-content: space-evenly;
}
.flexbox2 {
    display: flex;
}
</style>
