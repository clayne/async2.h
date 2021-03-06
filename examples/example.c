#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "async2.h"

#define panic(message) (fputs(message, stderr), exit(EXIT_FAILURE))

struct f3_args {
    int res1;
    int res2;
};

struct f1_stack {
    int i;
    struct f3_args res;
    void *mem;
};

static async f4(s_astate state) {
    async_begin(state);
            puts("f4 call 1");
            async_yield;
            puts("f4 call 2");
    async_end;
}


static async f3(s_astate state) {
    int *i = state->locals;
    struct f3_args *res = state->args; /* Pointer assignment from locals or args is fine outside async_begin, but value assignment isn't. */
    async_begin(state);
            for (*i = 0; *i < 3; (*i)++) {
                printf("f3 %d\n", (*i) + 1);
                async_yield;
            }
            res->res1 = rand() % RAND_MAX;
            res->res2 = rand() % RAND_MAX;
    async_end;
}

static async f2(s_astate state) {
    async_begin(state);
            puts("f2 call");
    async_end;
}

static async f1(s_astate state) {
    struct f1_stack *locals = state->locals;
    char *text = state->args;

    async_begin(state);
            for (locals->i = 0; locals->i < 3; locals->i++) {
                printf("f0 %s %d\n", text, (locals->i) + 1);
                fawait(async_new(f3, &locals->res, int)) {
                    panic("f3 failed");
                } /* Create new coro from f3 and wait until it completes. */
                printf("Result: %d - %d\n", locals->res.res1, locals->res.res2);
            }
            fawait(async_vgather(2, async_new(f4, NULL, ASYNC_NONE), async_new(f4, NULL, ASYNC_NONE))) {
                panic("vgather failed");
            }
            fawait(async_sleep(0)) {
                panic("sleep failed");
            }
            fawait(async_wait_for(async_sleep(1000000), 0)) {
                assert(async_errno == ASYNC_ECANCELED);
                async_errno = 0;
            } else {
                panic("Error that should never happen");
            }

            locals->mem = async_alloc(512); /* This memory will be freed automatically after function end*/
    async_end;
}


int main(void) {
    struct async_event_loop *loop = async_get_event_loop();
    loop->init(); /* Init event loop and create some tasks to run them later. */
    async_create_task(async_new(f1, "first", struct f1_stack));
    async_create_task(async_new(f2, NULL, ASYNC_NONE));
    async_create_task(async_new(f1, "second", struct f1_stack));
    loop->run_forever(); /* Block execution and run all tasks. */
    loop->destroy();
    return 0;
}
