const WebSocket = require('ws');
const TelegramBot = require('node-telegram-bot-api');

// -------- TELEGRAM SETUP --------
const TELEGRAM_TOKEN = 'YOUR_BOT_TOKEN_HERE';  // ← Replace with your bot token
const YOUR_USER_ID = 123456789;  // ← Your user ID

// Force IPv4 for Telegram connections
process.env.NODE_OPTIONS = '--dns-result-order=ipv4first';

const bot = new TelegramBot(TELEGRAM_TOKEN, { 
    polling: true,
    baseApiUrl: 'https://api.telegram.org'
});

// -------- WEBSOCKET SETUP --------
const wss = new WebSocket.Server({ port: 8080 }, () => {
    console.log('🚀 Emergency WebSocket Server is running on port 8080');
});

let connectedDevices = new Set();
let esp32Device = null;  // Store the ESP32 device separately

// -------- TELEGRAM COMMANDS --------
bot.onText(/\/start/, (msg) => {
    const chatId = msg.chat.id;
    bot.sendMessage(chatId,
        `🔐 ESP32 Alarm Bot\n\n` +
        `🔴 /on  - Trigger alarm\n` +
        `🔕 /off - Stop alarm\n` +
        `📡 /status - Check ESP32`
    );
});

bot.onText(/\/on/, (msg) => {
    const chatId = msg.chat.id;
    
    if (chatId != YOUR_USER_ID) {
        bot.sendMessage(chatId, '❌ Unauthorized.');
        return;
    }
    
    // Check if ESP32 is connected
    if (!esp32Device || esp32Device.readyState !== WebSocket.OPEN) {
        bot.sendMessage(chatId, '❌ ESP32 not connected.');
        return;
    }
    
    // Send to ESP32 only
    esp32Device.send('EMERGENCY_TRIGGERED');
    bot.sendMessage(chatId, '🚨 ALARM TRIGGERED!');
    console.log(`📱 Telegram /on from ${chatId}`);
});

bot.onText(/\/off/, (msg) => {
    const chatId = msg.chat.id;
    
    if (chatId != YOUR_USER_ID) {
        bot.sendMessage(chatId, '❌ Unauthorized.');
        return;
    }
    
    if (!esp32Device || esp32Device.readyState !== WebSocket.OPEN) {
        bot.sendMessage(chatId, '❌ ESP32 not connected.');
        return;
    }
    
    esp32Device.send('EMERGENCY_CANCELLED');
    bot.sendMessage(chatId, '🔕 ALARM STOPPED!');
    console.log(`📱 Telegram /off from ${chatId}`);
});

bot.onText(/\/status/, (msg) => {
    const chatId = msg.chat.id;
    
    if (chatId != YOUR_USER_ID) {
        bot.sendMessage(chatId, '❌ Unauthorized.');
        return;
    }
    
    const status = esp32Device && esp32Device.readyState === WebSocket.OPEN 
        ? '🟢 ESP32 is CONNECTED' 
        : '🔴 ESP32 is DISCONNECTED';
    bot.sendMessage(chatId, `📡 Status: ${status}`);
});

// -------- WEBSOCKET HANDLER --------
wss.on('connection', (ws) => {
    console.log('📱 A new device has connected to the server.');
    connectedDevices.add(ws);
    
    // Mark this as the ESP32 if it sends a specific message
    let isEsp32 = false;

    ws.on('message', (message) => {
        const msg = message.toString();
        console.log(`📩 Signal received: ${msg}`);
        
        // Identify ESP32 by the messages it sends
        if (msg === 'HEARTBEAT' || msg === 'ESP32_CONNECTED' || msg === 'ALARM_ACK') {
            isEsp32 = true;
            esp32Device = ws;
            console.log('✅ ESP32 identified!');
        }
        
        // Relay to ALL other connected devices
        for (let device of connectedDevices) {
            if (device !== ws && device.readyState === WebSocket.OPEN) {
                device.send(msg);
            }
        }
    });

    ws.on('close', () => {
        console.log('❌ A device disconnected.');
        connectedDevices.delete(ws);
        if (esp32Device === ws) {
            esp32Device = null;
            console.log('⚠️ ESP32 disconnected.');
        }
    });
});

console.log('\n========================================');
console.log('🚀 Server Started');
console.log('========================================');
console.log(`📡 WebSocket: ws://0.0.0.0:8080`);
console.log(`🤖 Telegram bot running...`);
console.log(`👤 Authorized user ID: ${YOUR_USER_ID}`);
console.log('========================================');