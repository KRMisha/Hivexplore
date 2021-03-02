const serverUrl = 'ws:localhost:5678';

export default class SocketClient {
    private socket: WebSocket = new WebSocket(serverUrl);

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    private callbacks: Map<string, Map<string | undefined, Array<(data: any) => void>>> = new Map();

    constructor() {
        this.socket.onmessage = (messageEvent: MessageEvent) => {
            const message = JSON.parse(messageEvent.data);

            const eventCallbacks = this.callbacks.get(message.event);

            if (eventCallbacks === undefined) {
                console.warn(`Unknown socket event received: ${message.event}`);
                return;
            }

            const droneId = message.droneId ?? undefined;
            const droneCallbacks = eventCallbacks.get(droneId);

            if (droneCallbacks === undefined) {
                console.warn(`Unregistered drone ID ${droneId} for socket event ${message.event}`);
                return;
            }

            for (const callback of droneCallbacks) {
                callback(message.data);
            }
        };

        this.socket.onopen = () => {
            console.log(`Connection to ${serverUrl} successful`);
        };

        this.socket.onclose = () => {
            console.log(`Connection to ${serverUrl} closed`);
        };
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    bindMessage(event: string, callback: (data: any) => void) {
        // undefined represents an event not related to a specific drone
        this.bind(event, undefined, callback);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    bindDroneMessage(event: string, droneId: string, callback: (data: any) => void) {
        this.bind(event, droneId, callback);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    sendMessage(event: string, data: any) {
        // undefined represents an event not related to a specific drone
        this.send(event, undefined, data);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    sendDroneMessage(event: string, droneId: string, data: any) {
        this.send(event, droneId, data);
    }

    close() {
        this.socket.close();
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    private bind(event: string, droneId: string | undefined, callback: (data: any) => void) {
        if (!this.callbacks.has(event)) {
            this.callbacks.set(event, new Map());
        }

        if (!this.callbacks.get(event)!.has(droneId)) {
            this.callbacks.get(event)!.set(droneId, []);
        }

        this.callbacks
            .get(event)!
            .get(droneId)!
            .push(callback);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    private send(event: string, droneId: string | undefined, data: any) {
        // Convert date to local timezone by stripping the timezone offset
        const timestampUtc = new Date();
        const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

        const payload = JSON.stringify({
            event: event,
            droneId: droneId ?? null,
            data: data,
            timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
        });

        this.socket.send(payload);
    }
}
