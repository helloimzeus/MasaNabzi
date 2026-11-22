import os
import requests
import time

ESP32_IP = os.environ["ESP32_IP"]

TRADINGVIEW_URL = "https://scanner.tradingview.com/turkey/scan"

payload = {
    "symbols": {
        "tickers": [
            "BIST:TBORG",
            "BIST:NETGL",
            "BIST:TRHOL"
        ],
        "query": {
            "types": []
        }
    },
    "columns": [
        "close"
    ]
}


while True:

    try:

        # BIST fiyatlarini al
        response = requests.post(
            TRADINGVIEW_URL,
            json=payload,
            timeout=10
        )

        response.raise_for_status()

        result = response.json()

        prices = {}

        for item in result["data"]:

            symbol = item["s"].split(":")[1]
            price = item["d"][0]

            prices[symbol] = price


        tborg = prices.get("TBORG", 0)
        netgl = prices.get("NETGL", 0)
        trhol = prices.get("TRHOL", 0)


        print(
            f"TBORG: {tborg} TL | "
            f"NETGL: {netgl} TL | "
            f"TRHOL: {trhol} TL"
        )


        # ESP32'ye gonder
        esp_response = requests.get(
            f"http://{ESP32_IP}/bist",
            params={
                "tuborg": tborg,
                "netgl": netgl,
                "trhol": trhol
            },
            timeout=3
        )

        print("ESP32:", esp_response.text)


    except Exception as e:

        print("Hata:", e)


    # 15 saniyede bir yenile
    time.sleep(15)