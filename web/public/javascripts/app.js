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
const startToggleButton = document.getElementById('startToggleButton');
const whiteLineToggle = document.getElementById('whiteLineToggle');
const distanceInput = document.getElementById("distanceInput");
const timeInput = document.getElementById("timeInput");
const paceInput = document.getElementById("paceInput");
const currentSpeedDisplay = document.getElementById('current-speed');
const distanceDisplay = document.getElementById('distance');
const averagePaceDisplay = document.getElementById('average-pace');
const timeDisplay = document.getElementById('time');
const speedReadingsDisplay = document.getElementById('speedReadingsDisplay');
const steerReadingsDisplay = document.getElementById('steerReadingsDisplay');
let speedReadings = [];
let steerReadings = [];

const speedKP = 2.5;
const speedKI = 0.55;
const speedKD = 0.01;
const SPEED_MAX_INTEGRAL = 5000
const SPEED_MAX_ACCELERATION = 5000

document.getElementById("speedKPInput").value = speedKP;
document.getElementById("speedKIInput").value = speedKI;
document.getElementById("speedKDInput").value = speedKD;
document.getElementById("SPEED_MAX_INTEGRALInput").value = SPEED_MAX_INTEGRAL;
document.getElementById("SPEED_MAX_ACCELERATIONInput").value = SPEED_MAX_ACCELERATION;

const steerKP = 0.05;
const steerKI = 0.001;
const steerKD = 0.02;
const STEER_MAX_INTEGRAL = 5000


document.getElementById("steerKPInput").value = steerKP;
document.getElementById("steerKIInput").value = steerKI;
document.getElementById("steerKDInput").value = steerKD;
document.getElementById("STEER_MAX_INTEGRALInput").value = STEER_MAX_INTEGRAL;


const ctxSpeed = document.getElementById('speedChart').getContext('2d');
let speedChart = new Chart(ctxSpeed, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Speed',
            data: [],
            borderColor: 'blue',
            fill: false,
            tension: 0.1,
            pointRadius: 0
        }]
    },
    options: {
        scales: {
            x: {
                title: {
                    display: true,
                    text: 'Time (arbitrary units)'
                }
            },
            y: {
                title: {
                    display: true,
                    text: 'Speed'
                }
            }
        }
    }
});

const ctxSteer = document.getElementById('steerChart').getContext('2d');
let steerChart = new Chart(ctxSteer, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Steer',
            data: [],
            borderColor: 'blue',
            fill: false,
            tension: 0.1,
            pointRadius: 0
        }]
    },
    options: {
        scales: {
            x: {
                title: {
                    display: true,
                    text: 'Time (arbitrary units)'
                }
            },
            y: {
                title: {
                    display: true,
                    text: 'Steer'
                }
            }
        }
    }
});


// App state for user-set parameters
export let distance = { value: 0.0, unit: "meters" };
export let time = { value: 0.0, unit: "seconds" };
export let pace = { value: 0.0, unit: "m/s" };
export let mode = "RACE";

// App state for BLE-received data
let currentSpeed = { value: 0.0, unit: "m/s" };
let receivedDistance = { value: 0.0, unit: "meters" };
let averagePace = { value: 0.0, unit: "m/s" };
let elapsedTime = { value: 0.0, unit: "seconds" };

// Other state
let manualControl = false;
let running = false;
let isWhiteLine = false;
let lastUpdated = ["distance", "time"];
let currentX = 90;
let currentY = 1500;
let lastSentX = null;
let lastSentY = null;
let movementRequested = false;

// Log function
export function log(message) {
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
    resetDataUIState();
    currentSpeed.value = 0.0;
    receivedDistance.value = 0.0;
    averagePace.value = 0.0;
    elapsedTime.value = 0.0;
}

export function handleRunStopped(data) {
    running = false;
    startToggleButton.innerText = "GO";
}

// Update BLE-received data state
export function updateDataState(newData) {
    if (newData.currentSpeed && typeof newData.currentSpeed.value === 'number' && newData.currentSpeed.value >= 0) {
        currentSpeed.value = convertToMperS(newData.currentSpeed.value, newData.currentSpeed.unit || "m/s");
        currentSpeed.unit = "m/s";
        speedReadings.push(currentSpeed.value)
        speedReadingsDisplay.innerHTML = [...speedReadings].reverse();

        // Update the chart
        speedChart.data.labels = speedReadings.map((_, index) => index);
        speedChart.data.datasets[0].data = speedReadings;
        speedChart.update();
    }
    if (newData.distance && typeof newData.distance.value === 'number' && newData.distance.value >= 0) {
        receivedDistance.value = convertToMeters(newData.distance.value, newData.distance.unit || "meters");
        receivedDistance.unit = "meters";
    }
    if (newData.averagePace && typeof newData.averagePace.value === 'number' && newData.averagePace.value >= 0) {
        averagePace.value = convertToMperS(newData.averagePace.value, newData.averagePace.unit || "m/s");
        averagePace.unit = "m/s";
    }
    if (newData.time && typeof newData.time.value === 'number' && newData.time.value >= 0) {
        elapsedTime.value = convertToSeconds(newData.time.value, newData.time.unit || "seconds");
        elapsedTime.unit = "seconds";
    }
    if (newData.steeringAngle && typeof newData.steeringAngle === 'number') {
        steerReadings.push(newData.steeringAngle)
        steerReadingsDisplay.innerHTML = [...steerReadings].reverse();
        // Update the chart
        steerChart.data.labels = steerReadings.map((_, index) => index);
        steerChart.data.datasets[0].data = steerReadings;
        steerChart.update();
    }
    updateDataDisplay();
}

// Update display elements with BLE data
function updateDataDisplay() {
    currentSpeedDisplay.innerHTML =
        `${currentSpeed.value.toFixed(2)} m/s<br>` +
        `${mps_to_miph(currentSpeed.value).toFixed(2)} mph<br>` +
        `${mps_to_kmh(currentSpeed.value).toFixed(2)} kmh<br>`;
    distanceDisplay.textContent = receivedDistance.value.toFixed(2);
    distanceDisplay.innerHTML =
        `${receivedDistance.value.toFixed(2)} m<br>` +
        `${m_to_ft(receivedDistance.value).toFixed(2)} ft<br>` +
        `${m_to_mi(receivedDistance.value).toFixed(2)} miles<br>` +
        `${m_to_km(receivedDistance.value).toFixed(2)} km<br>`;
    averagePaceDisplay.innerHTML =
        `${averagePace.value.toFixed(2)} m/s<br>` +
        `${mps_to_miph(averagePace.value).toFixed(2)} mph<br>` +
        `${mps_to_kmh(averagePace.value).toFixed(2)} kmh<br>`;
    timeDisplay.textContent = elapsedTime.value.toFixed(2);
}

// Unit conversion functions
function convertToMeters(distance, units) {
    switch (units.toLowerCase()) {
        case "kilometers":
            return distance * 1000;
        case "miles":
            return distance * 1609.34;
        case "meters":
        default:
            return distance;
    }
}

function convertToSeconds(time, units) {
    switch (units.toLowerCase()) {
        case "minutes":
            return time * 60;
        case "hours":
            return time * 3600;
        case "seconds":
        default:
            return time;
    }
}

function convertToMperS(speed, units) {
    switch (units.toLowerCase()) {
        case "km/h":
            return speed / 3.6;
        case "mph":
            return speed * 0.44704;
        case "m/s":
        default:
            return speed;
    }
}


function mps_to_kmh(speed) {
    return speed * 3.6;
}
function mps_to_miph(speed) {
    return speed * 2.23694;
}

function m_to_km(meters) {
    return meters / 1000;
}
function m_to_mi(meters) {
    return meters / 1609.34;
}
function m_to_ft(meters) {
    return meters * 3.28084;
}



// Request movement update
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


// Reset X axis
function resetXAxis() {
    xSlider.value = 90;
    currentX = 90;
    xValue.textContent = '90';
    requestMovementUpdate();
}

// Reset Y axis
function resetYAxis() {
    ySlider.value = 1500;
    currentY = 1500;
    yValue.textContent = currentY;
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

    if (running && isConnected()) {
        const data = JSON.stringify({
            type: "settings",
            distance: distance.value,
            time: time.value,
            pace: pace.value,
            isWhiteLine: isWhiteLine,
        });
        sendCommand(data, log, true);
        log(`Sent updated settings: ${data}`);
    }
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
    if (document.activeElement !== distanceInput) {
        distanceInput.value = isFinite(distance.value) ? distance.value.toFixed(3) : "";
        distanceInput.style.color = missingValue === "distance" ? "red" : "black";
    }
    if (document.activeElement !== timeInput) {
        timeInput.value = isFinite(time.value) ? time.value.toFixed(3) : "";
        timeInput.style.color = missingValue === "time" ? "red" : "black";
    }
    if (document.activeElement !== paceInput) {
        paceInput.value = isFinite(pace.value) ? pace.value.toFixed(3) : "";
        paceInput.style.color = missingValue === "pace" ? "red" : "black";
    }

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

// Reset UI state
function resetDataUIState() {
    currentSpeedDisplay.textContent = '0.00';
    distanceDisplay.textContent = '0.00';
    averagePaceDisplay.textContent = '0.00';
    timeDisplay.textContent = '0.00';
}

// Event listeners
connectBtn.addEventListener('click', handleConnect);
disconnectBtn.addEventListener('click', handleDisconnect);

xSlider.addEventListener('input', function () {
    currentX = parseInt(this.value);
    xValue.textContent = currentX;
    requestMovementUpdate();
});

ySlider.addEventListener('input', function () {
    currentY = parseInt(this.value);
    yValue.textContent = currentY;
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
        speedReadings = running ? [] : speedReadings
        steerReadings = running ? [] : steerReadings

        const data = JSON.stringify({
            type: "running",
            running: running,
            distance: distanceInput.value,
            time: timeInput.value,
            pace: paceInput.value,
            isWhiteLine: whiteLineToggle.checked,
            mode: document.querySelector('input[name="mode"]:checked')?.value,
            speedKP: document.getElementById("speedKPInput")?.value || speedKP,
            speedKI: document.getElementById("speedKDInput")?.value || speedKI,
            speedKD: document.getElementById("speedKIInput")?.value || speedKD,
            SPEED_MAX_INTEGRAL: document.getElementById("SPEED_MAX_INTEGRALInput")?.value || SPEED_MAX_INTEGRAL,
            SPEED_MAX_ACCELERATION: document.getElementById("SPEED_MAX_ACCELERATIONInput")?.value || SPEED_MAX_ACCELERATION,
            steerKP: document.getElementById("steerKPInput")?.value || steerKP,
            steerKI: document.getElementById("steerKDInput")?.value || steerKI,
            steerKD: document.getElementById("steerKIInput")?.value || steerKD,
            STEER_MAX_INTEGRAL: document.getElementById("STEER_MAX_INTEGRALInput")?.value || STEER_MAX_INTEGRAL,
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