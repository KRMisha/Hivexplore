export function getCurrentTimestamp() {
    const timestampUtc = new Date();
    return new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);
}
