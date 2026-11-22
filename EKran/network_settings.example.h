#pragma once
#include <WiFi.h>

// Bu dosyayi network_settings.h olarak kopyalayip kendi aginiza gore duzenleyin.
const char* ssid = "WIFI_ADINIZ";
const char* password = "WIFI_PAROLANIZ";

// Ornek adreslerdir. Kart icin yerel aginizda ayrilmis bir adres kullanin.
IPAddress localIP(192, 0, 2, 50);
IPAddress gateway(192, 0, 2, 1);
IPAddress subnet(255, 255, 255, 0);
