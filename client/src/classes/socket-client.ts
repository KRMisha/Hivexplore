export default class SocketClient {
    private socket: WebSocket;

    // TODO refactor to fix no-explicit-any tslint error
    // eslint-disable-next-line  @typescript-eslint/no-explicit-any
    private callbacks: Map<string, Array<(data: any) => void>>;

    constructor(serverIpAddress: string, serverPort: string) {
        const serverUrl = serverIpAddress + ':' + serverPort;
        this.socket = new WebSocket(serverUrl);

        this.callbacks = new Map();

        this.socket.onmessage = (event: MessageEvent) => {
            const jsonContent = JSON.parse(event.data);

            const callbacks = this.callbacks.get(jsonContent.event);

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
    bind(eventName: string, callback: (data: any) => void) {
        if (!this.callbacks.has(eventName)) {
            this.callbacks.set(eventName, []);
        }
        this.callbacks.get(eventName)!.push(callback);
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
