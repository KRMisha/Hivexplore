export function getLocalTimestamp(): string {
    const utcTimestamp = new Date();
    const localTimestamp = new Date(utcTimestamp.getTime() - utcTimestamp.getTimezoneOffset() * 60 * 1000);
    return localTimestamp.toISOString().replace('Z', ''); // Remove the trailing Z since the timestamp is not in UTC
}
