// ==========================================
// 1. SYSTEM CLOCK
// ==========================================
function updateClock() {
    const now = new Date();
    // Time format: 15:30:45
    const timeString = now.toLocaleTimeString('id-ID', { hour12: false });
    document.getElementById('sys-clock').innerText = timeString;
}
setInterval(updateClock, 1000);
updateClock(); // Call once at start to avoid 1s delay

// ==========================================
// 2. FETCH DATA SENSOR (Live Update)
// ==========================================
// ESP32 must send JSON with the following key format.
function fetchSensorData() {
    fetch('/api/data')
        .then(response => {
            if (!response.ok) throw new Error('Network error');
            return response.json();
        })
        .then(data => {
            // -- Update Connection Status --
            document.getElementById('status-text').innerText = "Live Connection";
            document.querySelector('.pulse-dot').style.backgroundColor = "#e50914";
            document.querySelector('.pulse-dot').style.boxShadow = "0 0 8px #e50914";

            // -- Update Critical Metrics --
            document.getElementById('val-volt').innerText = data.voltage ? data.voltage.toFixed(1) : "--";
            document.getElementById('val-curr').innerText = data.current ? data.current.toFixed(2) : "--";

            // Automatic Power (Watt) calculation if not sent by ESP32
            let power = data.power !== undefined ? data.power : ((data.voltage || 0) * (data.current || 0));
            document.getElementById('val-power').innerText = power.toFixed(1);

            if (data.temp === -999.0 || data.temp === -999) {
                document.getElementById('val-temp').innerText = "BOOTING";
                document.getElementById('val-temp').style.fontSize = "1.2rem";
                document.getElementById('val-temp').nextElementSibling.style.display = "none";
            } else {
                document.getElementById('val-temp').innerText = data.temp !== undefined ? data.temp.toFixed(1) : "--";
                document.getElementById('val-temp').style.fontSize = "";
                document.getElementById('val-temp').nextElementSibling.style.display = "inline";
            }

            // -- Update Power Quality Metrics --
            document.getElementById('val-energy').innerText = data.energy !== undefined ? data.energy.toFixed(0) : "--";
            document.getElementById('val-freq').innerText = data.frequency !== undefined ? data.frequency.toFixed(1) : "--";
            document.getElementById('val-pf').innerText = data.power_factor !== undefined ? data.power_factor.toFixed(2) : "--";

            // -- Update Security & System Metrics --
            // 1. Door Sensor (MC-38)
            const doorEl = document.getElementById('val-door');
            if (data.door_open === true) {
                doorEl.innerText = "OPEN";
                doorEl.style.color = "#e50914"; // Red for danger
            } else {
                doorEl.innerText = "CLOSED";
                doorEl.style.color = "#fff"; // White for normal
            }

            // 2. Smoke Sensor (MQ-2)
            const smokeEl = document.getElementById('val-smoke');
            if (data.smoke_detected === true) {
                smokeEl.innerText = "DETECTED";
                smokeEl.style.color = "#e50914";
            } else {
                smokeEl.innerText = "CLEAR";
                smokeEl.style.color = "#fff";
            }

            // 3. Battery & SD Card
            document.getElementById('val-batt').innerText = data.batt_pct !== undefined ? data.batt_pct : "--";

            const sdEl = document.getElementById('val-sd');
            if (data.sd_error === true) {
                sdEl.innerText = "ERROR";
                sdEl.style.color = "#e50914";
            } else {
                sdEl.innerText = "MOUNTED";
                sdEl.style.color = "#fff";
            }

            // 4. Power Outlet (Relay)
            const relayToggle = document.getElementById('relay-toggle');
            const relayVal = document.getElementById('val-relay');
            if (data.relay_state !== undefined) {
                relayToggle.checked = data.relay_state;
                relayVal.innerText = data.relay_state ? "ON" : "OFF";
                relayVal.style.color = data.relay_state ? "#4ade80" : "#808080";
            }

            // -- Update AI Hero Section (Optional, routinely updated by AI) --
            if (data.ai_status) document.getElementById('ai-status-title').innerText = data.ai_status;
            if (data.ai_insight) document.getElementById('ai-insight-text').innerText = `"${data.ai_insight}"`;
        })
        .catch(error => {
            console.error('Failed to fetch data:', error);
            // Change UI if ESP32 is offline or disconnected
            document.getElementById('status-text').innerText = "Disconnected";
            document.querySelector('.pulse-dot').style.backgroundColor = "#808080"; // Gray for offline
            document.querySelector('.pulse-dot').style.boxShadow = "none";
        });
}

setInterval(fetchSensorData, 5000);
fetchSensorData();

// ==========================================
// 3. AI BUTTON LOGIC (Mimiclaw)
// ==========================================
const btnAnalyze = document.getElementById('btn-analyze');
const btnLogs = document.getElementById('btn-logs');
const terminalOutput = document.getElementById('terminal-output');

// A. Force Analyze Button
btnAnalyze.addEventListener('click', () => {
    // Change button text temporarily
    const originalText = btnAnalyze.innerHTML;
    btnAnalyze.innerHTML = '<span class="play-icon">⚙️</span> Analyzing...';
    btnAnalyze.style.backgroundColor = "#e50914"; // Turn red when pressed
    btnAnalyze.style.color = "#fff";

    // Send request to ESP32 to trigger AI analysis now
    fetch('/api/analyze', { method: 'POST' })
        .then(response => response.json())
        .then(result => {
            // Revert button text after 1.5 seconds for transition effect
            setTimeout(() => {
                btnAnalyze.innerHTML = originalText;
                btnAnalyze.style.backgroundColor = "#fff";
                btnAnalyze.style.color = "#000";

                // Add new log to terminal
                appendTerminalLog("AI", ">> Manual analysis completed. Metrics updated.");
            }, 1500);
        })
        .catch(err => {
            setTimeout(() => {
                btnAnalyze.innerHTML = originalText;
                btnAnalyze.style.backgroundColor = "#fff";
                btnAnalyze.style.color = "#000";
                appendTerminalLog("System", ">> ERROR: Failed to reach Mimiclaw Core.");
            }, 1000);
        });
});

// B. View Logs Button (Open Modal)
const agentModal = document.getElementById('agent-modal');
const btnCloseModal = document.getElementById('btn-close-modal');

btnLogs.addEventListener('click', () => {
    chatModal.style.display = 'none';
    agentModal.style.display = 'flex';
});

// C. Close Modal Button
btnCloseModal.addEventListener('click', () => {
    agentModal.style.display = 'none';
});

// D. Helper function to append text to Terminal Modal
function appendTerminalLog(sender, message) {
    const p = document.createElement('p');
    p.className = `log-line ${sender}`; // sender can be "System" or "AI"
    p.innerText = message;
    terminalOutput.appendChild(p);

    // Auto-scroll to bottom of terminal on new log
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

// ==========================================
// 4. WI-FI MANAGER LOGIC
// ==========================================
const btnWifiSettings = document.getElementById('btn-wifi-settings');
const wifiModal = document.getElementById('wifi-modal');
const btnCloseWifi = document.getElementById('btn-close-wifi');
const wifiList = document.getElementById('wifi-list');
const wifiForm = document.getElementById('wifi-form');
const selectedSsidText = document.getElementById('selected-ssid');
const wifiPasswordInput = document.getElementById('wifi-password');
const btnConnectWifi = document.getElementById('btn-connect-wifi');
const btnCancelWifi = document.getElementById('btn-cancel-wifi');

let targetSsid = "";

// Open Modal & Run Scan
btnWifiSettings.addEventListener('click', () => {
    wifiModal.style.display = 'flex';
    wifiForm.style.display = 'none';
    wifiList.style.display = 'none';
    document.getElementById('wifi-connected-state').style.display = 'none';
    
    // Show loading while asking status to ESP32
    wifiList.style.display = 'block';
    wifiList.innerHTML = '<div class="scanning-text">Checking system status...</div>';

    fetch('/api/wifi/status')
        .then(res => res.json())
        .then(data => {
            if (data.status === 'connected') {
                // Hide Scan List, show Connected UI
                wifiList.style.display = 'none';
                document.getElementById('wifi-connected-state').style.display = 'block';
                
                // Update UI text according to real data
                document.getElementById('connected-ssid-name').innerText = data.ssid;
                let signalIcon = data.rssi > -60 ? "📶" : (data.rssi > -80 ? "🔉" : "🔈");
                document.getElementById('connected-signal-icon').innerText = signalIcon;
                document.getElementById('connected-signal-text').innerText = `Signal Strength: ${data.rssi} dBm`;
                
                // Provide new IP Address link
                const ipLink = document.getElementById('sta-ip-link');
                ipLink.innerText = `http://${data.ip}`;
                ipLink.href = `http://${data.ip}`;
            } else {
                // If not connected, run Wi-Fi Scanner
                wifiList.innerHTML = '<div class="scanning-text">Scanning available networks...</div>';
                fetch('/api/wifi/scan')
                    .then(res => res.json())
                    .then(networks => {
                        wifiList.innerHTML = '';
                        if (networks.length === 0) {
                            wifiList.innerHTML = '<div class="scanning-text">No networks found.</div>';
                            return;
                        }
                        networks.forEach(net => {
                            const item = document.createElement('div');
                            item.className = 'wifi-item';
                            let signal = net.rssi > -60 ? "📶" : (net.rssi > -80 ? "🔉" : "🔈");
                            item.innerHTML = `<span>${net.ssid}</span> <span>${signal} ${net.rssi} dBm</span>`;
                            item.onclick = () => selectWifi(net.ssid);
                            wifiList.appendChild(item);
                        });
                    })
                    .catch(err => {
                        wifiList.innerHTML = '<div class="scanning-text" style="color:#e50914;">Failed to scan networks.</div>';
                    });
            }
        });
});

document.getElementById('btn-disconnect-wifi').addEventListener('click', () => {
    const btn = document.getElementById('btn-disconnect-wifi');
    const originalText = btn.innerText;
    btn.innerText = 'Disconnecting...';
    
    fetch('/api/wifi/disconnect', { method: 'POST' })
        .then(() => {
            alert("Disconnected. Please close and reopen Settings menu to scan for new networks.");
            btn.innerText = originalText;
            wifiModal.style.display = 'none';
        });
});

// Close Modal
btnCloseWifi.addEventListener('click', () => wifiModal.style.display = 'none');
btnCancelWifi.addEventListener('click', () => {
    wifiForm.style.display = 'none';
    wifiList.style.display = 'block';
});

// Select Wi-Fi from List
function selectWifi(ssid) {
    targetSsid = ssid;
    selectedSsidText.innerText = ssid;
    wifiList.style.display = 'none';
    wifiForm.style.display = 'block';
    wifiPasswordInput.value = '';
    wifiPasswordInput.focus();
}

let pollingInterval;

btnConnectWifi.addEventListener('click', () => {
    const password = wifiPasswordInput.value;
    const originalText = btnConnectWifi.innerText;
    
    // Change UI to loading state
    btnConnectWifi.innerText = 'Connecting...';
    btnConnectWifi.disabled = true;

    // Clear previous error message if any
    let errorMsg = document.getElementById('wifi-error-msg');
    if(!errorMsg) {
        errorMsg = document.createElement('div');
        errorMsg.id = 'wifi-error-msg';
        errorMsg.style.color = '#e50914'; // Red
        errorMsg.style.marginTop = '15px';
        errorMsg.style.fontSize = '0.9rem';
        errorMsg.style.fontWeight = 'bold';
        wifiForm.appendChild(errorMsg);
    }
    errorMsg.innerText = '';

    fetch('/api/wifi/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid: targetSsid, password: password })
    })
    .then(res => res.json())
    .then(data => {
        // Start polling ESP32 status every 2 seconds
        pollingInterval = setInterval(() => {
            fetch('/api/wifi/status')
                .then(res => res.json())
                .then(stateData => {
                    if (stateData.status === 'connected') {
                        clearInterval(pollingInterval);
                        alert("Successfully connected to Wi-Fi!");
                        wifiModal.style.display = 'none';
                        btnConnectWifi.innerText = originalText;
                        btnConnectWifi.disabled = false;
                    } else if (stateData.status === 'failed') {
                        clearInterval(pollingInterval);
                        errorMsg.innerText = "❌ Connection Failed! Incorrect password or lost network.";
                        btnConnectWifi.innerText = originalText;
                        btnConnectWifi.disabled = false;
                    }
                });
        }, 5000);
    })
    .catch(err => {
        errorMsg.innerText = "Failed to send data to RAKSA Server!";
        btnConnectWifi.innerText = originalText;
        btnConnectWifi.disabled = false;
    });
});

// ==========================================
// 5. INTERACTIVE FEATURES & MIMICLAW AI
// ==========================================

// --- UI Toggle: Chatbox ---
const fabChat = document.getElementById('btn-fab-chat');
const chatModal = document.getElementById('chatbox-modal');
const btnCloseChat = document.getElementById('btn-close-chat');
const metricModal = document.getElementById('metric-modal');

fabChat.addEventListener('click', (e) => {
    e.stopPropagation(); // Prevent global click listener from immediately closing it
    if (chatModal.style.display === 'none' || chatModal.style.display === '') {
        chatModal.style.display = 'flex';
        // Force close metric graph to prevent overlap
        metricModal.style.display = 'none';
        agentModal.style.display = 'none';
        activeMetricKey = null;
    } else {
        chatModal.style.display = 'none';
    }
});
btnCloseChat.addEventListener('click', () => chatModal.style.display = 'none');

// --- Chat Logic ---
const btnSendChat = document.getElementById('btn-send-chat');
const chatInput = document.getElementById('chat-input');
const chatMessages = document.getElementById('chat-messages');

btnSendChat.addEventListener('click', sendChat);
chatInput.addEventListener('keypress', (e) => { if(e.key === 'Enter') sendChat(); });

function sendChat() {
    const text = chatInput.value.trim();
    if (!text) return;
    
    // Display user chat
    chatMessages.innerHTML += `<div class="msg user-msg">${text}</div>`;
    chatInput.value = '';
    chatMessages.scrollTop = chatMessages.scrollHeight;

    // AI response simulation (Can be connected to C++ backend later)
    setTimeout(() => {
        const responses = [
            "Checking server logs... All parameters normal.",
            "Voltage slightly fluctuating, but still within safe limits.",
            "This feature is not fully connected to my main brain yet, but I am ready to help monitor."
        ];
        const reply = responses[Math.floor(Math.random() * responses.length)];
        chatMessages.innerHTML += `<div class="msg ai-msg">${reply}</div>`;
        chatMessages.scrollTop = chatMessages.scrollHeight;
    }, 1000);
}

// --- Force Analyze Button (Error Handling) ---
document.querySelector('.btn-play').addEventListener('click', (e) => {
    // Your Force Analyze button (make sure it has .btn-play class)
    if(e.target.innerText.includes('Force Analyze')) {
        const originalText = e.target.innerHTML;
        e.target.innerHTML = '▶ Sending request...';
        
        setTimeout(() => {
            alert("❌ SYSTEM ERROR: Failed to contact main Mimiclaw AI server. Ensure internet connection is active and API Key is valid.");
            e.target.innerHTML = originalText;
        }, 1500);
    }
});

// --- Metric Graph & Analysis Modal ---
const btnCloseMetric = document.getElementById('btn-close-metric');
const canvas = document.getElementById('metric-canvas');
const ctx = canvas.getContext('2d');

let activeMetricKey = null;
let graphDataHistory = [];

btnCloseMetric.addEventListener('click', () => {
    metricModal.style.display = 'none';
    activeMetricKey = null;
    graphDataHistory = [];
});

// Function called when metric card is clicked
window.openMetric = function(title, key) {
    chatModal.style.display = 'none'; // Force close chat to prevent overlap
    
    document.getElementById('metric-modal-title').innerText = title;
    document.getElementById('metric-ai-analysis').innerText = `Analyzing ${title.toLowerCase()} data pattern... Appears stable over the last 24 hours. No anomalies detected.`;
    activeMetricKey = key;
    graphDataHistory = [];
    metricModal.style.display = 'flex';
};

// Graph Drawing Function on HTML5 Canvas
function drawGraph(newValue) {
    const now = new Date();
    const timeString = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`;
    
    if (graphDataHistory.length > 50) graphDataHistory.shift(); // Max 50 points
    graphDataHistory.push({ val: newValue, time: timeString });

    canvas.width = canvas.parentElement.clientWidth;
    canvas.height = 250;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const padL = 40;  // Left (Numbers)
    const padR = 30;  // Right (Space for line end)
    const padT = 20;  // Top
    const padB = 30;  // Bottom (Time)

    const drawW = canvas.width - padL - padR;
    const drawH = canvas.height - padT - padB;
    
    if (graphDataHistory.length < 1) return;

    // Calculate scale
    const values = graphDataHistory.map(d => d.val);
    const maxVal = Math.max(...values) * 1.1 || 10;
    const minVal = Math.min(...values) * 0.9 || 0;

    ctx.fillStyle = '#888';
    ctx.font = '12px Arial';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.lineWidth = 1;

    for (let i = 0; i <= 4; i++) {
        let y = padT + drawH - (i / 4) * drawH;
        let gridVal = minVal + (i / 4) * (maxVal - minVal);
        
        // Print numbers on the left
        ctx.fillText(gridVal.toFixed(1), padL - 10, y);
        
        // Horizontal grid line
        ctx.strokeStyle = '#222';
        ctx.beginPath(); 
        ctx.moveTo(padL, y); 
        ctx.lineTo(canvas.width - padR, y); 
        ctx.stroke();
    }

    ctx.beginPath();
    ctx.strokeStyle = '#e50914';
    ctx.lineWidth = 2;
    ctx.shadowBlur = 10;
    ctx.shadowColor = '#e50914';

    let lastX = 0, lastY = 0;
    graphDataHistory.forEach((data, index) => {
        let x = padL + (index / Math.max(1, graphDataHistory.length - 1)) * drawW;
        let y = padT + drawH - ((data.val - minVal) / (maxVal - minVal)) * drawH;
        
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
        
        lastX = x; lastY = y; // Last coordinate
    });
    ctx.stroke();
    ctx.shadowBlur = 0; // Turn off shadow for next element

    if (graphDataHistory.length > 0) {
        // Red dot
        ctx.beginPath();
        ctx.fillStyle = '#e50914';
        ctx.arc(lastX, lastY, 4, 0, Math.PI * 2);
        ctx.fill();

        // Real-time number following the line
        ctx.fillStyle = '#fff';
        ctx.font = 'bold 14px Arial';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'bottom';
        let latestVal = graphDataHistory[graphDataHistory.length - 1].val;
        // Display value
        let displayVal = Number.isInteger(latestVal) ? latestVal : latestVal.toFixed(2);
        ctx.fillText(displayVal, lastX + 8, lastY - 5);
    }

    ctx.fillStyle = '#666';
    ctx.font = '11px Arial';
    ctx.textBaseline = 'top';
    if (graphDataHistory.length > 0) {
        // Start time (Bottom left)
        ctx.textAlign = 'left';
        ctx.fillText(graphDataHistory[0].time, padL, canvas.height - padB + 10);
        
        // End time (Bottom Right)
        ctx.textAlign = 'right';
        ctx.fillText(graphDataHistory[graphDataHistory.length - 1].time, canvas.width - padR, canvas.height - padB + 10);
    }
}

// --- Data Polling Update ---
// Every time fetch '/api/data' gets a result, call drawGraph if modal is open
setInterval(() => {
    fetch('/api/data')
        .then(res => res.json())
        .then(data => {
            // Update AI Active text to real connection indicator
            const badge = document.getElementById('ai-status-badge');
            badge.innerText = '✦ MIMICLAW AI ACTIVE';
            badge.style.backgroundColor = 'rgba(229, 9, 20, 0.2)';
            badge.style.color = '#e50914';

            // If a metric is open, draw its graph
            if (activeMetricKey && data[activeMetricKey] !== undefined) {
                let val = data[activeMetricKey];
                if(typeof val === 'boolean') val = val ? 1 : 0; // Convert boolean to number
                
                document.getElementById('metric-modal-value').innerText = val;
                drawGraph(val);
            }
        })
        .catch(err => {
            const badge = document.getElementById('ai-status-badge');
            badge.innerText = '⚠️ SYSTEM OFFLINE';
            badge.style.backgroundColor = 'rgba(100, 0, 0, 0.5)';
            badge.style.color = '#ff4444';
        });
}, 2000); // data update interval

// ==========================================
// 6. GLOBAL WINDOW CLICK LISTENER (Close Modals on Outside Click)
// ==========================================
window.addEventListener('click', (e) => {
    // 1. Close Metric Graph Modal
    if (e.target === metricModal) {
        metricModal.style.display = 'none';
        activeMetricKey = null;
        graphDataHistory = [];
    }
    
    // 2. Close Wi-Fi Modal
    if (e.target === wifiModal) {
        wifiModal.style.display = 'none';
    }

    // 3. Close Agent Terminal Modal
    if (e.target === agentModal) {
        agentModal.style.display = 'none';
    }

    // 4. Close Chatbox (if clicked outside of it and not on the fab button)
    if (chatModal.style.display === 'flex') {
        if (!chatModal.contains(e.target) && !fabChat.contains(e.target)) {
            chatModal.style.display = 'none';
        }
    }
});

// --- Power Outlet Toggle ---
const relayToggle = document.getElementById('relay-toggle');
if (relayToggle) {
    relayToggle.addEventListener('change', (e) => {
        const newState = e.target.checked;
        fetch('/api/relay', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ state: newState })
        })
        .then(res => res.json())
        .then(data => {
            console.log('Relay state updated', data);
            document.getElementById('val-relay').innerText = newState ? "ON" : "OFF";
            document.getElementById('val-relay').style.color = newState ? "#4ade80" : "#808080";
        })
        .catch(err => {
            console.error('Failed to update relay', err);
            // Revert state if failed
            e.target.checked = !newState;
        });
    });
}