<template>
    <div>BatteryLevel: {{ battery }}</div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    name: 'Drone',
    props: {
        battery: Number
    }
});
</script>
