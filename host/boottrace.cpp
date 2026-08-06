//
// boottrace.cpp — a trace of the game's first moments, for a board that
// starts up and then goes quiet.
//
// INSTRUMENTATION. It is compiled in but silent unless the defaults block
// carries --rapi-trace-boot, so one image serves an ordinary boot and a
// traced one. Its removal is recorded in the README's backlog.
//
// The problem it exists for: The Ur-Quan Masters parses its command line
// BEFORE it initialises logging — upstream says so in a comment, because the
// log file's name is one of the options — so everything up to that point is
// silent by design. A board that stops anywhere in there produces no output
// at all, and "no output" cannot be told apart from "never started".
//
// So the trace brackets that window. Every one of these is a link-time
// --wrap: upstream is not edited, and nothing here runs when the switch is
// absent.
//
// Everything logs with SDL2Circle_Log. The game runs on the application
// core, so every function here is called off core 0 by construction, and a
// device — the serial console included — belongs to core 0 alone.
//
#include <SDL2/SDL_circle.h>
#include "defaults.h"

static const char From[] = "boottrace";

extern "C" {

// The game's entry point, renamed by the build, and the genuine versions of
// the calls it makes before it can log anything of its own.
int __real_uqm_main(int argc, char **argv);
struct option;
int __real_getopt_long(int argc, char *const argv[], const char *shortopts,
                       const struct option *longopts, int *longind);
char *__real_getenv(const char *name);
void __real_log_init(int max_lines);

// The entry point. This is the line that separates "the application core
// never got going" from "the game started and stopped somewhere inside".
int __wrap_uqm_main(int argc, char **argv)
{
    if (!rapi_trace_boot)
        return __real_uqm_main(argc, argv);

    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "uqm_main entered on the application core, argc=%d", argc);
    for (int i = 0; i < argc; i++)
        SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                       "  argv[%d] = \"%s\"", i,
                       argv[i] != nullptr ? argv[i] : "(null)");
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "  argv[%d] = %s", argc,
                   argv[argc] == nullptr ? "NULL (terminated)"
                                         : "NOT NULL — argv is unterminated");

    const int result = __real_uqm_main(argc, argv);

    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "uqm_main returned %d", result);
    return result;
}

// The option scan. Upstream loops on this until it answers -1, so a call
// that never returns, or one that never reaches -1, hangs the game here with
// nothing on the wire.
int __wrap_getopt_long(int argc, char *const argv[], const char *shortopts,
                       const struct option *longopts, int *longind)
{
    if (!rapi_trace_boot)
        return __real_getopt_long(argc, argv, shortopts, longopts, longind);

    static int s_call = 0;
    const int call = ++s_call;

    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "getopt_long call %d entered", call);
    const int c = __real_getopt_long(argc, argv, shortopts, longopts, longind);
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "getopt_long call %d returned %d", call, c);
    return c;
}

// getenv, because the option scan calls it on every pass to decide its
// ordering, and this system has no environment for it to walk.
char *__wrap_getenv(const char *name)
{
    if (!rapi_trace_boot)
        return __real_getenv(name);

    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "getenv(\"%s\") entered", name != nullptr ? name : "(null)");
    char *value = __real_getenv(name);
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "getenv returned %s", value != nullptr ? value : "NULL");
    return value;
}

// The moment the game can speak for itself. Reaching this line and then
// falling silent means the fault is after the option scan, not in it.
void __wrap_log_init(int max_lines)
{
    if (!rapi_trace_boot)
    {
        __real_log_init(max_lines);
        return;
    }

    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE, "log_init entered");
    __real_log_init(max_lines);
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "log_init returned — the game can log for itself from here");
}

}   // extern "C"
