const serverUrl = 'ws:localhost:5678';

export default class SocketClient {
    private socket: WebSocket = new WebSocket(serverUrl);

    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    private callbacks: Map<string, Map<string | undefined, Array<(data: any) => void>>> = new Map();

    constructor() {
        this.socket.onmessage = (event: MessageEvent) => {
            const jsonMessage = JSON.parse(event.data);

            const eventCallbacks = this.callbacks.get(jsonMessage.event);

            if (eventCallbacks === undefined) {
                console.warn(`Unknow socket event received: ${jsonMessage.event}`);
                return;
            }

            const droneId = jsonMessage.droneId ?? undefined;
            const droneCallbacks = eventCallbacks.get(droneId);

            if (droneCallbacks === undefined) {
                console.warn(`Unregistered drone id ${droneId} for socket event ${jsonMessage.event}`);
                return;
            }

            for (const callback of droneCallbacks) {
                callback(jsonMessage.data);
            }
        };

        this.socket.onopen = () => {
            console.log(`Connection to ${serverUrl} successful`);
        };

        this.socket.onclose = () => {
            console.log(`Connection to ${serverUrl} closed`);
        };
    }

    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    bind(eventName: string, droneId: string | undefined, callback: (data: any) => void) {
        if (!this.callbacks.has(eventName)) {
            this.callbacks.set(eventName, new Map());
        }

        if (!this.callbacks.get(eventName)!.has(droneId)) {
            this.callbacks.get(eventName)!.set(droneId, []);
        }

        this.callbacks
            .get(eventName)!
            .get(droneId)!
            .push(callback);
    }

    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    send(eventName: string, droneId: string | undefined, data: any) {
        // Convert date to local timezone by stripping the timezone offset
        const timestampUtc = new Date();
        const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

        const payload = JSON.stringify({
            event: eventName,
            droneId: droneId ?? null,
            data: data,
            timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
        });

        this.socket.send(payload);
    }

    close() {
        this.socket.close();
    }
}
