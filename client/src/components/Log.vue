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

    <div ref="logRef" class="tab-view-container">
        <TabView v-model:activeIndex="activeTabIndex" @click="scrollToBottom">
            <TabPanel v-for="(logName, index) in orderedLogNames" :key="logName" :header="logName">
                <ScrollPanel class="scroll-panel">
                    <div v-if="index === activeTabIndex" class="log-text">
                        <div v-for="(logLine, index) in logs.get(logName)" :key="index">
                            > {{ logLine }}
                            <br />
                        </div>
                    </div>
                </ScrollPanel>
            </TabPanel>
        </TabView>
    </div>
</template>

<script lang="ts">
import { defineComponent, inject, onMounted, ref } from 'vue';
import { SocketClient } from '@/classes/socket-client';

export default defineComponent({
    name: 'Log',
    setup() {
        // Variables

        const socketClient: SocketClient | undefined = inject('socketClient');
        const logs = ref<Map<string, string[]>>(new Map());
        const logsBuffer = new Map<string, string[]>();
        const orderedLogNames = ref<string[]>([]);
        const activeTabIndex = ref(0);
        const isAutoscrollEnabled = ref(true);
        const logRef = ref<HTMLElement | undefined>(undefined);
        let scrollPanels: HTMLCollectionOf<Element>;

        // Functions

        function addLogTab(tabName: string) {
            if (!logs.value.has(tabName) && !logsBuffer.has(tabName)) {
                logs.value.set(tabName, []);
                logsBuffer.set(tabName, []);
                orderedLogNames.value.push(tabName);

                orderedLogNames.value.sort((first: string, second: string): number => {
                    const firstLogNames = ['Server', 'Map'];

                    const indexOfFirst = firstLogNames.indexOf(first);
                    const indexOfSecond = firstLogNames.indexOf(second);

                    // If the indices are the same (-1), sort alphabetically
                    if (indexOfFirst === indexOfSecond) {
                        return first.localeCompare(second);
                    }

                    // If log names aren't in firstLogNames, they should go to the end
                    const orderOfFirst = indexOfFirst === -1 ? Infinity : indexOfFirst;
                    const orderOfSecond = indexOfSecond === -1 ? Infinity : indexOfSecond;

                    return orderOfFirst > orderOfSecond ? 1 : -1;
                });
            }
        }

        function scrollToBottom() {
            if (isAutoscrollEnabled.value) {
                const scrollPanel = scrollPanels[activeTabIndex.value];
                if (scrollPanel !== undefined) {
                    scrollPanel.scrollTop = scrollPanel.scrollHeight;
                }
            }
        }

        function renderNewLogs() {
            let mustRender = false;
            for (const [logName, logsArray] of logsBuffer) {
                if (logsArray.length === 0) {
                    continue;
                }

                logs.value.get(logName)!.push(...logsArray);

                const maxLogCount = 512;
                logs.value.get(logName)!.splice(0, Math.max(0, logs.value.get(logName)!.length - maxLogCount));

                const activeTabName = orderedLogNames[activeTabIndex.value];
                mustRender = mustRender || activeTabName == logName;

                logsBuffer.set(logName, []);
            }

            if (mustRender) {
                // Wait for the DOM to update and scroll to the bottom
                setTimeout(scrollToBottom, 0);
            }
        }

        interface Log {
            name: string;
            message: string;
        }
        function onLogReception(log: Log) {
            addLogTab(log.name);

            logsBuffer.get(log.name)!.push(log.message);
        }

        // Actions

        onMounted(() => {
            scrollPanels = logRef.value!.getElementsByClassName('p-scrollpanel-content');
        });

        socketClient!.bindMessage('log', onLogReception);

        addLogTab('Server');
        addLogTab('Map');

        const renderIntervalMs = 250;
        window.setInterval(renderNewLogs, renderIntervalMs);

        return {
            logs,
            orderedLogNames,
            activeTabIndex,
            scrollToBottom,
            isAutoscrollEnabled,
            logRef,
        };
    },
});
</script>

<style scoped lang="scss">
.header {
    width: 50%;
}

.tab-view-container {
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
