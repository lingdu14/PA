#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

extern void isa_reg_display();
extern void show_wp();

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args) {
    return -1;
}

static int cmd_si(char *args) {
    // 提取参数：如果没有参数，默认步数为 1
    uint64_t n = 1; 
    if (args != NULL) {
        // 尝试读取一个无符号长整型，如果输入类似 "-1" 这种会被 sscanf 正常识别或者你可以加判断
        if (sscanf(args, "%llu", &n) != 1) {
            printf("Invalid argument. Usage: si [N]\n");
            return 0;
        }
    }

    cpu_exec(n);
    printf("Successfully run %llu instructions!\n", n); // 这一句其实可以不加，cpu_exec 内部如果有需要会自己打印
    return 0;
}

static int cmd_info(char *args) {
    if (args == NULL) {
        printf("Illegal number of parameters.\n");
        printf("Usage: info r (print registers) | info w (print watchpoints)\n");
        return 0;
    }

    if (strcmp(args, "r") == 0) {
        isa_reg_display(); // 调用 CPU 模块提供的打印接口
    } 
    else if (strcmp(args, "w") == 0) {
        // show_wp();         // 调用 watchpoint 模块提供的打印接口
    } 
    else {
        printf("Unknown argument '%s'.\n", args);
    }
    return 0;
}

static int cmd_x(char *args){
    if(args == NULL){
        printf("Illegal parameters. Usage: x N EXPR\n");
        return 0;
    }

    // 1. 提取第一个参数 N (扫描长度)
    // 这里使用 strtok 提取第一个数字。
    // 注意：要传入 args 而不是 NULL，来初始化 strtok 的上下文。
    char *arg_n = strtok(args, " ");
    if(arg_n == NULL){
        printf("Missing length parameter.\n");
        return 0;
    }
    int N = atoi(arg_n); 

    // 2. 获取剩余的所有字符串作为表达式 (EXPR)
    // strtok 会把找到的空格变成 '\0'。
    // 所以 arg_n 的末尾之后，就是剩余字符串的起点。
    // 用 arg_n 的地址加上它的长度，再加 1（跳过那个被变成 '\0' 的空格）。
    char *arg_expr = arg_n + strlen(arg_n) + 1; 

    // 检查是否真的有表达式输入
    // 这里需要注意，有可能用户输入了 "x 3" 后面加了一大堆空格但没有表达式
    // 跳过开头的多余空格
    while (*arg_expr == ' ') {
        arg_expr++;
    }

    if(*arg_expr == '\0'){
        printf("Missing expression.\n");
        return 0;
    }

    // 3. 将整个表达式字符串交给 expr 求值
    bool succ = true;
    vaddr_t addr = expr(arg_expr, &succ);

    if(!succ)
    {
        printf("Invalid Expression!\n");
        return 0; // 注意：NEMU 的命令处理函数通常返回 0 表示继续，返回 -1 表示退出
    }

    // 4. 打印内存内容
    printf("Bytes : \tLow ===> High\n");
    for (int i = 0; i < N; i++){
        uint32_t data = vaddr_read(addr + 4 * i, 4);
        printf("0x%08x :\t", addr + 4 * i);

        // 按字节打印，处理小端序显示
        for(int j = 0; j < 4; j++){
            printf("%02x ", data & 0xff);
            data = data >> 8 ;
        }
        printf("\n");
    }

    return 0;
}

static int cmd_p(char *args) {
    if (args == NULL) {
        printf("Usage: p EXPR\n");
        return 0;
    }

    bool success = true;
    // 调用在 expr.c 中写好的 expr() 函数
    uint32_t res = expr(args, &success);

    if (success) {
        // 打印计算结果，同时显示十进制和十六进制
        printf("%d (0x%08x)\n", res, res);
    } else {
        // 提示的错误信息
        printf("Evaluation failed.\n");
    }
    return 0;
}

static int cmd_help(char *args);

static struct {
    char *name;
    char *description;
    int (*handler) (char *);
} cmd_table [] = {
    { "help", "Display informations about all supported commands", cmd_help },
    { "c", "Continue the execution of the program", cmd_c },
    { "q", "Exit NEMU", cmd_q },
    { "si", "Step one instruction exactly (or N instructions if specified: si [N])", cmd_si },
    { "info", "Print status: info r (print registers) | info w (print watchpoints)", cmd_info },
    { "x", "Scan memory: x N EXPR", cmd_x },
    { "p", "Print expression: p EXPR", cmd_p },

    /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i ++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else {
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

void ui_mainloop(int is_batch_mode) {
    if (is_batch_mode) {
        cmd_c(NULL);
        return;
    }

    while (1) {
        char *str = rl_gets();
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) { continue; }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef HAS_IOE
        extern void sdl_clear_event_queue(void);
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) { return; }
                break;
            }
        }

        if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
    }
}
