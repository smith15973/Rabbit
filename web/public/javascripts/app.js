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
let lastUpdated = ["distance", "time"];
let distance = { value: 0.0, unit: "meters" };
let time = { value: 0.0, unit: "seconds" };
let pace = { value: 0.0, unit: "m/s" };

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

// update distance, time, and pace values accordingly
//determine what the current value being changed is
// replace the least recently updated value with the new calculated value
function handleDTPInput(event) {
    const name = event.currentTarget.name;
    const inputValue = event.currentTarget.value;

    // Handle empty input or non-numeric values
    const value = inputValue === "" ? 0 : parseFloat(inputValue);

    // Check if the parsed value is valid
    if (isNaN(value) || value < 0) {
        console.warn(`Invalid input for ${name}: ${inputValue}`);
        return; // Don't process invalid inputs
    }

    // Update the corresponding value
    if (name === "distance") {
        distance.value = value;
    } else if (name === "time") {
        time.value = value;
    } else if (name === "pace") {
        pace.value = value;
    }

    // Update the lastUpdated array
    // Remove the input if it's already in the array
    lastUpdated = lastUpdated.filter(item => item !== name);
    // Add the current input to the end (most recently updated)
    lastUpdated.push(name);

    // If we have more than 2 items, remove the oldest one
    if (lastUpdated.length > 2) {
        lastUpdated.shift();
    }

    // Calculate the missing value based on the two most recently updated values
    let missingValue = updateMissingValue();

    // Update the UI
    updateUI(missingValue);
}

function updateMissingValue() {
    // The missing value is the one not in the lastUpdated array
    const allValues = ["distance", "time", "pace"];
    const missingValue = allValues.find(value => !lastUpdated.includes(value));

    // Calculate the missing value
    try {
        if (missingValue === "distance") {
            distance.value = calculateDistance(time.value, pace.value);
        } else if (missingValue === "time") {
            time.value = calculateTime(distance.value, pace.value);
        } else if (missingValue === "pace") {
            pace.value = calculatePace(distance.value, time.value);
        }
        return missingValue;
    } catch (error) {
        console.error("Calculation error:", error);
        // Keep the previous value if there's an error
    }
}

function updateUI(missingValue) {
    // Format values for display and handle potential NaN or Infinity
    distanceInput.value = isFinite(distance.value) ? distance.value : "";
    timeInput.value = isFinite(time.value) ? time.value : "";
    paceInput.value = isFinite(pace.value) ? pace.value : "";

    distanceInput.style.color = "black";
    timeInput.style.color = "black";
    paceInput.style.color = "black";
    
    if (missingValue === "distance") {
        distanceInput.style.color = "red";
    } else if (missingValue === "time") {
        timeInput.style.color = "red";
    } else if (missingValue === "pace") {
        paceInput.style.color = "red";
    }

    console.log("Last updated:", lastUpdated);
}

function calculateDistance(time, pace) {
    if (time <= 0 || pace <= 0) {
        return 0; // Default value for invalid inputs
    }
    return time * pace;
}

function calculateTime(distance, pace) {
    if (distance <= 0 || pace <= 0) {
        return 0; // Default value for invalid inputs
    }
    return distance / pace;
}

function calculatePace(distance, time) {
    if (distance <= 0 || time <= 0) {
        return 0; // Default value for invalid inputs
    }
    return distance / time;
}

function convertToMeters(distance, units) {

}
function convertToSeconds(time, units) {

}
function convertToMperS(speed, units) {

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

distanceInput.addEventListener('input', (e) => handleDTPInput(e));
timeInput.addEventListener('input', (e) => handleDTPInput(e));
paceInput.addEventListener('input', (e) => handleDTPInput(e));

// Initial log
log('Web app loaded. Click "Connect to ESP32" to begin.');
log(`Browser: ${navigator.userAgent}`);
log(`Is secure context: ${window.isSecureContext}`);