
static void _amain(void);
static void jhc_arch_assert(void);
static int jhc_argc;
static char **jhc_argv;
static char *jhc_progname;
static jmp_buf jhc_uncaught;

static HsInt jhc_stdrnd[2] A_UNUSED = { 1 , 1 };
static HsInt jhc_data_unique A_UNUSED;
#ifdef __WIN32__
static char *jhc_options_os =  "mingw32";
static char *jhc_options_arch = "i386";
#else
struct utsname jhc_utsname;
static char *jhc_options_os = "(unknown os)";
static char *jhc_options_arch = "(unknown arch)";
#endif


#if _JHC_PROFILE

static uintmax_t jhc_prof_function_calls;
static uintmax_t jhc_prof_case_statements;
static uintmax_t jhc_prof_updates;

#define jhc_update_inc()   jhc_prof_updates++
#define jhc_function_inc() jhc_prof_function_calls++
#define jhc_case_inc()     jhc_prof_case_statements++

#else

#define jhc_update_inc()    do { } while(0)
#define jhc_function_inc()  do { } while(0)
#define jhc_case_inc()      do { } while(0)

#endif

static void A_COLD
jhc_print_profile(void) {
#ifndef __WIN32__
        struct tms tm;
        times(&tm);
#endif
        if(!(_JHC_PROFILE || getenv("JHC_RTS_PROFILE"))) return;

        fprintf(stderr, "\n-----------------\n");
        fprintf(stderr, "Profiling: %s\n", jhc_progname);
        fprintf(stderr, "Command: %s\n", jhc_command);
        fprintf(stderr, "Complie: %s\n", jhc_c_compile);
        fprintf(stderr, "Version: %s\n\n", jhc_version);
        jhc_alloc_print_stats();
#ifndef __WIN32__
        float cpt = (float)sysconf(_SC_CLK_TCK);
        fprintf(stderr, "User Time:   %.2fs\n", (float)tm.tms_utime/cpt);
        fprintf(stderr, "System Time: %.2fs\n", (float)tm.tms_stime/cpt);
        fprintf(stderr, "Total Time:  %.2fs\n", (float)(tm.tms_stime + tm.tms_utime)/cpt);
#endif

#if _JHC_PROFILE
        fprintf(stderr, "\nFunction Calls:   %llu\n", (unsigned long long)jhc_prof_function_calls);
        fprintf(stderr, "Case Statements:  %llu\n", (unsigned long long)jhc_prof_case_statements);
        fprintf(stderr, "Updates:          %llu\n", (unsigned long long)jhc_prof_updates);
#endif
        fprintf(stderr, "-----------------\n");
}


static void A_NORETURN A_UNUSED A_COLD
jhc_exit(int n) {
        jhc_print_profile();
        exit(n);
}

static void  A_NORETURN A_UNUSED  A_COLD
jhc_error(char *s) {
        fputs(s,stderr);
        fputs("\n",stderr);
        jhc_print_profile();
        exit(255);
}

static void  A_NORETURN A_UNUSED  A_COLD
jhc_case_fell_off(int n) {
        fflush(stdout);
        fprintf(stderr, "\n%s:%i: case fell off\n", __FILE__, n);
        abort();
}

static HsBool A_UNUSED
jhc_wait_for_input(FILE *f,HsInt timeout) {
#if JHC_isPosix
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fileno(f),&fds);
        struct timeval to = {  0, timeout * 1000 };
        int retval = select(1,&fds,NULL,&fds,&to);
        if(retval)
                return HS_BOOL_TRUE;
        else
                return HS_BOOL_FALSE;
#else
        return HS_BOOL_FALSE;
#endif

}

#ifdef __WIN32__
#define jhc_setjmp(jb) setjmp(*(jmp_buf *)jb)
#define jhc_longjmp(jb) longjmp(*(jmp_buf *)jb,1)
#define getchar_unlocked() getchar()
#define putchar_unlocked(x) putchar(x)
#define getc_unlocked(x) getc(x)
#define putc_unlocked(x,y) putc(x,y)
#else
#define jhc_setjmp(jb) sigsetjmp(*(jmp_buf *)jb,0)
#define jhc_longjmp(jb) siglongjmp(*(jmp_buf *)jb,1)
#endif

struct jhc_continuation {
    void *argument;
    jmp_buf jump_buf;
};

#define prim_umaxbound(t) ((t)~((t)0))
#define prim_maxbound(t) ((t)(~((t)1 << (sizeof(t)*8 - 1))))
#define prim_minbound(t) ((t)(((t)1 << (sizeof(t)*8 - 1))))


inline static int A_UNUSED
jhc_utf8_getchar(void)
{
    return getchar_unlocked();
}

inline static int A_UNUSED
jhc_utf8_getc(FILE *f)
{
    return getc_unlocked(f);
}

inline static int A_UNUSED
jhc_utf8_putchar(int ch)
{
    return putchar_unlocked(ch);
}

inline static int A_UNUSED
jhc_utf8_putc(int ch, FILE *f)
{
    return putc_unlocked(ch,f);
}


int  A_COLD
main(int argc, char *argv[])
{
        /* A few random assertions about the architecture that the compiler
         * assumes. should be true of any but the oddest of beasts.
         */

        assert(sizeof(HsPtr) == sizeof(HsFunPtr));
        assert(sizeof(HsPtr) == sizeof(intptr_t));
        assert(sizeof(HsPtr) == sizeof(uintptr_t));
        assert(CHAR_BIT == 8);
        assert(EOF == -1);

        jhc_arch_assert();
        jhc_malloc_init();
        jhc_argc = argc - 1;
        jhc_argv = argv + 1;
        jhc_progname = argv[0];
#if JHC_isPosix
        if(!uname(&jhc_utsname)) {
                jhc_options_arch = jhc_utsname.machine;
                jhc_options_os   = jhc_utsname.sysname;
        }
#endif
        setlocale(LC_ALL,"");
        if (jhc_setjmp(jhc_uncaught))
                jhc_error("Uncaught Exception");
        else
                _amain();
        jhc_print_profile();
        return 0;
}



