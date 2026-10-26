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
    const cm = document.getElementById('chatbox-modal');
    if (cm) cm.style.display = 'none';
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
// 5. METRIC GRAPH & ANALYSIS MODAL
// ==========================================
const metricModal = document.getElementById('metric-modal');

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
    const cm = document.getElementById('chatbox-modal');
    if (cm) cm.style.display = 'none'; // Force close chat to prevent overlap
    
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
    const cm = document.getElementById('chatbox-modal');
    const fab = document.getElementById('btn-fab-chat');
    if (cm && cm.style.display === 'flex') {
        if (!cm.contains(e.target) && fab && !fab.contains(e.target)) {
            cm.style.display = 'none';
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

// ==========================================
// 7. MIMICLAW AI WEBSOCKET CHAT
// ==========================================
const chatModal = document.getElementById('chatbox-modal');
const fabChat = document.getElementById('btn-fab-chat');
const btnCloseChat = document.getElementById('btn-close-chat');
const chatMessages = document.getElementById('chat-messages');
const chatInput = document.getElementById('chat-input');
const btnSendChat = document.getElementById('btn-send-chat');
const chatWsStatus = document.getElementById('chat-ws-status');
const chatQuickChips = document.getElementById('chat-quick-chips');

let chatWs = null;
let wsReconnectTimer = null;

function escapeHtml(str) {
    return str
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

function formatMarkdown(text) {
    if (!text) return "";
    let safe = escapeHtml(text);
    // Bold: **text**
    safe = safe.replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>');
    // Italic: *text*
    safe = safe.replace(/(^|[^*])\*([^*]+)\*/g, '$1<em>$2</em>');
    // Inline code: `text`
    safe = safe.replace(/`([^`]+)`/g, '<code>$1</code>');
    // Bullet list items
    safe = safe.replace(/(?:^|\n)[ \t]*[-*][ \t]+(.*)/g, '<br>• $1');
    // Line breaks
    safe = safe.replace(/\n/g, '<br>');
    return safe;
}

function showTypingIndicator() {
    if (document.getElementById('typing-indicator')) return;
    const typing = document.createElement('div');
    typing.id = 'typing-indicator';
    typing.className = 'typing-indicator';
    typing.innerHTML = '<span class="dot"></span><span class="dot"></span><span class="dot"></span>';
    chatMessages.appendChild(typing);
    chatMessages.scrollTop = chatMessages.scrollHeight;
}

function hideTypingIndicator() {
    const typing = document.getElementById('typing-indicator');
    if (typing) {
        typing.remove();
    }
}

function appendChatMessage(role, content) {
    hideTypingIndicator();
    const msgDiv = document.createElement('div');
    msgDiv.className = `msg ${role === 'user' ? 'user-msg' : 'ai-msg'}`;
    if (role === 'user') {
        msgDiv.textContent = content;
    } else {
        msgDiv.innerHTML = formatMarkdown(content);
    }
    chatMessages.appendChild(msgDiv);
    chatMessages.scrollTop = chatMessages.scrollHeight;
}

function updateWsStatus(online) {
    if (!chatWsStatus) return;
    if (online) {
        chatWsStatus.innerText = 'Online';
        chatWsStatus.classList.remove('offline');
        chatWsStatus.classList.add('online');
    } else {
        chatWsStatus.innerText = 'Offline';
        chatWsStatus.classList.remove('online');
        chatWsStatus.classList.add('offline');
    }
}

function connectChatWebSocket() {
    if (chatWs && (chatWs.readyState === WebSocket.OPEN || chatWs.readyState === WebSocket.CONNECTING)) {
        return;
    }

    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${location.host}/ws/chat`;

    try {
        chatWs = new WebSocket(wsUrl);

        chatWs.onopen = () => {
            console.log('[MimiClaw WS] Connected to', wsUrl);
            updateWsStatus(true);
            if (wsReconnectTimer) {
                clearTimeout(wsReconnectTimer);
                wsReconnectTimer = null;
            }
        };

        chatWs.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                if (data.type === 'status' && data.status === 'thinking') {
                    showTypingIndicator();
                } else if (data.type === 'chat' && data.role === 'ai') {
                    appendChatMessage('ai', data.content);
                } else if (data.type === 'pong') {
                    // Heartbeat response from ESP32
                } else if (data.content) {
                    appendChatMessage('ai', data.content);
                }
            } catch (err) {
                appendChatMessage('ai', event.data);
            }
        };

        chatWs.onclose = () => {
            console.warn('[MimiClaw WS] Connection closed. Retrying in 3s...');
            updateWsStatus(false);
            hideTypingIndicator();
            scheduleWsReconnect();
        };

        chatWs.onerror = (err) => {
            console.error('[MimiClaw WS] Connection error:', err);
            updateWsStatus(false);
            hideTypingIndicator();
            try { chatWs.close(); } catch(e) {}
        };
    } catch (e) {
        console.error('[MimiClaw WS] Init failed:', e);
        updateWsStatus(false);
        scheduleWsReconnect();
    }
}

function scheduleWsReconnect() {
    if (!wsReconnectTimer) {
        wsReconnectTimer = setTimeout(() => {
            wsReconnectTimer = null;
            connectChatWebSocket();
        }, 3000);
    }
}

// 30-second keep-alive ping to prevent NAT/router socket timeouts
setInterval(() => {
    if (chatWs && chatWs.readyState === WebSocket.OPEN) {
        chatWs.send(JSON.stringify({ type: 'ping' }));
    }
}, 30000);

function sendUserMessage(customText) {
    const text = (customText !== undefined ? customText : (chatInput ? chatInput.value : '')).trim();
    if (!text) return;

    appendChatMessage('user', text);
    if (chatInput) chatInput.value = '';
    showTypingIndicator();

    if (!chatWs || chatWs.readyState !== WebSocket.OPEN) {
        setTimeout(() => {
            appendChatMessage('ai', '⚠️ *Koneksi ke ESP32 terputus.* Sedang mencoba menghubungkan kembali...');
            connectChatWebSocket();
        }, 500);
        return;
    }

    const payload = JSON.stringify({
        type: 'chat',
        message: text
    });
    chatWs.send(payload);
}

// Event Listeners with proper propagation stop and modal mutual exclusion
if (fabChat) {
    fabChat.addEventListener('click', (e) => {
        e.stopPropagation();
        const isHidden = chatModal.style.display === 'none' || !chatModal.style.display;
        chatModal.style.display = isHidden ? 'flex' : 'none';
        if (isHidden) {
            // Close other modals to prevent visual overlay collision
            if (metricModal) metricModal.style.display = 'none';
            if (agentModal) agentModal.style.display = 'none';
            const wifi = document.getElementById('wifi-modal');
            if (wifi) wifi.style.display = 'none';
            activeMetricKey = null;

            connectChatWebSocket();
            setTimeout(() => {
                if (chatInput) chatInput.focus();
                if (chatMessages) chatMessages.scrollTop = chatMessages.scrollHeight;
            }, 100);
        }
    });
}

if (chatModal) {
    chatModal.addEventListener('click', (e) => {
        e.stopPropagation(); // Clicking inside chat modal does not trigger outside-click close
    });
}

if (btnCloseChat) {
    btnCloseChat.addEventListener('click', (e) => {
        e.stopPropagation();
        chatModal.style.display = 'none';
    });
}

if (btnSendChat) {
    btnSendChat.addEventListener('click', (e) => {
        e.stopPropagation();
        sendUserMessage();
    });
}

if (chatInput) {
    chatInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            sendUserMessage();
        }
    });
}

if (chatQuickChips) {
    chatQuickChips.addEventListener('click', (e) => {
        const btn = e.target.closest('.chip-btn');
        if (btn && btn.dataset.prompt) {
            sendUserMessage(btn.dataset.prompt);
        }
    });
}

// Connect WebSocket on initial dashboard load
connectChatWebSocket();