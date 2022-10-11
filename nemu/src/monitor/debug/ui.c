#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
static int cmd_help(char *args);
static int cmd_c(char *args);
static int cmd_q(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_p(char *args);
static int cmd_x(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static struct
{
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* TODO: Add more commands */
    {"si", "si [N] - Run N steps and stop", cmd_si},
    {"info", "info SUBCMD - Print some info of program. SUBCMD = r(register)/w(watchpoint)", cmd_info},
    {"p", "p EXPR - Calculate the EXP", cmd_p},
    {"x", "x N EXPR - Scan N*4 bytes from address EXPR", cmd_x},
    {"w", "w EXPR - Set watchpoint ,stop the program when the value of EXPR changed", cmd_w},
    {"d", "d N - Delete watchpoint N", cmd_d}};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_si(char *args)
{
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
    cpu_exec(1);
  else
    cpu_exec(strtoul(arg, NULL, 10));
  return 0;
}

static int cmd_info(char *args)
{
  char *arg = strtok(NULL, " ");

  if (arg == NULL)
  {
    printf("No SUBCMD input. Use \"help\" for help.\n");
  }
  else if (strcmp(arg, "r") == 0)
    isa_reg_display();
  else if (strcmp(arg, "w") == 0)
    watchpoints_display();
  else
  {
    printf("SUBCMD error. Use \"help\" for help.\n");
  }
  return 0;
}

static int cmd_p(char *args)
{
  if (args == NULL)
  {
    printf("No EXPR provided. Use \"help\" for help\n");
    return 0;
  }
  bool success = true;
  uint32_t result = expr(args, &success);
  if (success)
  {
    printf("%s = %d(%#x)\n", args, result, result);
  }
  return 0;
}

static int cmd_x(char *args)
{
  char *argN = strtok(NULL, " ");
  char *argEXPR = strtok(NULL, " ");
  if (argN == NULL || argEXPR == NULL)
  {
    printf("No N or EXPR provided. Use \"help\" for help\n");
    return 0;
  }
  int N = atoi(argN);
  bool success;
  uint32_t EXPR = expr(argEXPR, &success);
  if (!success)
  {
    printf("EXPR = %s is error!\n", argEXPR);
    return 0;
  }
  int i;
  printf("paddr \t\t data\n");
  for (i = 0; i < N; i++)
  {
    /*TODO: paddr_read ? vaddr_read? I think it should be vaddr_read! */
    printf("%#10x \t\t %#10x \n", EXPR, vaddr_read(EXPR, 4)); /* len = 4  */
    EXPR += 4;
  }
  return 0;
}

static int cmd_w(char *args)
{
  if (args == NULL)
  {
    printf("No EXPR provided. Use \"help\" for help\n");
    return 0;
  }
  printf("%s", args);
  WP *wp = new_wp();
  strcpy(wp->exp, args);
  bool success = true;
  wp->old_value = expr(wp->exp, &success);
  if (!true)
  {
    return -1;
  }
  printf("Watchpoint %d is setted.\n", wp->NO);
  return 0;
}

static int cmd_d(char *args)
{
  char *arg = strtok(NULL, " ");

  if (arg == NULL)
  {
    printf("No checkpoint chosen. Use \"help\" for help.\n");
    return 0;
  }
  bool success = true;
  int n = expr(arg, &success);
  if (!success)
  {
    printf("exp wrong!\n");
  }
  free_wp(n);
  printf("Watchpoint %d is deleted\n", n);
  return 0;
}

void ui_mainloop(int is_batch_mode)
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
