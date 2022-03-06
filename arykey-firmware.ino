// AryKey Firmware
//
// input: sha512(pin, domain, username)
//   /4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a
//
// output:
//   OK
//   ERROR
//   INVALID

#include <Arduino.h>
#include <Keyboard.h>
#include <KDF.h>
#include <base64.hpp>
#include "LedState.h"

extern const char *seed;

LedState ledState(LED_BUILTIN);
static const uint16_t BUFFER_SIZE = 256;
static const uint16_t INPUT_SIZE = 128;
static const uint16_t OUTPUT_SIZE = 88;
char input[BUFFER_SIZE];
char outputBase64[OUTPUT_SIZE + 1];
bool isExecuting = false;

// Methods
void processSerial(void);
void processInputCommand();
bool processInputHash(size_t inputLength);
bool derivePassword(const char *input);
void type();


void processSerial(void) {
  static byte index = 0;
  while (Serial.available() > 0 && isExecuting == false) {
    char readChar = Serial.read();
    if (readChar != '\n') {
      if (index < BUFFER_SIZE) input[index++] = readChar;
    } else {
      input[index] = '\0'; // terminate the char array
      index = 0;
      isExecuting = true;
    }
  }
}

/**
   Remove first char '/'
*/
void processInputCommand() {
  size_t inputLength = strlen(input);
  if ((inputLength == (INPUT_SIZE + 1)) && input[0] == '/') {

    if (processInputHash(inputLength)) {
      Serial.println("OK");
      ledState.setPulsing();

    } else {
      Serial.println("ERROR");
      ledState.setOff();
    }
  } else {
    Serial.println("INVALID");
  }

  memset(&input[0], 0, sizeof(input));
  isExecuting = false;
}

bool processInputHash(size_t inputLength) {
  // skip 1 char regarding the prefix '/'
  // & add one extra character for the null terminator
  char *plain = (char *) malloc (inputLength);  // + 1 - 1
  if (plain) {
    // Remove prefix '/' (copy string & skip prefix)
    strncpy(plain, input + 1, inputLength); // dst, src + start, end
    size_t plainLength = strlen(plain);
    plain[plainLength] = '\0';
    Serial.printf("Input:%s\n", plain);

    // check if valid hex input
    for (int i = 0; i < plainLength; i++) {
      if (!isHexadecimalDigit(plain[i])) return false;
    }
    return derivePassword(plain);
  }
  return false;
}

bool derivePassword(const char *input) {
  size_t inputLength = strlen(input);
  if (inputLength != INPUT_SIZE) return false;

  uint8_t *binResult = (uint8_t *) malloc (INPUT_SIZE);
  if (binResult) {
    KDF *kdf = new KDF(seed);
    int length = kdf->derivePassword(binResult, input, INPUT_SIZE); // dst, src, srcLength
    delete kdf;
    kdf = NULL;

    uint8_t * base64Result = (uint8_t *) malloc(OUTPUT_SIZE);
    if (base64Result) {
      encode_base64(binResult, length, base64Result); // src, srcLength, dst
      free(binResult);

      memset(&outputBase64[0], 0, OUTPUT_SIZE + 1);

      strncat(outputBase64, (char *) base64Result, OUTPUT_SIZE); // dst, src

      free(base64Result);

      return true;
    }
  }
  return false;
}

void type() {
  if (strlen(outputBase64) != 0) {
    ledState.setBlinking();

    delay(150);
    Keyboard.print(outputBase64);

    delay(150);
    memset(&outputBase64[0], 0, OUTPUT_SIZE + 1);

    delay(150);
    ledState.setOn();
  }
}

//////////////////////////
// Thread 0: First core //
//////////////////////////

void setup() {
  Serial.begin(115200);
  Keyboard.begin();
  delay(1000);
}

void loop() {
  if (BOOTSEL) {
    type();
    while (BOOTSEL);
  } else {
    processSerial();
    if (isExecuting) processInputCommand();
  }
}

///////////////////////////
// Thread 1: Second core //
///////////////////////////

void loop1() {
  ledState.loop();
}
