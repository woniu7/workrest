#ifndef CONFIG_H
#define CONFIG_H

// Timing knobs of the work/rest cycle.
//
// Deliberately NOT included from rest.h: rest.h reaches every translation unit (view.h and
// keyboard.h both pull it in), so keeping these numbers there means editing one of them
// rebuilds gui.c / sdl3.c too and re-parses all of GTK4 / SDL3 for nothing.
// Include this header only from the .c files that actually read the values — today that is
// just the per-platform rest.c. Do not add it to rest.h "for convenience"; that undoes the
// whole point. For the same reason Options stays in rest.h: it is part of the core API.

#define WORK_SECONDS     2700 // One work stretch (45 min)
#define BREAK_SECONDS    180  // One break (3 min)
#define INIT_SECONDS     3    // Startup / post-unlock rest, just long enough to notice
#define DELAY_SECONDS    1    // Keys stay ignored this long after a countdown starts (mis-touch guard)
#define POSTPONE_SECONDS 120  // Seconds the 'r' key postpones work by

// The 'l' key jams the countdown to this: not infinite, just far enough out (~11.5 days) that
// the break is effectively postponed until the user resets it.
#define POSTPONE_LONG_SECONDS 999999

#endif // CONFIG_H
