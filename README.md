# Temple Automatic Music System 🕉️🎶

This project is an **automated music playback system for temples**, designed to play devotional songs at specific times of the day without requiring manual intervention. Built using microcontrollers (like Arduino), this system utilizes a Real-Time Clock (RTC) module and a relay to trigger music or bell sounds through a speaker or amplifier.

---

## 📌 Features

- ⏰ **Time-based playback** using DS1307 RTC
- 💾 **EEPROM configuration** for storing custom time slots
- 🔘 **Button-based setup mode** to configure start/end times
- 🔔 **Relay control** to switch on music systems or temple bells
- 🖥️ **I2C LCD display** for user feedback and status display
- 💡 **Auto exit setup mode** after inactivity
- 🔋 Low power and standalone operation

---

## 🔧 Hardware Requirements

- Arduino (Uno/Nano)
- DS1307 RTC module
- 16x2 I2C LCD Display
- Push Buttons (x2)
- Relay Module (5V)
- Speaker or Amplifier (controlled by relay)
- Optional: EEPROM (if not using internal storage)

---

## ⚙️ How It Works

1. On boot, the system displays the current time.
2. Press the **setup button** to enter configuration mode.
3. Use buttons to set:
   - Start Time 1 & End Time 1
   - Start Time 2 & End Time 2
4. Configured time slots are stored in EEPROM.
5. At the configured times, the relay is activated to play music.
6. LCD displays the current mode and playback status.

---

## 🧠 Logic Flow

```
[Start] 
   ↓
[Load Time Slots from EEPROM]
   ↓
[Check RTC Every Second]
   ↓
[If Time Matches Slot → Activate Relay]
   ↓
[LCD Shows Status]
   ↓
[Setup Button → Enter Config Mode]
   ↓
[Inactivity Timeout → Exit Setup Mode]
   ↓
[Loop Back]
```

---

## 📂 File Structure

```
temple_automatic_music_system/
├── temple_music.ino        # Main Arduino sketch
├── README.md               # Project Documentation
└── diagrams/               # (Optional) Circuit or flow diagrams
```

---

## 📜 License

This project is open-source and released under the [MIT License](LICENSE).

---

## 🙏 Acknowledgements

Inspired by traditional temple rituals and the need for automation in rural areas.

> 💡 *Feel free to contribute or adapt this system for your local temple or spiritual center.*
