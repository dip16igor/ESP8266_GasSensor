#include <Adafruit_NeoPixel.h>
// #include <ESP8266WiFi.h>
#include <FastBot.h>
#include <ESP8266HTTPClient.h>
#include "secrets.h"

#define PIN 2      // Pin connected to the data input of the NeoPixel strip
#define NUM_LEDS 8 // Number of LEDs in the strip

#define Buzzer 5
#define Key 12
#define LED1 2  // D4 BLUE LED "COM"
#define LED2 16 // D0 BLUE LED "WAKE"
#define GasLevelOffset 76

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

String ServerAddress = SERVER_ADDRESS; // Server address
String key = SERVER_KEY;               // Server access key
String StationID = STATION_ID;
String sensor = SENSOR_ID;

String idAdmin1 = ADMIN_CHAT_ID; // Chat ID where the bot writes logs and listens for commands
FastBot bot(BOT_TOKEN);
#define TelegramDdosTimeout 5000 // timeout
// unsigned long bot_lasttime; // last time messages' scan has been done
bool Start = false;
const unsigned long BOT_MTBS = 3600; // mean time between scan messages

WiFiClient wclient;

HTTPClient http; // Create Object of HTTPClient
int httpCode;    // server response code
String payload;  // server response string
String error;    // error code description

int rssi = 0;
int rssi1 = -99;

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// #define DEBUG_UART

bool Alarm = false;
bool AlarmKeyEnable = true;
int countAlarm1 = 0;
int countAlarm2 = 0;
int countAlarmKey = 0;
bool buzzerHi = false;
bool triger_Key = false;
bool triger_Alarm = false;
int timer_5min = 0;
int timer_sec = 0;
int counterTimer = 0;
int counterTimer1 = 0;
int counterTimerLongPress = 0;
bool StartTimer = false;

int gasLevel = 0;
#define avg 256
unsigned long Summ;

unsigned long TimeTotal1;
unsigned long TimeTotal2;
unsigned long TimeTotal;

long TimeToSend;
unsigned long TimeToSend1;

float alpha = 0.9; // Alpha value
float beta = 0.6;  // Beta value

float previousValue = 0.0; // Previous filtered value
float filteredValue = 0.0; // Filtered value
int data[avg];
int count_data = 0;
bool fl = true;
bool restart = 0;
unsigned long uptime;
float inputValue = 0;

void setup_wifi(void);
void newMsg(FB_msg &msg);
void SendToServer(void);

void OffLEDBLUE(void);
void OnLEDBLUE(void);
void OffLEDBLUE2(void);
void OnLEDBLUE2(void);
void OffBuzzer(void);
void OnBuzzer(void);

void setup()
{
  pinMode(LED1, OUTPUT);           // COM
  pinMode(LED2, OUTPUT);           // COM
  pinMode(Key, INPUT_PULLDOWN_16); // KEY

  // OffLEDBLUE();
  OffLEDBLUE2();
  Serial.begin(115200); // GPIO1, GPIO3 (TX/RX pin on ESP-12E Development Board) Аппаратный UART
  Serial.println("");
  Serial.println("RESET!");
  strip.begin();
  strip.setBrightness(255);
  strip.show();            // Initialize all pixels to off
  pinMode(Buzzer, OUTPUT); // COM
  OnBuzzer();
  delay(50);
  OffBuzzer();

  setup_wifi();

  OnBuzzer();
  delay(200);
  OffBuzzer();
  // #ifdef DEBUG_UART

  // #endif
}

void loop()
{

  // gasLevel = analogRead(A0); // Read the sound level from analog pin A0

  // Read input value (e.g., sensor data)
  inputValue = analogRead(A0) * 1;

  for (int i = avg - 1; i > 0; i--) // shift data in buffer
  {
    data[i] = data[i - 1];
  }

  data[0] = (int)inputValue; // put last measured value in first array cell

  Summ = 0;
  for (int i = 0; i < avg; i++) // calculate sum of all values in buffer
  {
    Summ += data[i];
  }
  filteredValue = (Summ / avg);

  // // Update filtered value using alpha-beta filter
  // filteredValue = alpha * (previousValue + inputValue - filteredValue);
  // filteredValue += beta * (inputValue - previousValue);

  // // Store current filtered value as previous value for next iteration
  // previousValue = filteredValue;

  gasLevel = (int)filteredValue;

  // gasLevel = (int)inputValue;

#ifdef DEBUG_UART
  // Serial.println(gasLevel);
  if (fl)
  {

    if (count_data == 512)
    {
      count_data = 0;
      for (int iii = 0; iii < 512; iii++)
      {
        Serial.println(data[iii]);
      }
      // Serial.println();
      // fl = false;
    }

    data[count_data] = gasLevel;
    count_data++;
  }

#endif

  for (int i = 0; i < NUM_LEDS; i++)
  {
    int ledBrightness = 0;

    // strip.clear();
    // strip.setPixelColor(1, strip.Color(0, 127, 0)); // Red

    if (i < gasLevel / (1024 / NUM_LEDS))
    {
      // Set LED color based on sound level
      int brightness = 255;
      // strip.setPixelColor(i, strip.Color(0, 255, 0)); // Red
      if (i >= 2 && i < 5)
        strip.setPixelColor(i, strip.Color(brightness - 127, brightness - 127, 0)); // Set LED color and brightness
      else if (i >= 5)
        strip.setPixelColor(i, strip.Color(brightness, 0, 0));
      else
        strip.setPixelColor(i, strip.Color(0, brightness, 0));
    }
    else if (i == gasLevel / (1024 / NUM_LEDS))
    {
      // Calculate intermediate value for brightness based on remainder
      ledBrightness = map(gasLevel % (1024 / NUM_LEDS), 0, 1024 / NUM_LEDS, 0, 255);
      uint32_t gammaCorrectedColor1 = strip.gamma32(ledBrightness); // Apply gamma correction using Gamma32
      if (i >= 2 && i < 5)
        strip.setPixelColor(i, strip.Color(gammaCorrectedColor1 / 2, gammaCorrectedColor1 / 2, 0)); // Set LED color and brightness
      else if (i >= 5)
        strip.setPixelColor(i, strip.Color(gammaCorrectedColor1, 0, 0));
      else
        strip.setPixelColor(i, strip.Color(0, gammaCorrectedColor1, 0));
    }
    else
    {
      strip.setPixelColor(i, strip.Color(1, 0, 2)); // Off
    }
  }

  if (gasLevel >= 4 * 1024 / NUM_LEDS) // Alarm
  {
    Alarm = true;
    if (triger_Alarm == false)
    {
      bot.sendMessage("ALARM!! Gas detected!", idAdmin1);
      Serial.println("ALARM!! Gas detected!");
      triger_Alarm = true;
    }
  }
  else if (gasLevel < 4 * 1024 / NUM_LEDS)
  {
    Alarm = false;
    if (triger_Alarm)
    {
      bot.sendMessage("Gas level is OK", idAdmin1);
      Serial.println("Gas level is OK");
      triger_Alarm = false;
    }
  }

  if (Alarm)
  {
    if (buzzerHi)
    {
      if (AlarmKeyEnable)
        OnBuzzer();

      strip.setPixelColor(7, strip.Color(255, 0, 2));
      countAlarm1++;
      if (countAlarm1 >= 500)
      {
        OffBuzzer();
        buzzerHi = false;
        countAlarm1 = 0;
      }
    }
    else
    {
      OffBuzzer();
      strip.setPixelColor(7, strip.Color(1, 0, 2));
      countAlarm2++;
      if (countAlarm2 >= 1000)
      {
        buzzerHi = true;
        countAlarm2 = 0;
      }
    }
  }
  else
  {
    if (buzzerHi)
    {
      OnBuzzer();
      countAlarm1++;
      if (countAlarm1 >= 500)
      {
        OffBuzzer();
        buzzerHi = false;
        countAlarm1 = 0;
      }
    }
    else
      OffBuzzer();
  }

  int buttonState = digitalRead(Key); // Read the state of the button

  if (buttonState == HIGH && fl == false)
  {
    fl = true;
    // bot.sendMessage("Button is pressed!", idAdmin1);
  }

  if (buttonState == HIGH && Alarm)
  { // The button is pressed
    countAlarmKey = 0;
    if (AlarmKeyEnable)
    {
      bot.sendMessage("Button is pressed!", idAdmin1);
      Serial.println("Button is pressed!");
    }
    AlarmKeyEnable = false;
  }

  if (buttonState == HIGH && Alarm == false)
  {
    delay(20);

    counterTimerLongPress++;
    if (counterTimerLongPress > 50)
    {
      counterTimerLongPress = 0;
      timer_5min = 0;
      timer_sec = 0;
      StartTimer = false;
      OnBuzzer();
      delay(300);
      OffBuzzer();
    }

    if (buttonState == HIGH && triger_Key == false)
    {
      timer_5min = timer_sec + 60 * 5;
      StartTimer = true;
      TimeTotal1 = millis();

      triger_Key = true;

      if (timer_5min > 6 * 60 * 5)
      {
        timer_5min = 6 * 60 * 5;
        OnBuzzer();
        delay(100);
        OffBuzzer();
        delay(100);
        OnBuzzer();
        delay(100);
        OffBuzzer();
      }
      else
      {
        OnBuzzer();
        delay(20);
        OffBuzzer();
      }
    }
  }
  if (buttonState == LOW)
  {
    triger_Key = false;
    counterTimerLongPress = 0;
  }

  if (StartTimer) // Timer is running
  {
    TimeTotal2 = millis();

    TimeTotal = (TimeTotal2 - TimeTotal1) / 1000;

    timer_sec = timer_5min - TimeTotal;

    if (gasLevel <= 2 * (1024 / NUM_LEDS)) // display timer scale if gas level is normal
    {
      for (int ii = 0; ii <= timer_sec / 60 / 5; ii++) // Timer_5min
      {
        strip.setPixelColor(NUM_LEDS - ii, strip.Color(0, 0, 255)); //
      }
      int ledBrightness = map(timer_sec % (60 * 5), 0, 60 * 5, 10, 255);
      uint32_t gammaCorrectedColor = strip.gamma32(ledBrightness); // Apply gamma correction using Gamma32
      // Serial.println(timer_sec / 60 / 5);
      if (timer_sec)
      {
        if (timer_sec % 2)
          strip.setPixelColor(NUM_LEDS - 1 - timer_sec / 60 / 5, strip.Color(0, gammaCorrectedColor / 2 + gammaCorrectedColor / 5 + 10, gammaCorrectedColor / 2 + gammaCorrectedColor / 5 + 10));
        else
          strip.setPixelColor(NUM_LEDS - 1 - timer_sec / 60 / 5, strip.Color(0, gammaCorrectedColor / 2, gammaCorrectedColor / 2));
      }
    }

    if (timer_sec == 3 || timer_sec == 2 || timer_sec == 1)
    {
      OnBuzzer();
      delay(50);
      OffBuzzer();
      delay(600);
    }
    if (timer_sec == 0)
    {
      timer_5min = 0;
      timer_sec = 0;
      StartTimer = false;

      strip.setPixelColor(7, strip.Color(200, 0, 50));
      strip.show();
      OnBuzzer();
      delay(200);
      OffBuzzer();
      strip.setPixelColor(7, strip.Color(0, 0, 0));
      strip.show();
      delay(50);
      OnBuzzer();
      strip.setPixelColor(7, strip.Color(200, 0, 50));
      strip.show();
      delay(200);
      OffBuzzer();
      strip.setPixelColor(7, strip.Color(0, 0, 0));
      strip.show();
      delay(50);
      OnBuzzer();
      strip.setPixelColor(7, strip.Color(200, 0, 50));
      strip.show();
      delay(200);
      OffBuzzer();
      strip.setPixelColor(7, strip.Color(0, 0, 0));
      strip.show();
      delay(50);
      OnBuzzer();
      strip.setPixelColor(7, strip.Color(200, 0, 50));
      strip.show();
      delay(200);
      OffBuzzer();

      bot.sendMessage("Timer!!", idAdmin1);
      strip.setPixelColor(7, strip.Color(0, 0, 0));
      strip.show();
    }
  }

  countAlarmKey++;
  if (countAlarmKey > 100000)
  {
    countAlarmKey = 100000;
    AlarmKeyEnable = true;
  }

  // delay(100);   // Delay for visual effect (adjust if needed)
  strip.show(); // Update the LED strip

  if (restart)
  {
    Serial.println("Restart!");
    bot.tickManual();
    ESP.restart();
  }

  bot.tick(); // тикаем в луп
  // #ifdef DEBUG_UART

  //   TimeTotal2 = micros(); // время начала работы
  //   Serial.println("");
  //   Serial.print("Time: ");
  //   Serial.print(TimeTotal2 - TimeTotal1);
  // #endif

  if (TimeToSend >= 60) // send data to server once per minute
  {
    SendToServer();
    TimeToSend1 = millis() / 1000;
    TimeToSend = 0;
  }

  TimeToSend = millis() / 1000 - TimeToSend1;
}

// обработчик сообщений
void newMsg(FB_msg &msg)
{
  String chat_id = msg.chatID;
  String text = msg.text;
  String firstName = msg.first_name;
  String lastName = msg.last_name;
  String userID = msg.userID;
  String userName = msg.username;

  bot.notify(false);

  // выводим всю информацию о сообщении
  Serial.println(msg.toString());
  FB_Time t(msg.unix, 5);
  Serial.print(t.timeString());
  Serial.print(' ');
  Serial.println(t.dateString());

  if (chat_id == idAdmin1)
  {
    if (text == "/restart" && msg.chatID == idAdmin1)
      restart = 1;

    // обновление прошивки по воздуху из чата Telegram
    if (msg.OTA && msg.chatID == idAdmin1)
    {
      strip.setPixelColor(7, strip.Color(100, 0, 0));
      strip.show();
      bot.update();
      strip.setPixelColor(7, strip.Color(0, 100, 0));
      strip.show();
      delay(300);
    }
    if (text == "/ping")
    {
      strip.setPixelColor(7, strip.Color(100, 0, 100));
      strip.show();
      Serial.println("/ping");
      uptime = millis(); // + 259200000;
      int min = uptime / 1000 / 60;
      int hours = min / 60;
      int days = hours / 24;

      long rssi = WiFi.RSSI();
      int gasLevelPercent = map(gasLevel, GasLevelOffset, 1024, 0, 100);

      String time = "pong! Сообщение принято в ";
      time += t.timeString();
      time += ". Uptime: ";
      time += days;
      time += "d ";
      time += hours - days * 24;
      time += "h ";
      // time += min - days * 24 * 60 - (hours - days * 24) * 60;
      time += min - hours * 60;
      time += "m";

      time += ". RSSI: ";
      time += rssi;
      time += " dB";

      time += ". Gas: ";
      time += gasLevelPercent;
      time += " %";

      bot.sendMessage(time, chat_id);
      Serial.println("Message is sended");

      strip.setPixelColor(7, strip.Color(1, 0, 2));
      strip.show();
    }
    if (text == "/start" || text == "/start@dip16_GasSensor_bot")
    {
      strip.setPixelColor(7, strip.Color(100, 100, 0));
      strip.show();
      bot.showMenuText("Команды:", "\ping \t \restart", chat_id, false);
      delay(300);
      strip.setPixelColor(7, strip.Color(0, 0, 0));
      strip.show();
    }
  }
}

void setup_wifi()
{
  int counter_WiFi = 0;
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
    OnLEDBLUE2();
    strip.setPixelColor(7, strip.Color(127, 0, 127));
    strip.show(); // Update the LED strip
    delay(400);
    OffLEDBLUE2();
    strip.setPixelColor(7, strip.Color(1, 0, 2));
    strip.show(); // Update the LED strip
    counter_WiFi++;
    if (counter_WiFi >= 10)
    {
      break;
      //  Serial.print("ERROR! Unable connect to WiFi");
    }
  }

  if (counter_WiFi >= 10)
  {
    Serial.print("ERROR! Unable connect to WiFi");
    strip.setPixelColor(7, strip.Color(50, 50, 0));
    strip.show(); // Update the LED strip
    delay(1000);
  }
  else
  {
    // randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    bot.setChatID("");  // передай "" (пустую строку) чтобы отключить проверку
    bot.skipUpdates();  // пропустить непрочитанные сообщения
    bot.attach(newMsg); // обработчик новых сообщений

    bot.sendMessage("Gas sensor Restart OK", idAdmin1);
    // if (!idAdmin2.isEmpty())
    //   bot.sendMessage("Restart OK", idAdmin2);
    Serial.println("Message to telegram was sended");

    digitalWrite(LED1, LOW); // зажигаем синий светодиод
  }
}

void SendToServer(void)
{
  rssi = WiFi.RSSI();
  if (rssi >= 0)
  {
    rssi = rssi1;
  }
  else
  {
    rssi1 = rssi;
  }
  String serverPath = ServerAddress + "?key=" + key + "&station=" + StationID + "&GasRssi=" + rssi + "&GasLevel=" + inputValue;

#ifdef DEBUG_UART
  Serial.print("HTTP begin..\t");
  Serial.print("Server: ");
  Serial.println(serverPath);
#endif

  // TimeToSend1 = millis(); // время начала передачи

  http.begin(wclient, serverPath);

#ifdef DEBUG_UART
  Serial.print("HTTP GET..\t");
#endif

  httpCode = http.GET();

  error = http.errorToString(httpCode).c_str();
  payload = http.getString(); // payload = Response from server

  // TimeToSend2 = millis(); // время приема ответа

  // TimeToSend = TimeToSend2 - TimeToSend1;

#ifdef DEBUG_UART
  Serial.print("TimeToSend: ");
  Serial.print(TimeToSend);
  Serial.println(" ms ");
#endif

  if (httpCode > 0)
  {
#ifdef DEBUG_UART
    Serial.print("httpCode: ");
    Serial.print(httpCode);
    Serial.print("\t");
    Serial.print(error);
    if (httpCode == 200)
    {
      Serial.print(" [OK]\t");
    }
    Serial.print(" payload: ");
    Serial.print(payload);
    Serial.println(); // перевод строки
#endif
  }
  else
  {
#ifdef DEBUG_UART
    Serial.print("failed, error: "); // Display Error msg to PC
    Serial.print(httpCode);
    Serial.print(":\t");
    Serial.print(error);
    Serial.println(); // перевод строки
#endif
  }

  http.end(); // Close Connection

  // wclient.stop(); // отключение от WiFi
}

void OffLEDBLUE(void) // Turn off LED1 COM
{                     // turn the LED off (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
} // write 1 to blue LED port
void OnLEDBLUE(void) // Turn on LED1
{                    // turn the LED on by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
} // запись 0 в порт синего светодиода

void OffLEDBLUE2(void) // Turn off LED2 WAKE
{                      // turn the LED off (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN_AUX, HIGH);
} // write 1 to blue LED port
void OnLEDBLUE2(void) // Turn on LED2
{                     // turn the LED on by making the voltage LOW
  digitalWrite(LED_BUILTIN_AUX, LOW);
} // запись 0 в порт синего светодиода

void OnBuzzer(void) // Turn on buzzer
{                   // turn the buzzer on (HIGH is the voltage level)
  digitalWrite(Buzzer, HIGH);
} // write 1 to buzzer port
void OffBuzzer(void) // Turn off buzzer
{                    // turn the LED on by making the voltage LOW
  digitalWrite(Buzzer, LOW);
} // запись 0 в порт синего светодиода