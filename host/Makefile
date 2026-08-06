#
# The Ur-Quan Masters as a bootable Raspberry Pi kernel image.
#
# One board per invocation (RAPI_BOARD, default rpi5). The board selects
# three things at once: the circle-stdlib world the kernel is compiled and
# linked against, the shim archive built against that world, and — through
# the world's Config.mk — the CPU tuning and RASPPI value. Each board's
# newlib and libc++ are built for its own architecture, so an image is
# never portable between boards.
#
#   make                     the default board's image
#   make RAPI_BOARD=rpi3     that board's image
#   make clean-board         drop just this board's build/<board>/ tree
#
# Everything this invocation generates lands under build/<board>/: the
# kernel's own objects, every object from the game and from the four
# libraries it needs, and the final .elf/.map/.lst/.img. The object names
# are shared across boards but their contents are not (different -mcpu,
# different -DRASPPI, different world), so a flat build directory would
# silently relink one board's objects into another board's kernel — a
# corrupt image with no build error. A directory per board keeps all three
# isolated: no `make clean` between boards, and three boards can build at
# the same time without touching each other.
#
# None of the five upstream trees (../uqm/sc2, ../zlib, ../libpng, ../libogg,
# ../libvorbis) is ever written into: each is read-only input, and every
# object it produces is redirected here.
#
# Prerequisite: the board's world and shim archive exist —
# `make -C ../circle-libsdl2 deps`, or `make deps` at the repository root.
#

include ../mk/toolchain.mk

RAPI_BOARD ?= rpi5

include ../circle-libsdl2/circle-stdlib-$(RAPI_BOARD)/Config.mk

ROOT     = ..
SHIM     = $(ROOT)/circle-libsdl2
# The game's own tree sits one level down inside its repository: the
# upstream project keeps the game in sc2/ and its packaging tools beside it.
UQM      = $(ROOT)/uqm/sc2
ZLIB     = $(ROOT)/zlib
LIBPNG   = $(ROOT)/libpng
LIBOGG   = $(ROOT)/libogg
LIBVORBIS = $(ROOT)/libvorbis

OBJDIR = build/$(RAPI_BOARD)

# Circle's own image names, which the Raspberry Pi firmware selects between
# from the model sections of config.txt: the Pi 3 world boots kernel8.img,
# the Pi 4 world kernel8-rpi4.img, the Pi 5 world kernel_2712.img.
IMAGE_rpi3 = kernel8
IMAGE_rpi4 = kernel8-rpi4
IMAGE_rpi5 = kernel_2712
IMAGE = $(IMAGE_$(RAPI_BOARD))
ifeq ($(IMAGE),)
$(error unknown RAPI_BOARD '$(RAPI_BOARD)' — expected rpi3, rpi4 or rpi5)
endif

TARGET = $(OBJDIR)/$(IMAGE)

# Stated explicitly because the first rule this file sees comes from an
# included makefile, and that would otherwise decide the default goal.
.DEFAULT_GOAL := image

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

# This port's own code: the kernel wrapper, the syscall layer, and the POSIX
# calls newlib does not carry. No SDL — every SDL entry point the game uses
# comes from circle-libsdl2.
HOST_SRCS = main.cpp kernel.cpp defaults.cpp circle_syscalls.cpp \
	circle_stubs.cpp
HOST_OBJS = $(addprefix $(OBJDIR)/,$(HOST_SRCS:.cpp=.o))

# The Ur-Quan Masters' own source list.
#
# Upstream has no CMake and no configure script: it carries a shell-based
# build system whose file lists live in a `Makeinfo` fragment beside every
# directory, each one a shell script that appends to uqm_CFILES and picks
# subdirectories from the configuration. This list is that traversal
# performed once, with the choices this port makes:
#
#   graphics module   sdl        the software renderer, uploading finished
#                                320x240 frames to a streaming texture
#   sound module      mixsdl     upstream's own mixer over SDL audio, not
#                                OpenAL, which does not exist here
#   ogg decoder       libvorbis  not tremor, not none
#   mikmod            internal   upstream vendors it; there is no system one
#   zip I/O           on         the content packages are zip archives
#   thread library    SDL        see circle_threads.cpp for what answers it
#   netplay           off        there is no network stack under this
#   regex             internal   newlib has the header but not the code
#   getopt_long       system     newlib has it
#
# Files that describe a machine this is not are absent for that reason, and
# the reason is upstream's own configuration rather than a judgement made
# here: the OpenAL driver, the pthread thread backend, the Darwin and
# Symbian directories, the network transport.
#
# Files that ARE here and do nothing are left in on purpose, because
# upstream guards them itself: sdl1_common.c and pure.c compile away behind
# an SDL-major-version test, opengl.c behind HAVE_OPENGL, and the MMX, SSE
# and 3DNow scalers behind processor tests no AArch64 build passes. Each
# produces an empty object and says in its own source why.
UQM_SRCS =
UQM_SRCS += $(addprefix src/, \
	options.c port.c uqm.c)
UQM_SRCS += $(addprefix src/libs/callback/, \
	alarm.c async.c callback.c)
UQM_SRCS += $(addprefix src/libs/decomp/, \
	lzdecode.c lzencode.c update.c)
UQM_SRCS += $(addprefix src/libs/file/, \
	dirs.c files.c)
UQM_SRCS += $(addprefix src/libs/graphics/, \
	bbox.c boxint.c clipline.c cmap.c context.c dcqueue.c drawable.c \
	filegfx.c font.c frame.c gfx_common.c gfxload.c intersec.c \
	loaddisp.c pixmap.c resgfx.c tfb_draw.c tfb_prim.c widgets.c)
UQM_SRCS += $(addprefix src/libs/graphics/sdl/, \
	2xscalers_3dnow.c 2xscalers_mmx.c 2xscalers_sse.c 2xscalers.c \
	biadv2x.c bilinear2x.c canvas.c hq2x.c nearest2x.c opengl.c \
	palette.c png2sdl.c primitives.c pure.c rotozoom.c scalers.c \
	sdl_common.c sdl1_common.c sdl2_common.c sdl2_pure.c sdluio.c \
	triscan2x.c)
UQM_SRCS += $(addprefix src/libs/heap/, \
	heap.c)
UQM_SRCS += $(addprefix src/libs/input/, \
	input_common.c)
UQM_SRCS += $(addprefix src/libs/input/sdl/, \
	input.c keynames.c vcontrol.c)
UQM_SRCS += $(addprefix src/libs/list/, \
	list.c)
UQM_SRCS += $(addprefix src/libs/log/, \
	msgbox_stub.c uqmlog.c)
UQM_SRCS += $(addprefix src/libs/math/, \
	random.c random2.c sqrt.c)
UQM_SRCS += $(addprefix src/libs/memory/, \
	w_memlib.c)
UQM_SRCS += $(addprefix src/libs/mikmod/, \
	drv_nos.c load_it.c load_mod.c load_s3m.c load_stm.c load_xm.c \
	mdreg.c mdriver.c mdulaw.c mloader.c mlreg.c mlutil.c mmalloc.c \
	mmerror.c mmio.c mplayer.c munitrk.c mwav.c npertab.c sloader.c \
	virtch_common.c virtch.c virtch2.c)
UQM_SRCS += $(addprefix src/libs/resource/, \
	direct.c filecntl.c getres.c loadres.c propfile.c resinit.c \
	stringbank.c)
UQM_SRCS += $(addprefix src/libs/sound/, \
	audiocore.c fileinst.c music.c resinst.c sfx.c sound.c stream.c \
	trackplayer.c)
UQM_SRCS += $(addprefix src/libs/sound/decoders/, \
	aiffaud.c decoder.c dukaud.c modaud.c oggaud.c wav.c)
UQM_SRCS += $(addprefix src/libs/sound/mixer/, \
	mixer.c)
UQM_SRCS += $(addprefix src/libs/sound/mixer/nosound/, \
	audiodrv_nosound.c)
UQM_SRCS += $(addprefix src/libs/sound/mixer/sdl/, \
	audiodrv_sdl.c)
UQM_SRCS += $(addprefix src/libs/strings/, \
	getstr.c sfileins.c sresins.c stringhashtable.c strings.c \
	unicode.c)
UQM_SRCS += $(addprefix src/libs/task/, \
	tasklib.c)
UQM_SRCS += $(addprefix src/libs/threads/, \
	thrcommon.c)
UQM_SRCS += $(addprefix src/libs/threads/sdl/, \
	sdlthreads.c)
UQM_SRCS += $(addprefix src/libs/time/, \
	timecommon.c)
UQM_SRCS += $(addprefix src/libs/time/sdl/, \
	sdltime.c)
UQM_SRCS += $(addprefix src/libs/uio/, \
	charhashtable.c debug.c defaultfs.c fileblock.c fstypes.c gphys.c \
	io.c ioaux.c match.c mount.c mounttree.c paths.c physical.c \
	uiostream.c uioutils.c utils.c)
UQM_SRCS += $(addprefix src/libs/uio/stdio/, \
	stdio.c)
UQM_SRCS += $(addprefix src/libs/uio/zip/, \
	zip.c)
UQM_SRCS += $(addprefix src/libs/video/, \
	dukvid.c legacyplayer.c vfileins.c video.c videodec.c vidplayer.c \
	vresins.c)
UQM_SRCS += $(addprefix src/regex/, \
	regex.c)
UQM_SRCS += $(addprefix src/uqm/, \
	battle.c battlecontrols.c border.c build.c cleanup.c clock.c \
	cnctdlg.c collide.c comm.c commanim.c commglue.c confirm.c \
	cons_res.c credits.c cyborg.c demo.c displist.c dummy.c encount.c \
	flash.c fmv.c galaxy.c gameev.c gameinp.c gameopt.c gendef.c \
	getchar.c globdata.c gravity.c grpinfo.c hyper.c init.c intel.c \
	intro.c ipdisp.c load_legacy.c load.c loadship.c master.c menu.c \
	misc.c oscill.c outfit.c pickship.c plandata.c process.c restart.c \
	save.c settings.c setup.c setupmenu.c ship.c shipstat.c shipyard.c \
	sis.c sounds.c starbase.c starcon.c starmap.c state.c status.c \
	tactrans.c trans.c uqmdebug.c util.c velocity.c weapon.c)
UQM_SRCS += $(addprefix src/uqm/comm/arilou/, \
	arilouc.c)
UQM_SRCS += $(addprefix src/uqm/comm/blackur/, \
	blackurc.c)
UQM_SRCS += $(addprefix src/uqm/comm/chmmr/, \
	chmmrc.c)
UQM_SRCS += $(addprefix src/uqm/comm/comandr/, \
	comandr.c)
UQM_SRCS += $(addprefix src/uqm/comm/druuge/, \
	druugec.c)
UQM_SRCS += $(addprefix src/uqm/comm/ilwrath/, \
	ilwrathc.c)
UQM_SRCS += $(addprefix src/uqm/comm/melnorm/, \
	melnorm.c)
UQM_SRCS += $(addprefix src/uqm/comm/mycon/, \
	myconc.c)
UQM_SRCS += $(addprefix src/uqm/comm/orz/, \
	orzc.c)
UQM_SRCS += $(addprefix src/uqm/comm/pkunk/, \
	pkunkc.c)
UQM_SRCS += $(addprefix src/uqm/comm/rebel/, \
	rebel.c)
UQM_SRCS += $(addprefix src/uqm/comm/shofixt/, \
	shofixt.c)
UQM_SRCS += $(addprefix src/uqm/comm/slyhome/, \
	slyhome.c)
UQM_SRCS += $(addprefix src/uqm/comm/slyland/, \
	slyland.c)
UQM_SRCS += $(addprefix src/uqm/comm/spahome/, \
	spahome.c)
UQM_SRCS += $(addprefix src/uqm/comm/spathi/, \
	spathic.c)
UQM_SRCS += $(addprefix src/uqm/comm/starbas/, \
	starbas.c)
UQM_SRCS += $(addprefix src/uqm/comm/supox/, \
	supoxc.c)
UQM_SRCS += $(addprefix src/uqm/comm/syreen/, \
	syreenc.c)
UQM_SRCS += $(addprefix src/uqm/comm/talkpet/, \
	talkpet.c)
UQM_SRCS += $(addprefix src/uqm/comm/thradd/, \
	thraddc.c)
UQM_SRCS += $(addprefix src/uqm/comm/umgah/, \
	umgahc.c)
UQM_SRCS += $(addprefix src/uqm/comm/urquan/, \
	urquanc.c)
UQM_SRCS += $(addprefix src/uqm/comm/utwig/, \
	utwigc.c)
UQM_SRCS += $(addprefix src/uqm/comm/vux/, \
	vuxc.c)
UQM_SRCS += $(addprefix src/uqm/comm/yehat/, \
	yehatc.c)
UQM_SRCS += $(addprefix src/uqm/comm/zoqfot/, \
	zoqfotc.c)
UQM_SRCS += $(addprefix src/uqm/planets/, \
	calc.c cargo.c devices.c gentopo.c lander.c orbits.c oval.c \
	pl_stuff.c planets.c plangen.c pstarmap.c report.c roster.c scan.c \
	solarsys.c surface.c)
UQM_SRCS += $(addprefix src/uqm/planets/generate/, \
	genand.c genburv.c genchmmr.c gencol.c gendefault.c gendru.c \
	genilw.c genmel.c genmyc.c genorz.c genpet.c genpku.c genrain.c \
	gensam.c genshof.c gensly.c gensol.c genspa.c gensup.c gensyr.c \
	genthrad.c gentrap.c genutw.c genvault.c genvux.c genwreck.c \
	genyeh.c genzfpscout.c genzoq.c)
UQM_SRCS += $(addprefix src/uqm/ships/androsyn/, \
	androsyn.c)
UQM_SRCS += $(addprefix src/uqm/ships/arilou/, \
	arilou.c)
UQM_SRCS += $(addprefix src/uqm/ships/blackurq/, \
	blackurq.c)
UQM_SRCS += $(addprefix src/uqm/ships/chenjesu/, \
	chenjesu.c)
UQM_SRCS += $(addprefix src/uqm/ships/chmmr/, \
	chmmr.c)
UQM_SRCS += $(addprefix src/uqm/ships/druuge/, \
	druuge.c)
UQM_SRCS += $(addprefix src/uqm/ships/human/, \
	human.c)
UQM_SRCS += $(addprefix src/uqm/ships/ilwrath/, \
	ilwrath.c)
UQM_SRCS += $(addprefix src/uqm/ships/lastbat/, \
	lastbat.c)
UQM_SRCS += $(addprefix src/uqm/ships/melnorme/, \
	melnorme.c)
UQM_SRCS += $(addprefix src/uqm/ships/mmrnmhrm/, \
	mmrnmhrm.c)
UQM_SRCS += $(addprefix src/uqm/ships/mycon/, \
	mycon.c)
UQM_SRCS += $(addprefix src/uqm/ships/orz/, \
	orz.c)
UQM_SRCS += $(addprefix src/uqm/ships/pkunk/, \
	pkunk.c)
UQM_SRCS += $(addprefix src/uqm/ships/probe/, \
	probe.c)
UQM_SRCS += $(addprefix src/uqm/ships/shofixti/, \
	shofixti.c)
UQM_SRCS += $(addprefix src/uqm/ships/sis_ship/, \
	sis_ship.c)
UQM_SRCS += $(addprefix src/uqm/ships/slylandr/, \
	slylandr.c)
UQM_SRCS += $(addprefix src/uqm/ships/spathi/, \
	spathi.c)
UQM_SRCS += $(addprefix src/uqm/ships/supox/, \
	supox.c)
UQM_SRCS += $(addprefix src/uqm/ships/syreen/, \
	syreen.c)
UQM_SRCS += $(addprefix src/uqm/ships/thradd/, \
	thradd.c)
UQM_SRCS += $(addprefix src/uqm/ships/umgah/, \
	umgah.c)
UQM_SRCS += $(addprefix src/uqm/ships/urquan/, \
	urquan.c)
UQM_SRCS += $(addprefix src/uqm/ships/utwig/, \
	utwig.c)
UQM_SRCS += $(addprefix src/uqm/ships/vux/, \
	vux.c)
UQM_SRCS += $(addprefix src/uqm/ships/yehat/, \
	yehat.c)
UQM_SRCS += $(addprefix src/uqm/ships/zoqfot/, \
	zoqfot.c)
UQM_SRCS += $(addprefix src/uqm/supermelee/, \
	buildpick.c loadmele.c melee.c meleesetup.c pickmele.c)

UQM_OBJS = $(addprefix $(OBJDIR)/uqm/,$(UQM_SRCS:.c=.o))

# ---------------------------------------------------------------------------
# The four libraries the game genuinely needs
# ---------------------------------------------------------------------------
#
# These are not conveniences and they are not substitutable. The game's
# content packages ARE zip archives, every picture inside them is a PNG, and
# the 3DO music and speech are Ogg Vorbis. Nothing in circle-stdlib or in
# circle-libsdl2 provides any of it, so each library is a submodule of this
# repository and is compiled into the kernel image alongside the game. All
# four are plain portable C that ask nothing of an operating system beyond
# malloc and the standard library, which is why this is possible at all.
#
# Each is pinned at an upstream release tag, never a branch.

# zlib, entire. The zip reader needs inflate and libpng needs both halves;
# the gzip file interface is built too rather than curated out, so that a
# later use of it is a link and not a puzzle.
ZLIB_SRCS = adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c \
	gzread.c gzwrite.c infback.c inffast.c inflate.c inftrees.c \
	trees.c uncompr.c zutil.c
ZLIB_OBJS = $(addprefix $(OBJDIR)/zlib/,$(ZLIB_SRCS:.c=.o))

# libpng, the library proper plus its AArch64 row filters. example.c and
# pngtest.c are programs, not library sources. The NEON files guard
# themselves on the processor and compile to nothing where it is not there,
# so naming them costs nothing on a board that cannot use them — and every
# board here can.
PNG_SRCS = png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c \
	pngrio.c pngrtran.c pngrutil.c pngset.c pngtrans.c pngwio.c \
	pngwrite.c pngwtran.c pngwutil.c \
	arm/arm_init.c arm/filter_neon_intrinsics.c \
	arm/palette_neon_intrinsics.c
PNG_OBJS = $(addprefix $(OBJDIR)/libpng/,$(PNG_SRCS:.c=.o))

# libogg: the bitstream framing, and nothing else exists in it.
OGG_SRCS = src/bitwise.c src/framing.c
OGG_OBJS = $(addprefix $(OBJDIR)/libogg/,$(OGG_SRCS:.c=.o))

# libvorbis: the decoder and the vorbisfile convenience layer the game's ogg
# decoder is written against. The encoder (vorbisenc.c) and the analysis and
# psychoacoustic half it drives are left out — this board plays Ogg Vorbis
# and never writes it — as are the barkmel, psytune and tone test programs.
VORBIS_SRCS = $(addprefix lib/, \
	bitrate.c block.c codebook.c envelope.c floor0.c floor1.c info.c \
	lookup.c lpc.c lsp.c mapping0.c mdct.c psy.c registry.c res0.c \
	sharedbook.c smallft.c synthesis.c window.c vorbisfile.c)
VORBIS_OBJS = $(addprefix $(OBJDIR)/libvorbis/,$(VORBIS_SRCS:.c=.o))

OBJS = $(HOST_OBJS) $(UQM_OBJS) $(ZLIB_OBJS) $(PNG_OBJS) $(OGG_OBJS) \
	$(VORBIS_OBJS)

STANDARD = -std=gnu++17

# Stated, not inherited: this is Circle's own default, written down so the
# build's optimization level is a fact in the file rather than a forensic
# reconstruction.
OPTIMIZE = -O2

include $(CIRCLEHOME)/Rules.mk

# The world must be the multicore one circle-libsdl2 configures: the shim's
# core-split machinery cannot exist without it, and an object compiled
# against a single-core world links without complaint and then misbehaves.
# Every object here inherits the flag through Rules.mk's CPPFLAGS, so this
# check covers the whole build.
ifeq ($(findstring ARM_ALLOW_MULTI_CORE,$(DEFINE)),)
$(error the payload requires the multicore circle-stdlib world — build it with 'make -C ../circle-libsdl2 deps')
endif

# WHERE THIS GAME LIVES ON THE CARD. One card carries several games, and two
# of them writing a settings file into the FAT root would each silently
# overwrite the other's. Every game therefore keeps to a directory of its
# own, and nothing of this port's ever touches the root.
#
# It is a build-time constant, not a boot switch: a card's layout is decided
# when the card is staged, and a game that could be relocated at boot would
# only mean two places to look for the same file.
#
# The path is used ONLY by this port's own host layer — the kernel's
# argument list, the working directory, and the content and config
# directories written into uqmconf/config_unix.h. Upstream is never told
# about it and needs no patch: it looks where it is pointed.
RAPI_GAME_DIR ?= /games/urquan
DEFINE += -DRAPI_GAME_DIR='"$(RAPI_GAME_DIR)"'

# The include path, in the order the answers must be found.
#
# src/regex comes first on purpose. newlib ships a <regex.h> but no regcomp
# or regexec behind it, and the game's addon scan needs both, so upstream's
# own bundled regex — the fallback it carries for exactly this case — is
# compiled in. Its header has to be the one every translation unit sees,
# because a regex_t declared by one header and filled in by another
# implementation is a silent corruption rather than a link error.
#
# uqmconf/ carries config_unix.h, which upstream's build.sh generates while
# probing a desktop machine and which this port writes down instead.
# pngconf/ and oggconf/ do the same for the two libraries that also expect a
# configure step.
#
# The shim's SDL2/ directory goes on the path in its own right alongside the
# SDL2/-prefixed one, because the game spells its includes "SDL.h".
INCLUDE := -I $(UQM)/src/regex \
	-I $(SHIM)/include -I $(SHIM)/include/SDL2 \
	-I $(CURDIR)/uqmconf -I $(CURDIR)/pngconf -I $(CURDIR)/oggconf \
	-I $(UQM)/src -I $(UQM) \
	-I $(ZLIB) -I $(LIBPNG) \
	-I $(LIBOGG)/include -I $(LIBVORBIS)/include -I $(LIBVORBIS)/lib \
	$(CIRCLE_STDLIB_INCLUDES) $(INCLUDE)

# What the game's own build system would have put on the command line, for
# the configuration chosen in the source list above. Each of these selects
# which code exists rather than tuning it.
UQM_DEFINE = -DGFXMODULE_SDL -DSDL_DIR=SDL2 -DUSE_INTERNAL_MIKMOD \
	-DHAVE_JOYSTICK -DHAVE_ZIP=1 -DTHREADLIB_SDL -DNDEBUG

# The C sources are C99 with GNU extensions, which is what upstream's build
# asks for. It is stated here rather than added to CFLAGS because Rules.mk
# folds CFLAGS into the C++ flags as well, and a C standard handed to the
# C++ compiler is a warning on every C++ file in the build. $(STANDARD)
# above covers those.
#
# Three warnings are turned off for the game and the libraries, and only
# there: this is other people's code, compiled exactly as they wrote it, and
# a warning nobody in this repository can act on is noise that hides the
# ones that matter.
THIRDPARTY_QUIET = -Wno-unused-parameter -Wno-sign-compare \
	-Wno-unused-but-set-variable

# The game predates the compiler by twenty years, and GCC 14 turned a
# warning it relies on into an error: a call to a function whose declaration
# was never included. Upstream's own sources do this in a handful of places
# — uqm.c calls Alarm_init without including libs/alarm.h, and gets away
# with it on the compilers it was written for. Demoting it back to a warning
# is what every distribution packaging code of this age does, and it is
# preferable to editing a submodule that must not be edited.
#
# It is scoped to the game and the libraries alone. This port's own code is
# compiled with the error left in place, where it belongs.
THIRDPARTY_OLD_C = -Wno-error=implicit-function-declaration

UQM_CFLAGS    = -std=gnu99 $(UQM_DEFINE) $(THIRDPARTY_QUIET) $(THIRDPARTY_OLD_C) $(CFLAGS)
ZLIB_CFLAGS   = -std=gnu99 $(THIRDPARTY_QUIET) $(THIRDPARTY_OLD_C) $(CFLAGS)
PNG_CFLAGS    = -std=gnu99 $(THIRDPARTY_QUIET) $(THIRDPARTY_OLD_C) $(CFLAGS)
OGG_CFLAGS    = -std=gnu99 $(THIRDPARTY_QUIET) $(THIRDPARTY_OLD_C) $(CFLAGS)
VORBIS_CFLAGS = -std=gnu99 $(THIRDPARTY_QUIET) $(THIRDPARTY_OLD_C) $(CFLAGS)

# The shim comes in whole, every member, rather than only the members the
# link happens to need. An object file linked straight into the kernel beats
# an archive member of the same name and does it silently, so a stub in
# circle_stubs.cpp left behind after the library implemented the real thing
# would quietly win — the game would keep calling the stub and nothing would
# say so. Pulling the archive in whole turns that into a duplicate-symbol
# error at link time, which is the only moment it can still be cheap to fix.
LIBS = --whole-archive $(SHIM)/libSDL2-$(RAPI_BOARD).a --no-whole-archive \
	$(CIRCLE_STDLIB_LIBS)

# ---------------------------------------------------------------------------
# Filesystem syscalls, redirected at link time
# ---------------------------------------------------------------------------
#
# The game runs on the application core, and newlib's Circle glue drives
# FatFs and the SD card straight from wherever it is called — which off core
# 0 is illegal. circle_syscalls.cpp puts the shim's any-core I/O service
# underneath the C library instead; these flags are what put it there.
#
# --wrap rather than redefinition, because redefinition cannot work here:
# the glue defines _open, _read, _write, _close, _lseek, _fstat, opendir,
# readdir, closedir, dup and more in a SINGLE object file. Redefining part
# of that set either collides at link time or, worse, leaves the originals
# linked in beside the replacements with no diagnostic. --wrap renames the
# references instead and leaves the vendored glue untouched — and it is what
# keeps __real_* reachable, which is how the service performs the real work
# once it has reached core 0.
#
# Every name here must also appear in circle_syscalls.cpp: a wrap with no
# __wrap_ definition is an undefined reference, so the link is the check.
WRAPPED_SYSCALLS = \
	_open _close _read _write _lseek _fstat _stat lstat _unlink _rename \
	_fcntl mkdir chdir getcwd dup dup2 \
	opendir readdir closedir rewinddir

# Nothing named SDL_ is wrapped or replaced here. The surface layer the game
# draws through — software blits, colour keys, per-surface alpha, 8-bit
# paletted sources — and the texture formats it asks the renderer for are
# both SDL2's own job, so both live in circle-libsdl2 where every game
# reaches them.
#
# The link is driven by the cross ld directly (Circle's Rules.mk sets
# LD = <prefix>ld for GCC builds), so these are plain linker options, not
# compiler-driver -Wl, ones.
LDFLAGS += $(addprefix --wrap=,$(WRAPPED_SYSCALLS))

# ---------------------------------------------------------------------------
# Compilation
# ---------------------------------------------------------------------------

# This project's own objects, board-scoped. These more-specific rules win
# over Rules.mk's in-place %.o/%.d, so nothing is written beside the source.
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.d: %.cpp | $(OBJDIR)
	@$(CPP) $(CPPFLAGS) -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

# One pair of rules per upstream tree, each mirroring that tree's own
# directory layout under $(OBJDIR)/. The trees themselves are submodules,
# and are never written to.
define UPSTREAM_RULES
$$(OBJDIR)/$(1)/%.o: $(2)/%.c
	@mkdir -p $$(dir $$@)
	@echo "  CC    $$@"
	@$$(CC) $(3) -c -o $$@ $$<

$$(OBJDIR)/$(1)/%.d: $(2)/%.c
	@mkdir -p $$(dir $$@)
	@$$(CC) $(3) -M -MG -MT $$(@:.d=.o) -MT $$@ -MF $$@ $$<
endef

$(eval $(call UPSTREAM_RULES,uqm,$(UQM),$(UQM_CFLAGS)))
$(eval $(call UPSTREAM_RULES,zlib,$(ZLIB),$(ZLIB_CFLAGS)))
$(eval $(call UPSTREAM_RULES,libpng,$(LIBPNG),$(PNG_CFLAGS)))
$(eval $(call UPSTREAM_RULES,libogg,$(LIBOGG),$(OGG_CFLAGS)))
$(eval $(call UPSTREAM_RULES,libvorbis,$(LIBVORBIS),$(VORBIS_CFLAGS)))

# The game's main() is the program's entry point on a desktop, but here
# main() belongs to the Circle kernel (main.cpp in this directory), which
# calls the game once the board is up. Renaming the entry point through the
# preprocessor for this one translation unit keeps upstream's source
# untouched, and the kernel calls the new name.
$(OBJDIR)/uqm/src/uqm.o: $(UQM)/src/uqm.c
	@mkdir -p $(dir $@)
	@echo "  CC    $@ (entry point renamed)"
	@$(CC) $(UQM_CFLAGS) -Dmain=uqm_main -c -o $@ $<

$(OBJDIR)/uqm/src/uqm.d: $(UQM)/src/uqm.c
	@mkdir -p $(dir $@)
	@$(CC) $(UQM_CFLAGS) -Dmain=uqm_main -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

$(OBJDIR):
	@mkdir -p $(OBJDIR)

-include $(DEPS)

# The image link rule. sdl-app.mk lets the caller choose the linker script,
# so this build supplies its own: the shim's script with room made at image
# offset 0x800 for the patchable-defaults block. Everything else about the
# script — including the TLS-section adjacency binutils 2.44 and later
# require — is inherited from the shim's.
SDL_APP_LDSCRIPT = $(CURDIR)/uqm-defaults.ld
include $(SHIM)/sdl-app.mk

# The other half of the guarantee the linker script's ASSERTs begin: check
# the BYTES of the finished image, not just the link. A pre-boot writer
# refuses to stamp an image whose magic is not at 0x800, so an image that
# lost it would boot and then quietly ignore everything ever sent to it —
# including --rapi-debug-uart, which is the channel used to drive the board
# when nobody is at the bench. Better to have no image than that one.
.PHONY: image
image: $(TARGET).img
	@if [ "`dd if=$(TARGET).img bs=4 skip=512 count=1 2>/dev/null`" = "PM8D" ]; \
	then \
		echo "  GATE  $(TARGET).img defaults block at 0x800: present"; \
	else \
		echo "  GATE  $(TARGET).img: defaults block ABSENT at 0x800 — image deleted"; \
		rm -f $(TARGET).img; \
		exit 1; \
	fi

# Board-scoped clean: drop just this board's tree, leaving the other two
# boards' images and objects untouched.
.PHONY: clean-board
clean-board:
	rm -rf $(OBJDIR) build/$(RAPI_BOARD)-stub
