#define NO_LED_FEEDBACK_CODE
#define DECODE_NEC
#include <IRremote.hpp>
#include <Keypad.h>

// #define NDEBUG
#undef NDEBUG

#ifdef NDEBUG
#define DEBUG_OUTPUT(...) Serial.print(__VA_ARGS__)
#else
#define DEBUG_OUTPUT(...)
#endif

class PushButton {
private:
  const int debounce_ = 50;
  int pin_;
  unsigned long last_ = 0;
  bool released_ = true;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    last_ = millis();
  }

  bool isPushed() {
    unsigned long now = millis();
    if (now - last_ < debounce_) {
      return false;
    }
    last_ = now;

    int state = digitalRead(pin_);
    // No hold and repeat. The button is only considered pushed
    // if it was released in the previous scan.
    if (state == LOW && released_) {
      released_ = false;
      return true;
    }
    if (state == HIGH) {
      released_ = true;
    }
    return false;
  }
};

class Joystick {
private:
  const int sensitivity = 5;
  const int debounce_ = 50;

  unsigned long last_ = 0;
  int pinHigh_;
  int pinX_;
  int pinY_;

  uint8_t xPrev = 128;
  uint8_t yPrev = 128;

public:
  Joystick(int pinHigh, int pinX, int pinY)
      : pinHigh_(pinHigh), pinX_(pinX), pinY_(pinY) {}

  void begin() {
    pinMode(pinHigh_, OUTPUT);
    pinMode(pinX_, INPUT);
    pinMode(pinY_, INPUT);

    digitalWrite(pinHigh_, HIGH);
    xPrev = analogRead(pinX_) >> 2;
    yPrev = 255 - (analogRead(pinY_) >> 2); // Y axis is inverted.
    digitalWrite(pinHigh_, LOW);

    last_ = millis();
  }

  // Read x and y positions. Return true if either has shifted enough from the
  // last position.
  bool read(uint8_t &x, uint8_t &y) {
    unsigned long now = millis();
    if (now - last_ < debounce_) {
      return false;
    }
    last_ = now;

    digitalWrite(pinHigh_, HIGH);
    // analogRead has 10 bit accuracy. We use 8 bits.
    x = analogRead(pinX_) >> 2;
    y = 255 - (analogRead(pinY_) >> 2);
    digitalWrite(pinHigh_, LOW);

    int deltaX = abs(x - xPrev);
    int deltaY = abs(y - yPrev);

    if (deltaX >= sensitivity || deltaY >= sensitivity) {
      xPrev = x;
      yPrev = y;
      return true;
    } else {
      return false;
    }
  }
};

class IrRemote {
private:
  int pin_;

  enum TYPE { KEY = 1, JOYSTICK1 = 2, JOYSTICK2 = 3 };

  void Send(uint16_t type, uint16_t value) {
    IrSender.sendNECRaw((uint32_t)type << 24 | value);
    // A short delay seems to improve the signal quality.
    delay(40);
  }

public:
  IrRemote(int pin) : pin_(pin) {}

  begin() { IrSender.begin(pin_); }

  void SendKey(uint16_t key) {
    DEBUG_OUTPUT("Key ");
    DEBUG_OUTPUT(key);
    DEBUG_OUTPUT('\n');
    Send(TYPE::KEY, key);
  }

  void SendJoystick1(uint8_t x, uint8_t y) {
    DEBUG_OUTPUT("Joystick 1: ");
    DEBUG_OUTPUT(x);
    DEBUG_OUTPUT(", ");
    DEBUG_OUTPUT(y);
    DEBUG_OUTPUT('\n');

    Send(TYPE::JOYSTICK1, (uint16_t)x << 8 | y);
  }

  void SendJoystick2(uint8_t x, uint8_t y) {
    DEBUG_OUTPUT("Joystick 2: ");
    DEBUG_OUTPUT(x);
    DEBUG_OUTPUT(", ");
    DEBUG_OUTPUT(y);
    DEBUG_OUTPUT('\n');

    Send(TYPE::JOYSTICK2, (uint16_t)x << 8 | y);
  }
};

IrRemote remote(10);

Joystick joystick1(A5, A2, A3);
Joystick joystick2(A5, A6, A7);

PushButton directButtons[] = {PushButton(A0), PushButton(A1)};

constexpr int rows = 2;
constexpr int cols = 8;
byte buttonMatrix[rows][cols] = {{21, 22, 23, 24, 25, 26, 27, 28},
                                 {31, 32, 33, 34, 35, 36, 37, 38}};
byte pinsRows[rows] = {11, 12};
byte pinsCols[cols] = {2, 3, 4, 5, 6, 7, 8, 9};

Keypad keypad =
    Keypad(makeKeymap(buttonMatrix), pinsRows, pinsCols, rows, cols);

void setup() {
#ifdef NDEBUG
  Serial.begin(9600);
#endif

  remote.begin();

  for (int i = 0; i < sizeof(directButtons) / sizeof(PushButton); i++) {
    directButtons[i].begin();
  }

  joystick1.begin();
  joystick2.begin();
}

void loop() {
  int key = keypad.getKey();
  if (key != 0) {
    remote.SendKey(key);
    return;
  }

  for (int i = 0; i < sizeof(directButtons) / sizeof(PushButton); i++) {
    if (directButtons[i].isPushed()) {
      remote.SendKey(i + 1);
      return;
    }
  }

  uint8_t x, y;
  if (joystick1.read(x, y)) {
    remote.SendJoystick1(x, y);
    return;
  }
  if (joystick2.read(x, y)) {
    remote.SendJoystick2(x, y);
    return;
  }
}