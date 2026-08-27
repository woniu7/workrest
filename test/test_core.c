// Unit tests for the core state machine (src/rest.c), run against the mock platform + view.
//
// Everything is driven by the virtual clock in mock_platform.c: create a core, call the public
// rest_core_* API, advance time by hand, then assert on the view events the core emitted. No
// real time passes, so a 45-minute work stretch is one mock_advance() call.
#include "rest.h"
#include "config.h"
#include "mocks.h"
#include <stdio.h>

// --- tiny assertion framework ---
static int g_checks;
static int g_fails;

#define CHECK(cond, ...)                                        \
    do {                                                        \
        g_checks++;                                             \
        if (!(cond)) {                                          \
            g_fails++;                                          \
            printf("  FAIL [%s:%d]: ", __FILE__, __LINE__);     \
            printf(__VA_ARGS__);                                \
            printf("\n");                                       \
        }                                                       \
    } while (0)

// Fresh core with logging off and a clean view log. rest_core_new calls plat_init, which resets
// the mock clock, timer table and quit flag -- so each test starts fully isolated.
static RestCore *fresh(void) {
    Options opts;
    opts.debug = 0;
    mock_view_reset();
    return rest_core_new(&opts);
}

// =========================================================
// Tests
// =========================================================

// Startup enters the initial (post-launch) rest, and nothing else fires yet.
static void test_startup_enters_initial_rest(void) {
    RestCore *c = fresh();
    rest_core_start(c);

    CHECK(mock_view_count() == 1, "expected exactly 1 event, got %d", mock_view_count());
    CHECK(mock_view_last().type == EV_REST_BEGIN, "expected rest_begin as first event");
    CHECK(mock_view_last().value == INIT_SECONDS,
          "expected rest_begin(%d), got %d", INIT_SECONDS, mock_view_last().value);

    rest_core_free(c);
}

// The 1s countdown ticks down and, at zero, flips to work mode.
static void test_countdown_ticks_then_enters_work(void) {
    RestCore *c = fresh();
    rest_core_start(c);
    mock_view_reset(); // drop the initial rest_begin; look only at ticks + the transition

    mock_advance((long long)INIT_SECONDS * 1000);

    CHECK(mock_view_count_type(EV_TICK) == INIT_SECONDS,
          "expected %d ticks, got %d", INIT_SECONDS, mock_view_count_type(EV_TICK));
    CHECK(mock_view_at(0).value == INIT_SECONDS - 1,
          "first tick should be %d, got %d", INIT_SECONDS - 1, mock_view_at(0).value);
    CHECK(mock_view_at(INIT_SECONDS - 1).value == 0,
          "last tick should reach 0, got %d", mock_view_at(INIT_SECONDS - 1).value);
    CHECK(mock_view_last().type == EV_WORK_BEGIN, "should end in work mode");

    rest_core_free(c);
}

// After a full work stretch the core re-enters rest, this time for BREAK_SECONDS.
static void test_work_stretch_returns_to_break(void) {
    RestCore *c = fresh();
    rest_core_start(c);
    mock_advance((long long)INIT_SECONDS * 1000); // countdown -> work
    mock_view_reset();

    mock_advance((long long)WORK_SECONDS * 1000);

    CHECK(mock_view_count_type(EV_REST_BEGIN) == 1, "should enter rest once");
    CHECK(mock_view_last().type == EV_REST_BEGIN && mock_view_last().value == BREAK_SECONDS,
          "expected rest_begin(%d), got type=%d value=%d",
          BREAK_SECONDS, mock_view_last().type, mock_view_last().value);

    rest_core_free(c);
}

// Keys are ignored for DELAY_SECONDS after a countdown starts (mis-touch guard), then honoured.
static void test_keys_ignored_during_initial_delay(void) {
    RestCore *c = fresh();
    rest_core_start(c);

    rest_core_send_key(c, 'q');
    CHECK(!mock_quit_called(), "q during the delay window must be ignored");

    mock_advance((long long)DELAY_SECONDS * 1000); // delay elapses, keys count again
    rest_core_send_key(c, 'q');
    CHECK(mock_quit_called(), "q after the delay window must quit");

    rest_core_free(c);
}

// Enter a rest with keys already enabled: start, then let the mis-touch delay elapse.
static RestCore *resting_with_keys_enabled(void) {
    RestCore *c = fresh();
    rest_core_start(c);
    mock_advance((long long)DELAY_SECONDS * 1000);
    mock_view_reset();
    return c;
}

// 'b' restarts the rest countdown at BREAK_SECONDS and re-arms the mis-touch guard.
static void test_key_b_resets_countdown(void) {
    RestCore *c = resting_with_keys_enabled();

    rest_core_send_key(c, 'b');
    CHECK(mock_view_last().type == EV_REST_BEGIN && mock_view_last().value == BREAK_SECONDS,
          "b should restart rest at %d, got type=%d value=%d",
          BREAK_SECONDS, mock_view_last().type, mock_view_last().value);

    // The guard is back up: an immediate q is ignored again.
    rest_core_send_key(c, 'q');
    CHECK(!mock_quit_called(), "q right after b (guard re-armed) must be ignored");

    rest_core_free(c);
}

// 'c' abandons the break and starts a full WORK_SECONDS stretch immediately.
static void test_key_c_continue_working(void) {
    RestCore *c = resting_with_keys_enabled();

    rest_core_send_key(c, 'c');
    CHECK(mock_view_last().type == EV_WORK_BEGIN, "c should enter work mode");

    mock_view_reset();
    mock_advance((long long)WORK_SECONDS * 1000);
    CHECK(mock_view_last().type == EV_REST_BEGIN && mock_view_last().value == BREAK_SECONDS,
          "after a full work stretch, expected rest_begin(%d)", BREAK_SECONDS);

    rest_core_free(c);
}

// 'r' postpones work by POSTPONE_SECONDS -- shorter than a full stretch, so it is distinguishable.
static void test_key_r_postpones_work(void) {
    RestCore *c = resting_with_keys_enabled();

    rest_core_send_key(c, 'r');
    CHECK(mock_view_last().type == EV_WORK_BEGIN, "r should enter work mode");

    mock_view_reset();
    mock_advance((long long)(POSTPONE_SECONDS - 1) * 1000);
    CHECK(mock_view_count_type(EV_REST_BEGIN) == 0,
          "must still be working just before the postpone elapses");

    mock_advance(1000);
    CHECK(mock_view_last().type == EV_REST_BEGIN && mock_view_last().value == BREAK_SECONDS,
          "after POSTPONE_SECONDS, expected rest_begin(%d)", BREAK_SECONDS);

    rest_core_free(c);
}

// 'l' jams the countdown to a large value; the existing 1s timer keeps ticking down from it.
static void test_key_l_sets_large_countdown(void) {
    RestCore *c = resting_with_keys_enabled();

    rest_core_send_key(c, 'l');
    CHECK(mock_view_last().type == EV_TICK && mock_view_last().value == POSTPONE_LONG_SECONDS,
          "l should tick %d, got type=%d value=%d",
          POSTPONE_LONG_SECONDS, mock_view_last().type, mock_view_last().value);

    mock_view_reset();
    mock_advance(1000);
    CHECK(mock_view_last().type == EV_TICK && mock_view_last().value == POSTPONE_LONG_SECONDS - 1,
          "countdown should continue from the large value (%d), got %d",
          POSTPONE_LONG_SECONDS - 1, mock_view_last().value);

    rest_core_free(c);
}

// A session unlock restarts the cycle from the initial rest (the windows lock-screen hook path).
static void test_unlock_reenters_initial_rest(void) {
    RestCore *c = fresh();
    rest_core_start(c);
    mock_advance((long long)DELAY_SECONDS * 1000);
    mock_view_reset();

    mock_unlock();
    CHECK(mock_view_last().type == EV_REST_BEGIN && mock_view_last().value == INIT_SECONDS,
          "unlock should re-enter rest(%d), got type=%d value=%d",
          INIT_SECONDS, mock_view_last().type, mock_view_last().value);

    rest_core_free(c);
}

// Freeing a running core leaves no timers armed (stop_timers + plat_shutdown covered everything).
static void test_free_releases_all_timers(void) {
    RestCore *c = fresh();
    rest_core_start(c);
    CHECK(mock_timer_count() > 0, "a running rest should have timers armed");
    rest_core_free(c);
    CHECK(mock_timer_count() == 0, "free must release every timer, %d left", mock_timer_count());
}

// =========================================================
// Runner
// =========================================================
int main(void) {
    struct { const char *name; void (*fn)(void); } tests[] = {
        { "startup_enters_initial_rest",     test_startup_enters_initial_rest },
        { "countdown_ticks_then_enters_work", test_countdown_ticks_then_enters_work },
        { "work_stretch_returns_to_break",   test_work_stretch_returns_to_break },
        { "keys_ignored_during_initial_delay", test_keys_ignored_during_initial_delay },
        { "key_b_resets_countdown",          test_key_b_resets_countdown },
        { "key_c_continue_working",          test_key_c_continue_working },
        { "key_r_postpones_work",            test_key_r_postpones_work },
        { "key_l_sets_large_countdown",      test_key_l_sets_large_countdown },
        { "unlock_reenters_initial_rest",    test_unlock_reenters_initial_rest },
        { "free_releases_all_timers",        test_free_releases_all_timers },
    };
    int n = (int)(sizeof(tests) / sizeof(tests[0]));
    int i;

    for (i = 0; i < n; i++) {
        int before = g_fails;
        tests[i].fn();
        printf("%s %s\n", g_fails == before ? "ok  " : "FAIL", tests[i].name);
    }

    printf("\n%d checks, %d failed across %d tests\n", g_checks, g_fails, n);
    return g_fails ? 1 : 0;
}
