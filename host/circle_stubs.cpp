//
// circle_stubs.cpp — POSIX calls the game expects that this system does not
// have.
//
// Nothing here is SDL. Every SDL, SDL_image and SDL_mixer entry point the
// game uses comes from circle-libsdl2; a missing one is reported and waited
// for, never written here. What this file holds is the other kind of gap:
// three C-library functions that exist on the desktop the game was written
// for and do not exist in newlib's Circle port.
//
// These are plain definitions rather than --wrap interpositions, because
// there is nothing to interpose on — the symbols are absent from the C
// library entirely, so the link is what discovers them.
//
#include <cerrno>
#include <cstring>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {

// access() — asked for by the game's uio filesystem layer when it walks a
// content directory (src/libs/uio/stdio/stdio.c).
//
// Answered through stat(), which the syscall layer already wraps and
// marshals to core 0, so this inherits the any-core routing for free.
//
// The mode bits are answered as a FAT filesystem can answer them: existence
// is a real question and gets a real answer; read and execute permission are
// granted for anything that exists, and write permission is granted because
// nothing on this card is read-only to us. R_OK, W_OK and X_OK therefore
// reduce to F_OK, which is what a filesystem with no ownership and no
// permission bits can honestly report.
int access(const char *path, int mode)
{
    (void)mode;

    struct stat st;
    if (stat(path, &st) != 0)
        return -1;      // errno set by stat

    return 0;
}

// getuid() — the game asks for it in two places: to build a home directory
// path (src/libs/file/dirs.c) and inside the vendored MikMod's device
// probing (src/libs/mikmod/mdriver.c), which checks whether it is running as
// root before touching a sound device it will never find here.
//
// There is one user on a board with no user accounts, and it is not root:
// reporting 0 would send MikMod down its privileged path.
uid_t getuid(void)
{
    return 1;
}

// getpwuid() — the game calls it, through getHomeDir(), for the home
// directory to expand '~' against and to fall back on when $HOME is unset.
//
// The answer is this game's own directory on the card. The kernel already
// passes --contentdir and --configdir as absolute paths inside it and enters
// it before the game starts, so this seam only matters for a path the game
// expands itself — but an unanswered one would put the game's files at the
// FAT root, where every other game on the card writes too.
//
// The record is static and its strings are literals, which is exactly the
// contract: getpwuid returns a pointer to storage owned by the C library,
// and callers may not free or modify it.
struct passwd *getpwuid(uid_t uid)
{
    static char s_name[]  = "pi";
    static char s_dir[]   = RAPI_GAME_DIR;
    static char s_shell[] = "";
    static char s_empty[] = "";

    static struct passwd s_pw;

    if (uid != getuid())
    {
        errno = 0;      // "no such entry" is not an error, per POSIX
        return nullptr;
    }

    memset(&s_pw, 0, sizeof(s_pw));
    s_pw.pw_name  = s_name;
    s_pw.pw_passwd = s_empty;
    s_pw.pw_uid   = uid;
    s_pw.pw_gid   = uid;
    s_pw.pw_dir   = s_dir;
    s_pw.pw_shell = s_shell;
    return &s_pw;
}

}   // extern "C"
