//
// boottrace.h — the application core's progress, readable from core 0.
//
// INSTRUMENTATION. Its removal is recorded in the README's backlog.
//
// The trace in boottrace.cpp reports with SDL2Circle_Log, which puts a line
// in the calling core's ring for core 0's servo to drain. That is the right
// way to log off core 0 — and it is useless for diagnosing a board where
// the ring itself might be the thing that is broken, because a report that
// travels by the suspect mechanism cannot testify about it.
//
// So this is a second channel that shares nothing with the first: one word
// in memory. The application core stores a milestone into it; core 0 reads
// it and reports with its OWN logger, which writes straight to the device it
// owns and is already proven by every line the kernel prints before the game
// starts.
//
// The milestones bracket the log call itself, which is what makes the two
// failures distinguishable: reaching APPCORE_LOG_ENTERED and never
// APPCORE_LOG_RETURNED means the ring is where it stops.
//
#ifndef _uqm_boottrace_h
#define _uqm_boottrace_h

// Milestones, in the order the application core passes them.
#define BOOTTRACE_GATE_PASSED       1   // past the gate, running our code
#define BOOTTRACE_LOG_ENTERED       2   // about to log for the first time
#define BOOTTRACE_LOG_RETURNED      3   // that log call came back
#define BOOTTRACE_CALLING_GAME      4   // about to enter the game
#define BOOTTRACE_MAIN_ENTERED      5   // inside the game's entry point
#define BOOTTRACE_MAIN_REAL         6   // about to call upstream's own main
#define BOOTTRACE_GETOPT_FIRST      7   // the option scan has begun
#define BOOTTRACE_LOGINIT_ENTERED   8   // the game is initialising logging
#define BOOTTRACE_LOGINIT_RETURNED  9   // the game can log for itself now
#define BOOTTRACE_MAIN_RETURNED     10  // the game returned

extern "C" {

// Called from the application core. A plain release store, no device, no
// mailbox, nothing that can block.
void BootTraceMark(unsigned nMilestone);

// Called from core 0. The furthest milestone the application core reached.
unsigned BootTraceRead(void);

// A name for a milestone, for core 0 to put on the wire.
const char *BootTraceName(unsigned nMilestone);

}

#endif
