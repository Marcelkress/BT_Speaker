#include "ESP_I2S.h"
#include "BluetoothA2DPSink.h"

// i2s and bluetooth
const uint8_t I2S_SCK = 5;       /* Audio data bit clock - DAC: BCLK */
const uint8_t I2S_WS = 18;       /* Audio data left and right clock - DAC: LRC */
const uint8_t I2S_SDOUT = 19;    /* ESP32 audio data output (to speakers) - DAC: DIN */
I2SClass i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Bluetooth connection 
int statusLED = 14;
bool connected = false;

// On / off LED
int onOffLED = 32;

// Touch sensor variables
#define touchSensorPin 21
bool playing = false;

int lastTouchState = LOW;
unsigned long lastTapTime = 0;
int tapCount = 0;
float tapWaitTime = 500;

// Sleep variables
#define SLEEP_BUTTON_PIN 33  // Use GPIO 0 or another RTC-capable pin
unsigned long sleepPressStart = 0;
bool isPressed = false;
const unsigned long disconnectHoldDuration = 2000;  // 2 seconds
bool longPressHandled;

// Volume variables
int volumeUpPin = 23;
int lastUpState = HIGH; // the previous state from the input pin
int currentUpState; 

int volumeDownPin = 12;
int lastDownState = HIGH;
int currentDownState;

int currentVolumePercent = 50;
int volumeInterval = 10;

void setup() 
{
  Serial.begin(115200);
  delay(100);

  // -------- SLEEP -------
  // If waking from deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from deep sleep via button");
  }

  pinMode(SLEEP_BUTTON_PIN, INPUT_PULLUP);  // Button between pin and GND
  Serial.println("Running... hold button 2 sec and release to sleep");


  // ------- AUDIO AND BLUETOOTH SETUP -----------
  i2s.setPins(I2S_SCK, I2S_WS, I2S_SDOUT); // Setting i2s pins

  if (!i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) // Initialize i2s
  {
    Serial.println("Failed to initialize I2S!");
    while (1); // do nothing
  }
  
  a2dp_sink.set_avrc_rn_playstatus_callback(playingStatus); // Assigning callback method
  a2dp_sink.set_auto_reconnect(true, 200); // Set auto reconnect to true
  a2dp_sink.start("Mini boksen"); // Start with given name 

  // Setting pinmodes
  pinMode(statusLED, OUTPUT); 
  pinMode(onOffLED, OUTPUT);
  pinMode(volumeUpPin, INPUT_PULLUP);
  pinMode(volumeDownPin, INPUT_PULLUP);
  pinMode(touchSensorPin, INPUT);
  
  connected = false;

  // Setting LED status
  digitalWrite(statusLED, LOW);
  digitalWrite(onOffLED, HIGH);
}

void loop() 
{
  // Checking if device is still connected
  connected = a2dp_sink.is_avrc_connected();
  digitalWrite(statusLED, connected ? HIGH : LOW);
  
  // -------- TOUCH SENSOR -----------
  /*
  touchSensor();
  static bool lastTouch = false;
  bool currentTouch = digitalRead(touchSensorPin);  // HIGH = touched

  // Touch started - recorded
  if (currentTouch && !lastTouch) 
  {
    buttonPressTime = millis();
  }

  // When touch ends / when finger releases button
  if (!currentTouch && lastTouch) 
  {
    // Recording the duration of the press
    unsigned long pressDuration = millis() - buttonPressTime;

    // We count a tap if the button is held for between a min and max amount of time
    if (pressDuration > playPauseTime && pressDuration < maxPressTime) 
    {
        if (playing) 
        {
          a2dp_sink.pause();
        } 
        else 
        {
          a2dp_sink.play();
        }
    }
  }

  lastTouch = currentTouch;
  */
  touchSensor();

  // -------- VOLUME -----------
  volumeButtons();

  //----------- SLEEP -------------
  powerButton();
}

void touchSensor()
{
  int touchState = digitalRead(touchSensorPin);  // HIGH = touched
  unsigned long currentTime = millis();

  // Detect rising edge (tap)
  if (touchState == HIGH && lastTouchState == LOW) 
  {
    tapCount++;
    lastTapTime = currentTime;
  }

  // If it's been more than 100ms since the last tap, act based on count
  if (tapCount > 0 && (currentTime - lastTapTime > tapWaitTime)) 
  {
    if (tapCount == 1) {
      Serial.println("Single tap detected: Pausing...");
      if (playing) 
        {
          a2dp_sink.pause();
        } 
        else 
        {
          a2dp_sink.play();
        }
    }
    else if (tapCount == 2) {
      Serial.println("Double tap detected: Fast-forwarding...");
      a2dp_sink.next();
    }
    else {
      Serial.print("Multi-tap (");
      Serial.print(tapCount);
      Serial.println(") ignored.");
    }

    // Reset after action
    tapCount = 0;
  }

  lastTouchState = touchState;
}


void volumeButtons()
{
  currentUpState = digitalRead(volumeUpPin);

  if(lastUpState == LOW && currentUpState == HIGH)
  {
    currentVolumePercent += volumeInterval;

    if(currentVolumePercent > 100)
    {
      currentVolumePercent = 100;
    }

    a2dp_sink.set_volume((currentVolumePercent * 127) / 100);

    Serial.println("Volume: ");
    Serial.print(currentVolumePercent);
  }
  lastUpState = currentUpState;

  currentDownState = digitalRead(volumeDownPin);
  if(lastDownState == LOW && currentDownState == HIGH)
  {
    currentVolumePercent -= volumeInterval;

    if(currentVolumePercent < 0)
    {
      currentVolumePercent = 0;
    }

    a2dp_sink.set_volume((currentVolumePercent * 127) / 100);
    Serial.println("Volume: ");
    Serial.print(currentVolumePercent);
  }
  lastDownState = currentDownState;

  currentVolumePercent = (a2dp_sink.get_volume() * 100) / 127;
}

void powerButton()
{
  bool sleepButtonState = digitalRead(SLEEP_BUTTON_PIN) == LOW;

  // Press start
  if(sleepButtonState && !isPressed)
  {
    sleepPressStart = millis();
    isPressed = true;
    Serial.println("press started");
  }

  // Button is being held
  if(sleepButtonState && isPressed)
  {
    if(millis() - sleepPressStart > disconnectHoldDuration)
    {
      a2dp_sink.disconnect();
      Serial.println("Long press - disconnecting");
      return;
    }
  }

  // Button is released
  if(!sleepButtonState && isPressed)
  {
    unsigned long pressDuration = millis() - sleepPressStart;

    if(pressDuration < disconnectHoldDuration)
    {
      Serial.println("Short press - going to sleep");
      esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON_PIN, 0);  // Wake on LOW
      esp_deep_sleep_start();
    }

    isPressed = false;
  }
}


void playingStatus(esp_avrc_playback_stat_t playback)
{
  // Setting the playing bool according to the playback status of the device.
  switch (playback) 
  {
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      playing = true;
      Serial.println("Playing.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      playing = false;
      Serial.println("Paused.");
      break;
  }
}
