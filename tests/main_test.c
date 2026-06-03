#include <stdio.h>
#include "test_runner.h"

int g_passed = 0;
int g_failed = 0;

void suite_command(void);
void suite_city(void);
void suite_victory(void);
void suite_unit(void);

int main(void)
{
    printf("=== Tests Civilization ===\n");
    suite_command();
    suite_city();
    suite_victory();
    suite_unit();
    TEST_SUMMARY();
}
