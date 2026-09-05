// test_main.c - the ONE translation unit make_host.py compiles per combo.
// Pulls in the shared bookkeeping plus each test group, then runs them all.
// NOT part of the firmware.
//
// Why #include the .c files instead of compiling them separately? Each combo
// needs its own binary (different -D flags), and the library header is
// file-static - compiling groups separately would duplicate the library
// state. One TU per combo keeps one library instance and one event log.
#include "test_core.c"
#include "test_resched.c"
#include "test_gate.c"

int main(void)
{
    run_core_tests();
    run_resched_tests();
    run_gate_tests();
    if (g_fail == 0)
        printf("ALL PASS\n");
    else
        printf("%d FAILURE(S)\n", g_fail);
    return (g_fail == 0) ? 0 : 1;
}
