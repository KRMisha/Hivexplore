export default class SocketClient {

    private socket : WebSocket;

    private callbacks: Map<String, Array<((data: any) => void)>>;

    constructor(serverIpAddress: String, serverPort: String) {
        const serverUrl = serverIpAddress + ':' + serverPort;
        this.socket = new WebSocket(serverUrl);

        this.callbacks = new Map();

        this.socket.onmessage = (event: MessageEvent) => {
            const json = JSON.parse(event.data);

            const callbacks = this.callbacks.get(json.event);

            if (callbacks === undefined) {
                console.warn(`Unknown socket event received: ${json.event}`);
                return;
            }

            for (let callback of callbacks) {
                callback(json.data);
            }
        };

        this.socket.onopen = (event: Event) => {
            console.log(`Connection to ${serverUrl} successful`);
        };

        this.socket.onclose = (event: CloseEvent) => {
            console.log(`Connection to ${serverUrl} closed`);
        };
    }

    bind(eventName: String, callback: ((data: any) => void)) {
        if (!this.callbacks.has(eventName)) {
            this.callbacks.set(eventName, new Array());
        }
        this.callbacks.get(eventName)!.push(callback);
    }

    send(eventName: String, data: any) {
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
