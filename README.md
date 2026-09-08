# ESP32 PC Monitor

PC'nin CPU, RAM ve GPU değerlerini gösteren küçük bir izleyici projesi. ESP32 + OLED (SSD1306 128x64) üzerinde gerçek zamanlı olarak donanım verilerini çubuk grafiklerle gösterir.

## Özellikler

- CPU kullanımı + sıcaklık
- RAM kullanımı (kullanılan / toplam)
- GPU kullanımı + sıcaklık
- Tam genişlik dolum barları, sağa hizalı değerler
- Bağlantı koparsa ekranda uyarı

## Gerekli Malzemeler

| Parça | Açıklama |
|-------|----------|
| ESP32 | herhangi bir geliştirme kartı |
| OLED | SSD1306 128x64, I2C (0x3C) |
| Bağlantı kablo | 4 adet (VCC, GND, SDA, SCL) |

<img width="474" height="324" alt="OIP-4158305239" src="https://github.com/user-attachments/assets/417bdf7d-86e9-4b47-a97d-9b71f2fd3afd" />


## Kurulum

### ESP32 tarafı

1. `esp32_pc_monitor.ino` dosyasını Arduino IDE ile aç.
2. Şu kütüphaneleri kur:
   - Adafruit GFX Library
   - Adafruit SSD1306
3. Board olarak kendi ESP32 modelini seç ve kodu yükle.

### PC tarafı

```bash
pip install psutil pyserial pynvml
python3 pc_sender.py /dev/ttyUSB0   # Windows: python pc_sender.py COM3
```

### Çalıştırma

ESP32'yi USB ile PC'ye bağlıyken:

```bash
python3 pc_sender.py /dev/ttyUSB0
```

Seri port adını bulmak için: `ls /dev/ttyUSB*` (Linux) ya da Aygıt Yöneticisi (Windows).

## Veri Protokolü

PC -> ESP32'ye gönderilen satır formatı (saniyede bir):

```
cpu,cputemp,ramused,ramtotal,gpu,gputemp
45.20,58.00,7.40,16.00,23,44
```

Alanlar:

| Alan | Anlam | Birim |
|------|-------|-------|
| cpu | İşlemci kullanımı | % |
| cputemp | İşlemci sıcaklığı | °C |
| ramused | Kullanılan RAM | GB |
| ramtotal | Toplam RAM | GB |
| gpu | GPU kullanımı | % |
| gputemp | GPU sıcaklığı | °C |

GPU verisi alınamazsa (entegre/yok) `0,0` gönderilir, ekranda da sıcaklık gösterilmez.

## Notlar

- CPU sıcaklığı Windows'ta çoğu zaman okunamaz, o durumda ekranda sıcaklık gösterilmez.
- NVIDIA GPU için `pynvml` (veya `nvidia-smi`) kullanılır.
