// Global state
let state = {
    catPacks: [],
    hats: [],
    bonkPacks: [],
    selectedCatPack: '',
    selectedHat: '',
    selectedBonkPack: '',
    currentTab: 'cats',
    darkMode: false,
    catSize: 100,
    accentColor: '#4a90e2',
    uiOffset: 0,
    uiHorizontalOffset: 0,
    sfxVolume: 100,
    catFlipped: false,
    particleEffectsEnabled: true,
    particleDensity: 100,
    leftArmOffset: 0,
    rightArmOffset: 0,
    animationVerticalOffset: 0
};

// Track if items have been animated (to prevent re-animation on selection changes)
let itemsAnimated = {
    catPacks: false,
    hats: false,
    bonkPacks: false
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
    const tabButtons = document.querySelectorAll('.tab-button:not(.discord-button):not(.github-button)');
    tabButtons.forEach(button => {
        button.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const tabName = button.getAttribute('data-tab');
            switchTab(tabName);
        });
    });

    // Setup GitHub button to open URL in default browser and close UI
    const githubButton = document.getElementById('tab-github');
    if (githubButton) {
        githubButton.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            sendMessage('openURL', { url: 'https://github.com/CharlieRusso001/OpenBongo' });
            // Close the UI after opening GitHub
            setTimeout(() => {
                sendMessage('hideWindow');
            }, 100);
        });
    }

    // Setup Discord button to open URL in default browser and close UI
    const discordButton = document.getElementById('tab-discord');
    if (discordButton) {
        discordButton.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            sendMessage('openURL', { url: 'https://discord.gg/TVw6h5TBqJ' });
            // Close the UI after opening Discord
            setTimeout(() => {
                sendMessage('hideWindow');
            }, 100);
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

    // Setup cat size buttons
    const sizeBtns = document.querySelectorAll('.size-btn');
    if (sizeBtns.length > 0) {
        // Load saved cat size
        loadCatSizePreference();
        updateSizeButtons(state.catSize || 100);
        // Load size-specific preferences BEFORE setting up sliders
        // This ensures the correct values are loaded for the current size
        const currentSize = state.catSize || 100;
        // Load saved preferences (0 means base offset for size)
        const savedRightArmOffset = loadSizeSpecificPreference('rightArmOffset', currentSize);
        const savedAnimationVerticalOffset = loadSizeSpecificPreference('animationVerticalOffset', currentSize);
        state.rightArmOffset = savedRightArmOffset !== null ? savedRightArmOffset : 0;
        state.animationVerticalOffset = savedAnimationVerticalOffset !== null ? savedAnimationVerticalOffset : 0;

        sizeBtns.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const size = parseInt(e.target.dataset.size);
                state.catSize = size;
                updateSizeButtons(size);
                // Load saved preferences for new size (don't reset to defaults)
                const savedRightArmOffset = loadSizeSpecificPreference('rightArmOffset', size);
                const savedAnimationVerticalOffset = loadSizeSpecificPreference('animationVerticalOffset', size);
                state.rightArmOffset = savedRightArmOffset !== null ? savedRightArmOffset : 0;
                state.animationVerticalOffset = savedAnimationVerticalOffset !== null ? savedAnimationVerticalOffset : 0;
                
                // Update sliders and send to backend
                const rightArmOffsetSlider = document.getElementById('right-arm-offset-slider');
                const rightArmOffsetValue = document.getElementById('right-arm-offset-value');
                if (rightArmOffsetSlider && rightArmOffsetValue) {
                    rightArmOffsetSlider.value = state.rightArmOffset;
                    rightArmOffsetValue.textContent = state.rightArmOffset;
                    sendMessage('setRightArmOffset', { offset: state.rightArmOffset });
                }
                
                const animationVerticalOffsetSlider = document.getElementById('animation-vertical-offset-slider');
                const animationVerticalOffsetValue = document.getElementById('animation-vertical-offset-value');
                if (animationVerticalOffsetSlider && animationVerticalOffsetValue) {
                    animationVerticalOffsetSlider.value = state.animationVerticalOffset;
                    animationVerticalOffsetValue.textContent = state.animationVerticalOffset;
                    sendMessage('setAnimationVerticalOffset', { offset: state.animationVerticalOffset });
                }
                
                sendMessage('setCatSize', { size });
                saveCatSizePreference();
            });
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

    // Setup UI offset slider
    const uiOffsetSlider = document.getElementById('ui-offset-slider');
    const uiOffsetValue = document.getElementById('ui-offset-value');
    if (uiOffsetSlider && uiOffsetValue) {
        // Load saved UI offset
        loadUIOffsetPreference();
        uiOffsetSlider.value = state.uiOffset || 0;
        uiOffsetValue.textContent = state.uiOffset || 0;

        uiOffsetSlider.addEventListener('input', (e) => {
            const offset = parseInt(e.target.value);
            state.uiOffset = offset;
            uiOffsetValue.textContent = offset;
            sendMessage('setUIOffset', { offset });
            saveUIOffsetPreference();
        });
    }

    // Setup UI horizontal offset slider
    const uiHorizontalOffsetSlider = document.getElementById('ui-horizontal-offset-slider');
    const uiHorizontalOffsetValue = document.getElementById('ui-horizontal-offset-value');
    if (uiHorizontalOffsetSlider && uiHorizontalOffsetValue) {
        // Load saved UI horizontal offset
        loadUIHorizontalOffsetPreference();
        uiHorizontalOffsetSlider.value = state.uiHorizontalOffset || 0;
        uiHorizontalOffsetValue.textContent = state.uiHorizontalOffset || 0;

        uiHorizontalOffsetSlider.addEventListener('input', (e) => {
            const offset = parseInt(e.target.value);
            state.uiHorizontalOffset = offset;
            uiHorizontalOffsetValue.textContent = offset;
            sendMessage('setUIHorizontalOffset', { offset });
            saveUIHorizontalOffsetPreference();
        });
    }

    // Setup left arm offset slider
    const leftArmOffsetSlider = document.getElementById('left-arm-offset-slider');
    const leftArmOffsetValue = document.getElementById('left-arm-offset-value');
    if (leftArmOffsetSlider && leftArmOffsetValue) {
        loadLeftArmOffsetPreference();
        leftArmOffsetSlider.value = state.leftArmOffset || 0;
        leftArmOffsetValue.textContent = state.leftArmOffset || 0;
        // Send loaded value to backend on startup
        sendMessage('setLeftArmOffset', { offset: state.leftArmOffset || 0 });

        leftArmOffsetSlider.addEventListener('input', (e) => {
            const offset = parseFloat(e.target.value);
            state.leftArmOffset = offset;
            leftArmOffsetValue.textContent = offset;
            sendMessage('setLeftArmOffset', { offset });
            saveLeftArmOffsetPreference();
        });
    }

    // Setup right arm offset slider
    const rightArmOffsetSlider = document.getElementById('right-arm-offset-slider');
    const rightArmOffsetValue = document.getElementById('right-arm-offset-value');
    if (rightArmOffsetSlider && rightArmOffsetValue) {
        loadRightArmOffsetPreference();
        rightArmOffsetSlider.value = state.rightArmOffset || 0;
        rightArmOffsetValue.textContent = state.rightArmOffset || 0;
        // Send loaded value to backend on startup
        sendMessage('setRightArmOffset', { offset: state.rightArmOffset || 0 });

        rightArmOffsetSlider.addEventListener('input', (e) => {
            const offset = parseFloat(e.target.value);
            state.rightArmOffset = offset;
            rightArmOffsetValue.textContent = offset;
            sendMessage('setRightArmOffset', { offset });
            saveRightArmOffsetPreference();
            // Save size-specific preference
            saveSizeSpecificPreference('rightArmOffset', state.catSize, offset);
        });
    }

    // Setup animation vertical offset slider
    const animationVerticalOffsetSlider = document.getElementById('animation-vertical-offset-slider');
    const animationVerticalOffsetValue = document.getElementById('animation-vertical-offset-value');
    if (animationVerticalOffsetSlider && animationVerticalOffsetValue) {
        loadAnimationVerticalOffsetPreference();
        animationVerticalOffsetSlider.value = state.animationVerticalOffset || 0;
        animationVerticalOffsetValue.textContent = state.animationVerticalOffset || 0;
        // Send loaded value to backend on startup
        sendMessage('setAnimationVerticalOffset', { offset: state.animationVerticalOffset || 0 });

        animationVerticalOffsetSlider.addEventListener('input', (e) => {
            const offset = parseFloat(e.target.value);
            state.animationVerticalOffset = offset;
            animationVerticalOffsetValue.textContent = offset;
            sendMessage('setAnimationVerticalOffset', { offset });
            saveAnimationVerticalOffsetPreference();
            // Save size-specific preference
            saveSizeSpecificPreference('animationVerticalOffset', state.catSize, offset);
        });
    }

    // Setup cat flip toggle
    const catFlipToggle = document.getElementById('cat-flip-toggle');
    if (catFlipToggle) {
        // Load saved cat flip preference
        loadCatFlipPreference();
        catFlipToggle.checked = state.catFlipped;

        catFlipToggle.addEventListener('change', (e) => {
            state.catFlipped = e.target.checked;
            sendMessage('setCatFlip', { flipped: state.catFlipped });
            saveCatFlipPreference();
        });
    }

    // Setup particle effects toggle
    const particleEffectsToggle = document.getElementById('particle-effects-toggle');
    if (particleEffectsToggle) {
        loadParticleEffectsPreference();
        particleEffectsToggle.checked = state.particleEffectsEnabled;

        particleEffectsToggle.addEventListener('change', (e) => {
            state.particleEffectsEnabled = e.target.checked;
            saveParticleEffectsPreference();
            // Remove existing effects and reapply if enabled
            removeAllParticleEffects();
            if (state.particleEffectsEnabled) {
                checkAndApplySeasonalEffects();
            }
        });
    }

    // Setup particle density slider
    const particleDensitySlider = document.getElementById('particle-density-slider');
    const particleDensityValue = document.getElementById('particle-density-value');
    if (particleDensitySlider && particleDensityValue) {
        loadParticleDensityPreference();
        particleDensitySlider.value = state.particleDensity || 100;
        particleDensityValue.textContent = (state.particleDensity || 100) + '%';

        particleDensitySlider.addEventListener('input', (e) => {
            const density = parseInt(e.target.value);
            state.particleDensity = density;
            particleDensityValue.textContent = density + '%';
            saveParticleDensityPreference();
            // Reapply effects with new density
            if (state.particleEffectsEnabled) {
                removeAllParticleEffects();
                checkAndApplySeasonalEffects();
            }
        });
    }

    // Setup SFX volume slider
    const sfxVolumeSlider = document.getElementById('sfx-volume-slider');
    const sfxVolumeValue = document.getElementById('sfx-volume-value');
    if (sfxVolumeSlider && sfxVolumeValue) {
        loadSFXVolumePreference();
        sfxVolumeSlider.value = state.sfxVolume || 100;
        sfxVolumeValue.textContent = (state.sfxVolume || 100) + '%';

        sfxVolumeSlider.addEventListener('input', (e) => {
            const volume = parseInt(e.target.value);
            state.sfxVolume = volume;
            sfxVolumeValue.textContent = volume + '%';
            sendMessage('setSFXVolume', { volume });
            saveSFXVolumePreference();
        });
    }

    // Apply initial dark mode
    applyDarkMode(state.darkMode);

    // Setup shutdown button
    const shutdownBtn = document.getElementById('shutdown-btn');
    if (shutdownBtn) {
        shutdownBtn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            sendMessage('shutdown');
        });
    }

    // Check for seasonal/holiday effects based on current date
    checkAndApplySeasonalEffects();

    // Load initial data for active tab
    switchTab('cats');
}

function switchTab(tabName) {
    console.log('Switching to tab:', tabName);

    // Update state
    state.currentTab = tabName;

    // Reset animation flags when switching tabs (so items animate in)
    if (tabName === 'cats') {
        itemsAnimated.catPacks = false;
    } else if (tabName === 'hats') {
        itemsAnimated.hats = false;
    } else if (tabName === 'sfx') {
        itemsAnimated.bonkPacks = false;
    }

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
    } else if (tabName === 'sfx') {
        console.log('Loading SFX for sfx tab...');
        requestBonkPacks();
    }
}

function requestInitialData() {
    // Request initial cat pack, hat, and bonk pack
    console.log('Requesting initial data...');
    sendMessage('getSelectedCatPack');
    sendMessage('getSelectedHat');
    sendMessage('getSelectedBonkPack');

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

function requestBonkPacks() {
    console.log('Requesting bonk packs...');
    sendMessage('getBonkPacks');
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
    window.receiveMessage = function (message) {
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
                // Update selection state without recreating items
                updateCatPackSelection();
            }
            break;
        case 'selectedHat':
            const hatName = (data && data.name) ? data.name : '';
            if (hatName) {
                console.log('Selected hat:', hatName);
                state.selectedHat = hatName;
                // Update selection state without recreating items
                updateHatSelection();
            }
            break;
        case 'selectedBonkPack':
            const bonkPackName = (data && data.name) ? data.name : '';
            if (bonkPackName) {
                console.log('Selected bonk pack:', bonkPackName);
                state.selectedBonkPack = bonkPackName;
                // Update selection state without recreating items
                updateBonkPackSelection();
            }
            break;
        case 'catSize':
            const size = (data && data.size) ? data.size : 100;
            if (size >= 50 && size <= 200) {
                state.catSize = size;
                updateSizeButtons(size);
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
        case 'restartRequired':
            const message = (data && data.message) ? data.message : 'Please restart OpenBongo for this change to take effect.';
            showRestartModal(message);
            break;
        case 'uiOffset':
            const offset = (data && data.offset !== undefined) ? data.offset : 0;
            if (offset >= -50 && offset <= 50) {
                state.uiOffset = offset;
                const uiOffsetSlider = document.getElementById('ui-offset-slider');
                const uiOffsetValue = document.getElementById('ui-offset-value');
                if (uiOffsetSlider && uiOffsetValue) {
                    uiOffsetSlider.value = offset;
                    uiOffsetValue.textContent = offset;
                }
            }
            break;
        case 'uiHorizontalOffset':
            const horizontalOffset = (data && data.offset !== undefined) ? data.offset : 0;
            if (horizontalOffset >= -50 && horizontalOffset <= 50) {
                state.uiHorizontalOffset = horizontalOffset;
                const uiHorizontalOffsetSlider = document.getElementById('ui-horizontal-offset-slider');
                const uiHorizontalOffsetValue = document.getElementById('ui-horizontal-offset-value');
                if (uiHorizontalOffsetSlider && uiHorizontalOffsetValue) {
                    uiHorizontalOffsetSlider.value = horizontalOffset;
                    uiHorizontalOffsetValue.textContent = horizontalOffset;
                }
            }
            break;
        case 'sfxVolume':
            const volume = (data && data.volume !== undefined) ? data.volume : 100;
            if (volume >= 0 && volume <= 100) {
                state.sfxVolume = volume;
                const sfxVolumeSlider = document.getElementById('sfx-volume-slider');
                const sfxVolumeValue = document.getElementById('sfx-volume-value');
                if (sfxVolumeSlider && sfxVolumeValue) {
                    sfxVolumeSlider.value = volume;
                    sfxVolumeValue.textContent = volume + '%';
                }
            }
            break;
        case 'catFlip':
            const flipped = (data && data.flipped !== undefined) ? data.flipped : false;
            state.catFlipped = flipped;
            const catFlipToggle = document.getElementById('cat-flip-toggle');
            if (catFlipToggle) {
                catFlipToggle.checked = flipped;
            }
            break;
        case 'leftArmOffset':
            const leftArmOffset = (data && data.offset !== undefined) ? data.offset : 0;
            if (leftArmOffset >= -50 && leftArmOffset <= 50) {
                state.leftArmOffset = leftArmOffset;
                const leftArmOffsetSlider = document.getElementById('left-arm-offset-slider');
                const leftArmOffsetValue = document.getElementById('left-arm-offset-value');
                if (leftArmOffsetSlider && leftArmOffsetValue) {
                    leftArmOffsetSlider.value = leftArmOffset;
                    leftArmOffsetValue.textContent = leftArmOffset;
                }
            }
            break;
        case 'rightArmOffset':
            const rightArmOffset = (data && data.offset !== undefined) ? data.offset : 0;
            if (rightArmOffset >= -50 && rightArmOffset <= 50) {
                state.rightArmOffset = rightArmOffset;
                const rightArmOffsetSlider = document.getElementById('right-arm-offset-slider');
                const rightArmOffsetValue = document.getElementById('right-arm-offset-value');
                if (rightArmOffsetSlider && rightArmOffsetValue) {
                    rightArmOffsetSlider.value = rightArmOffset;
                    rightArmOffsetValue.textContent = rightArmOffset;
                }
            }
            break;
        case 'animationVerticalOffset':
            const animVerticalOffset = (data && data.offset !== undefined) ? data.offset : 0;
            if (animVerticalOffset >= -100 && animVerticalOffset <= 100) {
                state.animationVerticalOffset = animVerticalOffset;
                const animationVerticalOffsetSlider = document.getElementById('animation-vertical-offset-slider');
                const animationVerticalOffsetValue = document.getElementById('animation-vertical-offset-value');
                if (animationVerticalOffsetSlider && animationVerticalOffsetValue) {
                    animationVerticalOffsetSlider.value = animVerticalOffset;
                    animationVerticalOffsetValue.textContent = animVerticalOffset;
                }
            }
            break;
        case 'bonkPackList':
            const bonkPacks = Array.isArray(data) ? data : [];
            console.log('Updating bonk pack list with', bonkPacks.length, 'packs');
            updateBonkPackList(bonkPacks);
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

    const shouldAnimate = !itemsAnimated.catPacks;
    itemsAnimated.catPacks = true;

    state.catPacks.forEach((pack, index) => {
        const item = document.createElement('div');
        item.className = 'selection-item';
        if (pack.name === state.selectedCatPack) {
            item.classList.add('selected');
        }

        // Only set animation delay if items haven't been animated yet
        if (shouldAnimate) {
            item.style.animationDelay = `${0.1 + (index * 0.05)}s`;
        } else {
            // If already animated, set opacity to 1 immediately
            item.style.opacity = '1';
            item.style.transform = 'translateY(0) scale(1)';
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

function updateCatPackSelection() {
    // Update selection state without recreating items
    const list = document.getElementById('cat-pack-list');
    if (!list) return;

    const items = list.querySelectorAll('.selection-item');
    items.forEach(item => {
        const nameElement = item.querySelector('.selection-item-name');
        if (nameElement) {
            const itemName = nameElement.textContent;
            if (itemName === state.selectedCatPack) {
                item.classList.add('selected');
            } else {
                item.classList.remove('selected');
            }
        }
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

    const shouldAnimate = !itemsAnimated.hats;
    itemsAnimated.hats = true;

    state.hats.forEach((hat, index) => {
        const item = document.createElement('div');
        item.className = 'selection-item';
        if (hat.name === state.selectedHat) {
            item.classList.add('selected');
        }

        // Only set animation delay if items haven't been animated yet
        if (shouldAnimate) {
            item.style.animationDelay = `${0.1 + (index * 0.05)}s`;
        } else {
            // If already animated, set opacity to 1 immediately
            item.style.opacity = '1';
            item.style.transform = 'translateY(0) scale(1)';
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

function updateHatSelection() {
    // Update selection state without recreating items
    const list = document.getElementById('hat-list');
    if (!list) return;

    const items = list.querySelectorAll('.selection-item');
    items.forEach(item => {
        const nameElement = item.querySelector('.selection-item-name');
        if (nameElement) {
            const itemName = nameElement.textContent;
            if (itemName === state.selectedHat) {
                item.classList.add('selected');
            } else {
                item.classList.remove('selected');
            }
        }
    });
}

function updateBonkPackList(bonkPacks) {
    state.bonkPacks = Array.isArray(bonkPacks) ? bonkPacks : [];
    const list = document.getElementById('bonk-pack-list');
    if (!list) return;

    list.innerHTML = '';

    // Add "No SFX" option at the beginning
    const noSFXOption = { name: 'No SFX', iconPath: '' };
    const allBonkPacks = [noSFXOption, ...state.bonkPacks];

    if (allBonkPacks.length === 0) {
        const emptyMsg = document.createElement('div');
        emptyMsg.className = 'settings-placeholder';
        emptyMsg.textContent = 'No bonk packs available';
        list.appendChild(emptyMsg);
        return;
    }

    const shouldAnimate = !itemsAnimated.bonkPacks;
    itemsAnimated.bonkPacks = true;

    allBonkPacks.forEach((pack, index) => {
        const item = document.createElement('div');
        item.className = 'selection-item';
        if (pack.name === state.selectedBonkPack) {
            item.classList.add('selected');
        }

        // Only set animation delay if items haven't been animated yet
        if (shouldAnimate) {
            item.style.animationDelay = `${0.1 + (index * 0.05)}s`;
        } else {
            item.style.opacity = '1';
            item.style.transform = 'translateY(0) scale(1)';
        }

        // Icon - handle "No SFX" specially to avoid double emoji
        if (pack.name === 'No SFX') {
            // For "No SFX", show mute icon directly (no img element)
            const emojiIcon = document.createElement('div');
            emojiIcon.className = 'selection-item-icon';
            emojiIcon.style.display = 'flex';
            emojiIcon.style.alignItems = 'center';
            emojiIcon.style.justifyContent = 'center';
            emojiIcon.style.fontSize = '48px';
            emojiIcon.textContent = '🔇';
            item.appendChild(emojiIcon);
        } else {
            // For other packs, use img with fallback
            const icon = document.createElement('img');
            icon.className = 'selection-item-icon';
            icon.src = pack.iconPath || '';
            icon.alt = pack.name || 'Bonk Pack';
            icon.onerror = () => {
                // Fallback to emoji if image fails
                icon.style.display = 'none';
                const emojiIcon = document.createElement('div');
                emojiIcon.className = 'selection-item-icon';
                emojiIcon.style.display = 'flex';
                emojiIcon.style.alignItems = 'center';
                emojiIcon.style.justifyContent = 'center';
                emojiIcon.style.fontSize = '48px';
                emojiIcon.textContent = '🔊';
                item.insertBefore(emojiIcon, item.firstChild);
            };
            item.appendChild(icon);
        }

        // Name
        const name = document.createElement('span');
        name.className = 'selection-item-name';
        name.textContent = pack.name || 'Unknown';

        item.appendChild(name);

        item.addEventListener('click', () => {
            selectBonkPack(pack.name);
        });

        list.appendChild(item);
    });
}

function updateBonkPackSelection() {
    // Update selection state without recreating items
    const list = document.getElementById('bonk-pack-list');
    if (!list) return;

    const items = list.querySelectorAll('.selection-item');
    items.forEach(item => {
        const nameElement = item.querySelector('.selection-item-name');
        if (nameElement) {
            const itemName = nameElement.textContent;
            if (itemName === state.selectedBonkPack) {
                item.classList.add('selected');
            } else {
                item.classList.remove('selected');
            }
        }
    });

    // Ensure accent color is applied (in case it wasn't set yet)
    if (state.accentColor) {
        updateAccentColor(state.accentColor);
    }
}

function selectBonkPack(name) {
    if (!name) return;
    sendMessage('selectBonkPack', { name });
    state.selectedBonkPack = name;
    updateBonkPackSelection();
}

function selectCatPack(name) {
    if (!name) return;
    sendMessage('selectCatPack', { name });
    state.selectedCatPack = name;
    updateCatPackSelection();
}

function selectHat(name) {
    if (!name) return;
    sendMessage('selectHat', { name });
    state.selectedHat = name;
    updateHatSelection();
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

function loadUIOffsetPreference() {
    try {
        const saved = localStorage.getItem('openBongoUIOffset');
        if (saved !== null) {
            state.uiOffset = parseInt(saved) || 0;
        }
    } catch (e) {
        console.error('Failed to load UI offset preference:', e);
    }
}

function saveUIOffsetPreference() {
    try {
        localStorage.setItem('openBongoUIOffset', state.uiOffset.toString());
    } catch (e) {
        console.error('Failed to save UI offset preference:', e);
    }
}

function loadUIHorizontalOffsetPreference() {
    try {
        const saved = localStorage.getItem('openBongoUIHorizontalOffset');
        if (saved !== null) {
            state.uiHorizontalOffset = parseInt(saved) || 0;
        }
    } catch (e) {
        console.error('Failed to load UI horizontal offset preference:', e);
    }
}

function saveUIHorizontalOffsetPreference() {
    try {
        localStorage.setItem('openBongoUIHorizontalOffset', state.uiHorizontalOffset.toString());
    } catch (e) {
        console.error('Failed to save UI horizontal offset preference:', e);
    }
}

function loadCatFlipPreference() {
    try {
        const saved = localStorage.getItem('openBongoCatFlip');
        if (saved !== null) {
            state.catFlipped = saved === 'true';
        }
    } catch (e) {
        console.error('Failed to load cat flip preference:', e);
    }
}

function saveCatFlipPreference() {
    try {
        localStorage.setItem('openBongoCatFlip', state.catFlipped.toString());
    } catch (e) {
        console.error('Failed to save cat flip preference:', e);
    }
}

function checkAndApplySeasonalEffects() {
    if (!state.particleEffectsEnabled) {
        return;
    }

    const currentDate = new Date();
    const month = currentDate.getMonth(); // 0-indexed (0 = January)
    const day = currentDate.getDate();

    // Check for specific holidays first (they override month-based effects)
    if ((month === 11 && day === 31) || (month === 0 && day === 1)) {
        // New Year's Eve (Dec 31) or New Year's Day (Jan 1)
        addFireworksEffect();
    } else if (month === 11 && day === 25) {
        // Christmas (Dec 25) - candy canes and Christmas items
        addChristmasEffect();
    } else if (month === 1 && day === 14) {
        // Valentine's Day (Feb 14)
        addHeartsEffect();
    } else if (month === 10) {
        // November - falling leaves
        addLeavesEffect();
    } else if (month === 11) {
        // December (but not Christmas) - snow
        addSnowEffect();
    }
}

function removeAllParticleEffects() {
    // Clear fireworks interval
    if (fireworksIntervalId) {
        clearInterval(fireworksIntervalId);
        fireworksIntervalId = null;
    }

    const containers = [
        document.getElementById('snow-container'),
        document.getElementById('leaves-container'),
        document.getElementById('hearts-container'),
        document.getElementById('christmas-container'),
        document.getElementById('fireworks-container')
    ];

    containers.forEach(container => {
        if (container && container.parentNode) {
            container.parentNode.removeChild(container);
        }
    });
}

function loadParticleEffectsPreference() {
    try {
        const saved = localStorage.getItem('openBongoParticleEffects');
        if (saved !== null) {
            state.particleEffectsEnabled = saved === 'true';
        }
    } catch (e) {
        console.error('Failed to load particle effects preference:', e);
    }
}

function saveParticleEffectsPreference() {
    try {
        localStorage.setItem('openBongoParticleEffects', state.particleEffectsEnabled.toString());
    } catch (e) {
        console.error('Failed to save particle effects preference:', e);
    }
}

function loadParticleDensityPreference() {
    try {
        const saved = localStorage.getItem('openBongoParticleDensity');
        if (saved !== null) {
            state.particleDensity = parseInt(saved) || 100;
        }
    } catch (e) {
        console.error('Failed to load particle density preference:', e);
    }
}

function saveParticleDensityPreference() {
    try {
        localStorage.setItem('openBongoParticleDensity', state.particleDensity.toString());
    } catch (e) {
        console.error('Failed to save particle density preference:', e);
    }
}

function loadSFXVolumePreference() {
    try {
        const saved = localStorage.getItem('openBongoSFXVolume');
        if (saved !== null) {
            state.sfxVolume = parseInt(saved) || 100;
        }
    } catch (e) {
        console.error('Failed to load SFX volume preference:', e);
    }
}

function saveSFXVolumePreference() {
    try {
        localStorage.setItem('openBongoSFXVolume', state.sfxVolume.toString());
    } catch (e) {
        console.error('Failed to save SFX volume preference:', e);
    }
}

function loadLeftArmOffsetPreference() {
    try {
        const saved = localStorage.getItem('openBongoLeftArmOffset');
        if (saved !== null) {
            state.leftArmOffset = parseFloat(saved) || 0;
        }
    } catch (e) {
        console.error('Failed to load left arm offset preference:', e);
    }
}

function saveLeftArmOffsetPreference() {
    try {
        localStorage.setItem('openBongoLeftArmOffset', state.leftArmOffset.toString());
    } catch (e) {
        console.error('Failed to save left arm offset preference:', e);
    }
}

function loadRightArmOffsetPreference() {
    try {
        // Load size-specific preference first, fallback to general preference
        const currentSize = state.catSize || 100;
        const sizeSpecific = loadSizeSpecificPreference('rightArmOffset', currentSize);
        if (sizeSpecific !== null) {
            state.rightArmOffset = sizeSpecific;
        } else {
            const saved = localStorage.getItem('openBongoRightArmOffset');
            if (saved !== null) {
                state.rightArmOffset = parseFloat(saved) || 0;
            }
        }
    } catch (e) {
        console.error('Failed to load right arm offset preference:', e);
    }
}

function saveRightArmOffsetPreference() {
    try {
        localStorage.setItem('openBongoRightArmOffset', state.rightArmOffset.toString());
    } catch (e) {
        console.error('Failed to save right arm offset preference:', e);
    }
}

function loadAnimationVerticalOffsetPreference() {
    try {
        // Load size-specific preference first, fallback to general preference
        const currentSize = state.catSize || 100;
        const sizeSpecific = loadSizeSpecificPreference('animationVerticalOffset', currentSize);
        if (sizeSpecific !== null) {
            state.animationVerticalOffset = sizeSpecific;
        } else {
            const saved = localStorage.getItem('openBongoAnimationVerticalOffset');
            if (saved !== null) {
                state.animationVerticalOffset = parseFloat(saved) || 0;
            }
        }
    } catch (e) {
        console.error('Failed to load animation vertical offset preference:', e);
    }
}

function saveAnimationVerticalOffsetPreference() {
    try {
        localStorage.setItem('openBongoAnimationVerticalOffset', state.animationVerticalOffset.toString());
    } catch (e) {
        console.error('Failed to save animation vertical offset preference:', e);
    }
}

// Base offsets are now baked into the backend - slider value of 0 means base offset for size
// No need for getSizeSpecificDefaults anymore

// Save size-specific preference
function saveSizeSpecificPreference(setting, size, value) {
    try {
        const key = `openBongo_${setting}_size_${size}`;
        localStorage.setItem(key, value.toString());
    } catch (e) {
        console.error(`Failed to save size-specific preference for ${setting} at size ${size}:`, e);
    }
}

// Load size-specific preference
function loadSizeSpecificPreference(setting, size) {
    try {
        const key = `openBongo_${setting}_size_${size}`;
        const saved = localStorage.getItem(key);
        if (saved !== null) {
            return parseFloat(saved);
        }
    } catch (e) {
        console.error(`Failed to load size-specific preference for ${setting} at size ${size}:`, e);
    }
    return null;
}

// Base offsets are now baked into the backend - no need to apply defaults
// Settings only reset to 0 (base offset) or saved values on reboot

function addSnowEffect() {
    // Create snow container
    const snowContainer = document.createElement('div');
    snowContainer.id = 'snow-container';
    snowContainer.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        pointer-events: none;
        z-index: 9999;
        overflow: hidden;
    `;
    document.body.appendChild(snowContainer);

    // Calculate count based on density (base 50, scaled by density percentage)
    const baseCount = 50;
    const snowflakeCount = Math.floor(baseCount * (state.particleDensity / 100));

    for (let i = 0; i < snowflakeCount; i++) {
        createSnowflake(snowContainer, i);
    }
}

function createSnowflake(container, index) {
    const snowflake = document.createElement('div');
    snowflake.className = 'snowflake';
    snowflake.textContent = '❄';

    const size = Math.random() * 10 + 10;
    const duration = Math.random() * 3 + 2;
    const delay = Math.random() * 2;
    const startX = Math.random() * 100;
    const drift = (Math.random() - 0.5) * 40; // Random horizontal drift

    snowflake.style.cssText = `
        position: absolute;
        color: white;
        font-size: ${size}px;
        top: -20px;
        left: ${startX}%;
        opacity: ${Math.random() * 0.5 + 0.5};
        animation: snowfall ${duration}s linear infinite;
        animation-delay: ${delay}s;
        text-shadow: 0 0 5px rgba(255, 255, 255, 0.5);
    `;

    // Set custom property for drift amount
    snowflake.style.setProperty('--drift', `${drift}px`);

    container.appendChild(snowflake);
}

function addLeavesEffect() {
    const container = document.createElement('div');
    container.id = 'leaves-container';
    container.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        pointer-events: none;
        z-index: 9999;
        overflow: hidden;
    `;
    document.body.appendChild(container);

    const baseCount = 30;
    const leafCount = Math.floor(baseCount * (state.particleDensity / 100));
    const leafEmojis = ['🍂', '🍁', '🍃'];

    for (let i = 0; i < leafCount; i++) {
        createLeaf(container, leafEmojis[Math.floor(Math.random() * leafEmojis.length)]);
    }
}

function createLeaf(container, emoji) {
    const leaf = document.createElement('div');
    leaf.className = 'leaf';
    leaf.textContent = emoji;

    const size = Math.random() * 15 + 15;
    const duration = Math.random() * 4 + 3;
    const delay = Math.random() * 2;
    const startX = Math.random() * 100;
    const drift = (Math.random() - 0.5) * 60;
    const rotation = Math.random() * 360;

    leaf.style.cssText = `
        position: absolute;
        font-size: ${size}px;
        top: -30px;
        left: ${startX}%;
        opacity: ${Math.random() * 0.4 + 0.6};
        animation: leafFall ${duration}s linear infinite;
        animation-delay: ${delay}s;
        transform: rotate(${rotation}deg);
    `;

    leaf.style.setProperty('--drift', `${drift}px`);
    leaf.style.setProperty('--rotation', `${rotation + 720}deg`);

    container.appendChild(leaf);
}

function addHeartsEffect() {
    const container = document.createElement('div');
    container.id = 'hearts-container';
    container.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        pointer-events: none;
        z-index: 9999;
        overflow: hidden;
    `;
    document.body.appendChild(container);

    const baseCount = 40;
    const heartCount = Math.floor(baseCount * (state.particleDensity / 100));
    const heartColors = ['❤️', '💕', '💖', '💗', '💓', '💝'];

    for (let i = 0; i < heartCount; i++) {
        createHeart(container, heartColors[Math.floor(Math.random() * heartColors.length)]);
    }
}

function createHeart(container, emoji) {
    const heart = document.createElement('div');
    heart.className = 'heart';
    heart.textContent = emoji;

    const size = Math.random() * 12 + 12;
    const duration = Math.random() * 3 + 2;
    const delay = Math.random() * 2;
    const startX = Math.random() * 100;
    const drift = (Math.random() - 0.5) * 30;

    heart.style.cssText = `
        position: absolute;
        font-size: ${size}px;
        top: -20px;
        left: ${startX}%;
        opacity: ${Math.random() * 0.5 + 0.5};
        animation: heartFall ${duration}s ease-in-out infinite;
        animation-delay: ${delay}s;
    `;

    heart.style.setProperty('--drift', `${drift}px`);

    container.appendChild(heart);
}

function addChristmasEffect() {
    const container = document.createElement('div');
    container.id = 'christmas-container';
    container.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        pointer-events: none;
        z-index: 9999;
        overflow: hidden;
    `;
    document.body.appendChild(container);

    const baseCount = 50;
    const itemCount = Math.floor(baseCount * (state.particleDensity / 100));
    const christmasItems = ['🎄', '🎁', '🎅', '🤶', '🦌', '⭐', '❄️', '🧦', '🔔'];

    for (let i = 0; i < itemCount; i++) {
        createChristmasItem(container, christmasItems[Math.floor(Math.random() * christmasItems.length)]);
    }
}

function createChristmasItem(container, emoji) {
    const item = document.createElement('div');
    item.className = 'christmas-item';
    item.textContent = emoji;

    const size = Math.random() * 12 + 12;
    const duration = Math.random() * 3 + 2;
    const delay = Math.random() * 2;
    const startX = Math.random() * 100;
    const drift = (Math.random() - 0.5) * 40;
    const rotation = Math.random() * 360;

    item.style.cssText = `
        position: absolute;
        font-size: ${size}px;
        top: -20px;
        left: ${startX}%;
        opacity: ${Math.random() * 0.5 + 0.5};
        animation: christmasFall ${duration}s linear infinite;
        animation-delay: ${delay}s;
        transform: rotate(${rotation}deg);
    `;

    item.style.setProperty('--drift', `${drift}px`);
    item.style.setProperty('--rotation', `${rotation + 720}deg`);

    container.appendChild(item);
}

let fireworksIntervalId = null;

function addFireworksEffect() {
    const container = document.createElement('div');
    container.id = 'fireworks-container';
    container.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        pointer-events: none;
        z-index: 9999;
        overflow: hidden;
    `;
    document.body.appendChild(container);

    // Create periodic fireworks bursts
    function createFirework() {
        if (!state.particleEffectsEnabled) {
            return;
        }
        const x = Math.random() * 100;
        const y = Math.random() * 50 + 20; // Between 20% and 70% from top

        const firework = document.createElement('div');
        firework.className = 'firework';
        firework.style.cssText = `
            position: absolute;
            left: ${x}%;
            top: ${y}%;
            width: 4px;
            height: 4px;
            background: white;
            border-radius: 50%;
            animation: fireworkBurst 1s ease-out forwards;
        `;

        container.appendChild(firework);

        // Create particles
        const colors = ['#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff', '#00ffff', '#ffffff'];
        const particleCount = 20;

        for (let i = 0; i < particleCount; i++) {
            setTimeout(() => {
                const particle = document.createElement('div');
                particle.className = 'firework-particle';
                const color = colors[Math.floor(Math.random() * colors.length)];
                const angle = (Math.PI * 2 * i) / particleCount;
                const distance = 50 + Math.random() * 30;

                const dx = Math.cos(angle) * distance;
                const dy = Math.sin(angle) * distance;

                particle.style.cssText = `
                    position: absolute;
                    left: ${x}%;
                    top: ${y}%;
                    width: 3px;
                    height: 3px;
                    background: ${color};
                    border-radius: 50%;
                    box-shadow: 0 0 6px ${color};
                    animation: fireworkParticle 1.5s ease-out forwards;
                    --dx: ${dx}px;
                    --dy: ${dy}px;
                `;

                container.appendChild(particle);

                // Remove particle after animation
                setTimeout(() => {
                    if (particle.parentNode) {
                        particle.parentNode.removeChild(particle);
                    }
                }, 1500);
            }, 100);
        }

        // Remove firework after animation
        setTimeout(() => {
            if (firework.parentNode) {
                firework.parentNode.removeChild(firework);
            }
        }, 1000);
    }

    // Calculate interval and initial count based on density
    const baseInterval = 800;
    const interval = Math.max(200, baseInterval * (100 / state.particleDensity)); // Faster with higher density
    const initialCount = Math.floor(3 * (state.particleDensity / 100));

    // Clear any existing interval
    if (fireworksIntervalId) {
        clearInterval(fireworksIntervalId);
    }

    // Create fireworks periodically
    fireworksIntervalId = setInterval(() => {
        if (!state.particleEffectsEnabled) {
            clearInterval(fireworksIntervalId);
            fireworksIntervalId = null;
            return;
        }
        createFirework();
    }, interval);

    // Also create some immediately
    for (let i = 0; i < initialCount; i++) {
        setTimeout(() => {
            if (state.particleEffectsEnabled) {
                createFirework();
            }
        }, i * 500);
    }
}

function showRestartModal(message) {
    const modal = document.getElementById('restart-modal');
    const messageEl = modal.querySelector('.modal-message');
    if (messageEl) {
        messageEl.textContent = message;
    }
    modal.style.display = 'flex';
}

function closeRestartModal() {
    const modal = document.getElementById('restart-modal');
    modal.style.display = 'none';
}

function updateSizeButtons(size) {
    const sizeBtns = document.querySelectorAll('.size-btn');
    sizeBtns.forEach(btn => {
        if (parseInt(btn.dataset.size) === size) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}
