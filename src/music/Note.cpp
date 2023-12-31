#include "Note.h"

namespace music {

  Note::Note(String name, double frequency) : name(name), frequency(frequency) {}

  String Note::getName() const {
    return name;
  }

  double Note::getFrequency() const {
    return frequency;
  }

}
