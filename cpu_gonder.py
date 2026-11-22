import os
import psutil
import requests
import subprocess
import time
import socket
import subprocess
import re

ESP32_IP = os.environ["ESP32_IP"]

while True:
    # PC IP
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        s.connect(("8.8.8.8", 80))
        pc_ip = s.getsockname()[0]
    finally:
        s.close()


# Ping
    try:
        result = subprocess.check_output(
            ["ping", "-n", "1", "8.8.8.8"],
            text=True
        )

        match = re.search(
            r"(?:time|süre)[=<](\d+)",
            result,
            re.IGNORECASE
        )

        if match:
            ping = int(match.group(1))
        else:
            ping = 0

        online = 1

    except:
        ping = 0
        online = 0

    cpu = psutil.cpu_percent(interval=1)

    memory = psutil.virtual_memory()

    ram = memory.percent
    used = memory.used / (1024 ** 3)
    total = memory.total / (1024 ** 3)

    # NVIDIA GPU bilgisi
    gpu_output = subprocess.check_output(
        [
            "nvidia-smi",
            "--query-gpu=utilization.gpu,temperature.gpu",
            "--format=csv,noheader,nounits"
        ],
        text=True
    ).strip()

    gpu_usage, gpu_temp = gpu_output.split(",")

    gpu_usage = gpu_usage.strip()
    gpu_temp = gpu_temp.strip()

    try:

        requests.get(
            f"http://{ESP32_IP}/cpu?value={cpu}",
            timeout=2
        )

        requests.get(
            f"http://{ESP32_IP}/ram?value={ram}",
            timeout=2
        )

        requests.get(
            f"http://{ESP32_IP}/memory?used={used:.1f}&total={total:.1f}",
            timeout=2
        )

        requests.get(
            f"http://{ESP32_IP}/gpu?usage={gpu_usage}&temp={gpu_temp}",
            timeout=2
        )
        requests.get(
            f"http://{ESP32_IP}/network"
            f"?ping={ping}"
            f"&pcip={pc_ip}"
            f"&online={online}",
            timeout=2
        )

        print(
            f"CPU: {cpu:.1f}% | "
            f"RAM: {ram:.1f}% | "
            f"GPU: {gpu_usage}% | "
            f"GPU Sicaklik: {gpu_temp} C"
        )

    except Exception as e:
        print("Baglanti hatasi:", e)

    time.sleep(1)