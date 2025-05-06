// BLE UUIDs (You'll need to match these in your ESP32 code)
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
const resetBoth = document.getElementById('resetBoth');
const indicatorDot = document.getElementById('indicatorDot');

// BLE objects
let device = null;
let server = null;
let service = null;
let characteristic = null;

// Throttle variables
let lastSentTime = 0;
const throttleInterval = 50; // 50ms for smoother continuous sending

// Current values
let currentX = 0;
let currentY = 0;

// Interval ID for continuous sending
let sendInterval = null;

// Log function
function log(message) {
    const p = document.createElement('p');
    p.textContent = `${new Date().toLocaleTimeString()}: ${message}`;
    logContent.appendChild(p);
    logContent.scrollTop = logContent.scrollHeight;
}

// Connect to BLE device
async function connect() {
    console.log("HEY")
    try {
        log('Requesting Bluetooth device...');

        // Request device with specified service
        device = await navigator.bluetooth.requestDevice({
            filters: [{
                services: [ESP32_SERVICE_UUID]
            }]
        });

        log(`Device selected: ${device.name || 'Unnamed Device'}`);

        // Add event listener for disconnection
        device.addEventListener('gattserverdisconnected', onDisconnected);

        // Connect to GATT server
        log('Connecting to GATT server...');
        server = await device.gatt.connect();

        // Get service
        log('Getting primary service...');
        service = await server.getPrimaryService(ESP32_SERVICE_UUID);

        // Get characteristic
        log('Getting characteristic...');
        characteristic = await service.getCharacteristic(CONTROL_CHARACTERISTIC_UUID);

        // Update UI
        statusText.textContent = 'Connected';
        statusText.className = 'status connected';
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;

        log('Connected successfully!');

        // Start continuous sending
        startContinuousSending();

    } catch (error) {
        log(`Error: ${error}`);
        disconnect();
    }
}

// Handle disconnection
function onDisconnected() {
    log('Device disconnected');

    // Stop continuous sending
    stopContinuousSending();

    // Update UI
    statusText.textContent = 'Disconnected';
    statusText.className = 'status disconnected';
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;

    // Clear variables
    device = null;
    server = null;
    service = null;
    characteristic = null;
}

// Disconnect from device
function disconnect() {
    if (device && device.gatt.connected) {
        device.gatt.disconnect();
    } else {
        onDisconnected();
    }
}

// Send values to ESP32
async function sendValues() {
    if (!characteristic) {
        return;
    }

    // Throttle sending data
    const now = Date.now();
    if (now - lastSentTime < throttleInterval) {
        return;
    }
    lastSentTime = now;

    try {
        // Format: "X:value,Y:value"
        const valueString = `X:${currentX},Y:${currentY}`;

        // Convert string to ArrayBuffer
        const encoder = new TextEncoder();
        const data = encoder.encode(valueString);

        // Write value to characteristic
        await characteristic.writeValue(data);
        log(`Sent values: ${valueString}`);

    } catch (error) {
        log(`Error sending data: ${error}`);
    }
}

// Start continuous sending
function startContinuousSending() {
    if (sendInterval) {
        return; // Already running
    }
    sendInterval = setInterval(() => {
        sendValues();
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
    // Map -100,100 to 0%,100% for CSS positioning
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
        sendValues();
    }
}

// Reset Y axis
function resetYAxis() {
    ySlider.value = 0;
    currentY = 0;
    yValue.textContent = '0';
    updateIndicator();
    if (characteristic) {
        sendValues();
    }
}

// // Add event listeners
connectBtn.addEventListener('click', () => {console.log("HEY"); connect()});
disconnectBtn.addEventListener('click', disconnect);

// Slider events
xSlider.addEventListener('input', function () {
    currentX = parseInt(this.value);
    xValue.textContent = currentX;
    updateIndicator();
    // Values will be sent by continuous sending
});

ySlider.addEventListener('input', function () {
    currentY = parseInt(this.value);
    yValue.textContent = currentY;
    updateIndicator();
    // Values will be sent by continuous sending
});

// Return to center on release
xSlider.addEventListener('mouseup', resetXAxis);
xSlider.addEventListener('touchend', resetXAxis);

ySlider.addEventListener('mouseup', resetYAxis);
ySlider.addEventListener('touchend', resetYAxis);

// Reset buttons
resetBoth.addEventListener('click', function () {
    resetXAxis();
    resetYAxis();
});

// Initial log
log('Web app loaded. Click "Connect to ESP32" to begin.');
log(`Browser: ${navigator.userAgent}`);
log(`Is secure context: ${window.isSecureContext}`);