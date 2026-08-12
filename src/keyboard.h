#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "rest.h"  // RestCore

// Listens to terminal/console keyboard; keys read are fed into the core via rest_core_send_key.
// View-independent: both the GUI and CLI builds link and enable it, so the GUI version also responds to terminal keys.
void keyboard_start(RestCore *core);

#endif // KEYBOARD_H
