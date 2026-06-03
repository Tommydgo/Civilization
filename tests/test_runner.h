#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <string.h>

extern int g_passed;
extern int g_failed;

#define ASSERT_EQ(got, expected) do { \
    if ((got) == (expected)) { \
        g_passed++; \
    } else { \
        printf("  FAIL %s:%d  attendu=%d  obtenu=%d\n", \
            __FILE__, __LINE__, (int)(expected), (int)(got)); \
        g_failed++; \
    } \
} while (0)

#define ASSERT_TRUE(expr) do { \
    if (expr) { \
        g_passed++; \
    } else { \
        printf("  FAIL %s:%d  expression fausse: %s\n", \
            __FILE__, __LINE__, #expr); \
        g_failed++; \
    } \
} while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_STR_EQ(got, expected) do { \
    if (strcmp((got), (expected)) == 0) { \
        g_passed++; \
    } else { \
        printf("  FAIL %s:%d  attendu=\"%s\"  obtenu=\"%s\"\n", \
            __FILE__, __LINE__, (expected), (got)); \
        g_failed++; \
    } \
} while (0)

#define RUN_TEST(name, fn) do { \
    printf("  %-50s", name); \
    int before = g_failed; \
    fn(); \
    printf("%s\n", g_failed == before ? "OK" : ""); \
} while (0)

#define SUITE(name) printf("\n=== %s ===\n", name)

#define TEST_SUMMARY() do { \
    printf("\n%d passes, %d echecs\n", g_passed, g_failed); \
    return (g_failed > 0) ? 1 : 0; \
} while (0)

#endif
