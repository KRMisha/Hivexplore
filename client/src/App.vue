<template>
    <Button @click="batteryLevel++">
        click
    </Button>
    <Drone :battery="batteryLevel" @changeLedStatus="changeLedStatus"/>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import Drone from './components/Drone.vue';

export default defineComponent({
    name: 'App',
    components: {
        Drone
    },
    methods: {
        changeLedStatus(isLedOn: boolean) {
            console.log(isLedOn);
        }
    },
    setup() {
        const batteryLevel = ref(0);

        return {
            batteryLevel
        };
    }
});
</script>
