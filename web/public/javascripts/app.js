// Import Bluetooth functionality
import { 
    connect, 
    disconnect, 
    sendCommand, 
    requestMovementUpdate as bleRequestMovementUpdate,
    isConnected
} from './bluetooth.js';

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
const whiteLineToggle = document.getElementById('whiteLineToggle');
const distanceInput = document.getElementById("distanceInput");
const timeInput = document.getElementById("timeInput");
const paceInput = document.getElementById("paceInput");

// App state
let manualControl = false;
let running = false;
let isWhiteLine = false;

// Movement tracking
let currentX = 90;
let currentY = 0;
let lastSentX = null;
let lastSentY = null;
let movementRequested = false;

// Log function
function log(message) {
    const p = document.createElement('p');
    p.textContent = `${new Date().toLocaleTimeString()}: ${message}`;
    logContent.appendChild(p);
    logContent.scrollTop = logContent.scrollHeight;
}

// Connect handler
async function handleConnect() {
    const connected = await connect(log);
    if (connected) {
        statusText.textContent = 'Connected';
        statusText.className = 'status connected';
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
    }
}

// Disconnect handler
async function handleDisconnect() {
    await disconnect(log);
    statusText.textContent = 'Disconnected';
    statusText.className = 'status disconnected';
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
}

// Request movement update (called by UI events)
function requestMovementUpdate() {
    if (isConnected()) {
        const result = bleRequestMovementUpdate(log, manualControl, lastSentX, lastSentY, currentX, currentY);
        lastSentX = result.lastSentX;
        lastSentY = result.lastSentY;
        movementRequested = result.movementRequested;
    } else {
        movementRequested = true;
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
connectBtn.addEventListener('click', handleConnect);
disconnectBtn.addEventListener('click', handleDisconnect);

xSlider.addEventListener('input', function () {
    currentX = parseInt(this.value);
    xValue.textContent = currentX;
    updateIndicator();
    requestMovementUpdate();
});

ySlider.addEventListener('input', function () {
    currentY = parseInt(this.value);
    yValue.textContent = currentY;
    updateIndicator();
    requestMovementUpdate();
});

xSlider.addEventListener('mouseup', resetXAxis);
xSlider.addEventListener('touchend', resetXAxis);
ySlider.addEventListener('mouseup', resetYAxis);
ySlider.addEventListener('touchend', resetYAxis);

manualToggle.addEventListener('click', function () {
    try {
        manualControl = !manualControl;
        manualToggle.innerText = manualControl ? "Switch to Pacer" : "Switch to Manual Control";
        const data = JSON.stringify({
            type: "manualControl",
            enabled: manualControl
        });
        log(`Manual Control ${manualControl ? 'enabled' : 'disabled'}`);
        sendCommand(data, log, true);
        if (manualControl) {
            // Queue an immediate position update after mode change
            movementRequested = true;
        }
    } catch (error) {
        log(`Error toggling manual control: ${error}`);
    }
});

startToggleButton.addEventListener('click', function () {
    try {
        running = !running;
        startToggleButton.innerText = running ? "STOP" : "GO";

        const data = JSON.stringify({
            type: "running",
            running: running,
            distance: distanceInput.value,
            time: timeInput.value,
            pace: paceInput.value,
            isWhiteLine: isWhiteLine,
        });

        sendCommand(data, log, true);
        log(`Running state change requested: ${running}`);
    } catch (error) {
        log(`Error toggling running state: ${error}`);
    }
});

whiteLineToggle.addEventListener('change', function () {
    try {
        isWhiteLine = whiteLineToggle.checked;

        const data = JSON.stringify({
            type: "isWhiteLine",
            enabled: isWhiteLine,
        });
        sendCommand(data, log);
        log(`Set to follow ${isWhiteLine ? "WHITE" : "BLACK"} line`);
    } catch (error) {
        log(`Error toggling white line: ${error}`);
    }
});

// Initial log
log('Web app loaded. Click "Connect to ESP32" to begin.');
log(`Browser: ${navigator.userAgent}`);
log(`Is secure context: ${window.isSecureContext}`);