Have a 2-minute break every 45 minutes at work!

![Screenshot](images/screenshot.png)


- Press r: Continue to work(delay to rest) for 2 minutes
- Press q: Kill process.
- Press c: Continue to work(reset rest)
- Press l: Set the countdown to a very large value (postpone rest for a long time)
- Press b: Start a break countdown now (reset it if already counting down)
- After unlocking(resume from sleep、lock) all timer will reset. (windows only — there is no
  lock-screen integration on linux.)

Keys are case-insensitive, and are ignored for a short delay right after a countdown starts (to avoid mis-touch).

## Build

```sh
make                     # linux, gui view (default)
make PLATFORM=windows    # cross-compile for windows -> rest.exe
make VIEW=cli            # linux with the cli view
make VIEW=sdl3           # linux with the SDL3 view
make clean               # remove the current triple+view only
make distclean           # remove build/ entirely
```

Output goes to `build/<target triple>/<view>/`, the triple being whatever
`$(CC) -dumpmachine` reports:

```
build/x86_64-linux-gnu/gui/rest
build/x86_64-linux-gnu/sdl3/rest
build/x86_64-w64-mingw32/gui/rest.exe
```

So glibc / musl / mingw / other architectures never share objects, and neither do two
views — switching `VIEW` can not leave you with the previous view's binary.

- `PLATFORM`: `linux` (default) / `windows` — selects which `src/<platform>/` sources are
  compiled. It is a source selector, not a target name: a musl or arm64 build is still
  `PLATFORM=linux`, and only the triple in the output path changes.
- `VIEW`: `gui` (default) / `cli` / `sdl3`
  - `gui`: GTK4 on linux, Win32 on windows — the only view with a per-platform implementation
  - `cli`: countdown printed to the terminal, no window. Plain stdio, one shared implementation
  - `sdl3`: SDL3 (>= 3.2), one implementation shared by both platforms. Needs only SDL3
    itself — the digits use SDL's built-in debug font, so there is no SDL_ttf dependency
    and no font file to ship.

You can also build a platform directly under its dir, e.g. `cd src/linux && make` — it
writes to the same top-level `build/` as the commands above, not to a second tree.
