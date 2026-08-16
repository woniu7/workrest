// Parts of the platform layer that are identical for every linux main loop backend, so they
// are not duplicated into platform-glib.c and platform-poll.c.
#include "../platform.h"
#include <time.h>

static PlatKeyFunc g_key_fn   = NULL;
static void       *g_key_user = NULL;

// =========================
// Key delivery
// =========================
void plat_set_key_handler(PlatKeyFunc fn, void *user) {
    g_key_fn   = fn;
    g_key_user = user;
}

// Every key source on linux already runs on the main loop thread -- the stdin watch, the GTK
// key controller, and the SDL poll driven by a platform timer -- so this dispatches straight
// through instead of hopping threads.
void plat_post_key(char key) {
    if (g_key_fn) g_key_fn(g_key_user, key);
}

// =========================
// Session unlock
// =========================
// Not wired up on linux by design: no lock-screen integration here. The hook exists for
// windows, which does get a session notification.
void plat_set_unlock_handler(PlatUnlockFunc fn, void *user) {
    (void)fn;
    (void)user;
}

// =========================
// Misc
// =========================
void plat_format_time(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_now);
}
