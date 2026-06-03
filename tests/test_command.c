#include "test_runner.h"
#include "command/command.h"

static void t_parse_move(void)
{
    Command c = command_parse("move 3 26 25");
    ASSERT_STR_EQ(c.verb, "move");
    ASSERT_EQ(c.args[0], 3);
    ASSERT_EQ(c.args[1], 26);
    ASSERT_EQ(c.args[2], 25);
}

static void t_parse_build(void)
{
    Command c = command_parse("build 0 unit 1");
    ASSERT_STR_EQ(c.verb, "build");
    ASSERT_EQ(c.args[0], 0);
    ASSERT_STR_EQ(c.str_arg, "unit");
    ASSERT_EQ(c.args[1], 1);
}

static void t_parse_attack(void)
{
    Command c = command_parse("attack 2 7");
    ASSERT_STR_EQ(c.verb, "attack");
    ASSERT_EQ(c.args[0], 2);
    ASSERT_EQ(c.args[1], 7);
}

static void t_parse_found(void)
{
    Command c = command_parse("create_city Paris");
    ASSERT_STR_EQ(c.verb, "create_city");
    ASSERT_STR_EQ(c.str_arg, "Paris");
}

static void t_parse_research(void)
{
    Command c = command_parse("research Agriculture");
    ASSERT_STR_EQ(c.verb, "research");
    ASSERT_STR_EQ(c.str_arg, "Agriculture");
}

static void t_parse_empty(void)
{
    Command c = command_parse("");
    ASSERT_EQ(c.verb[0], '\0');
}

static void t_parse_build_incomplete_no_type(void)
{
    // "build 0" — args[0]=0, str_arg vide, args[1]=NO_ID
    Command c = command_parse("build 0");
    ASSERT_STR_EQ(c.verb, "build");
    ASSERT_EQ(c.args[0], 0);
    ASSERT_EQ(c.str_arg[0], '\0');
    ASSERT_EQ(c.args[1], NO_ID);
}

static void t_parse_build_incomplete_no_id(void)
{
    // "build 0 unit" — args[1] absent = NO_ID
    Command c = command_parse("build 0 unit");
    ASSERT_EQ(c.args[0], 0);
    ASSERT_STR_EQ(c.str_arg, "unit");
    ASSERT_EQ(c.args[1], NO_ID);
}

void suite_command(void)
{
    SUITE("Parsing des commandes");
    RUN_TEST("move 3 26 25",                     t_parse_move);
    RUN_TEST("build 0 unit 1",                   t_parse_build);
    RUN_TEST("attack 2 7",                       t_parse_attack);
    RUN_TEST("found Paris",                      t_parse_found);
    RUN_TEST("research Agriculture",             t_parse_research);
    RUN_TEST("commande vide",                    t_parse_empty);
    RUN_TEST("build incomplet (pas de type)",    t_parse_build_incomplete_no_type);
    RUN_TEST("build incomplet (pas d'id)",       t_parse_build_incomplete_no_id);
}
