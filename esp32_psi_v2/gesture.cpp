#include "gesture.h"

bool Gesture::startGesture() {
  return _ges_sens.begin();
}

int Gesture::getDir() {
  if (_ges_sens.gestureAvailable()) {
    _dir = _ges_sens.readGesture();
    return _dir;
  }
  return -1;
}
