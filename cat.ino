#include <Servo.h>

// ============================================
// PIN DEFINITIONS
// ============================================
#define TRIG_PIN   9
#define ECHO_PIN  10
#define BUZZER     4
#define SERVO_PIN  6
#define LED_1      2
#define LED_2      3
#define LED_3      5

// ============================================
// DISTANCE ZONES (cm)
// ============================================
const int ZONE_VERY_FAR = 150;
const int ZONE_FAR      = 100;
const int ZONE_CURIOUS  = 70;
const int ZONE_ALERT    = 40;
const int ZONE_CLOSE    = 25;
const int ZONE_THREAT   = 10;

// ============================================
// SERVO (KEPALA KUCING) - FULL 180° RANGE
// ============================================
const int HEAD_MIN = 0;      // Diperluas dari 20 ke 0
const int HEAD_MAX = 180;    // Diperluas dari 160 ke 180
const int HEAD_CENTER = 90;
const int SCAN_STEP = 20;    // Diperbesar untuk sweep lebih lebar

// Zona servo untuk tracking
const int ZONE_FAR_LEFT = 30;
const int ZONE_LEFT = 60;
const int ZONE_CENTER_LEFT = 75;
const int ZONE_CENTER_RIGHT = 105;
const int ZONE_RIGHT = 120;
const int ZONE_FAR_RIGHT = 150;

// ============================================
// CAT PERSONALITY MODES
// ============================================
enum CatMood {
  SLEEPING,
  LAZY_WATCH,
  CURIOUS,
  STALKING,
  ALERT,
  AGGRESSIVE,
  PLAYING
};

CatMood currentMood = SLEEPING;
CatMood previousMood = SLEEPING;

// ============================================
// LED INDICATOR PATTERNS
// ============================================
enum LEDPattern {
  LED_OFF,
  LED_SLOW_BLINK,
  LED_MEDIUM_BLINK,
  LED_FAST_BLINK,
  LED_PULSE,
  LED_CHASE,
  LED_ALL_ON,
  LED_DANGER_FLASH
};

// ============================================
// GLOBAL VARIABLES
// ============================================
Servo headServo;
int currentAngle = HEAD_CENTER;
int targetAngle = HEAD_CENTER;
int scanDirection = 1;

// Detection & Tracking
long currentDistance = -1;
int objectAngle = HEAD_CENTER;
bool objectDetected = false;
unsigned long lastDetectionTime = 0;
unsigned long moodStartTime = 0;

// Motion detection
struct SensorData {
  long distance;
  int angle;
  unsigned long timestamp;
};

SensorData history[10];
int histIdx = 0;

bool isMoving = false;
bool isApproaching = false;
int objectSpeed = 0;

// Behavior timers
unsigned long lastMoveTime = 0;
unsigned long lastSoundTime = 0;
unsigned long lastPlayTime = 0;
unsigned long lastRandomTime = 0;
unsigned long lastLEDUpdate = 0;

// LED state
int ledStep = 0;
bool ledState = false;

// Behavior state
const int IDLE_TO_SLEEP = 10000;
const int PLAY_DURATION = 8000;
int consecutiveDetections = 0;

// Head position tracking
bool isLookingLeft = false;
bool isLookingRight = false;
bool isLookingCenter = true;

// ============================================
// SETUP
// ============================================
void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  noTone(BUZZER);
  
  headServo.attach(SERVO_PIN);
  headServo.write(HEAD_CENTER);
  delay(500);
  
  Serial.begin(9600);
  Serial.println(F("╔═══════════════════════════════════════╗"));
  Serial.println(F("║  🐱 ANIMATRONIC CAT GUARDIAN 🐱      ║"));
  Serial.println(F("║   Full 180° Movement System v5.0     ║"));
  Serial.println(F("║   Enhanced Range & Tracking          ║"));
  Serial.println(F("╚═══════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("🎯 FULL 180° SERVO RANGE ENABLED"));
  Serial.println(F("   Left: 0° | Center: 90° | Right: 180°"));
  Serial.println();
  Serial.println(F("Detection Indicators (LED):"));
  Serial.println(F("  💡 OFF        - SLEEPING (no activity)"));
  Serial.println(F("  💡 Slow Blink - LAZY WATCH (far object)"));
  Serial.println(F("  💡 Chase      - CURIOUS (something there)"));
  Serial.println(F("  💡 Medium     - STALKING (tracking)"));
  Serial.println(F("  💡 Fast Blink - ALERT (approaching!)"));
  Serial.println(F("  💡 DANGER!    - AGGRESSIVE (too close!)"));
  Serial.println(F("  💡 Pulse      - PLAYING (having fun)"));
  Serial.println();
  Serial.println(F("Commands: R=Reset | T=Test | L=LED Test | P=Play"));
  Serial.println(F("          S=Sleep | H=Help | I=Info | M=Servo Test"));
  Serial.println(F("          W=Wide Scan | F=Full Sweep"));
  Serial.println();
  
  // Initialize history
  for (int i = 0; i < 10; i++) {
    history[i].distance = -1;
    history[i].angle = HEAD_CENTER;
    history[i].timestamp = 0;
  }
  
  // Startup sequence with full range demonstration
  catWakeUpSequence();
  
  Serial.println(F("😴 Cat is SLEEPING... waiting for activity\n"));
  moodStartTime = millis();
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  currentDistance = readAndFilterDistance();
  storeHistory(currentDistance, currentAngle);
  analyzeMovement();
  updateHeadPosition();
  updateCatMood();
  executeBehavior();
  controlLEDIndicators();
  controlVoice();
  spontaneousBehaviors();
  displayStatus();
  checkCommands();
  
  delay(20);
}

// ============================================
// SENSOR FUNCTIONS
// ============================================
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 40000UL);
  if (duration == 0) return -1;
  
  long dist = duration * 0.034 / 2;
  return (dist > 400 || dist < 2) ? -1 : dist;
}

long readAndFilterDistance() {
  long readings[3];
  
  for (int i = 0; i < 3; i++) {
    readings[i] = readDistanceCm();
    if (i < 2) delay(8);
  }
  
  // Bubble sort
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2 - i; j++) {
      if (readings[j] > readings[j + 1]) {
        long temp = readings[j];
        readings[j] = readings[j + 1];
        readings[j + 1] = temp;
      }
    }
  }
  
  return readings[1];
}

void storeHistory(long dist, int angle) {
  history[histIdx].distance = dist;
  history[histIdx].angle = angle;
  history[histIdx].timestamp = millis();
  histIdx = (histIdx + 1) % 10;
}

void analyzeMovement() {
  int validCount = 0;
  long sumDist = 0;
  long minDist = 9999, maxDist = 0;
  int minAngle = 180, maxAngle = 0;
  
  for (int i = 0; i < 10; i++) {
    if (history[i].distance > 0 && history[i].distance < ZONE_VERY_FAR) {
      validCount++;
      sumDist += history[i].distance;
      
      if (history[i].distance < minDist) minDist = history[i].distance;
      if (history[i].distance > maxDist) maxDist = history[i].distance;
      if (history[i].angle < minAngle) minAngle = history[i].angle;
      if (history[i].angle > maxAngle) maxAngle = history[i].angle;
    }
  }
  
  if (validCount < 3) {
    isMoving = false;
    isApproaching = false;
    objectSpeed = 0;
    return;
  }
  
  long distRange = maxDist - minDist;
  int angleRange = maxAngle - minAngle;
  isMoving = (distRange > 8 || angleRange > 15);
  
  if (validCount >= 6) {
    long oldSum = 0, newSum = 0;
    int oldCount = 0, newCount = 0;
    
    for (int i = 0; i < 4; i++) {
      if (history[i].distance > 0) {
        oldSum += history[i].distance;
        oldCount++;
      }
    }
    
    for (int i = 6; i < 10; i++) {
      if (history[i].distance > 0) {
        newSum += history[i].distance;
        newCount++;
      }
    }
    
    if (oldCount > 0 && newCount > 0) {
      long oldAvg = oldSum / oldCount;
      long newAvg = newSum / newCount;
      long change = oldAvg - newAvg;
      
      isApproaching = (change > 8);
      
      unsigned long timeDiff = history[9].timestamp - history[0].timestamp;
      if (timeDiff > 0) {
        objectSpeed = abs(change * 1000 / timeDiff);
      }
    }
  }
}

// ============================================
// HEAD POSITION TRACKING
// ============================================
void updateHeadPosition() {
  // Update posisi kepala berdasarkan current angle
  if (currentAngle < ZONE_LEFT) {
    isLookingLeft = true;
    isLookingCenter = false;
    isLookingRight = false;
  } else if (currentAngle > ZONE_RIGHT) {
    isLookingLeft = false;
    isLookingCenter = false;
    isLookingRight = true;
  } else {
    isLookingLeft = false;
    isLookingCenter = true;
    isLookingRight = false;
  }
}

// ============================================
// MOOD SYSTEM
// ============================================
void updateCatMood() {
  unsigned long currentTime = millis();
  previousMood = currentMood;
  
  if (currentDistance > 0 && currentDistance < ZONE_VERY_FAR) {
    objectDetected = true;
    lastDetectionTime = currentTime;
    objectAngle = currentAngle;
    consecutiveDetections++;
  } else {
    objectDetected = false;
    consecutiveDetections = 0;
  }
  
  if (!objectDetected) {
    if (currentTime - lastDetectionTime > IDLE_TO_SLEEP) {
      currentMood = SLEEPING;
    } else if (currentTime - lastDetectionTime > IDLE_TO_SLEEP / 2) {
      currentMood = LAZY_WATCH;
    }
  } else {
    if (currentDistance < ZONE_THREAT) {
      currentMood = AGGRESSIVE;
      
    } else if (currentDistance < ZONE_CLOSE) {
      if (isApproaching && objectSpeed > 20) {
        currentMood = AGGRESSIVE;
      } else if (isApproaching) {
        currentMood = ALERT;
      } else {
        currentMood = STALKING;
      }
      
    } else if (currentDistance < ZONE_ALERT) {
      if (isMoving) {
        currentMood = STALKING;
      } else {
        currentMood = ALERT;
      }
      
    } else if (currentDistance < ZONE_CURIOUS) {
      if (isMoving) {
        currentMood = CURIOUS;
      } else {
        currentMood = LAZY_WATCH;
      }
      
    } else if (currentDistance < ZONE_FAR) {
      currentMood = LAZY_WATCH;
      
    } else {
      if (currentMood != SLEEPING) {
        currentMood = LAZY_WATCH;
      }
    }
    
    if (currentMood == CURIOUS && random(0, 100) < 2 && consecutiveDetections > 10) {
      currentMood = PLAYING;
      lastPlayTime = currentTime;
    }
  }
  
  if (currentMood == PLAYING && currentTime - lastPlayTime > PLAY_DURATION) {
    currentMood = CURIOUS;
  }
  
  if (currentMood != previousMood) {
    moodStartTime = currentTime;
    ledStep = 0;
    announceMoodChange();
  }
}

// ============================================
// LED INDICATOR CONTROL
// ============================================
void controlLEDIndicators() {
  LEDPattern pattern;
  
  switch (currentMood) {
    case SLEEPING:
      pattern = LED_OFF;
      break;
    case LAZY_WATCH:
      pattern = LED_SLOW_BLINK;
      break;
    case CURIOUS:
      pattern = LED_CHASE;
      break;
    case STALKING:
      pattern = LED_MEDIUM_BLINK;
      break;
    case ALERT:
      pattern = LED_FAST_BLINK;
      break;
    case AGGRESSIVE:
      pattern = LED_DANGER_FLASH;
      break;
    case PLAYING:
      pattern = LED_PULSE;
      break;
  }
  
  executeLEDPattern(pattern);
}

void executeLEDPattern(LEDPattern pattern) {
  unsigned long currentTime = millis();
  
  switch (pattern) {
    case LED_OFF:
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_3, LOW);
      break;
      
    case LED_SLOW_BLINK:
      if (currentTime - lastLEDUpdate > 1000) {
        ledState = !ledState;
        digitalWrite(LED_1, ledState);
        digitalWrite(LED_2, ledState);
        digitalWrite(LED_3, ledState);
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_MEDIUM_BLINK:
      if (currentTime - lastLEDUpdate > 400) {
        ledState = !ledState;
        digitalWrite(LED_1, ledState);
        digitalWrite(LED_2, ledState);
        digitalWrite(LED_3, ledState);
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_FAST_BLINK:
      if (currentTime - lastLEDUpdate > 150) {
        ledState = !ledState;
        digitalWrite(LED_1, ledState);
        digitalWrite(LED_2, ledState);
        digitalWrite(LED_3, ledState);
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_CHASE:
      if (currentTime - lastLEDUpdate > 200) {
        digitalWrite(LED_1, ledStep == 0 ? HIGH : LOW);
        digitalWrite(LED_2, ledStep == 1 ? HIGH : LOW);
        digitalWrite(LED_3, ledStep == 2 ? HIGH : LOW);
        ledStep = (ledStep + 1) % 3;
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_PULSE:
      if (currentTime - lastLEDUpdate > 80) {
        if (ledStep < 5) {
          digitalWrite(LED_1, HIGH);
          digitalWrite(LED_2, HIGH);
          digitalWrite(LED_3, HIGH);
          delay(ledStep * 2);
          digitalWrite(LED_1, LOW);
          digitalWrite(LED_2, LOW);
          digitalWrite(LED_3, LOW);
        } else {
          digitalWrite(LED_1, HIGH);
          digitalWrite(LED_2, HIGH);
          digitalWrite(LED_3, HIGH);
          delay((10 - ledStep) * 2);
          digitalWrite(LED_1, LOW);
          digitalWrite(LED_2, LOW);
          digitalWrite(LED_3, LOW);
        }
        ledStep = (ledStep + 1) % 10;
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_DANGER_FLASH:
      if (currentTime - lastLEDUpdate > 60) {
        if (ledStep % 2 == 0) {
          digitalWrite(LED_1, HIGH);
          digitalWrite(LED_2, HIGH);
          digitalWrite(LED_3, HIGH);
        } else {
          digitalWrite(LED_1, LOW);
          digitalWrite(LED_2, LOW);
          digitalWrite(LED_3, LOW);
        }
        ledStep++;
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_ALL_ON:
      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      digitalWrite(LED_3, HIGH);
      break;
  }
}

// ============================================
// BEHAVIORS - FULL 180° MOVEMENT
// ============================================
void executeBehavior() {
  switch (currentMood) {
    case SLEEPING:
      behaviorSleeping();
      break;
    case LAZY_WATCH:
      behaviorLazyWatch();
      break;
    case CURIOUS:
      behaviorCurious();
      break;
    case STALKING:
      behaviorStalking();
      break;
    case ALERT:
      behaviorAlert();
      break;
    case AGGRESSIVE:
      behaviorAggressive();
      break;
    case PLAYING:
      behaviorPlaying();
      break;
  }
}

void behaviorSleeping() {
  targetAngle = HEAD_CENTER;
  smoothMove(2);
}

void behaviorLazyWatch() {
  unsigned long currentTime = millis();
  
  // Wide scan dengan full range
  if (currentTime - lastMoveTime > 350) {
    targetAngle += scanDirection * SCAN_STEP;
    
    if (targetAngle >= HEAD_MAX - 10) {
      targetAngle = HEAD_MAX - 10;
      scanDirection = -1;
    } else if (targetAngle <= HEAD_MIN + 10) {
      targetAngle = HEAD_MIN + 10;
      scanDirection = 1;
    }
    
    smoothMove(5);
    lastMoveTime = currentTime;
  }
}

void behaviorCurious() {
  unsigned long currentTime = millis();
  
  if (objectDetected) {
    if (currentTime - lastMoveTime > 180) {
      // Tracking dengan head tilt simulation
      targetAngle = objectAngle + random(-10, 11);
      targetAngle = constrain(targetAngle, HEAD_MIN, HEAD_MAX);
      smoothMove(6);
      lastMoveTime = currentTime;
    }
  } else {
    behaviorLazyWatch();
  }
}

void behaviorStalking() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime > 90) {
    if (isMoving) {
      // Prediksi gerakan dengan menggunakan full range
      int prediction = (currentAngle < objectAngle) ? 10 : -10;
      targetAngle = objectAngle + prediction;
    } else {
      targetAngle = objectAngle + random(-6, 7);
    }
    
    targetAngle = constrain(targetAngle, HEAD_MIN, HEAD_MAX);
    smoothMove(8);
    lastMoveTime = currentTime;
  }
}

void behaviorAlert() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime > 40) {
    targetAngle = objectAngle + random(-4, 5);
    targetAngle = constrain(targetAngle, HEAD_MIN, HEAD_MAX);
    smoothMove(10);
    lastMoveTime = currentTime;
  }
}

void behaviorAggressive() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime > 30) {
    targetAngle = objectAngle;
    
    // Gerakan "hissing" lebih dramatis
    if (random(0, 10) < 5) {
      targetAngle += random(-15, 16);
    }
    
    targetAngle = constrain(targetAngle, HEAD_MIN, HEAD_MAX);
    smoothMove(15);
    lastMoveTime = currentTime;
  }
}

void behaviorPlaying() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime > 100) {
    // Playful movement menggunakan full 180° range
    targetAngle = random(HEAD_MIN + 10, HEAD_MAX - 10);
    smoothMove(random(8, 18));
    lastMoveTime = currentTime;
  }
}

// ============================================
// SMOOTH SERVO MOVEMENT
// ============================================
void smoothMove(int speed) {
  if (abs(currentAngle - targetAngle) <= speed) {
    currentAngle = targetAngle;
    headServo.write(currentAngle);
    return;
  }
  
  if (currentAngle < targetAngle) {
    currentAngle += speed;
    if (currentAngle > targetAngle) currentAngle = targetAngle;
  } else if (currentAngle > targetAngle) {
    currentAngle -= speed;
    if (currentAngle < targetAngle) currentAngle = targetAngle;
  }
  
  currentAngle = constrain(currentAngle, HEAD_MIN, HEAD_MAX);
  headServo.write(currentAngle);
}

// ============================================
// VOICE CONTROL
// ============================================
void controlVoice() {
  unsigned long currentTime = millis();
  
  if (currentMood == SLEEPING) {
    noTone(BUZZER);
    return;
  }
  
  int freq = 0;
  int duration = 0;
  int interval = 0;
  bool complexPattern = false;
  
  switch (currentMood) {
    case LAZY_WATCH:
      freq = 700;
      duration = 120;
      interval = 6000;
      break;
      
    case CURIOUS:
      freq = 1200;
      duration = 180;
      interval = 2000;
      complexPattern = true;
      break;
      
    case STALKING:
      freq = 500;
      duration = 250;
      interval = 1200;
      break;
      
    case ALERT:
      freq = 1600;
      duration = 150;
      interval = 600;
      complexPattern = true;
      break;
      
    case AGGRESSIVE:
      freq = 2800;
      duration = 80;
      interval = 200;
      complexPattern = true;
      break;
      
    case PLAYING:
      freq = random(1000, 2200);
      duration = random(100, 250);
      interval = random(400, 1200);
      complexPattern = true;
      break;
  }
  
  if (currentTime - lastSoundTime >= interval) {
    if (complexPattern) {
      if (currentMood == CURIOUS || currentMood == ALERT) {
        tone(BUZZER, freq);
        delay(duration / 2);
        tone(BUZZER, freq + 400);
        delay(duration / 2);
        noTone(BUZZER);
        
      } else if (currentMood == AGGRESSIVE) {
        for (int i = 0; i < 4; i++) {
          tone(BUZZER, freq + (i * 150));
          delay(25);
          noTone(BUZZER);
          delay(15);
        }
        
      } else if (currentMood == PLAYING) {
        tone(BUZZER, freq);
        delay(duration / 3);
        tone(BUZZER, freq + 300);
        delay(duration / 3);
        tone(BUZZER, freq - 200);
        delay(duration / 3);
        noTone(BUZZER);
      }
    } else {
      tone(BUZZER, freq);
      delay(duration);
      noTone(BUZZER);
    }
    
    lastSoundTime = currentTime;
  }
}

// ============================================
// SPONTANEOUS BEHAVIORS
// ============================================
void spontaneousBehaviors() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastRandomTime > random(15000, 30000)) {
    
    if (currentMood == LAZY_WATCH || currentMood == CURIOUS) {
      int action = random(0, 5);
      
      if (action == 0) {
        // Full stretch yawn
        Serial.println(F("   🥱 *big yawn with full stretch*"));
        int oldAngle = currentAngle;
        
        targetAngle = HEAD_MIN + 5;
        for(int i=0; i<25; i++) { smoothMove(3); delay(20); }
        delay(200);
        
        targetAngle = HEAD_MAX - 5;
        for(int i=0; i<35; i++) { smoothMove(5); delay(20); }
        delay(200);
        
        targetAngle = oldAngle;
        for(int i=0; i<25; i++) { smoothMove(3); delay(20); }
        
      } else if (action == 1) {
        // Wide panoramic scan
        Serial.println(F("   👀 *panoramic scan*"));
        int oldAngle = currentAngle;
        
        targetAngle = HEAD_MIN + 15;
        while(abs(currentAngle - targetAngle) > 5) {
          smoothMove(4);
          delay(25);
        }
        delay(300);
        
        targetAngle = HEAD_MAX - 15;
        while(abs(currentAngle - targetAngle) > 5) {
          smoothMove(4);
          delay(25);
        }
        delay(300);
        
        targetAngle = oldAngle;
        while(abs(currentAngle - targetAngle) > 5) {
          smoothMove(5);
          delay(25);
        }
        
      } else if (action == 2) {
        Serial.println(F("   😺 *meow*"));
        tone(BUZZER, 1100);
        delay(150);
        tone(BUZZER, 1400);
        delay(120);
        noTone(BUZZER);
        
      } else if (action == 3) {
        // Head shake
        Serial.println(F("   🙃 *head shake*"));
        int oldAngle = currentAngle;
        for(int i=0; i<4; i++) {
          targetAngle = oldAngle - 25;
          for(int j=0; j<10; j++) { smoothMove(8); delay(15); }
          targetAngle = oldAngle + 25;
          for(int j=0; j<10; j++) { smoothMove(8); delay(15); }
        }
        targetAngle = oldAngle;
        for(int j=0; j<15; j++) { smoothMove(5); delay(20); }
      }
    }
    
    lastRandomTime = currentTime;
  }
}

// ============================================
// DISPLAY & COMMANDS
// ============================================
void displayStatus() {
  static unsigned long lastDisplay = 0;
  
  if (millis() - lastDisplay > 800) {
    Serial.print(F("🐱 "));
    
    switch (currentMood) {
      case SLEEPING: Serial.print(F("😴 SLEEP    ")); break;
      case LAZY_WATCH: Serial.print(F("😌 LAZY     ")); break;
      case CURIOUS: Serial.print(F("🤔 CURIOUS  ")); break;
      case STALKING: Serial.print(F("😼 STALKING ")); break;
      case ALERT: Serial.print(F("😾 ALERT    ")); break;
      case AGGRESSIVE: Serial.print(F("😡 AGGRESSV ")); break;
      case PLAYING: Serial.print(F("😸 PLAYING  ")); break;
    }
    
    Serial.print(F(" | Head:"));
    if (currentAngle < 100) Serial.print(F(" "));
    if (currentAngle < 10) Serial.print(F(" "));
    Serial.print(currentAngle);
    Serial.print(F("°→"));
    if (targetAngle < 100) Serial.print(F(" "));
    if (targetAngle < 10) Serial.print(F(" "));
    Serial.print(targetAngle);
    Serial.print(F("°"));
    
    // Position indicator
    if (isLookingLeft) Serial.print(F(" [◀LEFT]"));
    else if (isLookingRight) Serial.print(F(" [RIGHT▶]"));
    else Serial.print(F(" [CENTER]"));
    
    Serial.print(F(" | Dist:"));
    
    if (currentDistance > 0) {
      if (currentDistance < 100) Serial.print(F(" "));
      if (currentDistance < 10) Serial.print(F(" "));
      Serial.print(currentDistance);
      Serial.print(F("cm"));
    } else {
      Serial.print(F(" ---"));
    }
    
    Serial.print(F(" | Mov:"));
    Serial.print(isMoving ? F("Y") : F("N"));
    Serial.print(F(" | App:"));
    Serial.print(isApproaching ? F("Y") : F("N"));
    Serial.println();
    
    lastDisplay = millis();
  }
}

void announceMoodChange() {
  Serial.println();
  Serial.print(F(">>> "));
  
  switch (currentMood) {
    case SLEEPING:
      Serial.println(F("😴 SLEEP mode - LED: OFF"));
      break;
    case LAZY_WATCH:
      Serial.println(F("😌 LAZY WATCH - LED: Slow Blink"));
      break;
    case CURIOUS:
      Serial.println(F("🤔 CURIOUS - LED: Chase Pattern"));
      break;
    case STALKING:
      Serial.println(F("😼 STALKING - LED: Medium Blink"));
      break;
    case ALERT:
      Serial.println(F("😾 ALERT! - LED: Fast Blink"));
      break;
    case AGGRESSIVE:
      Serial.println(F("😡 AGGRESSIVE! - LED: DANGER FLASH!"));
      break;
    case PLAYING:
      Serial.println(F("😸 PLAYING - LED: Pulse Effect"));
      break;
  }
  
  Serial.println();
}

void checkCommands() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'r':
      case 'R':
        Serial.println(F("\n>>> SYSTEM RESET"));
        currentAngle = HEAD_CENTER;
        targetAngle = HEAD_CENTER;
        headServo.write(HEAD_CENTER);
        currentMood = SLEEPING;
        noTone(BUZZER);
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);
        digitalWrite(LED_3, LOW);
        Serial.println(F("✓ Reset complete\n"));
        break;
        
      case 't':
      case 'T':
        Serial.println(F("\n>>> BUZZER TEST"));
        testBuzzer();
        break;
        
      case 'l':
      case 'L':
        Serial.println(F("\n>>> LED TEST"));
        testLEDs();
        break;
        
      case 'm':
      case 'M':
        Serial.println(F("\n>>> SERVO MOVEMENT TEST (FULL 180°)"));
        testServoMovement();
        break;
        
      case 'w':
      case 'W':
        Serial.println(F("\n>>> WIDE SCAN MODE"));
        performWideScan();
        break;
        
      case 'f':
      case 'F':
        Serial.println(F("\n>>> FULL SWEEP DEMO"));
        performFullSweep();
        break;
        
      case 'p':
      case 'P':
        currentMood = PLAYING;
        lastPlayTime = millis();
        Serial.println(F("\n>>> PLAY MODE ACTIVATED! 😸\n"));
        break;
        
      case 's':
      case 'S':
        currentMood = SLEEPING;
        Serial.println(F("\n>>> SLEEP MODE FORCED 😴\n"));
        break;
        
      case 'i':
      case 'I':
        printInfo();
        break;
        
      case 'h':
      case 'H':
        printHelp();
        break;
    }
  }
}

// ============================================
// TEST FUNCTIONS
// ============================================
void testServoMovement() {
  Serial.println(F("  Testing FULL 180° servo movement..."));
  Serial.println();
  
  Serial.println(F("  1. Center position (90°)"));
  targetAngle = HEAD_CENTER;
  for(int i=0; i<20; i++) {
    smoothMove(5);
    delay(30);
  }
  Serial.print(F("     Current angle: "));
  Serial.println(currentAngle);
  delay(1000);
  
  Serial.println(F("  2. Moving to FAR LEFT (0°)"));
  targetAngle = HEAD_MIN;
  while(abs(currentAngle - targetAngle) > 2) {
    smoothMove(4);
    delay(30);
  }
  Serial.print(F("     Current angle: "));
  Serial.println(currentAngle);
  delay(1500);
  
  Serial.println(F("  3. Moving to FAR RIGHT (180°)"));
  targetAngle = HEAD_MAX;
  while(abs(currentAngle - targetAngle) > 2) {
    smoothMove(4);
    delay(30);
  }
  Serial.print(F("     Current angle: "));
  Serial.println(currentAngle);
  delay(1500);
  
  Serial.println(F("  4. Back to CENTER (90°)"));
  targetAngle = HEAD_CENTER;
  while(abs(currentAngle - targetAngle) > 2) {
    smoothMove(4);
    delay(30);
  }
  Serial.print(F("     Current angle: "));
  Serial.println(currentAngle);
  delay(500);
  
  Serial.println(F("  5. Full range scanning (5 cycles)"));
  for(int cycle=0; cycle<5; cycle++) {
    Serial.print(F("     Cycle "));
    Serial.print(cycle + 1);
    Serial.println(F("/5..."));
    
    // Ke kiri penuh
    targetAngle = HEAD_MIN + 20;
    while(abs(currentAngle - targetAngle) > 5) {
      smoothMove(6);
      delay(20);
    }
    
    // Ke kanan penuh
    targetAngle = HEAD_MAX - 20;
    while(abs(currentAngle - targetAngle) > 5) {
      smoothMove(6);
      delay(20);
    }
  }
  
  // Kembali ke center
  targetAngle = HEAD_CENTER;
  while(abs(currentAngle - targetAngle) > 5) {
    smoothMove(5);
    delay(30);
  }
  
  Serial.println();
  Serial.println(F("  ✓ Full 180° servo test complete!"));
  Serial.println(F("  Range verified: 0° to 180°"));
  Serial.println();
}

void performWideScan() {
  Serial.println(F("  Performing wide area scan..."));
  Serial.println();
  
  int positions[] = {0, 30, 60, 90, 120, 150, 180};
  
  for(int i=0; i<7; i++) {
    Serial.print(F("  Scanning position: "));
    Serial.print(positions[i]);
    Serial.println(F("°"));
    
    targetAngle = positions[i];
    while(abs(currentAngle - targetAngle) > 3) {
      smoothMove(5);
      delay(25);
    }
    
    // Take reading
    long dist = readAndFilterDistance();
    Serial.print(F("    Distance detected: "));
    if(dist > 0) {
      Serial.print(dist);
      Serial.println(F(" cm"));
    } else {
      Serial.println(F("No object"));
    }
    
    delay(500);
  }
  
  // Return to center
  targetAngle = HEAD_CENTER;
  while(abs(currentAngle - targetAngle) > 3) {
    smoothMove(5);
    delay(25);
  }
  
  Serial.println();
  Serial.println(F("  ✓ Wide scan complete!"));
  Serial.println();
}

void performFullSweep() {
  Serial.println(F("  Performing full 180° sweep demo..."));
  Serial.println();
  
  Serial.println(F("  1. Slow sweep: 0° → 180°"));
  targetAngle = HEAD_MIN;
  while(abs(currentAngle - targetAngle) > 3) {
    smoothMove(3);
    delay(40);
  }
  
  targetAngle = HEAD_MAX;
  while(abs(currentAngle - targetAngle) > 3) {
    smoothMove(3);
    delay(40);
  }
  delay(500);
  
  Serial.println(F("  2. Fast sweep: 180° → 0°"));
  targetAngle = HEAD_MIN;
  while(abs(currentAngle - targetAngle) > 3) {
    smoothMove(8);
    delay(20);
  }
  delay(500);
  
  Serial.println(F("  3. Rapid oscillation"));
  for(int i=0; i<8; i++) {
    targetAngle = (i % 2 == 0) ? HEAD_MIN + 30 : HEAD_MAX - 30;
    while(abs(currentAngle - targetAngle) > 5) {
      smoothMove(12);
      delay(15);
    }
    delay(100);
  }
  
  Serial.println(F("  4. Smooth wave pattern"));
  for(int angle=0; angle<=180; angle+=10) {
    targetAngle = angle;
    while(abs(currentAngle - targetAngle) > 3) {
      smoothMove(4);
      delay(25);
    }
    delay(80);
  }
  
  // Return to center
  targetAngle = HEAD_CENTER;
  while(abs(currentAngle - targetAngle) > 3) {
    smoothMove(5);
    delay(25);
  }
  
  Serial.println();
  Serial.println(F("  ✓ Full sweep demo complete!"));
  Serial.println();
}

void testBuzzer() {
  Serial.println(F("  Testing buzzer sounds..."));
  Serial.println();
  
  Serial.println(F("  1. Low tone (Stalking)"));
  tone(BUZZER, 500);
  delay(300);
  noTone(BUZZER);
  delay(200);
  
  Serial.println(F("  2. Medium tone (Curious)"));
  tone(BUZZER, 1200);
  delay(200);
  tone(BUZZER, 1600);
  delay(200);
  noTone(BUZZER);
  delay(200);
  
  Serial.println(F("  3. High tone (Alert)"));
  tone(BUZZER, 1800);
  delay(250);
  noTone(BUZZER);
  delay(200);
  
  Serial.println(F("  4. Hiss pattern (Aggressive)"));
  for (int i = 0; i < 5; i++) {
    tone(BUZZER, 2800 + (i * 100));
    delay(40);
    noTone(BUZZER);
    delay(30);
  }
  delay(200);
  
  Serial.println(F("  5. Playful chirp"));
  tone(BUZZER, 1500);
  delay(100);
  tone(BUZZER, 1800);
  delay(100);
  tone(BUZZER, 1300);
  delay(100);
  noTone(BUZZER);
  
  Serial.println(F("\n  ✓ Buzzer test complete!\n"));
}

void testLEDs() {
  Serial.println(F("  Testing LED indicators..."));
  Serial.println();
  
  Serial.println(F("  1. Individual LED test"));
  Serial.print(F("     LED 1... "));
  digitalWrite(LED_1, HIGH);
  delay(500);
  digitalWrite(LED_1, LOW);
  Serial.println(F("✓"));
  
  Serial.print(F("     LED 2... "));
  digitalWrite(LED_2, HIGH);
  delay(500);
  digitalWrite(LED_2, LOW);
  Serial.println(F("✓"));
  
  Serial.print(F("     LED 3... "));
  digitalWrite(LED_3, HIGH);
  delay(500);
  digitalWrite(LED_3, LOW);
  Serial.println(F("✓"));
  
  delay(300);
  
  Serial.println(F("  2. Pattern tests"));
  
  Serial.println(F("     Slow Blink (LAZY WATCH)"));
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_3, HIGH);
    delay(1000);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    delay(1000);
  }
  
  Serial.println(F("     Chase Pattern (CURIOUS)"));
  for (int i = 0; i < 9; i++) {
    digitalWrite(LED_1, i % 3 == 0 ? HIGH : LOW);
    digitalWrite(LED_2, i % 3 == 1 ? HIGH : LOW);
    digitalWrite(LED_3, i % 3 == 2 ? HIGH : LOW);
    delay(200);
  }
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  
  Serial.println(F("     Fast Blink (ALERT)"));
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_3, HIGH);
    delay(150);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    delay(150);
  }
  
  Serial.println(F("     Danger Flash (AGGRESSIVE)"));
  for (int i = 0; i < 20; i++) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_3, HIGH);
    delay(60);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    delay(60);
  }
  
  Serial.println(F("\n  ✓ LED test complete!\n"));
}

void printInfo() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════╗"));
  Serial.println(F("║       SYSTEM INFORMATION           ║"));
  Serial.println(F("╠════════════════════════════════════╣"));
  Serial.print(F("║ Servo Range: "));
  Serial.print(HEAD_MIN);
  Serial.print(F("° - "));
  Serial.print(HEAD_MAX);
  Serial.println(F("° (FULL)  ║"));
  Serial.print(F("║ Current Mood: "));
  switch (currentMood) {
    case SLEEPING: Serial.println(F("SLEEPING        ║")); break;
    case LAZY_WATCH: Serial.println(F("LAZY WATCH      ║")); break;
    case CURIOUS: Serial.println(F("CURIOUS         ║")); break;
    case STALKING: Serial.println(F("STALKING        ║")); break;
    case ALERT: Serial.println(F("ALERT           ║")); break;
    case AGGRESSIVE: Serial.println(F("AGGRESSIVE      ║")); break;
    case PLAYING: Serial.println(F("PLAYING         ║")); break;
  }
  Serial.print(F("║ Current Angle: "));
  if (currentAngle < 100) Serial.print(F(" "));
  if (currentAngle < 10) Serial.print(F(" "));
  Serial.print(currentAngle);
  Serial.println(F("°           ║"));
  Serial.print(F("║ Target Angle: "));
  if (targetAngle < 100) Serial.print(F(" "));
  if (targetAngle < 10) Serial.print(F(" "));
  Serial.print(targetAngle);
  Serial.println(F("°            ║"));
  Serial.print(F("║ Head Position: "));
  if (isLookingLeft) Serial.println(F("LEFT        ║"));
  else if (isLookingRight) Serial.println(F("RIGHT       ║"));
  else Serial.println(F("CENTER      ║"));
  Serial.print(F("║ Distance: "));
  if (currentDistance > 0) {
    if (currentDistance < 100) Serial.print(F(" "));
    if (currentDistance < 10) Serial.print(F(" "));
    Serial.print(currentDistance);
    Serial.println(F(" cm             ║"));
  } else {
    Serial.println(F("N/A                ║"));
  }
  Serial.print(F("║ Object Moving: "));
  Serial.println(isMoving ? F("YES           ║") : F("NO            ║"));
  Serial.print(F("║ Approaching: "));
  Serial.println(isApproaching ? F("YES             ║") : F("NO              ║"));
  Serial.print(F("║ Object Speed: "));
  if (objectSpeed < 10) Serial.print(F(" "));
  Serial.print(objectSpeed);
  Serial.println(F(" cm/s         ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════╗"));
  Serial.println(F("║       COMMAND REFERENCE            ║"));
  Serial.println(F("╠════════════════════════════════════╣"));
  Serial.println(F("║ R - Reset system                   ║"));
  Serial.println(F("║ T - Test buzzer sounds             ║"));
  Serial.println(F("║ L - Test LED indicators            ║"));
  Serial.println(F("║ M - Test servo (FULL 180°)         ║"));
  Serial.println(F("║ W - Wide scan mode                 ║"));
  Serial.println(F("║ F - Full sweep demo                ║"));
  Serial.println(F("║ P - Force PLAY mode                ║"));
  Serial.println(F("║ S - Force SLEEP mode               ║"));
  Serial.println(F("║ I - Show system information        ║"));
  Serial.println(F("║ H - Show this help                 ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  Serial.println();
}

// ============================================
// STARTUP SEQUENCE
// ============================================
void catWakeUpSequence() {
  Serial.println(F("\n🐱 Cat is waking up... (Full 180° Range)"));
  delay(500);
  
  // LED wake up
  Serial.println(F("💡 Initializing indicators..."));
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_1, HIGH);
    delay(150);
    digitalWrite(LED_2, HIGH);
    delay(150);
    digitalWrite(LED_3, HIGH);
    delay(150);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    delay(200);
  }
  
  // All on briefly
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  delay(500);
  
  // Full range stretch movement
  Serial.println(F("🙆 Full range stretching..."));
  
  // Ke kiri penuh (0°)
  Serial.println(F("   Stretching left to 0°..."));
  targetAngle = HEAD_MIN + 10;
  for(int i=0; i<40; i++) {
    smoothMove(3);
    delay(20);
  }
  delay(600);
  
  // Ke kanan penuh (180°)
  Serial.println(F("   Stretching right to 180°..."));
  targetAngle = HEAD_MAX - 10;
  for(int i=0; i<70; i++) {
    smoothMove(3);
    delay(20);
  }
  delay(600);
  
  // Kembali ke center (90°)
  Serial.println(F("   Returning to center 90°..."));
  targetAngle = HEAD_CENTER;
  for(int i=0; i<40; i++) {
    smoothMove(3);
    delay(20);
  }
  delay(400);
  
  // Quick head shake using full range
  Serial.println(F("   Quick calibration shake..."));
  for(int i=0; i<3; i++) {
    targetAngle = HEAD_CENTER - 40;
    for(int j=0; j<15; j++) { smoothMove(8); delay(15); }
    targetAngle = HEAD_CENTER + 40;
    for(int j=0; j<15; j++) { smoothMove(8); delay(15); }
  }
  targetAngle = HEAD_CENTER;
  for(int j=0; j<20; j++) { smoothMove(5); delay(20); }
  
  // Morning meow
  Serial.println(F("🔊 Morning meow..."));
  tone(BUZZER, 900);
  delay(180);
  tone(BUZZER, 1300);
  delay(180);
  noTone(BUZZER);
  delay(300);
  tone(BUZZER, 1100);
  delay(220);
  noTone(BUZZER);
  
  // Turn off all LEDs
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  
  delay(500);
  Serial.println(F("✓ Cat is ready with FULL 180° range!"));
  Serial.print(F("✓ Servo at angle: "));
  Serial.print(currentAngle);
  Serial.println(F("° (Center position)"));
  Serial.println(F("✓ Movement range: 0° - 180° verified"));
  Serial.println();
}