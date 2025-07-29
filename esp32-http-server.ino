#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Define OLED parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define SNOOZE_DURATION 5

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 5.5  
#define UTC_OFFSET_DST 0

// Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET * 3600, 60000);

// Global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = {0, 1};
int alarm_minutes[] = {1, 10};
bool alarm_triggered[] = {false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - View Alarms", "5 - Delete Alarm"};

bool snoozed[] = {false, false};  
int snooze_hours[] = {0, 0};
int snooze_minutes[] = {0, 0};

void print_line(String text, int column, int row, int text_size, bool clear_display = true) {
  if (clear_display) {
    display.clearDisplay(); // Clear the display only if specified
  }
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); // (column, row)
  display.println(text);

  display.display();
}

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_OK, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(9600);

 // Initialize alarms to zero
  for (int i = 0; i < n_alarms; i++) {
    alarm_hours[i] = 0;
    alarm_minutes[i] = 0;
    alarm_triggered[i] = false;
    snoozed[i] = false;
    snooze_hours[i] = 0;
    snooze_minutes[i] = 0;
  }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3v internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Show the display buffer on the screen. You must call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);

  timeClient.begin();
  timeClient.update();

  // Synchronize time using NTP
  configTime(UTC_OFFSET * 3600, UTC_OFFSET_DST, NTP_SERVER);

  // Clear the buffer
  display.clearDisplay();

  print_line("Welcome to Medibox!", 10, 20, 2);
  delay(500);
  display.clearDisplay();
}

void loop() {
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }
  check_temp();
}



void print_time_now(void) {
  print_line(String(days), 0, 0, 2);
  print_line(":", 20, 0, 2);
  print_line(String(hours), 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}

void update_time() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  days = timeinfo.tm_mday;

}

void ring_alarm(int alarm_index) {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);
  bool break_happened = false;

  while (!break_happened) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_OK) == LOW) {  // Snooze alarm
        delay(200);
        snooze_alarm(alarm_index);
        break_happened = true;
        break;
      } else if (digitalRead(PB_CANCEL) == LOW) {  // Stop alarm
        delay(200);
        alarm_triggered[alarm_index] = false;  // Reset the alarm trigger
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void snooze_alarm(int alarm_index) {
  snoozed[alarm_index] = true;
  snooze_minutes[alarm_index] = alarm_minutes[alarm_index] + SNOOZE_DURATION;
  snooze_hours[alarm_index] = alarm_hours[alarm_index];

  if (snooze_minutes[alarm_index] >= 60) {
    snooze_minutes[alarm_index] -= 60;
    snooze_hours[alarm_index] = (snooze_hours[alarm_index] + 1) % 24;
  }

  display.clearDisplay();
  print_line("Snoozed for 5 min", 0, 0, 2);
  delay(1000);
}

void update_time_with_check_alarm(void) {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm(i);
        alarm_triggered[i] = true;
      }

      if (snoozed[i] && snooze_hours[i] == hours && snooze_minutes[i] == minutes) {
        ring_alarm(i);
        snoozed[i] = false;
      }
    }
  }
}

int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    } else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    } else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    } else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}

void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    } else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

void set_time_zone() {
  int temp_offset = UTC_OFFSET;

  while (true) {
    display.clearDisplay();
    print_line("Enter UTC offset: " + String(temp_offset), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_offset += 1;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_offset -= 1;
    } else if (pressed == PB_OK) {
      delay(200);
      timeClient.setTimeOffset(temp_offset * 3600);
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time zone set", 0, 0, 2);
  delay(1000);
}

void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}

void view_alarms() {
  display.clearDisplay();
  print_line("Alarm 1:    " + String(alarm_hours[0]) + ":" + String(alarm_minutes[0]), 0, 0, 2, false);
  print_line("Alarm 2:    " + String(alarm_hours[1]) + ":" + String(alarm_minutes[1]), 0, 32, 2, false);
  delay(6000);
}

void delete_alarm() {
  int temp_alarm = 0;

  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm " + String(temp_alarm + 1), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_alarm += 1;
      temp_alarm = temp_alarm % n_alarms;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_alarm -= 1;
      if (temp_alarm < 0) {
        temp_alarm = n_alarms - 1;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[temp_alarm] = 0;
      alarm_minutes[temp_alarm] = 0;
      alarm_triggered[temp_alarm] = false;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm deleted", 0, 0, 2);
  delay(1000);
}

void run_mode(int mode) {
  if (mode == 0) {
    set_time_zone();
  } else if (mode == 1 || mode == 2) {
    set_alarm(mode - 1);
  } else if (mode == 3) {
    view_alarms();
  } else if (mode == 4) {
    delete_alarm();
  }
}

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32) {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 40, 1);
  } else if (data.temperature < 24) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 40, 1);
  }

  if (data.humidity > 80) {
    display.clearDisplay();
    print_line("HUMIDITY HIGH", 0, 40, 1);
  } else if (data.humidity < 65) {
    display.clearDisplay();
    print_line("HUMIDITY LOW", 0, 40, 1);
  }
}