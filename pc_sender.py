#!/usr/bin/env python3
"""PC -> ESP32 sistem izleyici.

CPU, RAM ve GPU kullanımını toplayıp seri porttan ESP32'ye gönderir.
ESP32 ekranda çubuk olarak gösteriyor.

Gönderilen satır formatı:
    cpu,cputemp,ramused,ramtotal,gpu,gputemp
örnek:
    45.20,58.00,7.40,16.00,23,44

Kurulum:
    pip install psutil pyserial pynvml

Çalıştırma:
    python3 pc_sender.py /dev/ttyUSB0   (Linux)
    python pc_sender.py COM3            (Windows)
"""
import subprocess
import sys
import time

import psutil

try:
    import pynvml
    HAS_NVML = True
except ImportError:
    HAS_NVML = False

import serial  # pip install pyserial


def get_gpu():
    """NVIDIA kart varsa % kullanım ve sıcaklığı döndürür, yoksa (0, 0)."""
    # önce pynvml ile bakıyorum, kurulu değilse nvidia-smi ile
    if HAS_NVML:
        try:
            pynvml.nvmlInit()
            handle = pynvml.nvmlDeviceGetHandleByIndex(0)
            util = pynvml.nvmlDeviceGetUtilizationRates(handle)
            temp = pynvml.nvmlDeviceGetTemperature(
                handle, pynvml.NVML_TEMPERATURE_GPU
            )
            return util.gpu, temp
        except Exception:
            pass

    try:
        out = subprocess.run(
            ["nvidia-smi", "--query-gpu=utilization.gpu,temperature.gpu",
             "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=3,
        )
        if out.returncode == 0:
            u, t = out.stdout.strip().split(",")
            return int(u), int(t)
    except Exception:
        pass

    # le linux'ta bazı sistemlerde psutil gpu veriyi de çekebiliyor
    try:
        gpus = psutil.sensors_gpus()
        if gpus:
            u = gpus[0].load * 100
            t = gpus[0].current_temperature or 0
            return int(u), int(t)
    except Exception:
        pass

    return 0, 0


def get_cpu_temp():
    """İşlemci sıcaklığı (Linux). Bulunamazsa ya da Windows ise 0 döner."""
    for name, entries in psutil.sensors_temperatures().items():
        if name.lower() in ("coretemp", "k10temp", "cpu_thermal", "acpitz"):
            for e in entries:
                if e.current:
                    return e.current
    return 0.0


def main():
    if len(sys.argv) < 2:
        print("Kullanım: python3 pc_sender.py <seri_port>")
        print("Örnek   : python3 pc_sender.py /dev/ttyUSB0")
        sys.exit(1)

    port = sys.argv[1]

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # esp32 resetten sonra açılana kadar bekle
    except serial.SerialException as e:
        print(f"Seri port açılamadı: {e}")
        sys.exit(1)

    print(f"{port} açıldı, veri gönderiliyor... (Ctrl+C ile çık)")
    try:
        while True:
            cpu = psutil.cpu_percent(interval=None)
            cpu_temp = get_cpu_temp()
            mem = psutil.virtual_memory()
            ram_used = mem.used / (1024**3)
            ram_total = mem.total / (1024**3)
            gpu, gpu_temp = get_gpu()

            line = f"{cpu:.1f},{cpu_temp:.1f},{ram_used:.2f},{ram_total:.1f},{gpu},{gpu_temp}"
            ser.write((line + "\n").encode())
            print(line)
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        print("\nKapandı.")


if __name__ == "__main__":
    main()