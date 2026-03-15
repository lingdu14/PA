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

static int cmd_x(char *args) {
    if (args == NULL) {
        printf("Usage: x N EXPR\n");
        return 0;
    }

    // 1. 提取 N 的值
    // 第一次调用 strtok，传入 args
    char *n_str = strtok(args, " ");
    if (n_str == NULL) {
        printf("Usage: x N EXPR\n");
        return 0;
    }
    int N = atoi(n_str);

    // 2. 提取表达式 EXPR
    // 注意：这里的分隔符是一个空字符串 ""，或者 "\n"
    // 它的作用是不再用空格分割，而是直接把后面剩余的整个字符串全部提取出来
    // 这样才能正确处理带有空格的表达式，比如 "x 10 $eax + 4"
    char *expr_str = strtok(NULL, ""); 
    if (expr_str == NULL) {
        printf("Usage: x N EXPR\n");
        return 0;
    }

    // 3. 计算表达式的值
    bool succ = true;
    vaddr_t addr = expr(expr_str, &succ);
    if (!succ) {
        printf("Invalid Expression!\n");
        return 0; // 解析失败也返回0，让程序继续等待下一条命令
    }

    // 4. 打印内存 (vaddr_t 等价于 uint32_t)
    printf("Address     \tDword Value\tBytes (Low ===> High)\n");
    for (int i = 0; i < N; i++) {
        uint32_t data = vaddr_read(addr + 4 * i, 4);

        // 先打印地址和完整的 32 位值
        printf("0x%08x:\t0x%08x\t", addr + 4 * i, data);

        // 保留你原本优秀的按字节打印逻辑（小端序展示）
        uint32_t temp = data;
        for (int j = 0; j < 4; j++) {
            printf("%02x ", temp & 0xff);
            temp = temp >> 8;
        }
        printf("\n");
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
