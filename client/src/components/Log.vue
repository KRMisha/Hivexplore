<template>
    <Panel header="Logs" :toggleable="true">
        <template #icons>
            <div class="p-d-flex p-ai-center p-mr-2">
                <span class="p-mr-2">Autoscroll</span>
                <InputSwitch v-model="isAutoscrollEnabled" @change="scrollToBottom" />
            </div>
        </template>
        <div ref="tabViewRef" class="tab-view-container">
            <TabView v-model:activeIndex="activeTabIndex" @tab-change="scrollToBottom">
                <TabPanel v-for="logName in orderedLogNames" :key="logName" :header="logName">
                    <ScrollPanel class="scroll-panel">
                        <div v-for="(logLine, index) in logs.get(logName)" :key="index" class="log-line">
                            > {{ logLine }}
                        </div>
                    </ScrollPanel>
                </TabPanel>
            </TabView>
        </div>
    </Panel>
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
        const tabViewRef = ref<HTMLElement | undefined>(undefined);
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

        // TODO: Rename?
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

                const activeTabName = orderedLogNames.value[activeTabIndex.value];
                mustRender = mustRender || activeTabName === logName;

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
            scrollPanels = tabViewRef.value!.getElementsByClassName('p-scrollpanel-content');
            const renderIntervalMs = 250;
            window.setInterval(renderNewLogs, renderIntervalMs);
        });

        socketClient!.bindMessage('log', onLogReception);

        addLogTab('Server');
        addLogTab('Map');

        return {
            logs,
            orderedLogNames,
            activeTabIndex,
            scrollToBottom,
            isAutoscrollEnabled,
            tabViewRef,
        };
    },
});
// TODO: Rename Log to Logs
// TODO: Use reactive for Map
// TODO: Fix key
// TODO: Reorder setup logically
// TODO: Trim URI
</script>

<style lang="scss" scoped>
.p-panel::v-deep(.p-panel-header) .p-panel-icons {
    display: flex;
    font-size: 0.75rem;

    .p-inputswitch {
        width: 1.8rem;
        height: 0.6rem;
    }

    .p-inputswitch .p-inputswitch-slider::before {
        width: 1rem;
        height: 1rem;
        margin-top: -0.5rem;
    }

    .p-inputswitch.p-inputswitch-checked .p-inputswitch-slider::before {
        transform: translateX(1rem);
    }
}

.tab-view-container {
    min-height: 200px;
}

.scroll-panel {
    height: 200px;
}

.log-line {
    font-family: 'Courier New', monospace;
    font-size: 0.875rem;
    padding-left: 1.2em;
    text-indent: -1.2em;
}
</style>
