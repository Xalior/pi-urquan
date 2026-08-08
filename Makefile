#
# pi-urquan — The Ur-Quan Masters as a bootable bare-metal Raspberry Pi image.
#
#   make check-toolchain     report the cross compiler this build will use
#   make deps                the three circle-stdlib worlds and the shim
#                            archives built against them (long: the worlds
#                            build newlib and libc++ from source)
#   make deps-rpi4           the same for one board only, for a machine that
#                            cannot hold three worlds at once
#   make rpi5 | rpi4 | rpi3  one board's kernel image
#   make kernels             all three, built in parallel
#   make verify              truth-gate: every image exists and is non-empty
#   make media               download the freely redistributable game data
#                            into media/
#   make netboot             stage the Pi 5 image and its boot configuration
#                            into build/netboot-rpi5/
#   make card                stage the whole card into build/sd-card/, putting
#                            in whatever media/ holds and naming what it does
#                            not
#   make clean-boards        drop every board's build tree
#
# The three boards never share mutable state: each has its own circle-stdlib
# world, its own shim archive and its own object directory, so building them
# at the same time is safe and building one never disturbs another.
#
# The libc++ sources every world is built from are one immutable git tag, and
# CIRCLE_LLVM says where that checkout lives. The default puts it beside this
# repository, which is right for a plain clone and for a CI runner. Point
# several projects at one directory to fetch it once for all of them:
#
#   make deps CIRCLE_LLVM=/path/to/circle-llvm
#

include mk/toolchain.mk

# Stated explicitly because the first rule this file sees comes from an
# included makefile, and that would otherwise decide the default goal.
.DEFAULT_GOAL := kernels

BOARDS ?= rpi3 rpi4 rpi5

IMAGE_rpi3 = kernel8.img
IMAGE_rpi4 = kernel8-rpi4.img
IMAGE_rpi5 = kernel_2712.img

.PHONY: deps kernels verify media netboot card clean-boards $(BOARDS)
.PHONY: $(addprefix deps-,$(BOARDS))

deps:
	$(MAKE) -C circle-libsdl2 deps

# One board's dependencies: its own circle-stdlib world and the shim archive
# built against it. A machine with a small disk — a CI runner, most obviously
# — builds one board at a time and keeps only that board's world.
# Written as a static pattern rule over the board list rather than a plain
# pattern rule: these targets are phony, and make does not apply pattern rules
# to phony targets — it would quietly answer "nothing to be done" and leave
# the world unbuilt.
$(addprefix deps-,$(BOARDS)): deps-%:
	$(MAKE) -C circle-libsdl2 world BOARD=$*
	$(MAKE) -C circle-libsdl2 libSDL2-$*.a BOARD=$*

$(BOARDS): check-toolchain
	$(MAKE) -C host RAPI_BOARD=$@

# All three at once. Each sub-make owns a different world and a different
# output directory, so there is nothing for them to collide on.
#
# Each board is waited for BY PID, and its status kept. A bare `wait` reports
# only that the shell has no children left — it is success whatever the jobs
# did — so a board that failed to build would leave this target reporting
# success, and the truth-gate would then pass the board's PREVIOUS image,
# still on disk.
kernels: check-toolchain
	@pids=; fail=0; \
	for b in $(BOARDS); do $(MAKE) -C host RAPI_BOARD=$$b & pids="$$pids $$!"; done; \
	for p in $$pids; do wait $$p || fail=1; done; \
	exit $$fail

# Truth-gate: ask the filesystem, not the exit codes. An image that is
# missing or empty fails here even if the build claimed success.
verify:
	@fail=0; \
	for b in $(BOARDS); do \
		case $$b in \
			rpi3) img=host/build/rpi3/$(IMAGE_rpi3) ;; \
			rpi4) img=host/build/rpi4/$(IMAGE_rpi4) ;; \
			rpi5) img=host/build/rpi5/$(IMAGE_rpi5) ;; \
		esac; \
		if [ -s "$$img" ]; then \
			echo "  OK    $$img ($$(wc -c < $$img | tr -d ' ') bytes)"; \
		else \
			echo "  FAIL  $$img missing or empty"; fail=1; \
		fi; \
	done; \
	exit $$fail

# ---------------------------------------------------------------------------
# Game data
# ---------------------------------------------------------------------------
#
#   media/           what `make media` downloads. Gitignored, never shipped,
#                    and never part of a build.
#   build/sd-card/   what `make card` stages. It copies from media/ and
#                    fetches nothing.
#
# `card` does not depend on `media`, so a card built without it is complete
# except for the data and names the files that are absent.
#
# `make media` fetches all three of The Ur-Quan Masters' content packages
# from the project's own SourceForge file releases. Toys for Bob released the
# 3DO port's content in 2002 and the UQM project distributes these packages
# itself, so all three are freely redistributable — no login, no browser, no
# DRM anywhere in the fetch. SourceForge's download URL redirects to a
# mirror, which plain curl follows.
#
# A .uqm package is a ZIP archive under a different extension, and the game
# reads it as one: they are not unpacked, on the card or anywhere else.
#
# Each file is verified against a SHA256 computed from the copy this project
# fetched, so a later fetch is known to be identical, and against the ZIP
# magic. There is no upstream-published checksum for these packages, so the
# SHA256 is this project's own record rather than an independent one — that
# is the honest description of what the check proves. Re-running the target
# re-verifies rather than re-downloading.
MEDIA_DIR = media

UQM_VERSION  = 0.8.0
UQM_BASE_URL = https://sourceforge.net/projects/sc2/files/UQM/0.8

CONTENT_UQM    = uqm-$(UQM_VERSION)-content.uqm
CONTENT_SHA256 = 77d75ac25e6fb755a33c4ba3b38a7b7bc41fcbc02896891b0cc9ac9214b72eef

VOICE_UQM      = uqm-$(UQM_VERSION)-voice.uqm
VOICE_SHA256   = 9edbf51d77d8b533399c5f3afb549314a8210d7aab71ca2c51b4f24832337a45

MUSIC_UQM      = uqm-$(UQM_VERSION)-3domusic.uqm
MUSIC_SHA256   = 44cd3cec7e9569b4117adb4d77f1522890972566267391ae09631485d231d8b9

# The base package is what the game cannot start without. The other two are
# the 3DO port's voice acting and its remastered soundtrack, which the game
# picks up if they are there and does without if they are not.
MEDIA_PACKAGES = $(CONTENT_UQM) $(VOICE_UQM) $(MUSIC_UQM)

# sha256sum on Linux, shasum on macOS. Whichever exists; if neither does the
# target stops rather than accepting a download it cannot check.
SHA256SUM := $(firstword $(shell command -v sha256sum 2>/dev/null) \
                         $(shell command -v shasum 2>/dev/null))

# One package: fetch it if it is not here, then verify whatever is here.
# $(1) file name, $(2) expected SHA256.
define FETCH_PACKAGE
	@if [ -f "$(MEDIA_DIR)/$(1)" ]; then \
		echo "  MEDIA $(MEDIA_DIR)/$(1) already here — verifying"; \
	else \
		echo "  MEDIA fetching $(UQM_BASE_URL)/$(1)/download"; \
		curl -fL --retry 3 -o "$(MEDIA_DIR)/$(1).part" \
			"$(UQM_BASE_URL)/$(1)/download" || { \
			rm -f "$(MEDIA_DIR)/$(1).part"; \
			echo "  MEDIA download failed"; exit 1; }; \
		mv "$(MEDIA_DIR)/$(1).part" "$(MEDIA_DIR)/$(1)"; \
	fi
	@got=`$(SHA256SUM) -a 256 "$(MEDIA_DIR)/$(1)" 2>/dev/null || $(SHA256SUM) "$(MEDIA_DIR)/$(1)"`; \
	got=`echo "$$got" | awk '{print $$1}'`; \
	if [ "$$got" != "$(2)" ]; then \
		echo "  MEDIA SHA256 MISMATCH for $(MEDIA_DIR)/$(1)"; \
		echo "        expected $(2)"; \
		echo "        got      $$got"; \
		echo "        the file has been left in place for inspection, and is"; \
		echo "        NOT safe to put on a card."; \
		exit 1; \
	fi; \
	head -c 4 "$(MEDIA_DIR)/$(1)" | od -An -c | grep -q 'P   K 003 004' || { \
		echo "  MEDIA $(MEDIA_DIR)/$(1) is not a ZIP archive"; exit 1; }; \
	echo "  MEDIA $(MEDIA_DIR)/$(1) verified ($$(wc -c < $(MEDIA_DIR)/$(1) | tr -d ' ') bytes)"
endef

media:
	@if [ -z "$(SHA256SUM)" ]; then \
		echo "  MEDIA no checksum tool on this machine (sha256sum or shasum)"; \
		echo "        — refusing to download something that cannot be"; \
		echo "        verified."; \
		exit 1; \
	fi
	@mkdir -p $(MEDIA_DIR)
	$(call FETCH_PACKAGE,$(CONTENT_UQM),$(CONTENT_SHA256))
	$(call FETCH_PACKAGE,$(VOICE_UQM),$(VOICE_SHA256))
	$(call FETCH_PACKAGE,$(MUSIC_UQM),$(MUSIC_SHA256))
	@printf '%s\n' \
		"The Ur-Quan Masters $(UQM_VERSION) content packages" \
		"" \
		"Source:   $(UQM_BASE_URL)/<file>/download" \
		"Project:  https://sc2.sourceforge.net/downloads.php" \
		"Fetched:  `date -u '+%Y-%m-%d %H:%M:%S UTC'`" \
		"" \
		"$(CONTENT_UQM)" \
		"  SHA256: $(CONTENT_SHA256)" \
		"  The base game content. Required — the game cannot start without it." \
		"" \
		"$(VOICE_UQM)" \
		"  SHA256: $(VOICE_SHA256)" \
		"  The 3DO port's voice acting. Optional." \
		"" \
		"$(MUSIC_UQM)" \
		"  SHA256: $(MUSIC_SHA256)" \
		"  The 3DO port's remastered soundtrack. Optional." \
		"" \
		"What they are: Toys for Bob released the 3DO port of Star Control II" \
		"in 2002, and The Ur-Quan Masters project distributes these packages" \
		"itself. Each is a ZIP archive under a .uqm extension, read as one by" \
		"the game and never unpacked." \
		"" \
		"Licence: the game code in the packages is GPL-2.0-or-later; the" \
		"resource content is CC BY-NC-SA 2.5, per the project's own licensing" \
		"statement at https://sc2.sourceforge.net . Free redistribution is" \
		"the reason the project exists." \
		"" \
		"Verification: the SHA256 figures above were computed from the copies" \
		"this project fetched. Upstream publishes no checksums for these" \
		"files, so they record that a later fetch is identical to the first," \
		"and are not an independent attestation." \
		"" \
		"These files are not redistributed by this repository." \
		> $(MEDIA_DIR)/provenance.txt
	@echo "  MEDIA provenance written to $(MEDIA_DIR)/provenance.txt"

# The Pi 5 netboot bundle: the image the Pi 5 firmware looks for, plus the
# boot configuration it must be served alongside. Copy the contents into the
# TFTP root the board boots from (the Raspberry Pi firmware files themselves
# come from that root's existing installation, not from here).
NETBOOT_DIR = build/netboot-rpi5
netboot: rpi5
	@mkdir -p $(NETBOOT_DIR)
	@cp host/build/rpi5/$(IMAGE_rpi5) $(NETBOOT_DIR)/
	@cp host/config.txt host/cmdline.txt $(NETBOOT_DIR)/
	@echo "  STAGED $(NETBOOT_DIR)/"
	@ls -l $(NETBOOT_DIR)/

# The card, staged into a directory to copy onto media formatted elsewhere:
# the three kernels, boot configuration, and the game's own directory with
# whatever media/ happens to hold.
#
# Everything belonging to this game lives in one directory on the card, named
# by RAPI_GAME_DIR in host/Makefile. A card carries several games, and two of
# them writing their settings into the FAT root would each silently overwrite
# the other's. The two paths have to agree: the kernel enters this directory
# before the game starts and is given --contentdir and --configdir inside it,
# so a package staged anywhere else is a package the game never sees.
#
# The base package goes in content/packages/ and the optional ones in
# content/addons/, which is the layout the engine's own content management
# expects. Beside them goes `version`, the marker the engine uses to
# recognise a content directory at all: it is the one file the download does
# not contain, and it comes from the game's own source tree. Without it the
# game reports that it cannot find its content and stops, whatever else is
# in place.
#
# menu.key and uqm.key come from the same place and matter as much. They are
# the keyboard bindings — which key is "select", which is "cancel", which
# move the cursor — and the game reads them from the content directory's
# root. Without them it starts, draws, plays its music and then ignores
# every key pressed, silently and with nothing on screen to say why.
#
# This target downloads nothing. It copies what `make media` left and names
# what is absent.
CARD_DIR  = build/sd-card
CARD_GAME = $(CARD_DIR)/games/urquan
CARD_CONTENT = $(CARD_GAME)/content

card: kernels
	@rm -rf $(CARD_DIR)
	@mkdir -p $(CARD_CONTENT)/packages $(CARD_CONTENT)/addons $(CARD_GAME)/config
	@cp host/build/rpi3/$(IMAGE_rpi3) $(CARD_DIR)/
	@cp host/build/rpi4/$(IMAGE_rpi4) $(CARD_DIR)/
	@cp host/build/rpi5/$(IMAGE_rpi5) $(CARD_DIR)/
	@cp host/config.txt host/cmdline.txt $(CARD_DIR)/
	@cp uqm/sc2/content/version $(CARD_CONTENT)/version
	@cp uqm/sc2/content/menu.key uqm/sc2/content/uqm.key $(CARD_CONTENT)/
	@echo "  STAGED $(CARD_DIR)/"
	@if [ -f "$(MEDIA_DIR)/$(CONTENT_UQM)" ]; then \
		cp "$(MEDIA_DIR)/$(CONTENT_UQM)" $(CARD_CONTENT)/packages/; \
		echo "  DATA   $(CONTENT_UQM)"; \
	fi
	@for f in $(VOICE_UQM) $(MUSIC_UQM); do \
		if [ -f "$(MEDIA_DIR)/$$f" ]; then \
			cp "$(MEDIA_DIR)/$$f" $(CARD_CONTENT)/addons/; \
			echo "  DATA   $$f"; \
		fi; \
	done
	@echo
	@if [ -f "$(CARD_CONTENT)/packages/$(CONTENT_UQM)" ]; then :; else \
		echo "  ABSENT $(CONTENT_UQM). The game cannot start without it."; \
		echo "         'make media' fetches it — it is freely redistributable."; \
	fi
	@echo "  NOTE   The Raspberry Pi firmware files are not staged here either."
	@echo "         See README.md."

# Board build trees and staged output only. media/ is not touched: it holds
# downloaded data, which no build target deletes.
clean-boards:
	@for b in $(BOARDS); do $(MAKE) -C host RAPI_BOARD=$$b clean-board; done
	rm -rf $(NETBOOT_DIR) $(CARD_DIR)
