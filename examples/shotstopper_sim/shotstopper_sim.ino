/*
  shotstopper_sim.ino - Minimal shotStopper call-chain simulation test
  Tests the exact API sequence that shotStopper uses without GPIO dependencies.
  
  Simulates setBrewingState(true):  resetTimer() -> startTimer() -> tare()
  Simulates setBrewingState(false): stopTimer()
  
  Checks:
    - BLE stays connected through rapid successive writes
    - Weight reads correctly during all phases
    - Timer semantics (tare no longer resets timer on v2.1.0)
*/

#include <AcaiaArduinoBLE.h>

AcaiaArduinoBLE scale(false);

// ============================================================
// keepAlive - prevents 5s _lastPacket timeout disconnect
// ============================================================
void keepAlive() {
  BLE.poll();
  scale.newWeightAvailable();
}

// ============================================================
// readWeight - read weight with retry (up to 2s)
// ============================================================
float readWeight() {
  for (int i = 0; i < 20; i++) {
    keepAlive();
    if (scale.newWeightAvailable()) {
      return scale.getWeight();
    }
    delay(100);
  }
  return scale.getWeight();
}

// ============================================================
// printWeight - print current weight with timestamp
// ============================================================
void printWeight(const char* label) {
  float w = readWeight();
  Serial.print("  [");
  Serial.print(millis() / 1000.0, 1);
  Serial.print("s] ");
  Serial.print(label);
  Serial.print(": ");
  Serial.print(w, 2);
  Serial.println(" g");
}

// ============================================================
// waitWithKeepAlive - delay while keeping BLE alive
// ============================================================
void waitWithKeepAlive(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    keepAlive();
    delay(50);
  }
}

// ============================================================
// waitForEnter
// ============================================================
void waitForEnter() {
  Serial.println("Press Enter to continue...");
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    keepAlive();
    delay(100);
  }
  while (Serial.available()) Serial.read();
  Serial.println();
}

// ============================================================
// setup
// ============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  shotStopper Call Chain Simulation");
  Serial.println("  Eclair Scale v2.1.0");
  Serial.println("========================================");
  Serial.println();

  // --- BLE Init ---
  Serial.print("BLE init... ");
  if (!BLE.begin()) {
    Serial.println("FAIL");
    while (1) { delay(1000); }
  }
  Serial.println("OK");

  // --- Connect ---
  Serial.print("Connecting to Eclair scale... ");
  scale.init();
  waitWithKeepAlive(1000);
  if (!scale.isConnected()) {
    Serial.println("FAIL - no Eclair scale found");
    while (1) { delay(1000); }
  }
  Serial.println("OK");
  Serial.println();

  // ========================================
  // PHASE 1: Idle state check
  // ========================================
  Serial.println("--- Phase 1: Idle State ---");
  Serial.println("Expected: weight reads, timer is stopped");
  printWeight("idle weight");
  Serial.println("  -> visually confirm scale timer is 0:00 and STOPPED");
  Serial.println();
  waitForEnter();

  // ========================================
  // PHASE 2: Simulate setBrewingState(true)
  //   Exact sequence from shotStopper:
  //     scale.resetTimer();
  //     scale.startTimer();
  //     scale.tare();         // AUTOTARE
  // ========================================
  Serial.println("--- Phase 2: Brew Start (setBrewingState true) ---");
  Serial.println("Call chain: resetTimer() -> startTimer() -> tare()");

  // Step 2a: resetTimer
  Serial.print("  2a. resetTimer()... ");
  bool r_ok = scale.resetTimer();
  Serial.println(r_ok ? "OK" : "FAIL");
  waitWithKeepAlive(300);
  printWeight("after reset");

  // Step 2b: startTimer  
  Serial.print("  2b. startTimer()... ");
  bool s_ok = scale.startTimer();
  Serial.println(s_ok ? "OK" : "FAIL");
  waitWithKeepAlive(300);
  printWeight("after start");

  // Step 2c: tare (AUTOTARE)
  Serial.print("  2c. tare() (AUTOTARE)... ");
  bool t_ok = scale.tare();
  Serial.println(t_ok ? "OK" : "FAIL");
  waitWithKeepAlive(500);
  printWeight("after tare");

  // Check connectivity
  Serial.print("  Connection: ");
  Serial.println(scale.isConnected() ? "OK" : "DISCONNECTED");

  Serial.println("  -> Verify on scale: timer counting up, weight ~0");
  Serial.println();
  waitForEnter();

  // ========================================
  // PHASE 3: Simulate shot in progress
  //   Monitor weight while timer runs
  // ========================================
  Serial.println("--- Phase 3: Shot In Progress ---");
  Serial.println("Monitoring weight while timer runs (5s).");
  Serial.println("Place a cup or object on scale to simulate shot weight.");
  Serial.println();

  for (int i = 0; i < 10; i++) {
    printWeight("shot weight");
    waitWithKeepAlive(500);
  }

  Serial.print("  Connection: ");
  Serial.println(scale.isConnected() ? "OK" : "DISCONNECTED");
  Serial.println("  -> Verify on scale: timer still counting, weight = object weight");
  Serial.println();
  waitForEnter();

  // ========================================
  // PHASE 4: Tare-while-running (v2.1.0 key behavior)
  // ========================================
  Serial.println("--- Phase 4: Tare While Timer Running ---");
  Serial.println("KEY v2.1.0: tare must NOT stop/reset timer");

  Serial.print("  tare()... ");
  bool t2_ok = scale.tare();
  Serial.println(t2_ok ? "OK" : "FAIL");
  waitWithKeepAlive(500);
  printWeight("after tare (timer running)");

  Serial.print("  Connection: ");
  Serial.println(scale.isConnected() ? "OK" : "DISCONNECTED");
  Serial.println("  -> Verify: weight ~0, timer STILL counting (NOT stopped)");
  Serial.println();
  waitForEnter();

  // ========================================
  // PHASE 5: Simulate setBrewingState(false)
  //   Exact sequence: scale.stopTimer()
  // ========================================
  Serial.println("--- Phase 5: Brew End (setBrewingState false) ---");
  Serial.println("Call chain: stopTimer()");

  printWeight("weight before stop");

  Serial.print("  stopTimer()... ");
  bool e_ok = scale.stopTimer();
  Serial.println(e_ok ? "OK" : "FAIL");
  waitWithKeepAlive(500);
  printWeight("after stop");

  Serial.print("  Connection: ");
  Serial.println(scale.isConnected() ? "OK" : "DISCONNECTED");
  Serial.println("  -> Verify: timer FROZEN (not counting)");
  Serial.println();
  waitForEnter();

  // ========================================
  // PHASE 6: Post-shot drip delay simulation
  // ========================================
  Serial.println("--- Phase 6: Post-Shot (Drip Delay) ---");
  Serial.println("Monitoring final weight for 3s (simulating drip delay).");

  for (int i = 0; i < 6; i++) {
    printWeight("drip weight");
    waitWithKeepAlive(500);
  }
  printWeight("final weight");

  Serial.print("  Connection: ");
  Serial.println(scale.isConnected() ? "OK" : "DISCONNECTED");
  Serial.println();

  // ========================================
  // SUMMARY
  // ========================================
  Serial.println("========================================");
  Serial.println("  Test Sequence Complete");
  Serial.println();
  Serial.println("  Phase 1: Idle state check");
  Serial.println("  Phase 2: Brew start - resetTimer/startTimer/tare");
  Serial.println("  Phase 3: Shot in progress - weight monitoring");
  Serial.println("  Phase 4: Tare while running - KEY v2.1.0 test");
  Serial.println("  Phase 5: Brew end - stopTimer");
  Serial.println("  Phase 6: Post-shot drip monitoring");
  Serial.println();
  Serial.println("  VISUAL VERIFICATION:");
  Serial.println("    P2: Did timer start counting after startTimer?");
  Serial.println("    P4: Did tare reset weight but timer kept running?");
  Serial.println("    P5: Did stopTimer freeze the timer?");
  Serial.println("    All: Did BLE stay connected throughout?");
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
