#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ekran 128x64 oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static const uint8_t OLED_ADDR = 0x3C;

// wiring: SDA -> GPIO21, SCL -> GPIO22

// pc'den gelen veriler
// format: cpu,cputemp,ramused,ramtotal,gpu,gputemp
// örnek : 45.20,58.00,7.40,16.00,23,44
struct Stats {
  float cpu;      // %
  float cpuTemp;  // derece
  float ramUsed;  // GB
  float ramTotal; // GB
  int   gpu;      // %
  int   gpuTemp;  // derece
};

Stats stats = {0, 0, 0, 0, 0, 0};
bool hasData = false;
unsigned long lastRx = 0;

// seriden bir satır okuyup döndürür
String readLine() {
  static String buf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      String line = buf;
      buf = "";
      return line;
    }
    if (c != '\r' && c != '\n') buf += c;
  }
  return "";
}

// virgüllerle ayrılmış satırı alanlara böler
bool parseStats(String line, Stats& s) {
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  int c3 = line.indexOf(',', c2 + 1);
  int c4 = line.indexOf(',', c3 + 1);
  int c5 = line.indexOf(',', c4 + 1);
  if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 || c5 < 0) return false;

  s.cpu      = line.substring(0, c1).toFloat();
  s.cpuTemp  = line.substring(c1 + 1, c2).toFloat();
  s.ramUsed  = line.substring(c2 + 1, c3).toFloat();
  s.ramTotal = line.substring(c3 + 1, c4).toFloat();
  s.gpu      = line.substring(c4 + 1, c5).toInt();
  s.gpuTemp  = line.substring(c5 + 1).toInt();
  return true;
}

// pct 0-1 arası doluluk oranı, çerçeveli bar çizer
void drawBar(int x, int y, int w, int h, float pct) {
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  int fill = (int)((w - 2) * pct);
  if (fill < 0) fill = 0;
  if (fill > w - 2) fill = w - 2;
  if (fill > 0) display.fillRect(x + 1, y + 1, fill, h - 2, SSD1306_WHITE);
}

// üstte başlık + ayraç çizgisi
void drawTop(const char* title) {
  display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
}

void drawMonitor() {
  display.clearDisplay();
  drawTop("PC Monitor");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char tmp[12];

  // ---- CPU ----
  display.setCursor(0, 13);
  display.print("CPU");
  sprintf(tmp, "%d%%", (int)stats.cpu);
  display.setCursor(120 - 4 * 6, 13); // sağa yasla
  display.print(tmp);
  drawBar(0, 20, SCREEN_WIDTH, 3, stats.cpu / 100.0);

  // ---- RAM ----
  float ramPct = (stats.ramTotal > 0.1) ? stats.ramUsed / stats.ramTotal : 0;
  display.setCursor(0, 27);
  display.print("RAM");
  sprintf(tmp, "%.1f/%.1f", stats.ramUsed, stats.ramTotal);
  display.setCursor(120 - 7 * 6, 27); // "4.9/7.6" gibi 7 karakter
  display.print(tmp);
  drawBar(0, 34, SCREEN_WIDTH, 3, ramPct);

  // ---- GPU ----
  display.setCursor(0, 41);
  display.print("GPU");
  sprintf(tmp, "%d%%", stats.gpu);
  display.setCursor(120 - 4 * 6, 41);
  display.print(tmp);
  drawBar(0, 48, SCREEN_WIDTH, 3, stats.gpu / 100.0);

  // ---- sıcaklıklar altta ----
  if (stats.cpuTemp > 0.5) {
    display.setCursor(0, 57);
    display.print("C:");
    sprintf(tmp, "%d", (int)stats.cpuTemp);
    display.print(tmp);
  }
  if (stats.gpuTemp > 0) {
    sprintf(tmp, "G:%d", stats.gpuTemp);
    display.setCursor(120 - 4 * 6, 57);
    display.print(tmp);
  }

  // bağlantı koparsa uyarı göster
  bool timedOut = (millis() - lastRx > 3000);
  if (timedOut) {
    display.fillRect(0, 51, SCREEN_WIDTH, 13, SSD1306_BLACK);
    display.setCursor(0, 54);
    display.print("Baglanti yok!");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  drawMonitor();
}

void loop() {
  String line = readLine();
  if (line.length() > 0) {
    if (parseStats(line, stats)) {
      hasData = true;
      lastRx = millis();
      drawMonitor();
    }
  }

  // 3 saniye veri gelmezse "baglanti yok" ekranını yenile
  if (hasData && millis() - lastRx > 3000) {
    drawMonitor();
  }

  delay(20);
}