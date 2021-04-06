export interface Message {
    event: string;
    droneId: string | undefined;
    data: any; // eslint-disable-line @typescript-eslint/no-explicit-any
    timestamp: string;
}
