#include <Arduino.h>
extern "C" {
  #include "node_a.h"
  #include "node_b.h"
}

#include <stdarg.h>

// 전역 printf redirect
extern "C" int serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = Serial.vprintf(fmt, args);  // Serial Monitor 출력
    va_end(args);
    return len;
}

//#define ROLE_NODE_A 1
#define ROLE_NODE_B 1

static const int LED_PIN = 2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(1500);
  setvbuf(stdout, NULL, _IONBF, 0); // 버퍼링 끄기
  Serial.println("\n[MAIN] start");

#if defined(ROLE_NODE_A)
  node_a_setup();
#else
  node_b_setup();
#endif
}

void loop() {
  static uint32_t lastBlink=0, lastLog=0;
  uint32_t now=millis();

  if (now - lastBlink >= 250) {
    lastBlink = now;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  if (now - lastLog >= 1000) {
    lastLog = now;
    Serial.println("[MAIN] loop tick");
  }

#if defined(ROLE_NODE_A)
  node_a_loop();
#else
  node_b_loop();
#endif

  delay(2);
}
