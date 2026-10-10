#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
//  CONFIGURACIÓN
// =====================================================

// --- Servos ---
const int SERVO_PINS[] = {9, 6, 5, 3, 11, 10};
const int SERVO_COUNT  = sizeof(SERVO_PINS) / sizeof(SERVO_PINS[0]);

// --- Potenciómetros (solo lectura / monitoreo) ---
const int   POT_PINS[]   = {0, 1, 2, 3, 6};
const char* POT_LABELS[] = {"A0", "A1", "A2", "A3", "A6"};  // mismo orden que POT_PINS
const int   POT_COUNT    = sizeof(POT_PINS) / sizeof(POT_PINS[0]);

// Cambio mínimo (cuentas del ADC, 0-1023) para considerar que un pot se movió.
const int POT_THRESHOLD = 8;

// Tiempo que se muestra el marcador "<<" en el último pot movido
const unsigned long POT_HIGHLIGHT_MS = 1000;

// --- Botón ---
const int BUTTON_PIN    = 4;
const int LONG_PRESS_MS = 2000;   // pulsación larga = cambiar de modo

// --- Barrido de servos ---
const int ANGLE_MIN     = 20;
const int ANGLE_MAX     = 60;
const int ANGLE_STEP    = 5;
const int LOOP_DELAY_MS = 100;

// --- OLED ---
const int OLED_RESET   = 4;       // coincide con BUTTON_PIN; usa -1 si tu OLED no tiene reset
const int OLED_ADDRESS = 0x3C;

enum Mode { SINGLE_SERVO = 0, ALL_SERVOS = 1 };

// =====================================================
//  PROTOTIPOS
// =====================================================

const char* modeName();
void handleButton();
void readPots();
bool isPotHighlighted();
void logPotMoved(int index, int diff);
void updateSweep();
void onSweepFinished();
void attachAll();
void detachAll();
void logServoStatus();
void updateDisplay();

// =====================================================
//  ESTADO
// =====================================================

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
Servo servos[SERVO_COUNT];

// Servos / barrido
Mode mode       = ALL_SERVOS;
int  current    = 0;       // índice del servo activo (0..SERVO_COUNT-1)
int  angle      = 50;
int  direction  = +1;      // +1 sube, -1 baja
bool firstSweep = true;

// Potenciómetros
int potValue[POT_COUNT];       // lectura actual
int lastPotValue[POT_COUNT];   // última lectura tomada como referencia
int movedPot = -1;             // índice del último pot movido (-1 = ninguno)
unsigned long movedPotTime = 0;

// =====================================================
//  SETUP / LOOP
// =====================================================

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("[OLED] No se encontro la pantalla (revisa direccion/cableado)"));
  }
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("OLED OK"));
  display.display();
  delay(3000);

  // Lectura inicial como referencia para detectar movimiento
  for (int i = 0; i < POT_COUNT; i++) {
    lastPotValue[i] = analogRead(POT_PINS[i]);
    potValue[i] = lastPotValue[i];
  }
}

void loop() {
  handleButton();
  readPots();
  updateSweep();
  updateDisplay();
  delay(LOOP_DELAY_MS);
}

// =====================================================
//  UTILIDADES
// =====================================================

const char* modeName() {
  return (mode == SINGLE_SERVO) ? "UNO" : "TODOS";
}

// =====================================================
//  BOTÓN
// =====================================================
// Pulsación corta: siguiente servo.
// Pulsación larga (>= LONG_PRESS_MS): alterna entre UNO y TODOS.

void handleButton() {
  if (digitalRead(BUTTON_PIN) != LOW) return;

  int heldMs = 0;
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(100);
    heldMs += 100;
  }

  if (heldMs >= LONG_PRESS_MS) {
    mode = (mode == ALL_SERVOS) ? SINGLE_SERVO : ALL_SERVOS;
  } else {
    current = (current + 1) % SERVO_COUNT;
  }
}

// =====================================================
//  POTENCIÓMETROS
// =====================================================

// Lee los potenciómetros UNA sola vez por ciclo y detecta cuál se movió.
// Si varios se mueven a la vez, gana el de mayor cambio.
void readPots() {
  int biggestChange = 0;
  int candidate = -1;

  for (int i = 0; i < POT_COUNT; i++) {
    potValue[i] = analogRead(POT_PINS[i]);
    int diff = potValue[i] - lastPotValue[i];

    if (abs(diff) > POT_THRESHOLD && abs(diff) > biggestChange) {
      biggestChange = abs(diff);
      candidate = i;
    }
  }

  if (candidate != -1) {
    int diff = potValue[candidate] - lastPotValue[candidate];
    lastPotValue[candidate] = potValue[candidate];   // nueva referencia
    movedPot = candidate;
    movedPotTime = millis();
    logPotMoved(candidate, diff);
  }
}

// true mientras el último pot movido siga resaltado
bool isPotHighlighted() {
  return movedPot != -1 && (millis() - movedPotTime) < POT_HIGHLIGHT_MS;
}

// Ejemplo de salida:
//   [POT] P2 (A1) = 512 -> 50% | cambio: +37
void logPotMoved(int index, int diff) {
  Serial.print(F("[POT] P"));
  Serial.print(index + 1);                 // P1..P5
  Serial.print(F(" ("));
  Serial.print(POT_LABELS[index]);         // pin analógico real
  Serial.print(F(") = "));
  Serial.print(potValue[index]);
  Serial.print(F(" -> "));
  Serial.print(map(potValue[index], 0, 1023, 0, 100));
  Serial.print(F("% | cambio: "));
  if (diff > 0) Serial.print('+');
  Serial.println(diff);
}

// =====================================================
//  BARRIDO DE SERVOS
// =====================================================

void updateSweep() {
  if (angle > ANGLE_MAX) {
    onSweepFinished();
    direction = -1;
  } else if (angle < ANGLE_MIN) {
    direction = +1;
  } else {
    servos[current].write(angle);
  }
  angle += direction * ANGLE_STEP;
}

// Al terminar una pasada: avanza al siguiente servo y reconecta.
void onSweepFinished() {
  if (firstSweep) {
    firstSweep = false;
  } else if (mode == ALL_SERVOS) {
    current++;
  }

  if (current == SERVO_COUNT) {
    if (mode == ALL_SERVOS) detachAll();
    current = 0;
  }

  if (mode == SINGLE_SERVO) {
    servos[current].attach(SERVO_PINS[current]);
  } else {
    attachAll();
  }
  logServoStatus();
}

void attachAll() {
  for (int i = 0; i < SERVO_COUNT; i++) servos[i].attach(SERVO_PINS[i]);
}

void detachAll() {
  for (int i = 0; i < SERVO_COUNT; i++) servos[i].detach();
}

// Ejemplo de salida:
//   [SERVO] Activo: 2/6 | Pin: 6 | Modo: TODOS (1) | Conectados: todos
void logServoStatus() {
  Serial.print(F("[SERVO] Activo: "));
  Serial.print(current + 1);               // 1..N, igual que en la documentación
  Serial.print(F("/"));
  Serial.print(SERVO_COUNT);
  Serial.print(F(" | Pin: "));
  Serial.print(SERVO_PINS[current]);
  Serial.print(F(" | Modo: "));
  Serial.print(modeName());
  Serial.print(F(" ("));
  Serial.print((int)mode);
  Serial.print(F(") | Conectados: "));
  if (mode == SINGLE_SERVO) {
    Serial.println(F("solo este"));
  } else {
    Serial.println(F("todos"));
  }
}

// =====================================================
//  OLED
// =====================================================
// Con textSize(1) caben 21 caracteres x 8 líneas en 128x64.
// Diseño:
//   Servo 2/6  Pin 6
//   Modo: TODOS
//   --------------------
//   P1 A0: 512
//   P2 A1: 300 <<        (<< = el que estás moviendo)
//   P3 A2: 0
//   P4 A3: 1023
//   P5 A6: 17

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Encabezado: servo activo y modo
  display.print(F("Servo "));
  display.print(current + 1);
  display.print(F("/"));
  display.print(SERVO_COUNT);
  display.print(F("  Pin "));
  display.println(SERVO_PINS[current]);

  display.print(F("Modo: "));
  display.println(modeName());
  display.println(F("--------------------"));

  // Un renglón por potenciómetro
  for (int i = 0; i < POT_COUNT; i++) {
    display.print(F("P"));
    display.print(i + 1);
    display.print(F(" "));
    display.print(POT_LABELS[i]);
    display.print(F(": "));
    display.print(potValue[i]);

    if (isPotHighlighted() && i == movedPot) display.print(F(" <<"));
    display.println();
  }

  display.display();
}