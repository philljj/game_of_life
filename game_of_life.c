#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// globals
char   prog_name[] = "life";
pid_t  pid;
size_t archive = 0;
size_t debug = 0;
size_t quiet = 0;
size_t sleep_interval = 0;

size_t surv_l = 2;
size_t surv_h = 3;
size_t born = 3;

char * mem_index[128];

int print_usage_and_die(void);
void * safer_malloc(const size_t count);
int run_game(const size_t len);

void (*eval_rule)(char * board_dst, const char * board_src, const size_t len);
void traditional_rule(char * board_dst, const char * board_src, const size_t len);

enum status_t {
    DEAD,
    EVOLVING,
    STATIC,
    PERIODIC
};

struct state_t {
    enum status_t status;
    size_t        period;
};

size_t
get_len(const char * arg)
{
    const char * n = arg;

    while (*arg) {
        if (!isdigit(*arg)) {
            fprintf(stderr, "error: %c of %s is not a number\n",
                    *arg, arg);
            print_usage_and_die();
        }

        ++arg;
    }

    return atoi(n);
}

int
main(int    argc,
     char * argv[])
{
    if (argc < 2) { print_usage_and_die(); }

    int    opt = 0;
    size_t len = 0;

    while ((opt = getopt(argc, argv, "b:l:h:n:s:adq")) != -1) {
        switch (opt) {
        case 'a':
            archive = 1;
            break;

        case 'd':
            debug = 1;
            break;

        case 's':
            sleep_interval = get_len(optarg);
            break;

        case 'q':
            quiet = 1;
            break;

        case 'n':
            len = get_len(optarg);
            break;

        case 'b':
            born = get_len(optarg);
            break;

        case 'l':
            surv_l = get_len(optarg);
            break;

        case 'h':
            surv_h = get_len(optarg);
            break;

        case '?':
        default:
            print_usage_and_die();
        }
    }

    if (len >= 100) {
        fprintf(stderr, "error: number is too large! Must be < 100\n");
        print_usage_and_die();
    }

    if (len < 2) {
        fprintf(stderr, "error: need a length > 2\n");
        print_usage_and_die();
    }

    pid = getpid();

    eval_rule = traditional_rule;

    return run_game(len);
}



int
print_usage_and_die(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s -n <len> -b <born> -l <low> -h <high> -dq", prog_name);
    fprintf(stderr, "\noptions:\n");
    fprintf(stderr, "  n: the length of square board. 2-99\n");
    fprintf(stderr, "  b: the born number in BnSij rule\n");
    fprintf(stderr, "  l: the low survival number in BnSij rule\n");
    fprintf(stderr, "  h: the high survival number in BnSij rule\n");
    fprintf(stderr, "  s: sleep interval\n");
    fprintf(stderr, "  q: quiet mode. Enable to reduce print output\n");
    fprintf(stderr, "  d: debug mode (prints function entry)\n");


    exit(EXIT_FAILURE);
}



void *
safer_malloc(const size_t count)
{
    void * ptr = malloc(count);

    if (!ptr) {
        fprintf(stderr, "error: malloc(%zu) failed\n", count);
        exit(EXIT_FAILURE);
    }

    return ptr;
}



void
reset_board(char *       board,
            const size_t len_sq)
{
    for (size_t i = 0; i < len_sq; ++i) {
        board[i] = ' ';
    }

    return;
}



void
copy_board(char *       board_dst,
           const char * board_src,
           const size_t len_sq)
{
    for (size_t i = 0; i < len_sq; ++i) {
        board_dst[i] = board_src[i];
    }

    return;
}



size_t
same_board(const char * board_1,
           const char * board_2,
           const size_t len_sq)
{
    if (debug) { fprintf(stderr, "debug: same_board(\n"); }

    for (size_t i = 0; i < len_sq; ++i) {
        if (board_1[i] != board_2[i]) { return 0; }
    }

    return 1;
}



void
init_board(char *       board,
           const size_t len_sq)
{
    srand(pid);

    size_t index = 0;
    size_t times = rand() % len_sq;

    for (size_t i = 0; i < times; ++i) {
        index = rand() % len_sq;
        board[index] = 'o';
    }

    return;
}



void
print_board(char *       board,
            const size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        for (size_t j = 0; j < len; ++j) {
            printf("%c", board[i * len + j]);
        }
        printf("\n");
    }

    printf("\n");

    return;
}



void
swap_board(char * * b_1_ptr,
           char * * b_2_ptr)
{
    char * board = *b_1_ptr;

    *b_1_ptr = *b_2_ptr;
    *b_2_ptr = board;

    return;
}



size_t
u(const size_t i,
  const size_t j,
  const size_t len)
{
    return i > 0; 
}



size_t
d(const size_t i,
  const size_t j,
  const size_t len)
{
    return i < len - 1;
}



size_t
l(const size_t i,
  const size_t j,
  const size_t len)
{
    return j > 0;
}



size_t
r(const size_t i,
  const size_t j,
  const size_t len)
{
    return j < len - 1;
}



size_t
u_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!u(i, j, len)) { return 0; }

    return board[(i - 1) * len + j] == 'o';
}



size_t
d_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!d(i, j, len)) { return 0; }

    return board[(i + 1) * len + j] == 'o';
}



size_t
r_life(const char * board,
       const size_t i,
       const size_t j,
       const size_t len)
{
    if (!r(i, j, len)) { return 0; }

    return board[i * len + j + 1] == 'o';
}



size_t
l_life(const char * board,
       const size_t i,
       const size_t j,
       const size_t len)
{
    if (!l(i, j, len)) { return 0; }

    return board[i * len + j - 1] == 'o';
}



size_t
ur_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!u(i, j, len)) { return 0; }
    if (!r(i, j, len)) { return 0; }

    return board[(i - 1) * len + j + 1] == 'o';
}



size_t
ul_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!u(i, j, len)) { return 0; }
    if (!l(i, j, len)) { return 0; }

    return board[(i - 1) * len + j - 1] == 'o';
}



size_t
dr_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!d(i, j, len)) { return 0; }
    if (!r(i, j, len)) { return 0; }

    return board[(i + 1) * len + j + 1] == 'o';
}



size_t
dl_life(const char * board,
        const size_t i,
        const size_t j,
        const size_t len)
{
    if (!d(i, j, len)) { return 0; }
    if (!l(i, j, len)) { return 0; }

    return board[(i + 1) * len + j - 1] == 'o';
}



size_t (*check_dir[8])(const char * board, const size_t i, const size_t j,
                       const size_t len);

void
init_dir(void)
{
    check_dir[0] = u_life;
    check_dir[1] = d_life;
    check_dir[2] = r_life;
    check_dir[3] = l_life;
    check_dir[4] = ur_life;
    check_dir[5] = ul_life;
    check_dir[6] = dr_life;
    check_dir[7] = dl_life;

    return;
}



void
traditional_rule(char *       board_dst,
                 const char * board_src,
                 const size_t len)
{
    if (debug) { fprintf(stderr, "debug: traditional_rule(\n"); }

    for (size_t i = 0; i < len; ++i) {
        for (size_t j = 0; j < len; ++j) {
            size_t life_count = 0;

            for (size_t q = 0; q < 8; ++q) {
                life_count += check_dir[q](board_src, i, j, len);
            }

            if (board_src[i * len + j] == 'o') {
                if (life_count < surv_l || life_count > surv_h) {
                    // It dies!
                    board_dst[i * len + j] = ' ';
                }
                // else, it lives, do nothing.

                continue;
            }
            // else, dead cell.

            if (life_count == born) {
                // life, uh, finds a way
                board_dst[i * len + j] = 'o';
            }
        }
    }

    return;
}



size_t
life_exists(const char * board,
            const size_t len_sq)
{
    if (debug) { fprintf(stderr, "debug: life_exists(\n"); }

    for (size_t i = 0; i < len_sq; ++i) {
        if (board[i] == 'o') {
            return 1;
        }
    }

    return 0;
}



void
check_state(struct state_t * state,
            const char *     board_dst,
            const size_t     len_sq)
{
    if (debug) { fprintf(stderr, "debug: check_state(\n"); }

    if (!life_exists(board_dst, len_sq)) {
        state->status = DEAD;
        return;
    }

    size_t i = 0;
    char * root = mem_index[0];

    while (mem_index[i] && i < 128) { ++i; }

    if (same_board(board_dst, mem_index[i - 1], len_sq)) {
        state->status = STATIC;
        state->period = 1;
        return;
    }

    for (size_t j = i - 1; j > 1; --j) {
        if (same_board(board_dst, mem_index[j], len_sq)) {
            state->status = PERIODIC;
            state->period = i - j;
            return;
        }
    }

    return;
}



void
append_board(char * board_dst)
{
    if (debug) { fprintf(stderr, "debug: append_board(\n"); }

    size_t   i = 0;
    char *   root = mem_index[0];

    while (mem_index[i] && i < 128) { ++i; }

    if (i == 128 - 1) {
        for (size_t j = 1; j < 128; ++j) {
            free(mem_index[j]);
            mem_index[j] = 0;
        }

        mem_index[0] = root;
        i = 1;
    }

    mem_index[i] = board_dst;

    return;
}



int
run_game(const size_t len)
{
    int          status = EXIT_SUCCESS;
    const size_t len_sq = len * len;

    char * board_ini = safer_malloc(len_sq);
    char * board_src = safer_malloc(len_sq);
    char * board_dst = 0;

    reset_board(board_src, len_sq);

    init_board(board_src, len_sq);
    init_dir();
    memset(mem_index, 0, sizeof(mem_index));
    mem_index[0] = board_ini;

    copy_board(board_ini, board_src, len_sq);

    struct state_t state;

    state.status = EVOLVING;
    state.period = 1;

    size_t iter = 0;

    for (;;) {
        if (!quiet) {
            printf("\niteration %zu\n", iter);
            print_board(board_src, len);
        }

        board_dst = safer_malloc(len_sq);
        copy_board(board_dst, board_src, len_sq);

        eval_rule(board_dst, board_src, len);
        check_state(&state, board_dst, len_sq);

        if (state.status != EVOLVING) { break; }

        append_board(board_dst);

        swap_board(&board_src, &board_dst);

        ++iter;

        sleep(sleep_interval);
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    printf("finished at %d-%d-%d_%d:%d:%d\n",
           tm.tm_year + 1900, tm.tm_mon + 1,
           tm.tm_mday, tm.tm_hour, tm.tm_min,
           tm.tm_sec);

    switch (state.status) {
        case DEAD:
            printf("All life dead by %zu iterations\n", iter);
            break;
        case STATIC:
            printf("All life in stasis by %zu iterations\n", iter);
            break;
        case PERIODIC:
            printf("All life in periodicity of %zu\n by %zu iterations",
                   state.period, iter);
            break;
        case EVOLVING:
            printf("All life still evolving at %zu\n", iter);
            break;
    }

    printf("\ninitial board:\n");
    print_board(board_ini, len);

    printf("\nfinal board:\n");
    print_board(board_dst, len);


    return status;
}
