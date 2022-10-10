#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
enum
{
  TK_NOTYPE = 256,
  TK_EQ,          //等号
  TK_NOEQ,        //不等号
  TK_AND,         //与运算
  TK_OR,          //或运算
  TK_REG,         //寄存器
  TK_DEC,         //十进制数
  TK_HEX,         //十六进制数
  TK_DEREFERENCE, //解引用
  TK_POS,         //正号
  TK_NEG          //负号
  /* TODO: Add more token types */
};

static struct rule
{
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */
    {" +", TK_NOTYPE}, // spaces
    {"\\+", '+'},      // plus.
    {"\\-", '-'},
    {"\\*", '*'},
    {"/", '/'},
    {"\\(", '('},
    {"\\)", ')'},
    {"==", TK_EQ}, // equal
    {"!=", TK_NOEQ},
    {"&&", TK_AND},
    {"\\|\\|", TK_OR},
    {"\\$(\\$0|ra|sp|gp|tp|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", TK_REG},
    {"[0-9]+", TK_DEC},
    {"0[xX][0-9a-fA-F]", TK_HEX}};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() // 编译规则的正则表达式
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case TK_NOTYPE:
          break;
        case TK_DEC:
        case TK_HEX:
        case TK_REG:
          if (substr_len >= 32)
          {
            printf("Number too long!\n");
            assert(0);
          }
          tokens[nr_token].type = rules[i].token_type;
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          nr_token++;
          break;
        default:
          tokens[nr_token].type = rules[i].token_type;
          nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

// 检查表达式是否被成对的括号包括，返回值1代表是、0代表否、-1代表检查出错
uint32_t check_parentheses(int p, int q)
{
  int depth = 0;
  int result = 0;
  if (tokens[p].type == '(' && tokens[q].type == ')')
  {
    result = 1;
    for (int i = p + 1; i <= q - 1; i++)
    {
      if (depth < 0) // 检查最外侧括号是否匹配。
      {
        result = 0;
        break;
      }
      if (tokens[i].type == '(')
        depth++;
      else if (tokens[i].type == ')')
        depth--;
    }
  }
  depth = 0;
  for (int i = p; i <= q; i++)
  {
    if (depth < 0)
    {
      result = -1;
      break;
    }
    if (tokens[i].type == '(')
      depth++;
    else if (tokens[i].type == ')')
      depth--;
  }
  if (depth != 0)
    return -1;
  return result;
}

int op_level(int type)
{
  switch (type)
  {
  case TK_NEG:
  case TK_POS:
    return 1;
  case TK_DEREFERENCE:
    return 2;
  case '*':
  case '/':
    return 3;
  case '+':
  case '-':
    return 4;
  case TK_EQ:
  case TK_NOEQ:
    return 5;
  case TK_AND:
    return 6;
  case TK_OR:
    return 7;
  default:
    return 0;
  }
}

uint32_t findMainOp(int p, int q)
{
  uint32_t op = p;
  int depth = 0;
  int level = 0;
  for (int i = p; i <= q; i++)
  {
    printf("i=%d.\n", i);
    if (depth == 0)
    {
      if (tokens[i].type == '(')
      {
        depth++;
        continue;
      }
      else if (tokens[i].type == ')')
      {
        printf("Bad expression in [%d, %d] for unexpected '(' at %d", p, q, i);
        assert(0);
      }
      int new_level = op_level(tokens[i].type);
      if (new_level >= level)
      {
        op = i;
        level = new_level;
      }
    }
    else
    {
      if (tokens[i].type == '(')
        depth++;
      if (tokens[i].type == ')')
        depth--;
    }
  }
  if (depth != 0 || level == 0)
  {
    printf("Bad expression in [%d, %d].\n", p, q);
  }
  return op;
}

uint32_t eval(int p, int q, bool *success)
{
  if (p > q)
  {
    printf("Bad expression, p>q, p = %d, q = %d.\n", p, q);
    *success = false;
    assert(0);
  }
  else if (p == q)
  {
    if (tokens[p].type != TK_DEC && tokens[p].type != TK_HEX && tokens[p].type != TK_REG)
    {
      printf("Bad expression, a single no-num token.\n");
      *success = false;
      assert(0);
    }
    uint32_t value = 0;
    switch (tokens[p].type)
    {
    case TK_DEC:
      value = strtoul(tokens[p].str, NULL, 10);
      break;
    case TK_HEX:
      value = strtoul(tokens[p].str, NULL, 16);
      break;
    case TK_REG:
      value = isa_reg_str2val(&tokens[p].str[1], success);
      break;
    }
    return value;
  }
  int check = check_parentheses(p, q);
  printf("p=%d, q=%d.\n", p, q);
  if (check == 1) // 表达式被括号包裹的情形
  {
    return eval(p + 1, q - 1, success);
  }
  else if (check == -1)
  {
    printf("check parentheses error in [%d, %d].\n", p, q);
  }
  else //一般表达式的情形
  {
    uint32_t op = findMainOp(p, q); // 寻找主要运算符
    uint32_t val1 = 0;              //左操作数
    // 递归计算左右表达式的值
    if (tokens[p].type != TK_DEREFERENCE && tokens[p].type != TK_NEG && tokens[p].type != TK_POS)
      val1 = eval(p, op - 1, success);
    uint32_t val2 = eval(op + 1, q, success);
    switch (tokens[op].type)
    {
    case TK_EQ:
      return val1 == val2;
    case TK_NOEQ:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    case TK_DEREFERENCE:
      return vaddr_read(val2, 4);
    case TK_NEG:
      return -eval(op + 1, q, success);
    case TK_POS:
      return eval(op + 1, q, success);
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 == 0)
      {
        printf("An error occurred in [%d, %d]: divided by 0.\n", p, q);
        *success = false;
        assert(0);
      }
      else
        return val1 / val2;
    default:
      printf("Bad expression in [%d, %d].\n", p, q);
      *success = false;
      assert(0);
    }
  }
}

uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  // 分析表达式中解引用的项目
  for (int i = 0; i < nr_token; i++)
  {
    if (i == 0 || (tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != ')' && tokens[i - 1].type != TK_POS && tokens[i - 1].type != TK_NEG))
    {
      if (tokens[i].type == '*')
      {
        tokens[i].type = TK_DEREFERENCE;
      }
    }
  }
  // 分析表达式中+、-号表达加、减还是正、负
  for (int i = 0; i < nr_token; i++)
  {
    if (i == 0 || (tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != ')' && tokens[i - 1].type != TK_DEREFERENCE && tokens[i - 1].type != TK_POS && tokens[i - 1].type != TK_NEG))
    {
      switch (tokens[i].type)
      {
      case '+':
        tokens[i].type = TK_POS;
        break;
      case '-':
        tokens[i].type = TK_NEG;
        break;
      }
    }
  }
  printf("nr_token = %d.\n", nr_token);
  *success = true;
  return eval(0, nr_token - 1, success);
}
