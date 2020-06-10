
#ifdef __cplusplus
extern "C" {
#endif

/* signal numbers */
#define SIGHUP     1  // Hangup (POSIX)
#define SIGINT     2  // Terminal interrupt (ANSI)
#define SIGQUIT    3  // Terminal quit (POSIX)
#define SIGILL     4  // Illegal instruction (ANSI)
#define SIGTRAP    5  // Trace trap (POSIX)
#define SIGIOT     6  // IOT Trap (4.2 BSD)
#define SIGBUS     7  // BUS error (4.2 BSD)
#define SIGFPE     8  // Floating point exception (ANSI)
#define SIGKILL    9  // Kill(can't be caught or ignored) (POSIX)
#define SIGUSR1    10 // User defined signal 1 (POSIX)
#define SIGSEGV    11 // Invalid memory segment access (ANSI)
#define SIGUSR2    12 // User defined signal 2 (POSIX)
#define SIGPIPE    13 // Write on a pipe with no reader, Broken pipe (POSIX)
#define SIGALRM    14 // Alarm clock (POSIX)
#define SIGTERM    15 // Termination (ANSI)
#define SIGSTKFLT  16 // Stack fault
#define SIGCHLD    17 // Child process has stopped or exited, changed (POSIX)
#define SIGCONT    18 // Continue executing, if stopped (POSIX)
#define SIGSTOP    19 // Stop executing(can't be caught or ignored) (POSIX)
#define SIGTSTP    20 // Terminal stop signal (POSIX)
#define SIGTTIN    21 // Background process trying to read, from TTY (POSIX)
#define SIGTTOU    22 // Background process trying to write, to TTY (POSIX)
#define SIGURG     23 // Urgent condition on socket (4.2 BSD)
#define SIGXCPU    24 // CPU limit exceeded (4.2 BSD)
#define SIGXFSZ    25 // File size limit exceeded (4.2 BSD)
#define SIGVTALRM  26 // Virtual alarm clock (4.2 BSD)
#define SIGPROF    27 // Profiling alarm clock (4.2 BSD)
#define SIGWINCH   28 // Window size change (4.3 BSD, Sun)
#define SIGIO      29 // I/O now possible (4.2 BSD)
#define SIGPWR     30 // Power failure restart (System V)

/* sigprocmask */
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define _NSIG       64
#define _NSIG_BPW   64
#define _NSIG_WORDS	(_NSIG / _NSIG_BPW)

/* structs */
typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

struct sigaction {
	void (*sa_handler)(int);
	void (*sa_sigaction)(int, void *, void *);
	sigset_t sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer)(void);
};

typedef struct jmp_buf_t {
	long long reg[8];
	sigset_t mask;
} jmp_buf[1];

/* function prototype */
int  write(int fd, char *ptr, int sz);
int  setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);
int  sigaction(int, struct sigaction *, struct sigaction *);
int  sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int  alarm(unsigned int seconds);
int  sys_pause(void);
unsigned int sleep(unsigned int seconds);
void sys_exit(int code);

/* function alias */
#define pause sys_pause
#define exit sys_exit

#ifdef __cplusplus
}
#endif
