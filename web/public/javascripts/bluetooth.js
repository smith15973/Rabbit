// BLE UUIDs
const ESP32_SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const CONTROL_CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';

// BLE objects
let device = null;
let server = null;
let service = null;
let characteristic = null;
let pendingOperation = false;  // Tracks if any command is in flight
let commandQueue = [];  // Queue for critical commands

// Connect to BLE device
async function connect(logCallback) {
    try {
        logCallback('Requesting Bluetooth device...');
        device = await navigator.bluetooth.requestDevice({
            filters: [{ services: [ESP32_SERVICE_UUID] }]
        });
        logCallback(`Device selected: ${device.name || 'Unnamed Device'}`);
        device.addEventListener('gattserverdisconnected', () => onDisconnected(logCallback));
        logCallback('Connecting to GATT server...');
        server = await device.gatt.connect();
        logCallback('Getting primary service...');
        service = await server.getPrimaryService(ESP32_SERVICE_UUID);
        logCallback('Getting characteristic...');
        characteristic = await service.getCharacteristic(CONTROL_CHARACTERISTIC_UUID);
        logCallback('Connected successfully!');

        // Reset command state
        pendingOperation = false;
        commandQueue = [];
        
        return true;
    } catch (error) {
        logCallback(`Error: ${error}`);
        disconnect(logCallback);
        return false;
    }
}

// Handle disconnection
function onDisconnected(logCallback) {
    logCallback('Device disconnected');
    device = null;
    server = null;
    service = null;
    characteristic = null;
    pendingOperation = false;
    commandQueue = [];
}

// Disconnect from device
async function disconnect(logCallback) {
    if (device && device.gatt.connected) {
        try {
            // Clear any pending commands and immediately stop
            pendingOperation = false;
            commandQueue = [];

            // Send stop command directly (bypass queue for disconnect)
            if (characteristic) {
                const stopData = JSON.stringify({
                    type: "running",
                    running: false
                });
                const encoder = new TextEncoder();
                const data = encoder.encode(stopData);
                await characteristic.writeValue(data);
                logCallback('Sent stop command before disconnect');
            }

            await new Promise(resolve => setTimeout(resolve, 100));
            device.gatt.disconnect();
        } catch (error) {
            logCallback(`Error during disconnect: ${error}`);
            onDisconnected(logCallback);
        }
    } else {
        onDisconnected(logCallback);
    }
}

async function sendCommand(valueData, logCallback, isCritical = false) {
    if (!characteristic) {
        logCallback('Error: No characteristic available');
        return false;
    }

    // Convert to JSON string if object is passed
    let dataToSend = valueData;
    if (typeof valueData === 'object') {
        dataToSend = JSON.stringify(valueData);
    }

    if (isCritical) {
        // Critical commands go to the front of the queue
        queueCriticalCommand(dataToSend, logCallback);
        return true;
    } else {
        // For non-critical commands
        try {
            const encoder = new TextEncoder();
            const data = encoder.encode(dataToSend);
            await characteristic.writeValue(data);
            logCallback(`Sent command: ${dataToSend}`);
            return true;
        } catch (error) {
            logCallback(`Error sending command: ${error}`);
            return false;
        }
    }
}

// Add a critical command to the queue
function queueCriticalCommand(valueString, logCallback) {
    commandQueue.push({ valueString });
    processCommandQueue(logCallback);
}

// Process command queue and movement updates with priority handling
async function processCommandQueue(logCallback, manualControl, lastSentX, lastSentY, currentX, currentY, movementRequested) {
    if (!characteristic || pendingOperation) return { lastSentX, lastSentY, movementRequested };

    pendingOperation = true;
    let updatedLastSentX = lastSentX;
    let updatedLastSentY = lastSentY;
    let updatedMovementRequested = movementRequested;

    try {
        // Process critical commands first
        if (commandQueue.length > 0) {
            const command = commandQueue.shift();
            const encoder = new TextEncoder();
            const data = encoder.encode(command.valueString);

            await characteristic.writeValue(data);
            logCallback(`Sent critical command: ${command.valueString}`);

            // If this was a manual control toggle, reset movement tracking
            if (command.valueString.includes('"type":"manualControl"')) {
                updatedLastSentX = null;
                updatedLastSentY = null;
            }
        }
        // Then process movement if no commands and movement is needed
        else if (manualControl &&
            (lastSentX !== currentX || lastSentY !== currentY || movementRequested)) {

            updatedMovementRequested = false;
            const movementData = JSON.stringify({
                type: "movement",
                angle: currentX,
                motorSpeed: currentY
            });

            const encoder = new TextEncoder();
            const data = encoder.encode(movementData);

            await characteristic.writeValue(data);
            updatedLastSentX = currentX;
            updatedLastSentY = currentY;
            logCallback(`Sent movement: X:${currentX}, Y:${currentY}`);
        }
    } catch (error) {
        logCallback(`Error sending data: ${error}`);
    } finally {
        pendingOperation = false;

        // Check if we need to send more commands
        if (commandQueue.length > 0 ||
            (manualControl && (updatedLastSentX !== currentX || updatedLastSentY !== currentY || updatedMovementRequested))) {
            // Schedule next operation after a small delay
            setTimeout(() => processCommandQueue(
                logCallback, 
                manualControl, 
                updatedLastSentX, 
                updatedLastSentY, 
                currentX, 
                currentY, 
                updatedMovementRequested
            ), 10);
        }
    }

    return { 
        lastSentX: updatedLastSentX, 
        lastSentY: updatedLastSentY, 
        movementRequested: updatedMovementRequested 
    };
}

// Request movement update
function requestMovementUpdate(logCallback, manualControl, lastSentX, lastSentY, currentX, currentY) {
    const movementRequested = true;
    return processCommandQueue(logCallback, manualControl, lastSentX, lastSentY, currentX, currentY, movementRequested);
}

// Check connection status
function isConnected() {
    return device !== null && characteristic !== null;
}

// Export functions for use in app.js
export {
    connect,
    disconnect,
    sendCommand,
    requestMovementUpdate,
    processCommandQueue,
    isConnected
};