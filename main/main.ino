#include <algorithm>
#include <array>
#include <queue>
#include <vector>

#include <Servo.h>

#include "src/DYPlayerArduino.h"
#define DECODE_NEC // Decodes NEC, NEC2, and ONKYO
#include "src/IRremote.hpp"

#ifdef abs
#undef abs
#endif

template <typename T> T clamp(T v, T low, T high) {
  if (v < low) {
    return low;
  } else if (v > high) {
    return high;
  } else {
    return v;
  }
}

#ifdef abs
#undef abs
#endif

template <typename T> T clamp(T v, T low, T high) {
  if (v < low) {
    return low;
  } else if (v > high) {
    return high;
  } else {
    return v;
  }
}

void resetIdleCount();

// Pin assignments
constexpr uint8_t pinIrRemote = 3;
constexpr uint8_t pinPushButton = 9;
constexpr uint8_t pinEyeTilter = 2;
constexpr uint8_t pinEyeLedL = 4;
constexpr uint8_t pinEyeLedR = 5;
constexpr uint8_t pinHead = 6;
constexpr uint8_t pinArmL = 7;
constexpr uint8_t pinArmR = 8;
constexpr uint8_t pinMotorL1 = 10;
constexpr uint8_t pinMotorL2 = 11;
constexpr uint8_t pinMotorR1 = 12;
constexpr uint8_t pinMotorR2 = 13;

// Servo limits, in terms of pulse width in microseconds
constexpr int minArmL = 2200;
constexpr int maxArmL = 450;
constexpr int minArmR = 700;
constexpr int maxArmR = 2450;
constexpr int minHead = 550;
constexpr int maxHead = 1950;

enum AnimationType {
  // Change the value linearly to a target value, over a specified duration, in
  // milliseconds
  LinearToOver,
  // Change the value linearly to a target value, at a specified speed, in
  // fraction per milleseconds
  LinearToAt,
  // Change the value linearly by an increment, over a specified duration
  LinearBy,
  // Maintain the value for a specified duration
  Constant
};

struct Animation {
  AnimationType type;
  union {
    unsigned long millisDuration;
    float speed; // amount per millisecond
  };
  float value;

  static Animation linearToOver(float value, unsigned long duration) {
    return Animation{AnimationType::LinearToOver, {duration}, value};
  }

  static Animation linearToAt(float value, float speed) {
    return Animation{AnimationType::LinearToAt, {.speed = speed}, value};
  }

  static Animation linearBy(float value, unsigned long duration) {
    return Animation{AnimationType::LinearBy, {duration}, value};
  }

  static Animation instantTo(float value) {
    return Animation{AnimationType::LinearToOver, {0L}, value};
  }

  static Animation instantBy(float value) {
    return Animation{AnimationType::LinearBy, {0L}, value};
  }

  static Animation constantOver(unsigned long duration) {
    return Animation{AnimationType::Constant, {duration}};
  }
};

class Animatable {
public:
  static void updateFrame() {
    unsigned long millisNow = millis();
    for (int i = 0; i < animatables.size(); i++) {
      animatables[i]->updateFrame(millisNow);
    }
  }

  virtual void setValue(float value) { value_ = value; }
  float getValue() { return value_; }

  void queueAnimation(Animation animation) {
    if (animationQueue.empty()) {
      millisStart = millis();
      valueStart = value_;
    }

    if (animation.type == AnimationType::LinearBy) {
      // Calculate target value for LinearBy, since the parameter is the
      // increment, not the target value.
      constexpr float epsilon = 0.05;
      float valueCurrent = getValue();
      float valueTarget = clamp(valueCurrent + animation.value, 0.0f, 1.0f);
      if (std::abs(valueCurrent - valueTarget) <= epsilon) {
        return;
      }
      animation.value = valueTarget;
    } else if (animation.type == AnimationType::LinearToAt) {
      // Calculate duration for LinearToAt, since the speed, not the duration,
      // was given.
      float valueCurrent = getValue();

      unsigned long duration =
          std::abs(animation.value - valueCurrent) / animation.speed;
      animation.millisDuration = duration;
    }

    animationQueue.push(animation);
  }

  template <size_t N>
  void queueAnimations(const std::array<Animation, N> animations) {
    for (auto animation : animations) {
      queueAnimation(animation);
    }
  }

protected:
  static std::vector<Animatable *> animatables;

  float value_;
  std::queue<Animation> animationQueue;
  unsigned long millisStart = 0;
  float valueStart;

  Animatable() { animatables.push_back(this); }

  void transitionToNextAnimation(unsigned long millisNow) {
    if (animationQueue.empty()) {
      millisStart = 0;
      return;
    }
    millisStart = millisNow;
    valueStart = value_;
  }

  void updateFrame(unsigned long millisNow) {
    if (animationQueue.empty()) {
      return;
    }

    resetIdleCount();

    Animation animation = animationQueue.front();
    switch (animation.type) {
    case AnimationType::LinearToOver:
    case AnimationType::LinearToAt:
    case AnimationType::LinearBy: {
      // Target value and duration should've already been calculated.
      float fraction = static_cast<float>(millisNow - millisStart) /
                       static_cast<float>(animation.millisDuration);
      if (fraction >= 1.0) {
        setValue(animation.value);
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
      } else {
        float valueNext =
            (1.0 - fraction) * valueStart + fraction * animation.value;
        setValue(valueNext);
      }
      break;
    }

    case AnimationType::Constant: {
      if (millisNow - millisStart >= animation.millisDuration) {
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
      }
      break;
    }
    }
  }
};

std::vector<Animatable *> Animatable::animatables;

class AnimatableServo : public Animatable {
public:
  AnimatableServo(uint8_t pin, uint16_t min, uint16_t max)
      : pin_(pin), min_(min), max_(max), Animatable() {}

  void setValue(float value) override {
    Animatable::setValue(value);
    uint16_t us = static_cast<uint16_t>(value * static_cast<float>(max_) +
                                        (1 - value) * static_cast<float>(min_));
    // Specific to RP2040, since the Servo library uses PIO, attach() needs to
    // be called after setup() has begun. so we can't do it in the constructor.
    // For any other MCU, we can most likely move this to the constructor and
    // get rid of pin_
    if (!servo_.attached()) {
      if (max_ > min_) {
        servo_.attach(pin_, min_, max_);
      } else {
        servo_.attach(pin_, max_, min_);
      }
    }
    servo_.writeMicroseconds(us);
  }

private:
  Servo servo_;
  uint8_t pin_;
  uint16_t min_;
  uint16_t max_;
};

class Led : public Animatable {
public:
  Led(uint8_t pin) : pin_(pin), Animatable() { pinMode(pin, OUTPUT); }

  void setValue(float value) override {
    Animatable::setValue(value);
    analogWrite(pin_, static_cast<int>(value * 255.0));
  }

private:
  uint8_t pin_;
};

// Unidirectional motor, controlled by a PWM pin and a MOSFET
class UniMotor : public Animatable {
public:
  UniMotor(uint8_t pin) : pin_(pin), Animatable() { pinMode(pin, OUTPUT); }

  void setValue(float value) override {
    Animatable::setValue(value);
    analogWrite(pin_, static_cast<int>(value * 255.0));
  }

private:
  uint8_t pin_;
};

// Bidirectional motor, controlled by an H-bridge
class BiMotor : public Animatable {
  static constexpr float low_threshold = 0.1;

public:
  BiMotor(uint8_t pin1, uint8_t pin2) : pin1_(pin1), pin2_(pin2), Animatable() {
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
  }

  void setValue(float value) override {
    Animatable::setValue(value);

    int value1;
    int value2;
    if (abs(value) < low_threshold) {
      value1 = 0;
      value2 = 0;
    } else if (value > 0) {
      value1 = 0;
      value2 = static_cast<int>(value * 255.0);
    } else {
      value1 = static_cast<int>(-value * 255.0);
      value2 = 0;
    }

    analogWrite(pin1_, value1);
    analogWrite(pin2_, value2);
  }

private:
  uint8_t pin1_;
  uint8_t pin2_;
};

class PushButton {
  static constexpr int debounceThreshold = 100;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    last_ = millis();
  }

  bool isPushed() {
    unsigned long now = millis();
    if (now - last_ < debounceThreshold) {
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

private:
  int pin_;
  unsigned long last_ = 0;
  bool released_ = true;
};

class AudioQueue {
public:
  struct Entry {
    enum { Delay, Play } tag;
    unsigned long millisStart = 0;
    union {
      unsigned long duration;
      unsigned int index;
    };
  };

  static AudioQueue audioQueue;
  static void updateFrame() {
    unsigned long millisNow = millis();
    audioQueue.updateFrame(millisNow);
  }

  static void queueDelay(unsigned long duration) {
    Entry entry{
        .tag = Entry::Delay, .millisStart = millis(), duration = duration};
    audioQueue.queue.push(entry);
  }

  static void queuePlay(unsigned int index) {
    Entry entry{.tag = Entry::Play, .millisStart = millis(), index = index};
    audioQueue.queue.push(entry);
  }

  AudioQueue(DY::Player &player) : player_(player) {}

private:
  void updateFrame(unsigned long millisNow) {
    if (queue.empty()) {
      return;
    }

    resetIdleCount();

    Entry entry = queue.front();
    switch (entry.tag) {
    case Entry::Delay:
      if (millisNow > entry.millisStart + entry.duration) {
        queue.pop();
      }
      break;
    case Entry::Play:
      player_.playSpecified(entry.index);
      queue.pop();
      break;
    }
  }

protected:
  static std::queue<Entry> queue;
  DY::Player &player_;
};

std::queue<AudioQueue::Entry> AudioQueue::queue;

AnimatableServo head(pinHead, minHead, maxHead);
AnimatableServo leftArm(pinArmL, minArmL, maxArmL);
AnimatableServo rightArm(pinArmR, minArmR, maxArmR);
UniMotor eyeTilter(pinEyeTilter);
BiMotor leftTread(pinMotorL1, pinMotorL2);
BiMotor rightTread(pinMotorR1, pinMotorR2);
Led leftEye(pinEyeLedL);
Led rightEye(pinEyeLedR);
PushButton pushButton(pinPushButton);

DY::Player audioPlayer(&Serial1);
AudioQueue AudioQueue::audioQueue(audioPlayer);

static unsigned long millisIdleStart;
float breathingValue = 1;

void setIdleBreathing() {
  leftEye.setValue(breathingValue);
  rightEye.setValue(breathingValue);
}
void resetIdleCount() {
  if (isIdling()) {
    breathingValue = 1;
  }

  millisIdleStart = millis();
}

bool isIdling() { return millis() - millisIdleStart >= 10000; }

void demo() {
  // Hold, look left, right, then straight
  std::array<Animation, 5> headAnimation = {
      Animation::linearToAt(0.5, 0.0005), Animation::constantOver(500),
      Animation::linearToOver(0, 1000), Animation::linearToOver(1, 2000),
      Animation::linearToOver(0.5, 1000)};
  head.queueAnimations(headAnimation);

  // Hold, then blink twice
  std::array<Animation, 9> eyeAnimation = {
      Animation::instantTo(1),        Animation::constantOver(4500),
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20), Animation::constantOver(300),
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20)};
  leftEye.queueAnimations(eyeAnimation);
  rightEye.queueAnimations(eyeAnimation);

  // Wait, then play walle-e
  AudioQueue::queueDelay(5000);
  AudioQueue::queuePlay(2);
}

void playNextAudio() {
  constexpr int audioFileCount = 10;
  static int index = 2;
  index++;
  if (index > audioFileCount) {
    index = 1;
  }
  AudioQueue::queueDelay(100);
  AudioQueue::queuePlay(index);
}

void stop() {
  std::array<Animation, 1> animations = {Animation::linearToOver(0, 200)};
  leftTread.queueAnimations(animations);
  rightTread.queueAnimations(animations);
}

void spinLeft() {
  leftTread.queueAnimation(Animation::linearToOver(-1, 200));
  rightTread.queueAnimation(Animation::linearToOver(1, 200));
}

void spinRight() {
  leftTread.queueAnimation(Animation::linearToOver(1, 200));
  rightTread.queueAnimation(Animation::linearToOver(-1, 200));
}

void forward() {
  leftTread.queueAnimation(Animation::linearToOver(1, 200));
  rightTread.queueAnimation(Animation::linearToOver(1, 200));
}

void backward() {
  leftTread.queueAnimation(Animation::linearToOver(-1, 200));
  rightTread.queueAnimation(Animation::linearToOver(-1, 200));
}

void forwardLeft() {
  leftTread.queueAnimation(Animation::linearToOver(0, 200));
  rightTread.queueAnimation(Animation::linearToOver(1, 200));
}

void forwardRight() {
  leftTread.queueAnimation(Animation::linearToOver(1, 200));
  rightTread.queueAnimation(Animation::linearToOver(0, 200));
}

void backwardLeft() {
  leftTread.queueAnimation(Animation::linearToOver(0, 200));
  rightTread.queueAnimation(Animation::linearToOver(-1, 200));
}

void backwardRight() {
  leftTread.queueAnimation(Animation::linearToOver(-1, 200));
  rightTread.queueAnimation(Animation::linearToOver(0, 200));
}

void leftArmMove(float value) {
  leftArm.queueAnimation(Animation::linearBy(value, 100));
}

void rightArmMove(float value) {
  rightArm.queueAnimation(Animation::linearBy(value, 100));
}

void leftArmUp() { leftArm.queueAnimation(Animation::linearToAt(1, 0.001f)); }

void leftArmDown() { leftArm.queueAnimation(Animation::linearToAt(0, 0.001f)); }

void rightArmUp() { rightArm.queueAnimation(Animation::linearToAt(1, 0.001f)); }

void rightArmDown() {
  rightArm.queueAnimation(Animation::linearToAt(0, 0.001f));
}

void breathe() {
  std::array<Animation, 2> animations = {Animation::linearToOver(0, 2000),
                                         Animation::linearToOver(1, 2000)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void blinkTwice() {
  std::array<Animation, 7> animations = {
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20), Animation::constantOver(300),
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void lookLeft() { head.queueAnimation(Animation::linearToAt(0, 0.0005)); }

void lookRight() { head.queueAnimation(Animation::linearToAt(1, 0.0005)); }

void lookStraight() { head.queueAnimation(Animation::linearToAt(0.5, 0.0005)); }

void tiltEye() {
  std::array<Animation, 3> animations = {Animation::linearToOver(1, 0),
                                         Animation::linearToOver(1, 1600),
                                         Animation::linearToOver(0, 0)};
  eyeTilter.queueAnimations(animations);
}

void toggleLeftArm() {
  static uint8_t position = 1;
  if (position == 1) {
    leftArm.queueAnimation(Animation::linearToOver(0, 1000));
    position = 0;
  } else {
    leftArm.queueAnimation(Animation::linearToOver(1, 1000));
    position = 1;
  }
}

void toggleRightArm() {
  static uint8_t position = 1;
  if (position == 1) {
    rightArm.queueAnimation(Animation::linearToOver(0, 1000));
    position = 0;
  } else {
    rightArm.queueAnimation(Animation::linearToOver(1, 1000));
    position = 1;
  }
}

void lookAround() {
  // Turn left, right, then straight
  std::array<Animation, 3> animations1 = {
      Animation::linearToOver(0, 700),
      Animation::linearToOver(1, 1400),
      Animation::linearToOver(0.5, 700),
  };
  // Open eyes, hold while turning head, then blink twice
  std::array<Animation, 9> animations2 = {
      Animation::linearToOver(1, 0),  Animation::constantOver(3000),
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20), Animation::constantOver(300),
      Animation::linearToOver(0, 20), Animation::constantOver(100),
      Animation::linearToOver(1, 20)};

  head.queueAnimations(animations1);
  leftEye.queueAnimations(animations2);
  rightEye.queueAnimations(animations2);
}

void setup() {
#ifdef NDEBUG
  Serial.begin(115200);
#endif

  // Set up push button
  pushButton.begin();

  // Set up IR receiver
  IrReceiver.begin(pinIrRemote, false);

  // Set initial status
  head.setValue(0.5);
  leftArm.setValue(0);
  rightArm.setValue(0);
  eyeTilter.setValue(0);
  leftTread.setValue(0);
  rightTread.setValue(0);
  leftEye.setValue(1);
  rightEye.setValue(1);

  // Set up audio player and play "wall-e"
  // The dyplayer needs a little time to be ready after powering up.
  // A 300ms delay seems to be pretty reliable.
  delay(300);
  audioPlayer.begin();
  delay(50);
  audioPlayer.setVolume(15);
  delay(50);
  // For some reason, the dyplayer reverses tracks 1 and 2.
  audioPlayer.playSpecified(2);

  resetIdleCount();
}

void loop() {
  Animatable::updateFrame();
  AudioQueue::updateFrame();

  if (pushButton.isPushed()) {
    resetIdleCount();
    demo();
    return;
  }

  if (IrReceiver.decode()) {
    resetIdleCount();

    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    IrReceiver.resume();

    // The Roku remote control
    if (protocol == NEC) {
      switch (code) {
      case 3893872618: // Power
        demo();
        break;
      case 2573649898: // Back
        lookAround();
        break;
      case 4228106218: // Home
        lookStraight();
        break;

      case 3860449258: // Up
        forward();
        break;
      case 3425945578: // Down
        backward();
        break;
      case 3776890858: // Left
        spinLeft();
        break;
      case 3526215658: // Right
        spinRight();
        break;
      case 3576350698: // OK
        stop();
        break;

      case 2272839658: // Repeat
        blinkTwice();
        break;
      case 2640496618: // Sleep
        tiltEye();
        break;
      case 2657208298: // Star
        breathe();
        break;

      case 3409233898: // Rewind
        lookLeft();
        break;
      case 3008153578: // Play/Pause
        playNextAudio();
        break;
      case 2857748458: // F. Fwd
        lookRight();
        break;

      case 2907883498: // Netflix
        forwardLeft();
        break;
      case 2991441898: // Hulu
        backwardLeft();
        break;
      case 3024865258: // AmazonPrime
        forwardRight();
        break;
      case 4077701098: // Disney
        backwardRight();
        break;

      case 4144547818: // Vudu
        leftArmUp();
        break;
      case 2974730218: // HBO
        leftArmDown();
        break;
      case 2841036778: // YouTube
        rightArmUp();
        break;
      case 4177971178: // Sling
        rightArmDown();
        break;

      case 4027566058: // Vol Up
        AudioQueue::queuePlay(2);
        break;
      case 4010854378: // Vol Down
        AudioQueue::queuePlay(3);
        break;
      case 3743467498: // Mute
        AudioQueue::queuePlay(1);
        break;
      }
    }

    // My custom remote control. The library sometimes reports NEC2, sometimes
    // ONKYO. They are probably distinguishable from the raw code. I'll just
    // include them both.
    if (protocol == NEC2 || protocol == ONKYO) {
      uint16_t type = (code & 0xFF000000) >> 24;
      uint16_t value = (code & 0x0000FFFF);

      switch (type) {
      case 2: { // Left joystick
        uint8_t xRaw = (value & 0xFF00) >> 8;
        uint8_t yRaw = (value & 0x00FF) >> 0;

        // Normalize x and y values. Range: [-1,1]
        float xNormalized = (xRaw - 127.5) / 127.5;
        float yNormalized = (yRaw - 127.5) / 127.5;

        float xSign = xNormalized >= 0 ? 1 : -1;
        float ySign = yNormalized >= 0 ? 1 : -1;

        float left, right;
        if (xSign == ySign) {
          left = ySign * max(abs(xNormalized), abs(yNormalized));
          right = ySign * (abs(yNormalized) - abs(xNormalized));
        } else {
          right = ySign * max(abs(xNormalized), abs(yNormalized));
          left = ySign * (abs(yNormalized) - abs(xNormalized));
        }

        leftTread.setValue(left);
        rightTread.setValue(right);

#ifdef NDEBUG
        Serial.print("Tread L: ");
        Serial.print(left);
        Serial.print(" R: ");
        Serial.print(right);
        Serial.println();
#endif
        break;
      }

      case 3: { // Right joystick
        // Range with which a normalized x value is considered "center"
        const float thresholdLeft = -0.5;
        const float thresholdRight = 0.5;

        // Timestamp of last arm movement, to avoid moving arms too fast.
        static unsigned long millisLastArmMove = 0;
        unsigned long millisNow = millis();
        if (millisNow - millisLastArmMove < 500) {
          break;
        }

        // Joy stick positions. Range: [0,255]
        uint8_t xRaw = (value & 0xFF00) >> 8;
        uint8_t yRaw = (value & 0x00FF) >> 0;

        // Normalize x and y values. Range: [-1,1]
        float xNormalized = (xRaw - 127.5) / 127.5;
        float yNormalized = (yRaw - 127.5) / 127.5;

        bool moveLeft = xNormalized <= thresholdRight;
        bool moveRight = xNormalized >= thresholdLeft;

        // Calcualte the amount to move: nothing, a little, or a lot.
        float ySign = yNormalized >= 0 ? 1 : -1;
        float yAbsolute = abs(yNormalized);
        float moveAmount;
        if (yAbsolute > 0.95) {
          moveAmount = 0.2 * ySign;
        } else if (yAbsolute > 0.1) {
          moveAmount = 0.1 * ySign;
        } else {
          moveAmount = 0;
        }

        if (moveAmount != 0 && moveLeft) {
          leftArmMove(moveAmount);
          millisLastArmMove = millisNow;
#ifdef NDEBUG
          Serial.print("Arm L: ");
          Serial.println(moveAmount);
#endif
        }
        if (moveAmount != 0 && moveRight) {
          rightArmMove(moveAmount);
          millisLastArmMove = millisNow;
#ifdef NDEBUG
          Serial.print("Arm R: ");
          Serial.println(moveAmount);
#endif
        }

        break;
      }
      case 1: {
        // Button push
        uint16_t key = value;
        // TODO: assign buttons
        demo();
        break;
      }
      }
    }
  }

  if (isIdling()) {
    leftTread.setValue(0);
    rightTread.setValue(0);

    constexpr unsigned long breathingCycle = 3600;
    unsigned long idleDuration = (millis() - millisIdleStart) % breathingCycle;

    if (idleDuration < breathingCycle / 2.0) {
      breathingValue = 1.0 - idleDuration / (breathingCycle / 2.0);
    } else {
      breathingValue = -1.0 + idleDuration / (breathingCycle / 2.0);
    }

    setIdleBreathing();
  }
}
