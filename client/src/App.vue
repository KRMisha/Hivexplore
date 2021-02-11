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

<style lang="scss">
#app {
    font-family: Avenir, Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    text-align: center;
    color: #2c3e50;
    margin-top: 60px;
}
</style>
