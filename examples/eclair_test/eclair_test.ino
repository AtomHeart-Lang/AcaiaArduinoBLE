#include <AcaiaArduinoBLE.h>

void keepAlive(AcaiaArduinoBLE &scale) {
  BLE.poll();
  scale.newWeightAvailable();
}

void waitForEnter(AcaiaArduinoBLE &scale) {
  Serial.println("Press Enter to continue...");
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    keepAlive(scale);
    delay(100);
  }
  while (Serial.available()) Serial.read();
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  delay(1000);
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Eclair Scale v2.1.0 Protocol Test");
  Serial.println("========================================");
  Serial.println();

  if (!BLE.begin()) {
    Serial.println("FAIL: BLE.begin() failed");
    while (1) {}
  }
  Serial.println("BLE started");

  AcaiaArduinoBLE scale(false);
  scale.init();
  delay(500); keepAlive(scale);
  if (!scale.isConnected()) {
    Serial.println("FAIL: Scale not connected");
    while (1) {}
  }
  Serial.println("Connected to scale");
  Serial.println();

  // -- Test 1: Tare-only --
  {
    Serial.println("-- Test 1: Tare --");
    Serial.println("Expected: T 01 01 sent, weight resets, timer NOT affected");

    for (int i = 0; i < 5; i++) { scale.newWeightAvailable(); delay(100); BLE.poll(); }
    float w_before = scale.getWeight();

    scale.tare();
    delay(500); keepAlive(scale);

    float w_after = 0;
    for (int i = 0; i < 10 && w_after == 0; i++) {
      if (scale.newWeightAvailable()) w_after = scale.getWeight();
      delay(150); keepAlive(scale);
    }

    Serial.print("  Weight before: "); Serial.println(w_before, 1);
    Serial.print("  Weight after:  "); Serial.println(w_after, 1);
    if (w_after < 1.0 && w_after > -1.0)
      Serial.println("  PASS");
    else
      Serial.println("  WARN - tare may not have completed");
    Serial.println();
    waitForEnter(scale);
  }

  // -- Test 2: Reset Timer --
  {
    Serial.println("-- Test 2: Reset Timer --");
    Serial.println("Expected: R 01 01 sent, timer stops at 0:00");

    scale.resetTimer();
    delay(500); keepAlive(scale);

    for (int i = 0; i < 5; i++) {
      if (scale.newWeightAvailable())
        Serial.print("  Weight: "); Serial.println(scale.getWeight(), 2);
      delay(150);
    }

    Serial.println("  Verify on scale: timer = 0:00 and STOPPED");
    Serial.println("  -> visually confirm PASS/FAIL");
    Serial.println();
    waitForEnter(scale);
  }

  // -- Test 3: Start Timer --
  {
    Serial.println("-- Test 3: Start Timer --");
    Serial.println("Expected: S 01 01 sent, tare+reset+start, timer counting");

    scale.startTimer();
    Serial.println("  Waiting 3 seconds...");
    for(int _i=0;_i<30;_i++){ delay(100); keepAlive(scale); }

    for (int i = 0; i < 5; i++) {
      if (scale.newWeightAvailable())
        Serial.print("  Weight: "); Serial.println(scale.getWeight(), 2);
      delay(150);
    }

    Serial.println("  Verify on scale: timer MUST be counting up");
    Serial.println("  -> visually confirm PASS/FAIL");
    Serial.println();
    waitForEnter(scale);
  }

  // -- Test 4: Tare while timer running (KEY test) --
  {
    Serial.println("-- Test 4: Tare while timer running (v2.1.0 KEY) --");
    Serial.println("Expected: weight resets, timer KEEPS RUNNING");

    scale.tare();
    Serial.println("  Waiting 2 seconds...");
    for(int _i=0;_i<20;_i++){ delay(100); keepAlive(scale); }

    for (int i = 0; i < 5; i++) {
      if (scale.newWeightAvailable())
        Serial.print("  Weight: "); Serial.println(scale.getWeight(), 2);
      delay(150);
    }

    Serial.println("  Verify on scale:");
    Serial.println("    - Weight = 0 (tare OK)");
    Serial.println("    - Timer STILL running (NOT stopped)");
    Serial.println("  -> visually confirm PASS/FAIL");
    Serial.println();
    waitForEnter(scale);
  }

  // -- Test 5: Stop Timer --
  {
    Serial.println("-- Test 5: Stop Timer --");
    Serial.println("Expected: E 01 01 sent, timer freezes");

    scale.stopTimer();
    delay(500); keepAlive(scale);

    for (int i = 0; i < 5; i++) {
      if (scale.newWeightAvailable())
        Serial.print("  Weight: "); Serial.println(scale.getWeight(), 2);
      delay(150);
    }

    Serial.println("  Verify on scale: timer should be FROZEN");
    Serial.println("  -> visually confirm PASS/FAIL");
    Serial.println();
    waitForEnter(scale);
  }

  // -- Summary --
  Serial.println("========================================");
  Serial.println("  Test Sequence Complete");
  Serial.println();
  Serial.println("  1. Tare only");
  Serial.println("  2. Reset timer (R)");
  Serial.println("  3. Start timer (S) - tare+reset+start");
  Serial.println("  4. Tare while running - KEY v2.1.0 test");
  Serial.println("  5. Stop timer (E)");
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
