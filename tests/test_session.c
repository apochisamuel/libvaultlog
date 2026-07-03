#include "tm/session.h"
#include <stdio.h>
#include <assert.h>

static void test_create_destroy(void)
{
    tm_session_t *s = tm_session_create(1);
    assert(s != NULL);
    assert(!s->active);
    tm_session_destroy(s);
    printf("  create/destroy: PASS\n");
}

static void test_multiple_sessions(void)
{
    tm_session_t *s1 = tm_session_create(1);
    tm_session_t *s2 = tm_session_create(2);
    assert(s1 != s2);
    assert(s1->session_id == 1);
    assert(s2->session_id == 2);
    tm_session_destroy(s1);
    tm_session_destroy(s2);
    printf("  multiple sessions: PASS\n");
}

int main(void)
{
    printf("Session tests:\n");
    test_create_destroy();
    test_multiple_sessions();
    printf("All session tests passed.\n");
    return 0;
}
