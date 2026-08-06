# pi-urquan

**The Ur-Quan Masters running directly on a Raspberry Pi with no operating
system.** The board powers on and the game is what boots: no Linux, no
desktop, no launcher, and nothing else running beside it.

It builds for the Raspberry Pi 3, Pi 4 and Pi 5, all three from one source
tree.

## What this is

[The Ur-Quan Masters](https://sc2.sourceforge.net) is the free software
release of Star Control II. Toys for Bob released the 3DO port's source and
content in 2002, and the project has maintained and ported it ever since.
This repository is the thin layer that lets it run with nothing underneath: a
[Circle](https://github.com/rsta2/circle) kernel that brings the board up, and
[circle-libsdl2](https://github.com/Xalior/circle-libsdl2), an SDL2
implementation built on Circle's bare-metal drivers.

The game's own source is not copied or modified here. It is a submodule,
pinned at an upstream commit, and the build reads it without ever writing to
it. The same is true of the four libraries the game needs — zlib, libpng,
libogg and libvorbis — which are submodules pinned at upstream release tags
and compiled into the kernel image alongside the game.

Where the game needs something SDL2 does not cover, this repository supplies
it in `host/`. Where it needs something SDL2 *does* cover, that goes into
circle-libsdl2 instead, where every other game reaches it — there is no
private copy of any part of SDL in this repository.

Three processor cores are given separate work:

- **Core 0** owns the hardware. Circle's world lives here — interrupts, USB,
  the SD card, sound — and no other core touches a device.
- **Core 1** runs the game and nothing else. The game is internally
  multi-threaded and creates its threads through SDL, so all of them live
  here.
- **Core 2** puts finished frames on the screen. The game draws at 320x240
  and never learns the display's size; the picture is scaled once, at the
  end, onto whatever the screen is really showing.

## State of this port

**The whole game compiles and links for all three boards.** It has never
been run on hardware, and no frame has ever been rendered. What follows
describes what the code does, not what has been observed.

Everything the game asks of SDL2 is answered by circle-libsdl2: the software
surface layer it composes its screens with — blits, colour keys, per-surface
alpha, 8-bit paletted sources — the renderer, its textures and its logical
size, threads, mutexes, condition variables and semaphores, audio, keyboard,
mouse and game controllers. There is no reimplementation of any of it here.

It boots on a Raspberry Pi 5 and stops before drawing anything. Bring-up is
healthy — the card mounts, all four cores start, the display is declared and
the core split arms — and then the game produces no output at all.

The game parses its command line before it initialises logging, because the
log file's name is one of the options, so that window is silent by upstream's
own design and a stop inside it looks identical to a game that never started.
`host/boottrace.cpp` exists to tell those apart. It is off unless the image's
defaults block carries `--rapi-trace-boot`.

**Backlog: `host/boottrace.cpp`, the `--rapi-trace-boot` switch and the
`WRAPPED_TRACE` linker flags are instrumentation and are to be removed once
the start-up fault is found.**

The game's own configuration for this build:

| Choice | Setting |
|---|---|
| Graphics | The software renderer, uploading finished 320x240 frames to a streaming texture. |
| Sound | The game's own mixer over SDL audio. Not OpenAL, which does not exist here. |
| Ogg decoding | libvorbis. |
| Module music | The MikMod the game vendors. |
| Content packages | Read as ZIP archives, through zlib. |
| Threads | SDL threads. |
| Netplay | Off. There is no network stack under this. |

## What you need to supply

**This repository contains no game data**, and building the images does not
download any. `make media` is a separate step that fetches it.

The data *is* freely redistributable — that is the whole point of the
project — so unlike most ports, this one can fetch everything it needs:

```sh
make media
```

That downloads three files from The Ur-Quan Masters project's own SourceForge
releases, verifies each against a SHA256 and against the ZIP magic, and writes
a `provenance.txt` beside them:

| File | Size | What it is |
|---|---|---|
| `uqm-0.8.0-content.uqm` | 11 MB | The base game content. **Required** — the game cannot start without it. |
| `uqm-0.8.0-voice.uqm` | 115 MB | The 3DO port's voice acting. Optional. |
| `uqm-0.8.0-3domusic.uqm` | 19 MB | The 3DO port's remastered soundtrack. Optional. |

A `.uqm` package is a ZIP archive under a different extension. The game reads
it as one, so it is never unpacked.

The game code inside these packages is GNU General Public License version 2
or later; the resource content is Creative Commons BY-NC-SA 2.5. Both are the
project's own terms, published at <https://sc2.sourceforge.net>.

Re-running `make media` verifies what is already there rather than
downloading it again.

## Building

You need a Linux or macOS machine, GNU make, and the Arm GNU toolchain for
`aarch64-none-elf` (release 15.2.Rel1). Put its `bin` directory on your
`PATH`, or unpack it into `toolchains/` in this repository.

```sh
git clone --recursive https://github.com/Xalior/pi-urquan.git
cd pi-urquan
make deps       # long: builds newlib and libc++ from source, once per board
make kernels    # the three board images
make verify     # confirms each image exists and is not empty
```

`make deps` is the slow step, and it is slow once. It builds a complete C and
C++ world for each board, because each board's world is compiled for its own
processor.

Part of that world is libc++, whose sources are fetched from a git tag that
carries the bare-metal patches. One copy is enough for every board and for
every project on your machine, so tell the build where to keep it and it is
fetched once:

```sh
make deps CIRCLE_LLVM=/path/to/circle-llvm
```

The default puts that checkout beside this repository, which is the right
answer for a plain clone or a continuous-integration runner and needs no
setting at all.

The images land in `host/build/<board>/`:

| Board | Image |
|---|---|
| Pi 3 | `host/build/rpi3/kernel8.img` |
| Pi 4 | `host/build/rpi4/kernel8-rpi4.img` |
| Pi 5 | `host/build/rpi5/kernel_2712.img` |

Building one board on its own is `make rpi5`, and its dependencies alone are
`make deps-rpi5`, which is what a machine without room for three worlds
wants.

## Putting it on a card

```sh
make card
```

That stages the card into `build/sd-card/` for you to copy onto FAT32 media:
the three kernel images under the names each board's firmware looks for, the
boot configuration, and the game's own directory under `games/urquan/`.

`make card` downloads nothing. It copies in whatever `make media` left in
`media/` and names anything that is missing, so a card built without the data
step is a legitimate build that says exactly what it lacks.

The card layout is:

```
kernel8.img  kernel8-rpi4.img  kernel_2712.img
config.txt  cmdline.txt
games/urquan/content/version
games/urquan/content/packages/uqm-0.8.0-content.uqm
games/urquan/content/addons/uqm-0.8.0-voice.uqm
games/urquan/content/addons/uqm-0.8.0-3domusic.uqm
games/urquan/config/
```

`content/version` is the marker the engine uses to recognise a content
directory at all. It is the one file the download does not contain, and
`make card` takes it from the game's own source tree.

One thing is not staged and has to be added by hand: **the Raspberry Pi
firmware files** — `bootcode.bin`, `start*.elf`, `fixup*.dat` and, for the
Pi 4, `armstub8-rpi4.bin`. Take them from a Raspberry Pi OS card or from the
[firmware repository](https://github.com/raspberrypi/firmware).

Everything this game reads or writes stays inside `games/urquan/`. A card
carries several games, and two of them writing settings into the card's root
would each silently overwrite the other's.

### The thermal settings in `cmdline.txt`

One card boots any of the three boards, so all three read the same
`cmdline.txt`. It carries `socmaxtemp=70`, the temperature in degrees Celsius
at which the processor is slowed down to cool itself.

If your board has a fan, add `gpiofanpin=` and the GPIO pin it is wired to —
`gpiofanpin=45` is a Raspberry Pi 5 Case Fan or Active Cooler. Naming a fan
pin changes what happens at that temperature: the fan is switched on and the
processor is left at full speed, instead of being slowed down. That is what a
game wants, because a slowed processor drops frames.

### Boot options

`cmdline.txt` also accepts switches this kernel reads:

| Option | Effect |
|---|---|
| `rapi-split=0` | Run everything on core 0 instead of splitting the work across three. Slower, and useful for comparing the two against one image. |
| `rapi-perf=N` | Print a performance line to the serial console every N seconds. |
| `rapi-debug-uart` | Accept key presses from the serial console, so a board with no keyboard attached can still be driven. |

The same switches, and the game's own, can also be stamped into a built image
without rebuilding it: each image carries a patchable defaults block at offset
`0x800`, and anything written there is appended to the game's command line at
boot. The build refuses to produce an image whose block is not present.

## How the layers fit

`host/` holds everything this repository adds, and nothing else:

| File | What it is |
|---|---|
| `kernel.cpp`, `kernel.h`, `main.cpp` | The Circle kernel: brings up the serial console, the SD card and the filesystem, elects the three cores, and calls the game. |
| `circle_syscalls.cpp` | Puts the SD card underneath the C library in a way that is legal from a core that does not own the hardware. |
| `circle_stubs.cpp` | Three C-library functions newlib's Circle port does not carry: `access`, `getuid` and `getpwuid`. |
| `defaults.cpp`, `defaults.h`, `defaultsblock.h`, `uqm-defaults.ld` | The patchable defaults block at image offset `0x800`, and this kernel's use of it. |
| `uqmconf/config_unix.h` | The build configuration the game's own build system would otherwise generate by probing a desktop machine. |
| `pngconf/pnglibconf.h`, `oggconf/ogg/config_types.h` | The same, for the two libraries that also expect a configure step. |
| `config.txt`, `cmdline.txt` | Firmware boot configuration, one file for all three boards. |

The game's entry point is renamed by the preprocessor for one file, so that
`main` belongs to the Circle kernel and the game is a function it calls. That,
and the linker's `--wrap` on the filesystem calls, is the whole of the
intrusion into upstream: no patch, no fork, no edit.

## License

The code in this repository — the kernel layer in `host/` and the build — is
released under the GNU Lesser General Public License, version 3. See
[LICENSE](LICENSE).

The submodules are other people's work and carry their own terms, and all of
them matter before you distribute anything you build here:

- **The Ur-Quan Masters** is released under the GNU General Public License,
  version 2 or later.
- **Circle** is released under the GNU General Public License, version 3.
- **zlib** and **libpng** carry their own permissive licences; **libogg** and
  **libvorbis** carry the Xiph.Org BSD-style licence.

Building a kernel image here combines all of them, and the result is covered
by the GNU General Public License, version 3. Doing that for yourself is
straightforward; redistributing the result means satisfying every one of
those terms at once, including supplying complete source.

Star Control is a trademark of its respective owner. This project is not
affiliated with Toys for Bob, Accolade or Stardock.
