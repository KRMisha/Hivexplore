export function getLocalTimestamp(): string {
    const timestampUtc = new Date();
    const timestampLocalUnfiltered = new Date(timestampUtc.getTime() - timestampUtc.getTimezoneOffset() * 60 * 1000);
    return timestampLocalUnfiltered.toISOString().replace('Z', ''); // Remove the trailing Z since the timestamp is not in UTC
}
