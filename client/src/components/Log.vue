<template>
    <div class="p-d-flex p-p-3 header">
        <div class="p-text p-text-bold">
            Logs
        </div>
        <div class="p-text p-ml-auto">
            Autoscroll
        </div>
        <InputSwitch v-model="isAutoScrollEnabled" @change="scrollToBottom" class="p-ml-2"/>
    </div>

    <TabView class="tab-view" v-model:activeIndex="activeTabIndex" @click="scrollToBottom">
        <TabPanel v-for="logName in Array.from(logs.keys())" :key="logName" :header="logName">
            <ScrollPanel class="scroll-panel">
                <div class="log-text">
                    <div v-for="(logLine, index) in logs.get(logName)" :key="logLine + index">
                        > {{logLine}}
                        <br>
                    </div>
                </div>
            </ScrollPanel>
        </TabPanel>
    </TabView>
</template>

<script lang="ts">
import { defineComponent, inject, onMounted, ref } from 'vue'
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'Log',
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');
        const logs = ref<Map<string, Array<string>>>(new Map());
        const activeTabIndex = ref(0);
        const isAutoScrollEnabled = ref(true);

        onMounted(() => {
            interface Log {
                name: string;
                message: string;
            }
            socketClient!.bindMessage('log', (log: Log) => {
                if (!logs.value.has(log.name)) {
                    logs.value.set(log.name, []);
                }

                logs.value.get(log.name)!.push(log.message);
                const maxLogCount = 128;
                if (logs.value.get(log.name)!.length > maxLogCount) {
                    logs.value.get(log.name)!.shift();
                }

                // If the newly added log is in the current tab
                if (activeTabIndex.value == Array.from(logs.value.keys()).indexOf(log.name)) {
                    // Wait for the DOM to update and scroll to the bottom
                    setTimeout(scrollToBottom, 0);
                }
            });
        });

        function scrollToBottom() {
            if (isAutoScrollEnabled.value) {
                const scrollPanel= document.getElementsByClassName("p-scrollpanel-content")[activeTabIndex.value];
                if (scrollPanel !== undefined) {
                    scrollPanel.scrollTop = scrollPanel.scrollHeight;
                }
            }
        }

        return {
            logs,
            activeTabIndex,
            scrollToBottom,
            isAutoScrollEnabled,
        }
    },
});
</script>

<style scoped lang="scss">
.header {
    width: 50%;
}

.tab-view {
    width: 50%;
    height: 275px;
}

.scroll-panel {
    height: 200px;
}

.p-text {
    font-family: Roboto;
}

.log-text {
    font-family: 'Courier New', monospace;
}

</style>
