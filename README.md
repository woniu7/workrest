Have a 2-minute break every 45 minutes at work!

![Screenshot](images/screenshot.png)


- Press r: Continue to work(delay to rest) for 2 minutes
- Press q: Kill process.
- Press c: Continue to work(reset rest)
- Press l: Set the countdown to a very large value (postpone rest for a long time)
- Press b: Start a break countdown now (reset it if already counting down)
- After unlocking(resume from sleep、lock) all timer will reset.

Keys are case-insensitive, and are ignored for a short delay right after a countdown starts (to avoid mis-touch).

## Build

```sh
make                     # linux, gui view (default) -> build/linux/
make PLATFORM=windows    # cross-compile for windows -> rest.exe
make VIEW=cli            # linux with the cli view
make clean
```

- `PLATFORM`: `linux` (default) / `windows`
- `VIEW`: `gui` (default) / `cli`

You can also build a platform directly under its dir, e.g. `cd src/linux && make`.
