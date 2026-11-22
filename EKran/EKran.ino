#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <SD.h>
// =======================
// DNS SHIELD
// =======================
WiFiUDP dnsUdp;
WiFiUDP upstreamUdp;

const int DNS_PORT = 53;

// Filtreli upstream DNS
IPAddress upstreamDNS(76, 76, 2, 37);

// NetworkUDP bu kartta bir datagramdan en fazla 1460 bayt okur.
byte dnsBuffer[1460];
bool dnsSocketsReady = false;

unsigned long dnsQueries = 0;
unsigned long dnsBlocked = 0;

String lastDnsDomain = "";
String lastBlockedDomain = "";
// =======================
// WIFI
// =======================
#include "network_settings.h"

unsigned long lastReconnectAttempt = 0;

WebServer server(80);

// =======================
// PC VERILERI
// =======================
float cpuValue = 0;
float ramValue = 0;
float ramUsedGB = 0;
float ramTotalGB = 0;
int pingValue = 0;
String pcIp = "0.0.0.0";
bool internetOnline = false;
float gpuValue = 0;
int gpuTemp = 0;
float tuborgPrice = 0;
float netglPrice = 0;
float trholPrice = 0;
// =======================
// SD BLOCKLIST
// =======================
#define SD_CS 5

// 12.000 domain = yaklasik 48 KB RAM
#define MAX_BLOCKED_DOMAINS 12000

uint32_t blockHashes[MAX_BLOCKED_DOMAINS];

size_t blockHashCount = 0;

bool sdBlocklistLoaded = false;
// =======================
// EKRAN
// =======================
TFT_eSPI tft = TFT_eSPI();

// =======================
// TOUCH PINLERI
// =======================
#define TOUCH_CLK   25
#define TOUCH_MOSI  32
#define TOUCH_MISO  39
#define TOUCH_CS    33
#define TOUCH_IRQ   36

// =======================
// TOUCH KALIBRASYONU
// =======================
#define TOUCH_X_MIN 359
#define TOUCH_X_MAX 3675
#define TOUCH_Y_MIN 255
#define TOUCH_Y_MAX 3518

SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// =======================
// SAYFALAR
// =======================
enum Screen {
  HOME,
  PC_SCREEN,
  NET_SCREEN,
  BIST_SCREEN,
  DNS_SCREEN
};

Screen currentScreen = HOME;

bool touchReleased = true;


// =======================
// TOUCH OKUMA
// =======================
bool getTouch(int &x, int &y) {

  if (!touch.touched()) {
    touchReleased = true;
    return false;
  }

  if (!touchReleased) {
    return false;
  }

  TS_Point p = touch.getPoint();

  x = map(
    p.x,
    TOUCH_X_MIN,
    TOUCH_X_MAX,
    0,
    239
  );

  y = map(
    p.y,
    TOUCH_Y_MIN,
    TOUCH_Y_MAX,
    0,
    319
  );

  x = constrain(x, 0, 239);
  y = constrain(y, 0, 319);

  touchReleased = false;

  return true;
}


// =======================
// BUTON CIZ
// =======================
void drawButton(
  int x,
  int y,
  int w,
  int h,
  uint16_t color,
  const char* text
) {

  tft.fillRoundRect(
    x,
    y,
    w,
    h,
    8,
    color
  );

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    8,
    TFT_WHITE
  );

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_WHITE,
    color
  );

  tft.drawString(
    text,
    x + w / 2,
    y + h / 2,
    2
  );
}


// =======================
// BASLIK
// =======================
void drawHeader(const char* title) {

  tft.fillRect(
    0,
    0,
    240,
    45,
    TFT_DARKGREY
  );

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_WHITE,
    TFT_DARKGREY
  );

  tft.drawString(
    title,
    120,
    22,
    2
  );
}


// =======================
// ALT BILGI
// =======================
void drawFooter() {

  tft.drawFastHLine(
    10,
    294,
    220,
    TFT_DARKGREY
  );

  tft.setTextDatum(ML_DATUM);

  tft.setTextColor(
    TFT_GREEN,
    TFT_BLACK
  );

  tft.drawString(
    "ESP32",
    10,
    307,
    1
  );

  tft.setTextDatum(MR_DATUM);

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  tft.drawString(
    "ONLINE",
    230,
    307,
    1
  );
}


// =======================
// ANA MENU
// =======================
void drawHome() {

  currentScreen = HOME;

  tft.fillScreen(TFT_BLACK);

  drawHeader("YUSUF PANEL");

  drawButton(
    15,
    65,
    95,
    75,
    TFT_BLUE,
    "PC"
  );

  drawButton(
    130,
    65,
    95,
    75,
    TFT_DARKGREEN,
    "NETWORK"
  );

  drawButton(
    15,
    160,
    95,
    75,
    TFT_PURPLE,
    "BIST"
  );

  drawButton(
    130,
    160,
    95,
    75,
    TFT_ORANGE,
    "DNS"
  );

  drawFooter();
}


// =======================
// GERI BUTONU
// =======================
void drawBackButton() {

  drawButton(
    50,
    255,
    140,
    45,
    TFT_DARKGREY,
    "GERI"
  );
}


// =======================
// PC MONITOR
// =======================
void drawPCScreen() {

  currentScreen = PC_SCREEN;

  tft.fillScreen(TFT_BLACK);

  drawHeader("PC MONITOR");

  tft.setTextDatum(TL_DATUM);


  // =====================
  // CPU
  // =====================
  tft.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  tft.drawString(
    "CPU",
    20,
    70,
    2
  );

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  String cpuText =
    String(cpuValue, 1) + " %";

  tft.drawString(
    cpuText,
    140,
    70,
    2
  );


  // =====================
  // RAM
  // =====================
  tft.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  tft.drawString(
    "RAM",
    20,
    110,
    2
  );

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  String ramText =
    String(ramValue, 1) + " %";

  tft.drawString(
    ramText,
    140,
    110,
    2
  );


  // =====================
  // MEMORY
  // =====================
  tft.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  tft.drawString(
    "MEMORY",
    20,
    150,
    2
  );

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  String memoryText =
    String(ramUsedGB, 1)
    + " / "
    + String(ramTotalGB, 1)
    + " GB";

  tft.drawString(
    memoryText,
    100,
    150,
    2
  );


  // =====================
  // GPU
  // =====================
  tft.setTextColor(
    TFT_MAGENTA,
    TFT_BLACK
  );

  tft.drawString(
    "GPU",
    20,
    195,
    2
  );

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  String gpuText =
    String(gpuValue, 0)
    + " %   "
    + String(gpuTemp)
    + " C";

  tft.drawString(
    gpuText,
    100,
    195,
    2
  );


  drawBackButton();
}


// =======================
// NETWORK
// =======================
void drawNetworkScreen() {

  currentScreen = NET_SCREEN;

  tft.fillScreen(TFT_BLACK);

  drawHeader("NETWORK");

  tft.setTextDatum(TL_DATUM);


  // INTERNET
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("INTERNET", 20, 65, 2);

  if (internetOnline) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("ONLINE", 135, 65, 2);
  }
  else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("OFFLINE", 135, 65, 2);
  }


  // PING
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("PING", 20, 100, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  String pingText =
    String(pingValue) + " ms";

  tft.drawString(
    pingText,
    135,
    100,
    2
  );


  // PC IP
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("PC IP", 20, 135, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString(
    pcIp,
    20,
    160,
    2
  );


  // ESP IP
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("ESP IP", 20, 195, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString(
    WiFi.localIP().toString(),
    20,
    220,
    2
  );


  drawBackButton();
}


// =======================
// BIST
// =======================
void drawBistScreen() {

  currentScreen = BIST_SCREEN;

  tft.fillScreen(TFT_BLACK);

  drawHeader("BIST");

  tft.setTextDatum(TL_DATUM);


  // TUBORG
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("TUBORG", 20, 70, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  String tuborgText =
    String(tuborgPrice, 2) + " TL";

  tft.drawString(
    tuborgText,
    120,
    70,
    2
  );


  // NETGL
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("NETGL", 20, 110, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  String netglText =
    String(netglPrice, 2) + " TL";

  tft.drawString(
    netglText,
    120,
    110,
    2
  );


  // TRHOL
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("TRHOL", 20, 150, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  String trholText =
    String(trholPrice, 2) + " TL";

  tft.drawString(
    trholText,
    120,
    150,
    2
  );


  // DURUM
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  tft.drawString(
    "LIVE DATA",
    20,
    205,
    2
  );


  drawBackButton();
}


// =======================
// DNS SHIELD EKRANI
// =======================
String dnsDisplayName(const String& domain) {
  // Font 1: 35 karakter 210 piksele sigar.
  if (domain.length() == 0) return "-";
  if (domain.length() <= 35) return domain;
  return domain.substring(0, 15) + "..." + domain.substring(domain.length() - 17);
}

void refreshDnsScreen(bool force) {
  static unsigned long lastCheck = 0;
  static unsigned long shownQueries = 0;
  static bool shownActive = false;

  unsigned long now = millis();
  if (!force && now - lastCheck < 250) return;
  lastCheck = now;

  bool active = dnsSocketsReady && WiFi.status() == WL_CONNECTED;
  if (!force && dnsQueries == shownQueries && active == shownActive) return;
  shownActive = active;
  shownQueries = dnsQueries;

  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(96);
  tft.setTextColor(active ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(active ? "ACTIVE" : "OFFLINE", 130, 54, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(dnsQueries), 130, 80, 2);
  tft.drawString(String(dnsBlocked), 130, 106, 2);
  float rate = dnsQueries ? 100.0f * dnsBlocked / dnsQueries : 0.0f;
  tft.drawString(String(rate, 1) + "%", 130, 132, 2);

  tft.setTextPadding(210);
  tft.drawString(dnsDisplayName(lastDnsDomain), 15, 184, 1);
  tft.drawString(dnsDisplayName(lastBlockedDomain), 15, 226, 1);
  tft.setTextPadding(0);
}

void drawDnsScreen() {
  currentScreen = DNS_SCREEN;
  tft.fillScreen(TFT_BLACK);
  drawHeader("DNS SHIELD");
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("STATUS", 15, 54, 2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("QUERIES", 15, 80, 2);
  tft.drawString("LAST QUERY", 15, 162, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("BLOCKED", 15, 106, 2);
  tft.drawString("LAST BLOCK", 15, 204, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("BLOCK RATE", 15, 132, 2);

  drawBackButton();
  refreshDnsScreen(true);
}


// =======================
// BUTON KONTROL
// =======================
bool inside(
  int tx,
  int ty,
  int x,
  int y,
  int w,
  int h
) {

  return (
    tx >= x &&
    tx <= x + w &&
    ty >= y &&
    ty <= y + h
  );
}


// =======================
// WEB SERVER
// =======================
void setupWebServer() {


  // =====================
  // TEST
  // =====================
  server.on(
    "/hello",
    HTTP_GET,
    []() {

      server.send(
        200,
        "text/plain",
        "HELLO ESP32"
      );
    }
  );


  // =====================
  // CPU
  // =====================
  server.on(
    "/cpu",
    HTTP_GET,
    []() {

      if (server.hasArg("value")) {

        cpuValue =
          server.arg("value").toFloat();

        Serial.print("CPU: ");
        Serial.print(cpuValue);
        Serial.println("%");

        if (currentScreen == PC_SCREEN) {

          tft.fillRect(
            130,
            65,
            105,
            30,
            TFT_BLACK
          );

          tft.setTextDatum(TL_DATUM);

          tft.setTextColor(
            TFT_WHITE,
            TFT_BLACK
          );

          String text =
            String(cpuValue, 1)
            + " %";

          tft.drawString(
            text,
            140,
            70,
            2
          );
        }
      }

      server.send(
        200,
        "text/plain",
        "OK"
      );
    }
  );


  // =====================
  // RAM %
  // =====================
  server.on(
    "/ram",
    HTTP_GET,
    []() {

      if (server.hasArg("value")) {

        ramValue =
          server.arg("value").toFloat();

        Serial.print("RAM: ");
        Serial.print(ramValue);
        Serial.println("%");

        if (currentScreen == PC_SCREEN) {

          tft.fillRect(
            130,
            105,
            105,
            30,
            TFT_BLACK
          );

          tft.setTextDatum(TL_DATUM);

          tft.setTextColor(
            TFT_WHITE,
            TFT_BLACK
          );

          String text =
            String(ramValue, 1)
            + " %";

          tft.drawString(
            text,
            140,
            110,
            2
          );
        }
      }

      server.send(
        200,
        "text/plain",
        "OK"
      );
    }
  );


  // =====================
  // MEMORY
  // =====================
  server.on(
    "/memory",
    HTTP_GET,
    []() {

      if (server.hasArg("used")) {

        ramUsedGB =
          server.arg("used").toFloat();
      }

      if (server.hasArg("total")) {

        ramTotalGB =
          server.arg("total").toFloat();
      }

      Serial.print("MEMORY: ");
      Serial.print(ramUsedGB);
      Serial.print(" / ");
      Serial.print(ramTotalGB);
      Serial.println(" GB");

      if (currentScreen == PC_SCREEN) {

        // MEMORY DEGERININ OLDUGU ALANI TEMIZLE
        tft.fillRect(
          95,
          145,
          140,
          30,
          TFT_BLACK
        );

        tft.setTextDatum(TL_DATUM);

        tft.setTextColor(
          TFT_WHITE,
          TFT_BLACK
        );

        String text =
          String(ramUsedGB, 1)
          + " / "
          + String(ramTotalGB, 1)
          + " GB";

        tft.drawString(
          text,
          100,
          150,
          2
        );
      }

      server.send(
        200,
        "text/plain",
        "OK"
      );
    }
  );


  // =====================
  // GPU
  // =====================
  server.on(
    "/gpu",
    HTTP_GET,
    []() {

      if (server.hasArg("usage")) {

        gpuValue =
          server.arg("usage").toFloat();
      }

      if (server.hasArg("temp")) {

        gpuTemp =
          server.arg("temp").toInt();
      }

      Serial.print("GPU: ");
      Serial.print(gpuValue);
      Serial.print("% | ");
      Serial.print(gpuTemp);
      Serial.println(" C");

      if (currentScreen == PC_SCREEN) {

        // GPU DEGERININ OLDUGU ALANI TEMIZLE
        tft.fillRect(
          90,
          190,
          145,
          30,
          TFT_BLACK
        );

        tft.setTextDatum(TL_DATUM);

        tft.setTextColor(
          TFT_WHITE,
          TFT_BLACK
        );

        String gpuText =
          String(gpuValue, 0)
          + " %   "
          + String(gpuTemp)
          + " C";

        tft.drawString(
          gpuText,
          100,
          195,
          2
        );
      }

      server.send(
        200,
        "text/plain",
        "OK"
      );
    }
  );
server.on(
  "/network",
  HTTP_GET,
  []() {

    if (server.hasArg("ping")) {
      pingValue =
        server.arg("ping").toInt();
    }

    if (server.hasArg("pcip")) {
      pcIp =
        server.arg("pcip");
    }

    if (server.hasArg("online")) {
      internetOnline =
        server.arg("online") == "1";
    }


    Serial.print("PING: ");
    Serial.print(pingValue);
    Serial.print(" ms | PC IP: ");
    Serial.print(pcIp);
    Serial.print(" | INTERNET: ");
    Serial.println(
      internetOnline ? "ONLINE" : "OFFLINE"
    );


    if (currentScreen == NET_SCREEN) {
      drawNetworkScreen();
    }


    server.send(
      200,
      "text/plain",
      "OK"
    );
  }
);
server.on(
  "/bist",
  HTTP_GET,
  []() {

    if (server.hasArg("tuborg")) {
      tuborgPrice =
        server.arg("tuborg").toFloat();
    }

    if (server.hasArg("netgl")) {
      netglPrice =
        server.arg("netgl").toFloat();
    }

    if (server.hasArg("trhol")) {
      trholPrice =
        server.arg("trhol").toFloat();
    }


    Serial.print("TUBORG: ");
    Serial.println(tuborgPrice);

    Serial.print("NETGL: ");
    Serial.println(netglPrice);

    Serial.print("TRHOL: ");
    Serial.println(trholPrice);


    if (currentScreen == BIST_SCREEN) {
      drawBistScreen();
    }


    server.send(
      200,
      "text/plain",
      "OK"
    );
  }
);
  // SERVER BASLAT
  server.begin();

  Serial.println(
    "Web server hazir"
  );
}
// =======================
// DNS DOMAIN OKUMA
// =======================
uint16_t readDns16(const byte* field) {
  return (uint16_t(field[0]) << 8) | field[1];
}

String readDnsDomain(const byte* buffer, int length, int& questionEnd) {

  if (length < 17 || (buffer[2] & 0x80) || readDns16(buffer + 4) != 1) {
    return "";
  }

  int pos = 12;

  String domain = "";

  while (pos < length) {

    byte labelLength = buffer[pos++];

    // Domain sonu
    if (labelLength == 0) {
      if (pos + 4 > length || pos - 12 > 255) return "";
      questionEnd = pos + 4;  // QTYPE ve QCLASS dahil
      return domain;
    }

    // Hatali / desteklenmeyen paket
    if ((labelLength & 0xC0) != 0) {
      return "";
    }

    if (pos + labelLength > length || pos + labelLength - 12 >= 255) {
      return "";
    }

    if (domain.length() > 0) {
      domain += ".";
    }

    for (int i = 0; i < labelLength; i++) {

      char c = (char)buffer[pos++];

      // Kucuk harfe cevir
      if (c >= 'A' && c <= 'Z') {
        c = c + 32;
      }

      domain += c;
    }
  }

  return "";
}


// =======================
// DOMAIN HASH
// =======================
uint32_t hashDomain(const String& domain) {

  uint32_t hash = 2166136261UL;

  for (size_t i = 0; i < domain.length(); i++) {

    char c = domain[i];

    if (c >= 'A' && c <= 'Z') {
      c += 32;
    }

    hash ^= (uint8_t)c;
    hash *= 16777619UL;
  }

  return hash;
}


// =======================
// HASH SIRALAMA
// =======================
int compareHashes(
  const void* a,
  const void* b
) {

  uint32_t aa =
    *((const uint32_t*)a);

  uint32_t bb =
    *((const uint32_t*)b);

  if (aa < bb) return -1;
  if (aa > bb) return 1;

  return 0;
}


// =======================
// HASH LISTESINDE ARA
// =======================
bool hashExists(uint32_t target) {

  int left = 0;
  int right =
    (int)blockHashCount - 1;

  while (left <= right) {

    int middle =
      left + (right - left) / 2;

    uint32_t value =
      blockHashes[middle];

    if (value == target) {
      return true;
    }

    if (value < target) {
      left = middle + 1;
    }
    else {
      right = middle - 1;
    }
  }

  return false;
}


// =======================
// BLOCKLIST SATIRINI TEMIZLE
// =======================
String cleanBlocklistLine(
  String line
) {

  line.trim();

  if (
    line.length() == 0 ||
    line.startsWith("#") ||
    line.startsWith("!")
  ) {
    return "";
  }


  // Satir sonundaki yorumlari temizle
  int comment =
    line.indexOf('#');

  if (comment >= 0) {

    line =
      line.substring(
        0,
        comment
      );

    line.trim();
  }


  // HOSTS FORMAT:
  //
  // 0.0.0.0 example.com
  // 127.0.0.1 example.com

  if (
    line.startsWith("0.0.0.0") ||
    line.startsWith("127.0.0.1")
  ) {

    int space =
      line.indexOf(' ');

    int tab =
      line.indexOf('\t');

    int separator = -1;

    if (
      space >= 0 &&
      tab >= 0
    ) {

      separator =
        min(space, tab);
    }

    else if (space >= 0) {

      separator = space;
    }

    else {

      separator = tab;
    }


    if (separator < 0) {
      return "";
    }

    line =
      line.substring(
        separator + 1
      );

    line.trim();
  }


  // Birden fazla kolon varsa
  // sadece domaini al
  int space =
    line.indexOf(' ');

  if (space > 0) {
    line =
      line.substring(
        0,
        space
      );
  }

  int tab =
    line.indexOf('\t');

  if (tab > 0) {
    line =
      line.substring(
        0,
        tab
      );
  }


  line.trim();
  line.toLowerCase();


  // Sonunda nokta varsa kaldir
  while (
    line.endsWith(".")
  ) {

    line.remove(
      line.length() - 1
    );
  }


  // Desteklemedigimiz formatlar
  if (
    line.indexOf('*') >= 0 ||
    line.indexOf('/') >= 0 ||
    line.indexOf('^') >= 0 ||
    line.length() < 3
  ) {

    return "";
  }


  return line;
}


// =======================
// SD KARTTAN BLOCKLIST YUKLE
// =======================
void loadBlocklistFromSD() {

  Serial.println();
  Serial.println(
    "SD blocklist yukleniyor..."
  );


  // SD kart VSPI'nin normal pinlerini kullanir:
  // SCK  = 18
  // MISO = 19
  // MOSI = 23
  // CS   = 5

  if (!SD.begin(SD_CS)) {

    Serial.println(
      "SD kart bulunamadi!"
    );

    sdBlocklistLoaded = false;

    return;
  }


  File file =
    SD.open(
      "/blocklist.txt",
      FILE_READ
    );


  if (!file) {

    Serial.println(
      "blocklist.txt bulunamadi!"
    );

    SD.end();
    SPI.end();

    sdBlocklistLoaded = false;

    return;
  }


  blockHashCount = 0;


  while (
    file.available() &&
    blockHashCount <
      MAX_BLOCKED_DOMAINS
  ) {

    String line =
      file.readStringUntil('\n');

    String domain =
      cleanBlocklistLine(line);


    if (domain.length() == 0) {
      continue;
    }


    blockHashes[
      blockHashCount
    ] = hashDomain(domain);

    blockHashCount++;
  }


  file.close();


  // Hashleri sirala
  qsort(
    blockHashes,
    blockHashCount,
    sizeof(uint32_t),
    compareHashes
  );


  // Tekrarlananlari kaldir
  if (blockHashCount > 1) {

    size_t uniqueCount = 1;

    for (
      size_t i = 1;
      i < blockHashCount;
      i++
    ) {

      if (
        blockHashes[i] !=
        blockHashes[
          uniqueCount - 1
        ]
      ) {

        blockHashes[
          uniqueCount
        ] = blockHashes[i];

        uniqueCount++;
      }
    }

    blockHashCount =
      uniqueCount;
  }


  sdBlocklistLoaded =
    blockHashCount > 0;


  Serial.print(
    "Blocklist yuklendi: "
  );

  Serial.print(
    blockHashCount
  );

  Serial.println(
    " domain"
  );


  if (
    blockHashCount >=
    MAX_BLOCKED_DOMAINS
  ) {

    Serial.println(
      "UYARI: Blocklist RAM limiti doldu."
    );
  }


  // SD'yi artik kullanmayacagiz.
  // VSPI dokunmatiga birakiliyor.
  SD.end();

  SPI.end();

  Serial.println(
    "SD kapatildi, VSPI touch icin hazir."
  );
}


// =======================
// DOMAIN ENGEL KONTROLU
// =======================
bool isBlockedDomain(
  String domain
) {

  if (!sdBlocklistLoaded) {
    return false;
  }

  domain.toLowerCase();


  // Once domainin kendisine bak
  // Sonra parent domainlere bak.
  //
  // ads.example.com
  // example.com

  while (domain.length() > 0) {

    if (
      hashExists(
        hashDomain(domain)
      )
    ) {

      return true;
    }


    int dot =
      domain.indexOf('.');

    if (dot < 0) {
      break;
    }


    domain =
      domain.substring(
        dot + 1
      );
  }


  return false;
}


// =======================
// ENGELLENEN DNS CEVABI
// NXDOMAIN GONDER
// =======================
void sendBlockedResponse(
  byte* buffer,
  int length,
  IPAddress clientIP,
  uint16_t clientPort
) {

  // DNS response biti
  buffer[2] |= 0x80;

  // Recursion available
  buffer[3] |= 0x80;

  // RCODE = 3 = NXDOMAIN
  buffer[3] =
    (buffer[3] & 0xF0) | 0x03;

  // Answer count = 0
  buffer[6] = 0;
  buffer[7] = 0;

  // Authority count = 0
  buffer[8] = 0;
  buffer[9] = 0;

  // Additional count = 0
  buffer[10] = 0;
  buffer[11] = 0;

  dnsUdp.beginPacket(
    clientIP,
    clientPort
  );

  dnsUdp.write(
    buffer,
    length
  );

  dnsUdp.endPacket();
}


// =======================
// NORMAL DNS'I INTERNET'E
// YONLENDIR
// =======================
bool skipDnsName(const byte* packet, int length, int& pos) {
  while (pos < length) {
    int labelPos = pos;
    byte size = packet[pos++];
    if (size == 0) return true;
    if ((size & 0xC0) == 0xC0) {
      // Sikistirilmis ad iki bayt kullanir ve onceki bir ada isaret eder.
      if (pos >= length) return false;
      int target = ((size & 0x3F) << 8) | packet[pos++];
      return target >= 12 && target < labelPos;
    }
    if (size > 63 || pos + size > length) return false;
    pos += size;
  }
  return false;
}

bool isBlockedDnsResponse(const byte* packet, int length) {
  if (length < 12 || !(packet[2] & 0x80) || (packet[2] & 0x02)) return false;
  int rcode = packet[3] & 0x0F;
  if (rcode != 0 && rcode != 3) return false;

  int pos = 12;
  int questions = readDns16(packet + 4);
  if (questions == 1) {
    if (!skipDnsName(packet, length, pos) || pos + 4 > length) return false;
    pos += 4;
  } else if (questions != 0 || rcode != 3) {
    return false;
  }

  bool zeroAddress = false;
  int answers = readDns16(packet + 6);
  for (int i = 0; i < answers; ++i) {
    if (!skipDnsName(packet, length, pos) || pos + 10 > length) return false;
    uint16_t type = readDns16(packet + pos);
    uint16_t rrClass = readDns16(packet + pos + 2);
    uint16_t dataLength = readDns16(packet + pos + 8);
    pos += 10;
    if (pos + dataLength > length) return false;
    if (type == 1 && rrClass == 1 && dataLength == 4 &&
        packet[pos] == 0 && packet[pos + 1] == 0 &&
        packet[pos + 2] == 0 && packet[pos + 3] == 0) {
      zeroAddress = true;
    }
    pos += dataLength;
  }
  // NXDOMAIN, gercekte var olmayan alan adlarini da kapsar.
  return rcode == 3 || zeroAddress;
}

bool matchesDnsRequest(const byte* query, int questionEnd, const byte* reply, int length) {
  if (length < 12 || !(reply[2] & 0x80) || readDns16(reply) != readDns16(query)) return false;
  if ((reply[2] & 0x78) != (query[2] & 0x78)) return false;
  // Bazi hata cevaplari soru bolumunu tekrarlamaz.
  if (readDns16(reply + 4) == 0) return (reply[3] & 0x0F) != 0;
  if (readDns16(reply + 4) != 1 || length < questionEnd) return false;

  // Ilk soru sikistirilamaz. Adi harf buyuklugunden bagimsiz karsilastir.
  for (int i = 12; i < questionEnd - 4; ++i) {
    if (tolower(query[i]) != tolower(reply[i])) return false;
  }
  return memcmp(query + questionEnd - 4, reply + questionEnd - 4, 4) == 0;
}

void recordDnsBlock(const String& domain) {
  ++dnsBlocked;
  lastBlockedDomain = domain;
}

void forwardDnsRequest(
  const byte* buffer,
  int length,
  int questionEnd,
  IPAddress clientIP,
  uint16_t clientPort,
  const String& domain
) {
  static byte reply[sizeof(dnsBuffer)];
  if (!upstreamUdp.beginPacket(upstreamDNS, DNS_PORT)) return;
  if (upstreamUdp.write(buffer, length) != size_t(length) || !upstreamUdp.endPacket()) return;

  // DNS cevabini kisa sure bekle
  unsigned long startTime = millis();

  while (
    millis() - startTime < 1000
  ) {

    int responseSize =
      upstreamUdp.parsePacket();

    if (responseSize > 0) {
      if (responseSize > int(sizeof(reply)) || upstreamUdp.remoteIP() != upstreamDNS ||
          upstreamUdp.remotePort() != DNS_PORT) {
        upstreamUdp.clear();
        continue;
      }

      int received = upstreamUdp.read(reply, responseSize);
      if (received != responseSize || !matchesDnsRequest(buffer, questionEnd, reply, received)) continue;
      if (isBlockedDnsResponse(reply, received)) recordDnsBlock(domain);
      dnsUdp.beginPacket(clientIP, clientPort);
      dnsUdp.write(reply, received);
      dnsUdp.endPacket();
      return;
    }

    delay(1);
  }
}


// =======================
// DNS ISTEKLERINI DINLE
// =======================
void handleDns() {

  int packetSize =
    dnsUdp.parsePacket();

  if (packetSize <= 0) {
    return;
  }

  IPAddress clientIP =
    dnsUdp.remoteIP();

  uint16_t clientPort =
    dnsUdp.remotePort();

  if (packetSize > int(sizeof(dnsBuffer))) {
    dnsUdp.clear();
    return;
  }

  int length =
    dnsUdp.read(
      dnsBuffer,
      packetSize
    );

  if (length != packetSize) {
    return;
  }


  int questionEnd = 0;
  String domain =
    readDnsDomain(
      dnsBuffer,
      length,
      questionEnd
    );

  if (domain == "") {
    return;
  }


  dnsQueries++;

  lastDnsDomain = domain;

  Serial.print("DNS: ");
  Serial.println(domain);


  // ENGELLI DOMAIN
  if (isBlockedDomain(domain)) {

    recordDnsBlock(domain);

    Serial.print("BLOCKED: ");
    Serial.println(domain);

    sendBlockedResponse(
      dnsBuffer,
      questionEnd,
      clientIP,
      clientPort
    );

    return;
  }


  // NORMAL DOMAIN
  forwardDnsRequest(
    dnsBuffer,
    length,
    questionEnd,
    clientIP,
    clientPort,
    domain
  );
}
// =======================
// WIFI BAGLANTISINI KORU
// =======================
void maintainWiFi() {

  // Baglanti varsa hicbir sey yapma
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  // Her loop'ta yeniden baglanmaya calisma.
  // 5 saniyede bir dene.
  if (millis() - lastReconnectAttempt < 5000) {
    return;
  }

  lastReconnectAttempt = millis();

  Serial.println();
  Serial.println("WiFi koptu!");
  Serial.println("Yeniden baglaniliyor...");

  // DNS servisini gecici olarak pasif yap
  dnsSocketsReady = false;

  dnsUdp.stop();
  upstreamUdp.stop();

  // Eski WiFi baglantisini kapat
  WiFi.disconnect();

  delay(100);

  // Statik IP'yi tekrar uygula
  if (!WiFi.config(
    localIP,
    gateway,
    subnet
  )) {
    Serial.println("Statik IP tekrar ayarlanamadi!");
  }

  // WiFi'ye tekrar baglan
  WiFi.begin(
    ssid,
    password
  );

  unsigned long startTime = millis();

  // Maksimum 5 saniye bekle
  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 5000
  ) {

    delay(100);
  }


  // BAGLANTI BASARILI
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi tekrar baglandi!");

    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());


    // DNS soketlerini tekrar ac
    bool listenerReady =
      dnsUdp.begin(DNS_PORT);

    bool upstreamReady =
      upstreamUdp.begin(5353);

    dnsSocketsReady =
      listenerReady &&
      upstreamReady;


    if (dnsSocketsReady) {

      Serial.println(
        "DNS Shield tekrar aktif"
      );
    }

    else {

      Serial.println(
        "DNS soketleri tekrar acilamadi!"
      );
    }
  }

  else {

    Serial.println(
      "WiFi baglantisi basarisiz. "
      "5 saniye sonra tekrar denenecek."
    );
  }
}

// =======================
// SETUP
// =======================
void setup() {

  Serial.begin(115200);

  pinMode(
    21,
    OUTPUT
  );

  digitalWrite(
    21,
    HIGH
  );


 // =====================
// EKRAN
// =====================
tft.init();
tft.setRotation(0);


// =====================
// SD BLOCKLIST
// =====================
//
// SD kart once okunur.
// Sonra VSPI dokunmatiga verilir.
//
loadBlocklistFromSD();


// =====================
// TOUCH
// =====================
touchSPI.begin(
  TOUCH_CLK,
  TOUCH_MISO,
  TOUCH_MOSI,
  TOUCH_CS
);

touch.begin(touchSPI);

touch.setRotation(0);

  // =====================
  // WIFI BAGLANTISI
  // =====================

  WiFi.mode(WIFI_STA);

  // WiFi koparsa ESP32'nin kendi
  // otomatik yeniden baglanma ozelligini ac
  WiFi.setAutoReconnect(true);

  // WiFi bilgilerini flash'a gereksiz
  // tekrar tekrar yazma
  WiFi.persistent(false);


  // Statik IP:
  if (!WiFi.config(
    localIP,
    gateway,
    subnet
  )) {

    Serial.println(
      "Statik IP ayarlanamadi!"
    );
  }


  WiFi.begin(
    ssid,
    password
  );


  Serial.println(
    "WiFi baglaniliyor..."
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }


  Serial.println();

  Serial.println(
    "WiFi baglandi!"
  );


  Serial.print(
    "IP adresi: "
  );

  Serial.println(
    WiFi.localIP()
  );


  // =====================
  // DNS SHIELD BASLAT
  // =====================

  bool listenerReady =
    dnsUdp.begin(DNS_PORT);

  bool upstreamReady =
    upstreamUdp.begin(5353);

  dnsSocketsReady =
    listenerReady &&
    upstreamReady;


  Serial.println(
    dnsSocketsReady
      ? "DNS Shield hazir"
      : "DNS soketi acilamadi"
  );


  // WEB SERVER
  setupWebServer();


  // ANA EKRAN
  drawHome();

}
// =======================
// LOOP
// =======================
void loop() {

  // WiFi kopmus mu kontrol et
  maintainWiFi();


  // Web server
  server.handleClient();


  // DNS sadece WiFi ve DNS soketleri
  // hazirsa calissin
  if (
    WiFi.status() == WL_CONNECTED &&
    dnsSocketsReady
  ) {

    handleDns();
  }


  // DNS ekranini canli yenile
  if (currentScreen == DNS_SCREEN) {

    refreshDnsScreen(false);
  }

  int x;
  int y;


  // =====================
  // TOUCH
  // =====================
  if (getTouch(x, y)) {

    Serial.print("X: ");
    Serial.print(x);

    Serial.print(" Y: ");
    Serial.println(y);


    // ===================
    // ANA MENU
    // ===================
    if (currentScreen == HOME) {

      // PC
      if (
        inside(
          x,
          y,
          15,
          65,
          95,
          75
        )
      ) {

        drawPCScreen();
      }


      // NETWORK
      else if (
        inside(
          x,
          y,
          130,
          65,
          95,
          75
        )
      ) {

        drawNetworkScreen();
      }


      // BIST
      else if (
        inside(
          x,
          y,
          15,
          160,
          95,
          75
        )
      ) {

        drawBistScreen();
      }


      // DNS
      else if (
        inside(
          x,
          y,
          130,
          160,
          95,
          75
        )
      ) {

        drawDnsScreen();
      }
    }


    // ===================
    // GERI
    // ===================
    else {

      if (
        inside(
          x,
          y,
          50,
          255,
          140,
          45
        )
      ) {

        drawHome();
      }
    }
  }


  delay(10);
}
