<template>
    <Button @click="batteryLevel++">
        click
    </Button>
    <Drone :battery="batteryLevel" />
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import Drone from './components/Drone.vue';

export default defineComponent({
    name: 'App',
    components: {
        Drone
    },
    setup() {
        const batteryLevel = ref(0);

        return {
            batteryLevel
        };
    }
});
</script>
