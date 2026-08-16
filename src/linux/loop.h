#ifndef LOOP_H
#define LOOP_H

// fd watching, linux-internal.
//
// Deliberately NOT in platform.h: file descriptors are a POSIX concept and the windows
// keyboard listener uses a thread instead, so this would be an unimplementable declaration
// there. It is the contract between src/linux/keyboard.c and whichever main loop backend
// (platform-glib.c or platform-poll.c) is linked in.

// Return 1 to keep watching, 0 to stop. Called when the fd is readable, or on hangup/error
// (in which case read() returns <= 0 and the watcher should return 0).
typedef int (*LoopFdFunc)(void *user);

void loop_watch_fd(int fd, LoopFdFunc fn, void *user);

#endif // LOOP_H
