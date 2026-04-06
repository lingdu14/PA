#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i ++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

/* 创建一个新的监视点，从空闲链表中取出一个节点，插入到活动链表头部 */
WP* new_wp() {
    Assert(free_ != NULL, "Watch points exceeded maximum!\n");

    WP *wp = free_;          // 获取空闲链表头节点
    free_ = free_->next;     // 空闲链表头指针后移

    wp->next = head;         // 新节点指向当前活动链表头
    head = wp;               // 活动链表头更新为新节点

    return wp;
}

/* 释放指定编号的监视点：从活动链表中删除，并回收到空闲链表头部 */
bool free_wp(int N) {

    if (head == NULL) {
        printf("No watchpoints to free!\n");
        return false;
    }

    WP *wp = head;
    WP *prev = NULL;

    // 遍历活动链表，查找编号为 N 的监视点
    while (wp != NULL && wp->NO != N) {
        prev = wp;
        wp = wp->next;
    }

    if (wp == NULL) {        // 未找到对应编号
        printf("Invalid NO!\n");
        return false;
    }

    // 从活动链表中摘除 wp 节点
    if (prev == NULL) {      // 要删除的是头节点
        head = wp->next;
    } else {
        prev->next = wp->next;
    }

    // 将 wp 节点插入空闲链表头部
    wp->next = free_;
    free_ = wp;

    // 重置该监视点的表达式和值，避免残留数据影响后续使用
    memset(wp->expr, 0, sizeof(char) * 32);
    wp->val = 0;

    return true;
}

void show_wp(){
    if(head == NULL){
        printf("Empty watch points!\n");
        return;
    }
    printf("NO\t\tExpr\t\tVal\n");
    WP* wp = head;
    while(wp != NULL){
        printf("%2d\t\t%s\t\t0x%x(%u)\n",wp->NO,wp->expr,wp->val,wp->val);
        wp = wp->next;
    }
}

bool wp_changed() {
    WP* wp = head;
    bool flag = false;

    while(wp != NULL) {
        bool succ = true;
        uint32_t curr_val = expr(wp->expr, &succ);

        // 保护：如果求值失败，直接跳过或者报错
        if (!succ) {
            printf("Error: Evaluation failed for watchpoint %d\n", wp->NO);
            wp = wp->next;
            continue;
        }

        if(curr_val != wp->val) { // 注意:WP 结构体里定义的变量名是 val
            if(!flag) {
                printf("Reached watchpoints:\n");
            }
            // 用 0x%08x 格式额外打印一下十六进制，方便后续 debug
            printf("Watchpoint %d: %s \n  Old value: %u (0x%08x)\n  New value: %u (0x%08x)\n", 
                    wp->NO, wp->expr, wp->val, wp->val, curr_val, curr_val);

            wp->val = curr_val;
            flag = true;
        }
        wp = wp->next;
    }
    return flag;
}
