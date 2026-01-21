# ESP32 WordClock - WiFi Setup Guide

This WordClock project is built around the **Ikea Sannahed 35x35cm picture frame**, transforming it into an elegant word-based time display with integrated sensors and web control.

## 📶 WiFi Setup

The WordClock features an automatic WiFi configuration system with two modes of operation:

### Initial Setup (Access Point Mode)

When the WordClock is powered on for the first time or cannot connect to a saved WiFi network, it automatically enters **Access Point (AP) Mode**:

1. **Connect to the WordClock's WiFi network:**
   - Network Name (SSID): `WordClock-Setup`
   - Password: `wordclock123`

2. **Access the configuration interface:**
   - Open your web browser and navigate to: `http://192.168.4.1`
   - You should see a yellow banner indicating "Access Point Mode"

3. **Configure your WiFi:**
   - Enter your home WiFi network name (SSID)
   - Enter your WiFi password
   - Click "Connect to WiFi"
   - The WordClock will save the settings and restart

4. **Reconnect to your home network:**
   - After restart, the WordClock will connect to your home WiFi
   - You can now disconnect from the `WordClock-Setup` network and reconnect to your home WiFi

### Normal Operation (Station Mode)

Once configured, the WordClock connects to your home WiFi network automatically. You can access the web interface in two ways:

#### Option 1: Using mDNS hostname (Recommended)
```
http://wordclock.local
```
This works on most devices without needing to know the IP address. The mDNS (Multicast DNS) service makes the WordClock discoverable on your local network.

> **Note:** mDNS may not work on all Android devices or older Windows versions. If `wordclock.local` doesn't work, use the IP address method below.

#### Option 2: Using IP Address
1. Check your router's DHCP client list to find the WordClock's IP address
2. Alternatively, connect via serial monitor (115200 baud) to see the IP address printed during startup
3. Navigate to: `http://[IP_ADDRESS]` (e.g., `http://192.168.1.100`)

## 🌐 Web Interface Features

Once connected, the web interface allows you to:

- **Display Mode:** Switch between word clock and digital clock modes
- **LED Color Control:** Adjust RGBW color values with preset options
- **Timezone Configuration:** Set your local timezone for accurate time display
- **Sensor Monitoring:** View real-time temperature, humidity, pressure, and brightness data
- **WiFi Reconfiguration:** Change WiFi settings if needed
- **Firmware Updates:** Update firmware over WiFi without USB cable (see OTA section below)

## 🔄 Over-The-Air (OTA) Firmware Updates

The WordClock supports wireless firmware updates, eliminating the need for USB cable after initial setup.

### Web-Based OTA Update (Recommended)

1. **Access the update page:**
   - Open http://wordclock.local
   - Click the "🔄 Go to Firmware Update Page" button
   - Or directly visit: http://wordclock.local/update

2. **Upload new firmware:**
   - Select the firmware file (`.bin`)
   - Click "Update"
   - Wait for the progress bar to complete
   - Device will automatically reboot

### PlatformIO Targets

You can also use PlatformIO to upload firmware over WiFi using the following target: OTA, which uses ArduinoOTA.
And uploads the firmware to the device over the network.
For the first upload you need to upload via the USB target to set up the WiFi connection.

### Windows Firewall / Defender Note

If OTA updates fail or timeout:
- **Windows Defender Firewall** may block OTA connections on port 3232
- **Solution:** Temporarily disable firewall or add exception for Python/PlatformIO
- **Alternative:** Use the web-based OTA method which uses standard HTTP port 80

## 🛠️ Hardware

This project is designed to fit perfectly in the **Ikea Sannahed 35x35cm picture frame**, providing a clean and modern aesthetic for your word clock display.

## 📝 Additional Configuration

After WiFi setup, you can configure:
- **Timezone:** Select from common timezones or use custom POSIX timezone strings
- **Display Colors:** Full RGBW control for customized appearance
- **Display Mode:** Choose between word-based or digital time display

The WordClock automatically syncs time via NTP (Network Time Protocol) when connected to WiFi, ensuring accurate timekeeping.
