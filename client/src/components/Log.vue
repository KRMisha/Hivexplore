<template>
    <div class="p-d-flex p-p-3 header">
        <div class="p-text p-text-bold">
            Logs
        </div>
        <div class="p-text p-ml-auto">
            Autoscroll
        </div>
        <InputSwitch v-model="isAutoscrollEnabled" @change="scrollToBottom" class="p-ml-2" />
    </div>

    <TabView class="tab-view" v-model:activeIndex="activeTabIndex" @click="scrollToBottom">
        <TabPanel v-for="logName in logsOrdered" :key="logName" :header="logName">
            <ScrollPanel class="scroll-panel">
                <div class="log-text">
                    <div v-for="(logLine, index) in logs.get(logName)" :key="logLine + index">
                        > {{ logLine }}
                        <br />
                    </div>
                </div>
            </ScrollPanel>
        </TabPanel>
    </TabView>
</template>

<script lang="ts">
import { defineComponent, inject, ref } from 'vue';
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'Log',
    setup() {
        const socketClient: SocketClient | undefined = inject('socketClient');
        const logs = ref<Map<string, Array<string>>>(new Map());
        const logsOrdered = ref<Array<string>>([]);
        const activeTabIndex = ref(0);
        const isAutoscrollEnabled = ref(true);
        const scrollPanels = document.getElementsByClassName('p-scrollpanel-content');

        function scrollToBottom() {
            if (isAutoscrollEnabled.value) {
                const scrollPanel = scrollPanels[activeTabIndex.value];
                if (scrollPanel !== undefined) {
                    scrollPanel.scrollTop = scrollPanel.scrollHeight;
                }
            }
        }

        interface Log {
            name: string;
            message: string;
        }
        socketClient!.bindMessage('log', (log: Log) => {
            if (!logs.value.has(log.name)) {
                logs.value.set(log.name, []);
                logsOrdered.value.push(log.name);

                logsOrdered.value.sort((first: string, second: string): number => {
                    const logsOrder = ['Server', 'Map'];

                    const indexOfFirst = logsOrder.indexOf(first);
                    const indexOfSecond = logsOrder.indexOf(second);

                    // If the indices are the same (-1), sort alphabetically
                    if (indexOfFirst === indexOfSecond) {
                        return first.localeCompare(second);
                    }

                    // If log names aren't in logsOrder, they should go to the end
                    const orderOfFirst = indexOfFirst === -1 ? Infinity : indexOfFirst;
                    const orderOfSecond = indexOfSecond === -1 ? Infinity : indexOfSecond;

                    return orderOfFirst > orderOfSecond ? 1 : -1;
                });
            }

            logs.value.get(log.name)!.push(log.message);
            const maxLogCount = 1024;
            if (logs.value.get(log.name)!.length > maxLogCount) {
                logs.value.get(log.name)!.shift();
            }

            // If the newly added log is in the current tab
            if (activeTabIndex.value == Array.from(logs.value.keys()).indexOf(log.name)) {
                // Wait for the DOM to update and scroll to the bottom
                setTimeout(scrollToBottom, 0);
            }
        });

        return {
            logs,
            logsOrdered,
            activeTabIndex,
            scrollToBottom,
            isAutoscrollEnabled,
        };
    },
});
</script>

<style scoped lang="scss">
.header {
    width: 50%;
}

.tab-view {
    width: 50%;
    height: 280px;
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
