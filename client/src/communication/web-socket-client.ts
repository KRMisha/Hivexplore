import { Message } from '@/communication/message';
import { WebSocketEvent } from '@/communication/web-socket-event';
import { getLocalTimestamp } from '@/utils/local-timestamp';

const serverPort = 5678;
const serverUrl = `ws://${window.location.hostname}:${serverPort}`;
const baseConnectionTimeout = 2000; // Milliseconds
const maxConnectionTimeout = 8000; // Milliseconds

export class WebSocketClient {
    private socket!: WebSocket;

    private timeout = baseConnectionTimeout;

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    private callbacks = new Map<WebSocketEvent, Map<string | undefined, Array<(data: any) => void>>>();

    constructor() {
        this.connect();
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    bindMessage(event: WebSocketEvent, callback: (data: any) => void) {
        // undefined represents an event not related to a specific drone
        this.bind(event, undefined, callback);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    bindDroneMessage(event: WebSocketEvent, droneId: string, callback: (data: any) => void) {
        this.bind(event, droneId, callback);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    sendMessage(event: WebSocketEvent, data: any) {
        // undefined represents an event not related to a specific drone
        this.send(event, undefined, data);
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    sendDroneMessage(event: WebSocketEvent, droneId: string, data: any) {
        this.send(event, droneId, data);
    }

    close() {
        this.socket.close();
    }

    private connect() {
        this.socket = new WebSocket(serverUrl);

        this.socket.onmessage = (messageEvent: MessageEvent) => {
            const message: Message = JSON.parse(messageEvent.data);

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
            this.timeout = baseConnectionTimeout;
        };

        this.socket.onclose = () => {
            setTimeout(() => {
                this.connect();
            }, this.timeout);
            this.timeout = Math.min(this.timeout * 2, maxConnectionTimeout);
            console.log(`Connection to ${serverUrl} closed, retrying after ${this.timeout / 1000} seconds`);
        };

        this.socket.onerror = (event: Event) => {
            console.error(`WebSocket encountered an error: ${event}, closing socket`);
            this.socket.close();
        };
    }

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    private bind(event: WebSocketEvent, droneId: string | undefined, callback: (data: any) => void) {
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
    private send(event: WebSocketEvent, droneId: string | undefined, data: any) {
        const payload = JSON.stringify({
            event: event,
            droneId: droneId ?? null,
            data: data,
            timestamp: getLocalTimestamp(),
        });

        this.socket.send(payload);
    }
}