#include <Wire.h>
#include <Arduino_APDS9960.h>

class Gesture {
  public:
    Gesture(uint8_t intPin):
      Gesture(intPin, _sensitivity) {};
    Gesture(uint8_t intPin, int sensitivity) :
      _sensitivity(sensitivity),
      _dir(-1),
      _I2CCLR(0),
      _ges_sens(_I2CCLR, intPin) {};
    bool startGesture();
    int getDir();
  private:
    int _sensitivity;
    int _dir;
    TwoWire _I2CCLR;
    APDS9960 _ges_sens;
};
