# 🚨 ESP32 Telegram Alarm System

Control an ESP32 alarm system via Telegram! Send `/on` to trigger a siren on a Bluetooth speaker, and `/off` to stop it.

## Features
- 🔴 Trigger alarm via Telegram (`/on`)
- 🔕 Stop alarm via Telegram (`/off`)
- 🔊 Streams audio to Bluetooth speaker (A2DP)
- 🔗 WebSocket communication with VPS


## Hardware
- ESP32-WROOM
- Bluetooth speaker (JBL Go 4, Anker Soundcore, etc.)
- VPS (Hostinger, DigitalOcean, AWS, etc.)

## How It Works
Telegram App -> Telegram Bot -> VPS Server -> WebSocket -> ESP32 -> Bluetooth Speaker
(/on) (API) (Node.js) (A2DP) (Plays sound)


## Setup

### 1. ESP32
1. Copy `esp32/config_example.h` to `esp32/config.h`
2. Fill in your WiFi and VPS credentials
3. Open `esp32_alarm.ino` in Arduino IDE
4. Install required libraries:
   - WebSockets by Markus Sattler
   - ESP32-A2DP by pschatzmann
5. Upload to ESP32

### 2. VPS Server
1. Copy `server/.env.example` to `server/.env`
2. Fill in your Telegram bot token and user ID
3. Install dependencies:
   ```bash
   cd server
   npm install

4. Start the server
    node server.js
5.Run in tmux for persistence:
    tmux new -s alarm
    node server.js
    Press Ctrl+B, D to detach

### 3. Telegram Bot
1. Open Telegram and search for @BotFather
2. Send /newbot and follow instructions
3. Copy the bot token
4. Search for @userinfobot and send /start to get your user ID
5. Add both to .env file

Commands
Command 	Description
/start	    Welcome message
/on	        Trigger the alarm
/off	    Stop the alarm

Demo
https://youtube.com/shorts/wbdeGV3n-mY


Troubleshooting
ESP32 not connecting?

    Check WiFi credentials in config.h

    Ensure VPS IP is correct

    Verify WebSocket server is running

Speaker not playing?

    Put speaker in pairing mode

    Check speaker name matches in code

    JBL Go 4 requires auto-reconnect

Telegram bot not responding?

    Check bot token is correct

    Verify user ID is correct

    Check server logs for errors
