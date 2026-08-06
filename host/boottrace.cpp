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
#include "boottrace.h"
#include <atomic>

static const char From[] = "boottrace";

// The progress word. Core 0 reads it with its own logger, so it reports even
// when the log ring does not — see boottrace.h.
static std::atomic<unsigned> s_Progress{0};
static std::atomic<unsigned> s_AppWrites{0};
static std::atomic<unsigned> s_AppWriteBytes{0};

extern "C" {

void BootTraceMark(unsigned nMilestone)
{
    s_Progress.store(nMilestone, std::memory_order_release);
}

unsigned BootTraceRead(void)
{
    return s_Progress.load(std::memory_order_acquire);
}

void BootTraceCountAppWrite(unsigned nBytes)
{
    s_AppWrites.fetch_add(1, std::memory_order_relaxed);
    s_AppWriteBytes.fetch_add(nBytes, std::memory_order_relaxed);
}

unsigned BootTraceAppWrites(void)
{
    return s_AppWrites.load(std::memory_order_relaxed);
}

unsigned BootTraceAppWriteBytes(void)
{
    return s_AppWriteBytes.load(std::memory_order_relaxed);
}

const char *BootTraceName(unsigned nMilestone)
{
    switch (nMilestone)
    {
    case 0:                          return "nothing yet — the gate has not been passed";
    case BOOTTRACE_GATE_PASSED:      return "past the gate";
    case BOOTTRACE_LOG_ENTERED:      return "inside its first log call — the ring is the suspect";
    case BOOTTRACE_LOG_RETURNED:     return "first log call returned — the ring works";
    case BOOTTRACE_CALLING_GAME:     return "about to enter the game";
    case BOOTTRACE_MAIN_ENTERED:     return "inside the game's entry point";
    case BOOTTRACE_MAIN_REAL:        return "about to call upstream's own main";
    case BOOTTRACE_GETOPT_FIRST:     return "in the option scan";
    case BOOTTRACE_LOGINIT_ENTERED:  return "in log_init";
    case BOOTTRACE_LOGINIT_RETURNED: return "past log_init — the game can log for itself";
    case BOOTTRACE_MAIN_RETURNED:    return "the game returned";
    default:                         return "unknown milestone";
    }
}

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

    BootTraceMark(BOOTTRACE_MAIN_ENTERED);

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

    BootTraceMark(BOOTTRACE_MAIN_REAL);
    const int result = __real_uqm_main(argc, argv);
    BootTraceMark(BOOTTRACE_MAIN_RETURNED);

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
    if (call == 1)
        BootTraceMark(BOOTTRACE_GETOPT_FIRST);

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

    BootTraceMark(BOOTTRACE_LOGINIT_ENTERED);
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE, "log_init entered");
    __real_log_init(max_lines);
    BootTraceMark(BOOTTRACE_LOGINIT_RETURNED);
    SDL2Circle_Log(From, SDL2CIRCLE_LOG_NOTICE,
                   "log_init returned — the game can log for itself from here");
}

}   // extern "C"
