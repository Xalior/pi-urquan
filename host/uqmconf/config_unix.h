/*
 * config_unix.h — the compile-time configuration The Ur-Quan Masters
 * normally generates while configuring a desktop build.
 *
 * Upstream ships src/config_unix.h.in and its build.sh fills in the blanks
 * by probing the machine it is running on. There is no machine to probe
 * here: the answers are fixed by the C library this port links against
 * (newlib, as circle-stdlib builds it) and by where the card puts this
 * game's files. So they are written down, once, with the reason beside each
 * one.
 *
 * src/config.h picks this file for every compiler that is not MSVC, MinGW,
 * Cygwin or Symbian, by the name config_unix.h. It is found because this
 * directory is on the include path and upstream's own copy is never
 * generated — nothing in the game's own tree is written to or replaced.
 */

#ifndef CONFIG_UNIX_H_
#define CONFIG_UNIX_H_

/* WHERE THIS GAME LIVES ON THE CARD.
 *
 * RAPI_GAME_DIR comes from this port's makefile and nothing upstream knows
 * about it. One card carries several games, so each keeps to a directory of
 * its own and nothing of this port's ever touches the card's root.
 *
 * The content directory holds the packages and the addons; it is read only.
 * The config directory is the one place the game writes: its settings, its
 * save games and its melee teams. The kernel also names both on the command
 * line, so these constants are the fallback rather than the only answer —
 * but they must still be right, because the engine reaches for them
 * whenever a switch is left off.
 */
#ifndef RAPI_GAME_DIR
/* The apostrophe of "port's" is spelled out: the preprocessor tokenises an
 * #error's text, and a lone apostrophe in it warns about an unterminated
 * character constant on every translation unit that includes this file. */
#error RAPI_GAME_DIR is not defined - see the host Makefile of this port
#endif

/* Directory where the UQM game data is located */
#define CONTENTDIR RAPI_GAME_DIR "/content"

/* Directory where game data will be stored */
#define USERDIR RAPI_GAME_DIR "/config/"

/* Directory where config files will be stored */
#define CONFIGDIR USERDIR

/* Directory where supermelee teams will be stored.
 * The ${...} form is upstream's own: the engine puts UQM_CONFIG_DIR in the
 * environment once the config directory is settled, then expands it here. */
#define MELEEDIR "${UQM_CONFIG_DIR}/teams/"

/* Directory where save games will be stored */
#define SAVEDIR "${UQM_CONFIG_DIR}/save/"

/* Defined if words are stored with the most significant byte first.
 * AArch64 here is little-endian, so this stays undefined. */
/* #undef WORDS_BIGENDIAN */

/* Defined if your system has readdir_r of its own.
 *
 * newlib's Circle port does: it is declared in <dirent.h> and defined in the
 * same object file as the rest of the directory calls, which this kernel
 * links for opendir and readdir regardless. Left undefined, port.c supplies
 * its own and the link fails with a duplicate definition. */
#define HAVE_READDIR_R

/* Defined if your system has setenv of its own.
 * Spelled with a value because SDL's own config header defines the same
 * name, also as 1: two identical definitions are not a redefinition, and a
 * bare one here would warn on every file that includes both. */
#define HAVE_SETENV 1

/* Defined if your system has strupr of its own */
#define HAVE_STRUPR

/* Defined if your system has strcasecmp of its own.
 * Not using "HAVE_STRCASECMP" as that conflicts with SDL. */
#define HAVE_STRCASECMP_UQM

/* Defined if your system has stricmp of its own.
 * It does not; port.h maps stricmp onto strcasecmp instead. */
/* #undef HAVE_STRICMP */

/* Defined if your system has getopt_long */
#define HAVE_GETOPT_LONG

/* Defined if your system has iswgraph of its own */
#define HAVE_ISWGRAPH

/* Defined if your system has wchar_t of its own */
#define HAVE_WCHAR_T

/* Defined if your system has wint_t of its own */
#define HAVE_WINT_T

/* Defined if your system has _Bool of its own */
#define HAVE__BOOL

#endif  /* CONFIG_UNIX_H_ */
