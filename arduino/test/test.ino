const byte PIN_COUNT = 6;

const byte IN_PINS[] = {
  2,
  3,
  4,
  5,
  6,
  7
};

const byte OUT_PINS[] = {
  15,
  14,
  13,
  12,
  11,
  10
};

enum CurrentMode {
  MODE_CONNECTOR_TEST,
  MODE_ENCODER_TEST,
  MODE_NOT_SELECTED
};

String modeTexts[] = {
  "Connector / wire test",
  "Encoder test"
};

CurrentMode currentMode = MODE_NOT_SELECTED;

void setup() {
  Serial.begin(115200);
  promptMode();
}

void loop() {
  if (currentMode == MODE_CONNECTOR_TEST) {
    testConnector();
  } else if (currentMode == MODE_ENCODER_TEST) {
    testEncoder();
  }
  
  if (Serial.peek() != -1) {
    char input = Serial.read();
    Serial.println(input);
    Serial.readString();
    if (input == 'q') {
      promptMode();
    }
  }
}

void promptMode() {
  for (byte i = 0; i < PIN_COUNT; ++i) {
    pinMode(OUT_PINS[i], INPUT);
    pinMode(IN_PINS[i], INPUT);
  }
  
  currentMode = MODE_NOT_SELECTED;
  while (currentMode == MODE_NOT_SELECTED) {
    Serial.println("Select test mode:");
    for (byte i = 0; i < MODE_NOT_SELECTED; ++i) {
      Serial.print(i+1);
      Serial.print(": ");
      Serial.println(modeTexts[i]);
    }
    
    char input = promptChar();
    
    if (input == '1') {
      changeMode(MODE_CONNECTOR_TEST);
    } else if (input == '2') {
      changeMode(MODE_ENCODER_TEST);
    } else {
      Serial.print("Unknown mode: ");
      Serial.println(input);
      Serial.println();
    }
  }
}

void changeMode(CurrentMode mode) {
  Serial.print("Changing mode to: ");
  Serial.println(modeTexts[mode]);
  if (mode == MODE_CONNECTOR_TEST) {
    for (byte i = 0; i < PIN_COUNT; ++i) {
      pinMode(OUT_PINS[i], OUTPUT);
      pinMode(IN_PINS[i], INPUT_PULLUP);
    }
  } else if (mode == MODE_ENCODER_TEST) {
    for (byte i = 0; i < PIN_COUNT; ++i) {
      pinMode(OUT_PINS[i], INPUT);
      pinMode(IN_PINS[i], INPUT_PULLUP);
    }
  }
  currentMode = mode;

  Serial.println("To change mode press q");
}

byte currentPin = 0;
void testConnector() {
  digitalWrite(OUT_PINS[currentPin], HIGH);
  digitalWrite(OUT_PINS[currentPin == 0 ? 5 : currentPin - 1], LOW);
  // put your main code here, to run repeatedly:
  Serial.println("O123456 I123456");
  Serial.print(" ");
  for (byte i = 0; i < PIN_COUNT; ++i) {
    Serial.print(digitalRead(OUT_PINS[i]) ? "1" : "0");
  }

  Serial.print("  ");
  for (byte i = 0; i < PIN_COUNT; ++i) {
    Serial.print(digitalRead(IN_PINS[i]) ? "1" : "0");
  }
  Serial.println();
  currentPin = (currentPin + 1) % 6;
  delay(1000);
}

byte currentState = -1;
byte changes = 0;
void testEncoder() {
  byte state = 0;
  for (byte i = 0; i < PIN_COUNT; ++i) {
    state |= (digitalRead(IN_PINS[i]) ? 1 : 0) << i;
  }

  if (state != currentState) {
    if (changes % 10 == 0) {
      Serial.println();
      Serial.println(" S22L11");
      Serial.println(" WBAEBA");
      Serial.println("I123456");
    }
    currentState = state;
    Serial.print(" ");
    for (byte i = 0; i < PIN_COUNT; ++i) {
      Serial.print(bitRead(currentState, i) ? "1" : "0");
    }
    Serial.println("");
    changes++;
  }
}

char promptChar() {
  while (Serial.peek() == -1) {}
  char input = Serial.read();
  Serial.readString();
  Serial.println(input);
  return input;
}
