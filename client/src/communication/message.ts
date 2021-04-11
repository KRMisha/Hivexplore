import { WebSocketEvent } from '@/communication/web-socket-event';

export interface Message {
    event: WebSocketEvent;
    droneId: string | undefined;
    data: any; // eslint-disable-line @typescript-eslint/no-explicit-any
    timestamp: string;
}
