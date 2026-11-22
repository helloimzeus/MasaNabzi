// Ağ ayarları nereye yazılır?
// 1. Bu dosyanın bir kopyasını EKran.ino ile aynı klasöre kaydedin.
//    Kopyanın adı tam olarak network_settings.h olmalı.
//
//    EKran/
//      EKran.ino
//      network_settings.example.h   (paylaşılan örnek)
//      network_settings.h           (kendi ayarlarınızı buraya yazın)
//
// 2. network_settings.h kopyasını açın ve aşağıdaki beş değeri düzenleyin.
// 3. Kopyayı kaydedip Arduino IDE'de EKran.ino'yu açarak karta yükleyin.
//    EKran.ino içindeki #include "network_settings.h" satırı ayarları okur.

#pragma once
#include <WiFi.h>

// Wi-Fi adınızı ve parolanızı çift tırnakların arasına yazın.
const char* ssid = "WIFI_ADINIZ";
const char* password = "WIFI_PAROLANIZ";

// Aşağıdaki adresler örnektir. Her adresi dört sayı halinde, virgülle ayırın.
// Windows'ta ipconfig komutu ağ geçidini ve alt ağ maskesini gösterir.
IPAddress localIP(192, 0, 2, 50);    // ESP32 için ağınızda ayırdığınız sabit IP
IPAddress gateway(192, 0, 2, 1);     // Modeminizin/yönlendiricinizin yerel IP adresi
IPAddress subnet(255, 255, 255, 0);  // Wi-Fi bağlantınızın alt ağ maskesi
