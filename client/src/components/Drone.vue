<template>
    <Card style="width: 25rem; margin: auto; margin-bottom: 2em;">
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
import { defineComponent } from 'vue';

export default defineComponent({
    name: 'Drone',
    props: {
        battery: Number
    },
    emits: ['changeLedStatus'],
    data() {
        return {
            isLedOn: false
        };
    },
    methods: {
        changeLedStatus() {
            this.$emit('changeLedStatus', this.isLedOn);
        }
    }
});
</script>
