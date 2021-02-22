const serverUrl = 'ws:localhost:5678';

export default class SocketClient {
    private socket: WebSocket = new WebSocket(serverUrl);

    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    private callbacks: Map<[string, number?], Array<(data: any) => void>> = new Map();

    constructor() {
        this.socket.onmessage = (event: MessageEvent) => {
            const jsonContent = JSON.parse(event.data);

            const droneId = jsonContent.droneId === null ? undefined : jsonContent.droneId;
            const callbacks = this.callbacks.get([jsonContent.event, droneId]);

            if (callbacks === undefined) {
                console.warn(`Unknown socket event received: ${jsonContent.event}`);
                return;
            }

            for (const callback of callbacks) {
                callback(jsonContent.data);
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
    bind(eventName: string, droneId: number | undefined, callback: (data: any) => void) {
        if (!this.callbacks.has([eventName, droneId])) {
            this.callbacks.set([eventName, droneId], []);
        }
        this.callbacks.get([eventName, droneId])!.push(callback);
    }

    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    send(eventName: string, data: any) {
        // Convert date to local timezone by stripping the timezone offset
        const timestampUtc = new Date();
        const timestamp = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);

        const payload = JSON.stringify({
            event: eventName,
            data: data,
            timestamp: timestamp.toJSON().replace('Z', ''), // Remove the trailing Z since the timestamp is not in UTC
        });

        this.socket.send(payload);
    }

    close() {
        this.socket.close();
    }
}
