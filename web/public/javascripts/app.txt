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

// BLE objects
let device = null;
let server = null;
let service = null;
let characteristic = null;
let manualControl = false;
let running = false;

// Throttle variables
const throttleInterval = 50; // Adjusted to 50ms
let writeQueue = [];
let isWriting = false;
let sendInterval = null;
let currentX = 0;
let currentY = 0;

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
        startContinuousSending();
    } catch (error) {
        log(`Error: ${error}`);
        disconnect();
    }
}

// Handle disconnection
function onDisconnected() {
    log('Device disconnected');
    stopContinuousSending();
    statusText.textContent = 'Disconnected';
    statusText.className = 'status disconnected';
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
    device = null;
    server = null;
    service = null;
    characteristic = null;
    writeQueue = []; // Clear queue
    isWriting = false;
}

// Disconnect from device
async function disconnect() {
    if (device && device.gatt.connected) {
        try {
            await sendValues(`running$ running:false`, true);
            log('Sent stop command before disconnect');
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

// Send values to ESP32 with queuing
async function sendValues(valueString, isCritical = false) {
    if (!characteristic) {
        log('Error: No characteristic available');
        return;
    }
    writeQueue[isCritical ? 'unshift' : 'push']({ valueString, isCritical });
    if (!isWriting) {
        processWriteQueue();
    }
}

// Process the write queue
async function processWriteQueue() {
    if (writeQueue.length === 0 || !characteristic) {
        isWriting = false;
        return;
    }
    isWriting = true;
    const { valueString, isCritical } = writeQueue.shift();
    try {
        const encoder = new TextEncoder();
        const data = encoder.encode(valueString);
        await characteristic.writeValue(data);
        log(`Sent ${isCritical ? 'critical ' : ''}values: ${valueString}`);
    } catch (error) {
        log(`Error sending data: ${error}`);
        if (isCritical) {
            log(`Retrying critical command: ${valueString}`);
            writeQueue.unshift({ valueString, isCritical });
        }
    } finally {
        isWriting = false;
        processWriteQueue();
    }
}

// Start continuous sending
function startContinuousSending() {
    if (sendInterval) return;
    sendInterval = setInterval(() => {
    }, throttleInterval);
    log('Started continuous sending');
}

// Stop continuous sending
function stopContinuousSending() {
    if (sendInterval) {
        clearInterval(sendInterval);
        sendInterval = null;
        log('Stopped continuous sending');
    }
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
    if (characteristic) {
        sendValues(`movement$ X:${currentX},Y:${currentY}`);
    }
}

// Reset Y axis
function resetYAxis() {
    ySlider.value = 0;
    currentY = 0;
    yValue.textContent = '0';
    updateIndicator();
    if (characteristic) {
        sendValues(`movement$ X:${currentX},Y:${currentY}`);
    }
}

// Event listeners
connectBtn.addEventListener('click', connect);
disconnectBtn.addEventListener('click', disconnect);

xSlider.addEventListener('input', function () {
    currentX = parseInt(this.value);
    xValue.textContent = currentX;
    updateIndicator();
});

ySlider.addEventListener('input', function () {
    currentY = parseInt(this.value);
    yValue.textContent = currentY;
    updateIndicator();
});

// xSlider.addEventListener('mouseup', resetXAxis);
// xSlider.addEventListener('touchend', resetXAxis);
// ySlider.addEventListener('mouseup', resetYAxis);
// ySlider.addEventListener('touchend', resetYAxis);

manualToggle.addEventListener('click', function () {
    try {
        manualControl = !manualControl;
        manualToggle.innerText = manualControl ? "Switch to Pacer" : "Switch to Manual Control";
        sendValues(`manualControl$ manual:${manualControl}`); // Mark as critical
        log(`Manual Control set to ${manualControl}`);
        // document.getElementById('controlsDiv').style.display = manualControl ? 'flex' : 'none';
        // document.getElementById('pacerDiv').style.display = manualControl ? 'none' : 'flex';
    } catch (error) {
        log(`Error toggling manual control: ${error}`);
    }
});

startToggleButton.addEventListener('click', function () {
    try {
        running = !running;
        startToggleButton.innerText = running ? "STOP" : "GO";
        sendValues(`running$ running:${running}`, true); // Start/Stop is critical
        log(`Running set to ${running}`);
    } catch (error) {
        log(`Error toggling running state: ${error}`);
    }
});

// Initial log
log('Web app loaded. Click "Connect to ESP32" to begin.');
log(`Browser: ${navigator.userAgent}`);
log(`Is secure context: ${window.isSecureContext}`);