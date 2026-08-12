#ifndef VIEW_H
#define VIEW_H

#include "rest.h"  // RestCore

// View interface (display + input): implemented by either gui.c (graphical UI) or cli.c (terminal output), pick one.
// Which one is chosen is decided at compile time by which source file is linked (see the VIEW variable in the Makefile); no runtime check.
// The core (rest.c) calls these functions directly, holding no function pointers and doing no if-branching.
void view_init(RestCore *core);    // Initialize view and input (GUI creates a window / terminal sets up stdin); core is used to send keys back
void view_rest_begin(int seconds); // Enter rest: show the UI, give the initial seconds count
void view_tick(int seconds);       // Countdown update
void view_work_begin(void);        // Enter work: hide the UI
void view_destroy(void);           // Destroy the view

#endif // VIEW_H
