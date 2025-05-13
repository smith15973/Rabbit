// BLE UUIDs
const ESP32_SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const CONTROL_CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';

// Element references
const connectBtn = document.getElementById('connectBtn');
const disconnectBtn = document.getElementById('disconnectBtn');
const statusText = document.getElementById('statusText');
const logContent = document.getElementById('logContent');
const xSlider = document.getElementById('xSlider');
const ySlider = document.getElementById('ySlider');
const xValue = document.getElementById('xValue');
const yValue = document.getElementById('yValue');
const manualToggle = document.getElementById('manualToggle');
const indicatorDot = document.getElementById('indicatorDot');
const startToggleButton = document.getElementById('startToggleButton');
const whiteLineToggleButton = document.getElementById('whiteLineToggleButton');

// BLE objects
let device = null;
let server = null;
let service = null;
let characteristic = null;
let manualControl = false;
let running = false;
let isWhiteLine = false;

// Command and movement tracking
let currentX = 90;
let currentY = 0;
let pendingOperation = false;  // Tracks if any command is in flight
let movementRequested = false;
let lastSentX = null;
let lastSentY = null;
let commandQueue = [];  // Queue for critical commands

// Log function
function log(message) {
    const p = document.createElement('p');
    p.textContent = `${new Date().toLocaleTimeString()}: ${message}`;
    logContent.appendChild(p);
    logContent.scrollTop = logContent.scrollHeight;
}

// Connect to BLE device
async function connect() {
    try {
        log('Requesting Bluetooth device...');
        device = await navigator.bluetooth.requestDevice({
            filters: [{ services: [ESP32_SERVICE_UUID] }]
        });
        log(`Device selected: ${device.name || 'Unnamed Device'}`);
        device.addEventListener('gattserverdisconnected', onDisconnected);
        log('Connecting to GATT server...');
        server = await device.gatt.connect();
        log('Getting primary service...');
        service = await server.getPrimaryService(ESP32_SERVICE_UUID);
        log('Getting characteristic...');
        characteristic = await service.getCharacteristic(CONTROL_CHARACTERISTIC_UUID);
        statusText.textContent = 'Connected';
        statusText.className = 'status connected';
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
        log('Connected successfully!');
        
        // Reset command state
        pendingOperation = false;
        commandQueue = [];
        lastSentX = null;
        lastSentY = null;
    } catch (error) {
        log(`Error: ${error}`);
        disconnect();
    }
}

// Handle disconnection
function onDisconnected() {
    log('Device disconnected');
    statusText.textContent = 'Disconnected';
    statusText.className = 'status disconnected';
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
    device = null;
    server = null;
    service = null;
    characteristic = null;
    pendingOperation = false;
    commandQueue = [];
}

// Disconnect from device
async function disconnect() {
    if (device && device.gatt.connected) {
        try {
            // Clear any pending commands and immediately stop
            pendingOperation = false;
            commandQueue = [];
            
            // Send stop command directly (bypass queue for disconnect)
            if (characteristic) {
                const encoder = new TextEncoder();
                const data = encoder.encode(`running$ running:false`);
                await characteristic.writeValue(data);
                log('Sent stop command before disconnect');
            }
            
            await new Promise(resolve => setTimeout(resolve, 100));
            device.gatt.disconnect();
        } catch (error) {
            log(`Error during disconnect: ${error}`);
            onDisconnected();
        }
    } else {
        onDisconnected();
    }
}

// Send critical commands (like mode changes)
async function sendCommand(valueString, isCritical = true) {
    if (!characteristic) {
        log('Error: No characteristic available');
        return false;
    }
    
    if (isCritical) {
        // Critical commands go to the front of the queue
        queueCriticalCommand(valueString);
        return true;
    } else {
        // For non-critical commands (though we typically make all commands critical)
        try {
            const encoder = new TextEncoder();
            const data = encoder.encode(valueString);
            await characteristic.writeValue(data);
            log(`Sent command: ${valueString}`);
            return true;
        } catch (error) {
            log(`Error sending command: ${error}`);
            return false;
        }
    }
}

// Process command queue and movement updates with priority handling
async function processCommandQueue() {
    if (!characteristic || pendingOperation) return;
    
    pendingOperation = true;
    
    try {
        // Process critical commands first
        if (commandQueue.length > 0) {
            const command = commandQueue.shift();
            const encoder = new TextEncoder();
            const data = encoder.encode(command.valueString);
            
            await characteristic.writeValue(data);
            log(`Sent critical command: ${command.valueString}`);
            
            // If this was a manual control toggle, reset movement tracking
            if (command.valueString.includes('manualControl$')) {
                lastSentX = null;
                lastSentY = null;
            }
        }
        // Then process movement if no commands and movement is needed
        else if (manualControl && 
                (lastSentX !== currentX || lastSentY !== currentY || movementRequested)) {
            
            movementRequested = false;
            const valueString = `movement$ X:${currentX},Y:${currentY}`;
            const encoder = new TextEncoder();
            const data = encoder.encode(valueString);
            
            const response = await characteristic.writeValue(data);
            console.log(response);
            lastSentX = currentX;
            lastSentY = currentY;
            log(`Sent movement: X:${currentX}, Y:${currentY}`);
        }
    } catch (error) {
        log(`Error sending data: ${error}`);
    } finally {
        pendingOperation = false;
        
        // Check if we need to send more commands
        if (commandQueue.length > 0 || 
            (manualControl && (lastSentX !== currentX || lastSentY !== currentY || movementRequested))) {
            // Schedule next operation after a small delay
            setTimeout(processCommandQueue, 10);
        }
    }
}

// Add a critical command to the queue
function queueCriticalCommand(valueString) {
    commandQueue.push({ valueString });
    processCommandQueue();
}

// Request movement update (called by UI events)
function requestMovementUpdate() {
    movementRequested = true;
    processCommandQueue();
}

// Update visual indicator
function updateIndicator() {
    const dotX = ((currentX + 45) / 90) * 100;
    const dotY = ((100 - currentY) / 200) * 100;
    indicatorDot.style.left = `${dotX}%`;
    indicatorDot.style.top = `${dotY}%`;
}

// Reset X axis
function resetXAxis() {
    xSlider.value = 90;
    currentX = 90;
    xValue.textContent = '90';
    updateIndicator();
    requestMovementUpdate();
}

// Reset Y axis
function resetYAxis() {
    ySlider.value = 0;
    currentY = 0;
    yValue.textContent = '0';
    updateIndicator();
    requestMovementUpdate();
}

// Event listeners
connectBtn.addEventListener('click', connect);
disconnectBtn.addEventListener('click', disconnect);

xSlider.addEventListener('input', function() {
    currentX = parseInt(this.value);
    xValue.textContent = currentX;
    updateIndicator();
    requestMovementUpdate();
});

ySlider.addEventListener('input', function() {
    currentY = parseInt(this.value);
    yValue.textContent = currentY;
    updateIndicator();
    requestMovementUpdate();
});

xSlider.addEventListener('mouseup', resetXAxis);
xSlider.addEventListener('touchend', resetXAxis);
ySlider.addEventListener('mouseup', resetYAxis);
ySlider.addEventListener('touchend', resetYAxis);

manualToggle.addEventListener('click', function() {
    try {
        manualControl = !manualControl;
        manualToggle.innerText = manualControl ? "Switch to Pacer" : "Switch to Manual Control";
        sendCommand(`manualControl$ manual:${manualControl}`, true);
        log(`Manual Control mode change requested: ${manualControl}`);
        if (manualControl) {
            // Queue an immediate position update after mode change
            movementRequested = true;
        }
    } catch (error) {
        log(`Error toggling manual control: ${error}`);
    }
});

startToggleButton.addEventListener('click', function() {
    try {
        running = !running;
        startToggleButton.innerText = running ? "STOP" : "GO";
        sendCommand(`running$ running:${running}`, true);
        log(`Running state change requested: ${running}`);
    } catch (error) {
        log(`Error toggling running state: ${error}`);
    }
});

whiteLineToggleButton.addEventListener('click', function() {
    try {
        isWhiteLine = !isWhiteLine;
        whiteLineToggleButton.style.color = !isWhiteLine ? "white" : "black";
        whiteLineToggleButton.style.backgroundColor = !isWhiteLine ? "black" : "white";
        sendCommand(`isWhiteLine$ isWhiteLine:${isWhiteLine}`);
        log(`White line set to: ${isWhiteLine}`);
    } catch (error) {
        log(`Error toggling white line: ${error}`);
    }
});

// Initial log
log('Web app loaded. Click "Connect to ESP32" to begin.');
log(`Browser: ${navigator.userAgent}`);
log(`Is secure context: ${window.isSecureContext}`);