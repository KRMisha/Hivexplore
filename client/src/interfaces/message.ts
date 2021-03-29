export interface Message {
    event: string;
    droneId: string | undefined;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    data: any;
    timestamp: string;
}
