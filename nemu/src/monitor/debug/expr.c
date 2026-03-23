#include "nemu.h"
#include <stdlib.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
    TK_NOTYPE = 256, TK_DEC, TK_HEX, TK_REG, TK_MINUS, TK_PTR, TK_NOT, TK_DIV,
    TK_MUL, TK_ADD, TK_SUB, TK_EQ, TK_NEQ, TK_AND, TK_OR
        /* TODO: Add more token types */

};

static struct rule {
    char *regex;
    int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},    // spaces
    {"\\+", TK_ADD},         // plus
    {"==", TK_EQ},         // equal
    {"!=",TK_NEQ},
    {"0[xX][0-9a-fA-F]+",TK_HEX},
    {"[1-9][0-9]*|0",TK_DEC}, //decimial
    {"\\-",TK_SUB},
    {"\\*",TK_MUL}, //mult or ptr
    {"\\/",TK_DIV},
    {"\\(",'('},
    {"\\)",')'},
    {"\\&\\&", TK_AND},
    {"\\|\\|", TK_OR},
    {"\\!", TK_NOT},
    {"\\$e[abcd]x",TK_REG},
    {"\\$e[bs]p",TK_REG},
    {"\\$e[sd]i",TK_REG},
    {"\\$eip",TK_REG}

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i ++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    int priority; // used to find dominant op
    char str[32]; //every token should be no more than 32 char
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i ++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                        i, rules[i].regex, position, substr_len, substr_len, substr_start);
                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */
                // skip spaces
                if(rules[i].token_type == TK_NOTYPE) break;
                Assert(nr_token<32,"Too many tokens!");
                //classify the token
                tokens[nr_token].type = rules[i].token_type;
                // recognize TK_PTR and TK_MINUS
                if(tokens[nr_token].type == TK_MUL && ( nr_token == 0 ||(tokens[nr_token-1].type != \
                                TK_DEC && tokens[nr_token].type!= TK_HEX && tokens[nr_token-1].type != TK_REG &&
                                tokens[nr_token-1].type != ')')))
                    tokens[nr_token].type = TK_PTR;
                if(tokens[nr_token].type == TK_SUB && ( nr_token == 0 ||(tokens[nr_token-1].type != \
                                TK_DEC && tokens[nr_token].type!= TK_HEX && tokens[nr_token-1].type != TK_REG &&
                                tokens[nr_token-1].type != ')')))
                    tokens[nr_token].type = TK_MINUS;
                tokens[nr_token].priority = tokens[nr_token].type;
                //type is ordered by priority!

                switch (rules[i].token_type) {
                    case TK_DEC: case TK_HEX: case TK_REG:
                        Assert(substr_len<32,"Length of numbers should be no more than 31!\n");
                        //KISS protocol
                        strncpy(tokens[nr_token].str,substr_start,substr_len); //copy the string
                        tokens[nr_token].priority =-1; // priority for dec hex and reg
                        break;
                    default: break;
                }
                nr_token++;


                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

// 声明外部函数，用于读取内存和获取寄存器的值
// extern uint32_t vaddr_read(vaddr_t, int);
extern uint32_t isa_reg_str2val(const char *s, bool *success);

// 1. 检查表达式是否被一对匹配的最外层括号完全包裹
static bool check_parentheses(int p, int q) {
    if (tokens[p].type != '(' || tokens[q].type != ')') return false;
    int par = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(') par++;
        else if (tokens[i].type == ')') par--;
        // 如果在到达 q 之前 par 就提前降到了 0，说明最外层不是一对匹配的括号
        // 比如 (1+2)*(3+4)
        if (par == 0 && i < q) return false; 
    }
    return par == 0;
}

// 2. 寻找主运算符 (Dominant Operator)
static int dominant_op(int p, int q) {
    int level = 0;
    int op = -1;
    int min_prec = 0; // 记录最低的优先级 (数值越小优先级越低)

    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(') level++;
        else if (tokens[i].type == ')') level--;
        else if (level == 0) { // 只考虑不在括号内的运算符
            // 忽略操作数
            if (tokens[i].type == TK_DEC || tokens[i].type == TK_HEX || tokens[i].type == TK_REG) continue;

            int prec = 0;
            switch (tokens[i].type) {
                case TK_OR: prec = 1; break;
                case TK_AND: prec = 2; break;
                case TK_EQ: case TK_NEQ: prec = 3; break;
                case TK_ADD: case TK_SUB: prec = 4; break;
                case TK_MUL: case TK_DIV: prec = 5; break;
                case TK_NOT: case TK_MINUS: case TK_PTR: prec = 6; break;
            }

            if (prec > 0) {
                // 双目运算符（左结合）：如果有多个同级最低的，选最右边的
                // 单目运算符（右结合，prec == 6）：如果有多个，选最左边的
                if (op == -1 || prec < min_prec || (prec == min_prec && prec != 6)) {
                    min_prec = prec;
                    op = i;
                }
            }
        }
    }
    return op;
}

// 3. 递归求值核心函数
static uint32_t eval(int p, int q, bool *success) {
    if (!(*success) || p > q) {
        *success = false;
        return 0;
    }

    // Base case: 单个操作数
    if (p == q) {
        uint32_t val = 0;
        if (tokens[p].type == TK_DEC) val = atoi(tokens[p].str);
        else if (tokens[p].type == TK_HEX) sscanf(tokens[p].str, "%x", &val);
        else if (tokens[p].type == TK_REG) {
            // tokens[p].str 包含了 '$'，传入时通过 +1 跳过 '$' 符号
            val = isa_reg_str2val(tokens[p].str + 1, success);
        }
        return val;
    }

    // 去除最外层匹配的括号，进入内部求值
    if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1, success);
    }

    // 找到主运算符
    int op = dominant_op(p, q);
    if (op == -1) { *success = false; return 0; }

    // 处理单目运算符 (单目运算符只需递归计算右半部分)
    if (tokens[op].type == TK_NOT || tokens[op].type == TK_MINUS || tokens[op].type == TK_PTR) {
        uint32_t val2 = eval(op + 1, q, success);
        switch (tokens[op].type) {
            case TK_NOT: return !val2;
            case TK_MINUS: return -val2;
            case TK_PTR: return vaddr_read(val2, 4);
        }
    }

    // 处理双目运算符 (递归计算左右两半部分)
    uint32_t val1 = eval(p, op - 1, success);
    uint32_t val2 = eval(op + 1, q, success);

    switch (tokens[op].type) {
        case TK_ADD: return val1 + val2;
        case TK_SUB: return val1 - val2;
        case TK_MUL: return val1 * val2;
        case TK_DIV: 
                     if (val2 == 0) { 
                         printf("Error: Division by zero\n"); 
                         *success = false; 
                         return 0; 
                     }
                     return val1 / val2;
        case TK_EQ: return val1 == val2;
        case TK_NEQ: return val1 != val2;
        case TK_AND: return val1 && val2;
        case TK_OR: return val1 || val2;
        default: *success = false; return 0;
    }
}

uint32_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    // 提前进行全局括号数量平衡检查，防止死递归
    int par = 0; //检查括号个数和位置是否匹配
    for(int i =0; i < nr_token; i++){
        if(tokens[i].type == '(') par++;
        else if(tokens[i].type == ')') par--;
        if(par < 0 ) break; // e.g. (1+2)))((+4
    }
    if(par != 0 ){
        *success = false;
        printf("Unmatched Parentheses!\n");
        return 0;
    }

    *success = true;
    return eval(0, nr_token - 1, success);
    /* TODO: Insert codes to evaluate the expression. */

}
