<template>
    <Panel header="Logs" :toggleable="true">
        <template #icons>
            <div class="p-d-flex p-ai-center p-mr-2">
                <span class="p-mr-2">Autoscroll</span>
                <InputSwitch v-model="isAutoscrollEnabled" @change="updateScroll" />
            </div>
        </template>
        <div ref="tabViewRef">
            <TabView v-model:activeIndex="activeTabIndex" @tab-change="updateScroll">
                <TabPanel v-for="logGroup in orderedLogGroups" :key="logGroup" :header="logGroup">
                    <div class="scroll-panel">
                        <div v-for="(logLine, index) in logs.get(logGroup)" :key="index" class="log-line">> {{ logLine }}</div>
                    </div>
                </TabPanel>
            </TabView>
        </div>
    </Panel>
</template>

<script lang="ts">
import { computed, defineComponent, inject, nextTick, onMounted, reactive, ref } from 'vue';
import { SocketClient } from '@/classes/socket-client';

interface Log {
    group: string;
    line: string;
}

export default defineComponent({
    name: 'Logs',
    setup() {
        // Tabs
        const tabViewRef = ref<HTMLElement | undefined>(undefined);
        let scrollPanels: HTMLCollectionOf<Element>;
        const activeTabIndex = ref(0);

        // Scrolling
        const isAutoscrollEnabled = ref(true);
        async function updateScroll() {
            if (isAutoscrollEnabled.value) {
                await nextTick(); // Wait for the DOM to update and scroll to the bottom
                const scrollPanel = scrollPanels[activeTabIndex.value];
                if (scrollPanel !== undefined) {
                    scrollPanel.scrollTop = scrollPanel.scrollHeight;
                }
            }
        }

        // Logs
        const logs = reactive(new Map<string, string[]>());
        const logBuffers = new Map<string, string[]>();

        // Ordered log groups
        const initialLogGroups = ['Server', 'Map'];
        const orderedLogGroups = computed(() => {
            const orderedLogGroups = [...logs.keys()];

            orderedLogGroups.sort((first: string, second: string): number => {
                const indexOfFirst = initialLogGroups.indexOf(first);
                const indexOfSecond = initialLogGroups.indexOf(second);

                // If the indices are the same (-1), sort alphabetically
                if (indexOfFirst === indexOfSecond) {
                    return first.localeCompare(second);
                }

                // If log groups aren't in initialLogGroups, they should go to the end
                const orderOfFirst = indexOfFirst === -1 ? Infinity : indexOfFirst;
                const orderOfSecond = indexOfSecond === -1 ? Infinity : indexOfSecond;

                return orderOfFirst > orderOfSecond ? 1 : -1;
            });

            return orderedLogGroups;
        });

        // Log group management
        function addLogGroup(logGroup: string) {
            // Trim URI to extract Crazyflie address
            const trimmedLogGroup = logGroup.replace('radio://0/80/2M/', '');

            if (!logs.has(trimmedLogGroup)) {
                logs.set(trimmedLogGroup, []);
            }
            if (!logBuffers.has(trimmedLogGroup)) {
                logBuffers.set(trimmedLogGroup, []);
            }
        }

        for (const logGroup of initialLogGroups) {
            addLogGroup(logGroup);
        }

        // Log reception
        const socketClient: SocketClient | undefined = inject('socketClient');
        socketClient!.bindMessage('log', (log: Log) => {
            addLogGroup(log.group);
            logBuffers.get(log.group)!.push(log.line);
        });

        onMounted(() => {
            // Initialize scroll panel references
            scrollPanels = tabViewRef.value!.getElementsByClassName('scroll-panel');

            // Render new logs periodically
            const renderIntervalMs = 250;
            setInterval(() => {
                let mustRender = false;
                for (const [logGroup, logBuffer] of logBuffers) {
                    if (logBuffer.length === 0) {
                        continue;
                    }

                    logs.get(logGroup)!.push(...logBuffer);

                    const maxLogCount = 512;
                    logs.get(logGroup)!.splice(0, Math.max(0, logs.get(logGroup)!.length - maxLogCount));

                    const activeTabGroupName = orderedLogGroups.value[activeTabIndex.value];
                    mustRender = mustRender || activeTabGroupName === logGroup;

                    logBuffers.set(logGroup, []);
                }

                if (mustRender) {
                    updateScroll();
                }
            }, renderIntervalMs);
        });

        return {
            tabViewRef,
            activeTabIndex,
            isAutoscrollEnabled,
            updateScroll,
            logs,
            orderedLogGroups,
        };
    },
});
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

.scroll-panel {
    height: 200px;
    overflow-y: auto;
    scrollbar-color: #555 var(--surface-a);
    &::-webkit-scrollbar-track {
        background-color: var(--surface-a);
    }

    &::-webkit-scrollbar {
        background-color: var(--surface-a);
    }

    &::-webkit-scrollbar-thumb {
        background-color: #555;
    }
}

.log-line {
    font-family: 'Courier New', monospace;
    font-size: 0.875rem;
    padding-left: 1.2em;
    text-indent: -1.2em;
}
</style>
