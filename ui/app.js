// Global state
let state = {
    catPacks: [],
    hats: [],
    selectedCatPack: '',
    selectedHat: '',
    currentTab: 'cats',
    darkMode: false,
    catSize: 100,
    accentColor: '#4a90e2'
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded, initializing UI...');
    
    // Load dark mode preference first
    loadDarkModePreference();
    
    initializeUI();
    setupMessageHandlers();
    
    // Request initial data after a short delay to ensure everything is ready
    setTimeout(() => {
        console.log('Requesting initial data...');
        requestInitialData();
    }, 100);
});

function initializeUI() {
    // Setup custom navbar close button
    const closeBtn = document.getElementById('navbar-close-btn');
    if (closeBtn) {
        closeBtn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            sendMessage('hideWindow');
        });
    }
    
    // Setup tab switching
    const tabButtons = document.querySelectorAll('.tab-button:not(.discord-button)');
    tabButtons.forEach(button => {
        button.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const tabName = button.getAttribute('data-tab');
            switchTab(tabName);
        });
    });
    
    // Setup Discord button to open URL in default browser
    const discordButton = document.getElementById('tab-discord');
    if (discordButton) {
        discordButton.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            sendMessage('openURL', { url: 'https://discord.gg/TVw6h5TBqJ' });
        });
    }
    
    // Setup dark mode toggle
    const darkModeToggle = document.getElementById('dark-mode-toggle');
    if (darkModeToggle) {
        darkModeToggle.checked = state.darkMode;
        darkModeToggle.addEventListener('change', (e) => {
            state.darkMode = e.target.checked;
            applyDarkMode(state.darkMode);
            saveDarkModePreference();
        });
    }
    
    // Setup cat size slider
    const catSizeSlider = document.getElementById('cat-size-slider');
    const catSizeValue = document.getElementById('cat-size-value');
    if (catSizeSlider && catSizeValue) {
        // Load saved cat size
        loadCatSizePreference();
        catSizeSlider.value = state.catSize || 100;
        catSizeValue.textContent = state.catSize || 100;
        
        catSizeSlider.addEventListener('input', (e) => {
            const size = parseInt(e.target.value);
            state.catSize = size;
            catSizeValue.textContent = size;
            sendMessage('setCatSize', { size });
            saveCatSizePreference();
        });
    }
    
    // Setup accent color picker
    const accentColorPicker = document.getElementById('accent-color-picker');
    const colorPickerPreview = document.querySelector('.color-picker-preview');
    if (accentColorPicker && colorPickerPreview) {
        // Load saved accent color
        loadAccentColorPreference();
        accentColorPicker.value = state.accentColor;
        colorPickerPreview.style.background = state.accentColor;
        updateAccentColor(state.accentColor);
        
        accentColorPicker.addEventListener('input', (e) => {
            const color = e.target.value;
            state.accentColor = color;
            colorPickerPreview.style.background = color;
            updateAccentColor(color);
            saveAccentColorPreference();
            sendMessage('setAccentColor', { color });
        });
    }
    
    // Apply initial dark mode
    applyDarkMode(state.darkMode);
    
    // Load initial data for active tab
    switchTab('cats');
}

function switchTab(tabName) {
    console.log('Switching to tab:', tabName);
    
    // Update state
    state.currentTab = tabName;
    
    // Update tab buttons
    document.querySelectorAll('.tab-button').forEach(btn => {
        btn.classList.remove('active');
    });
    const activeButton = document.getElementById(`tab-${tabName}`);
    if (activeButton) {
        activeButton.classList.add('active');
    }
    
    // Update tab panels
    document.querySelectorAll('.tab-panel').forEach(panel => {
        panel.classList.remove('active');
    });
    const activePanel = document.getElementById(`panel-${tabName}`);
    if (activePanel) {
        activePanel.classList.add('active');
    }
    
    // Load data for the tab
    if (tabName === 'cats') {
        console.log('Loading cat packs for cats tab...');
        requestCatPacks();
    } else if (tabName === 'hats') {
        console.log('Loading hats for hats tab...');
        requestHats();
    }
}

function requestInitialData() {
    // Request initial cat pack and hat
    console.log('Requesting initial data...');
    sendMessage('getSelectedCatPack');
    sendMessage('getSelectedHat');
    
    // Also request lists for current tab
    if (state.currentTab === 'cats') {
        requestCatPacks();
    } else if (state.currentTab === 'hats') {
        requestHats();
    }
}

function requestCatPacks() {
    console.log('Requesting cat packs...');
    sendMessage('getCatPacks');
}

function requestHats() {
    console.log('Requesting hats...');
    sendMessage('getHats');
}

function setupMessageHandlers() {
    // Handle messages from C++ backend via MessageEvent
    window.addEventListener('message', (event) => {
        console.log('Received MessageEvent:', event);
        if (event.data) {
            handleMessage(event.data);
        }
    });
    
    // Also set up a global function that C++ can call directly
    window.receiveMessage = function(message) {
        console.log('Received message via receiveMessage:', message);
        if (typeof message === 'string') {
            try {
                message = JSON.parse(message);
            } catch (e) {
                console.error('Failed to parse message:', e);
                return;
            }
        }
        handleMessage(message);
    };
}

function handleMessage(message) {
    // Message might be a string (JSON) or an object
    let messageObj = message;
    if (typeof message === 'string') {
        try {
            messageObj = JSON.parse(message);
        } catch (e) {
            console.error('Failed to parse message:', e, 'Message:', message);
            return;
        }
    }
    
    if (!messageObj || !messageObj.type) {
        console.error('Invalid message format:', messageObj);
        return;
    }
    
    const type = messageObj.type;
    const data = messageObj.data;
    
    console.log('Received message:', type, data);
    
    switch (type) {
        case 'catPackList':
            // Data should be an array directly
            const catPacks = Array.isArray(data) ? data : [];
            console.log('Updating cat pack list with', catPacks.length, 'packs');
            updateCatPackList(catPacks);
            break;
        case 'hatList':
            // Data should be an array directly
            const hats = Array.isArray(data) ? data : [];
            console.log('Updating hat list with', hats.length, 'hats');
            updateHatList(hats);
            break;
        case 'selectedCatPack':
            const packName = (data && data.name) ? data.name : '';
            if (packName) {
                console.log('Selected cat pack:', packName);
                state.selectedCatPack = packName;
                // Refresh the list to show selection
                if (state.catPacks.length > 0) {
                    updateCatPackList(state.catPacks);
                }
            }
            break;
        case 'selectedHat':
            const hatName = (data && data.name) ? data.name : '';
            if (hatName) {
                console.log('Selected hat:', hatName);
                state.selectedHat = hatName;
                // Refresh the list to show selection
                if (state.hats.length > 0) {
                    updateHatList(state.hats);
                }
            }
            break;
        case 'catSize':
            const size = (data && data.size) ? data.size : 100;
            if (size >= 50 && size <= 200) {
                state.catSize = size;
                const catSizeSlider = document.getElementById('cat-size-slider');
                const catSizeValue = document.getElementById('cat-size-value');
                if (catSizeSlider && catSizeValue) {
                    catSizeSlider.value = size;
                    catSizeValue.textContent = size;
                }
            }
            break;
        case 'accentColor':
            const color = (data && data.color) ? data.color : '#4a90e2';
            if (color) {
                state.accentColor = color;
                const accentColorPicker = document.getElementById('accent-color-picker');
                const colorPickerPreview = document.querySelector('.color-picker-preview');
                if (accentColorPicker && colorPickerPreview) {
                    accentColorPicker.value = color;
                    colorPickerPreview.style.background = color;
                }
                updateAccentColor(color);
            }
            break;
        default:
            console.log('Unknown message type:', type);
    }
}

function updateCatPackList(catPacks) {
    state.catPacks = Array.isArray(catPacks) ? catPacks : [];
    const list = document.getElementById('cat-pack-list');
    if (!list) return;
    
    list.innerHTML = '';
    
    if (state.catPacks.length === 0) {
        const emptyMsg = document.createElement('div');
        emptyMsg.className = 'settings-placeholder';
        emptyMsg.textContent = 'No cat packs available';
        list.appendChild(emptyMsg);
        return;
    }
    
    state.catPacks.forEach(pack => {
        const item = document.createElement('div');
        item.className = 'selection-item';
        if (pack.name === state.selectedCatPack) {
            item.classList.add('selected');
        }
        
        // Icon
        const icon = document.createElement('img');
        icon.className = 'selection-item-icon';
        icon.src = pack.iconPath || '';
        icon.alt = pack.name || 'Cat Pack';
        icon.onerror = () => { 
            icon.style.display = 'none';
        };
        
        // Name
        const name = document.createElement('span');
        name.className = 'selection-item-name';
        name.textContent = pack.name || 'Unknown';
        
        item.appendChild(icon);
        item.appendChild(name);
        
        item.addEventListener('click', () => {
            selectCatPack(pack.name);
        });
        
        list.appendChild(item);
    });
}

function updateHatList(hats) {
    state.hats = Array.isArray(hats) ? hats : [];
    const list = document.getElementById('hat-list');
    if (!list) return;
    
    list.innerHTML = '';
    
    if (state.hats.length === 0) {
        const emptyMsg = document.createElement('div');
        emptyMsg.className = 'settings-placeholder';
        emptyMsg.textContent = 'No hats available';
        list.appendChild(emptyMsg);
        return;
    }
    
    state.hats.forEach(hat => {
        const item = document.createElement('div');
        item.className = 'selection-item';
        if (hat.name === state.selectedHat) {
            item.classList.add('selected');
        }
        
        // Icon
        const icon = document.createElement('img');
        icon.className = 'selection-item-icon';
        icon.src = hat.iconPath || '';
        icon.alt = hat.name || 'Hat';
        icon.onerror = () => { 
            icon.style.display = 'none';
        };
        
        // Name
        const name = document.createElement('span');
        name.className = 'selection-item-name';
        name.textContent = hat.name || 'Unknown';
        
        item.appendChild(icon);
        item.appendChild(name);
        
        item.addEventListener('click', () => {
            selectHat(hat.name);
        });
        
        list.appendChild(item);
    });
}

function selectCatPack(name) {
    if (!name) return;
    sendMessage('selectCatPack', { name });
    state.selectedCatPack = name;
    updateCatPackList(state.catPacks);
}

function selectHat(name) {
    if (!name) return;
    sendMessage('selectHat', { name });
    state.selectedHat = name;
    updateHatList(state.hats);
}

function sendMessage(type, data = {}) {
    // Send message to C++ backend via HTTP API (Crow server)
    const message = { type, ...data };
    
    console.log('Sending message:', message);
    
    fetch('/api/message', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(message)
    })
    .then(response => {
        if (!response.ok) {
            console.error('HTTP error:', response.status);
        }
        return response.text();
    })
    .then(text => {
        console.log('Response:', text);
    })
    .catch(err => {
        console.error('Failed to send message:', err);
    });
}

function loadDarkModePreference() {
    try {
        const saved = localStorage.getItem('openBongoDarkMode');
        if (saved !== null) {
            state.darkMode = saved === 'true';
        }
    } catch (e) {
        console.error('Failed to load dark mode preference:', e);
    }
}

function saveDarkModePreference() {
    try {
        localStorage.setItem('openBongoDarkMode', state.darkMode.toString());
    } catch (e) {
        console.error('Failed to save dark mode preference:', e);
    }
}

function applyDarkMode(enabled) {
    const body = document.body;
    const app = document.getElementById('app');
    if (enabled) {
        body.classList.add('dark-mode');
        if (app) app.classList.add('dark-mode');
    } else {
        body.classList.remove('dark-mode');
        if (app) app.classList.remove('dark-mode');
    }
}

function loadCatSizePreference() {
    try {
        const saved = localStorage.getItem('openBongoCatSize');
        if (saved !== null) {
            state.catSize = parseInt(saved) || 100;
        }
    } catch (e) {
        console.error('Failed to load cat size preference:', e);
    }
}

function saveCatSizePreference() {
    try {
        localStorage.setItem('openBongoCatSize', state.catSize.toString());
    } catch (e) {
        console.error('Failed to save cat size preference:', e);
    }
}

// Helper function to convert hex to RGB
function hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

// Helper function to darken a color
function darkenColor(hex, percent) {
    const rgb = hexToRgb(hex);
    if (!rgb) return hex;
    
    const factor = 1 - (percent / 100);
    const r = Math.round(rgb.r * factor);
    const g = Math.round(rgb.g * factor);
    const b = Math.round(rgb.b * factor);
    
    return `#${[r, g, b].map(x => {
        const hex = x.toString(16);
        return hex.length === 1 ? '0' + hex : hex;
    }).join('')}`;
}

// Helper function to convert hex to rgba string
function hexToRgba(hex, alpha) {
    const rgb = hexToRgb(hex);
    if (!rgb) return `rgba(74, 144, 226, ${alpha})`;
    return `rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, ${alpha})`;
}

// Update accent color CSS variables
function updateAccentColor(color) {
    const root = document.documentElement;
    root.style.setProperty('--accent-color', color);
    
    // Calculate darker shades (15% and 30% darker)
    const darker = darkenColor(color, 15);
    const darkest = darkenColor(color, 30);
    
    root.style.setProperty('--accent-color-dark', darker);
    root.style.setProperty('--accent-color-darker', darkest);
    
    // Update rgba values for shadows and overlays
    root.style.setProperty('--accent-color-rgba-20', hexToRgba(color, 0.2));
    root.style.setProperty('--accent-color-rgba-30', hexToRgba(color, 0.3));
    root.style.setProperty('--accent-color-rgba-15', hexToRgba(color, 0.15));
}

function loadAccentColorPreference() {
    try {
        const saved = localStorage.getItem('openBongoAccentColor');
        if (saved !== null) {
            state.accentColor = saved;
        }
    } catch (e) {
        console.error('Failed to load accent color preference:', e);
    }
}

function saveAccentColorPreference() {
    try {
        localStorage.setItem('openBongoAccentColor', state.accentColor);
    } catch (e) {
        console.error('Failed to save accent color preference:', e);
    }
}
