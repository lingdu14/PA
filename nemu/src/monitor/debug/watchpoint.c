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

/* 申请一个新的监视点节点 */
WP* new_wp() {
    /* 如果 free_ 链表为空，说明 32 个监视点已经全部用完 */
    Assert(free_ != NULL, "Error: Watchpoints exceeded maximum (32)!\n");

    /* 从 free_ 链表头部摘取一个空闲节点 */
    WP *wp = free_;
    free_ = free_->next;

    /* 将获取到的节点插入到 head 链表的头部 (头插法) */
    wp->next = head;
    head = wp;

    return wp;
}

/* 释放一个指定编号的监视点节点，并清空其数据 */
bool free_wp(int N) {
    if (head == NULL) {
        printf("No watchpoints to free!\n");
        return false;
    }

    WP *wp = head;
    WP *prec = NULL;

    /* 遍历 head 链表，寻找编号为 N 的监视点 */
    while (wp != NULL && wp->NO != N) {
        prec = wp;
        wp = wp->next;
    }

    /* 如果遍历到末尾仍未找到对应的节点 */
    if (wp == NULL) {
        printf("Watchpoint %d not found!\n", N);
        return false;
    }

    /* 找到了目标节点，将其从 head 链表中脱离 */
    if (prec != NULL) {
        // 目标节点在链表中间或尾部
        prec->next = wp->next; 
    } else {
        // 目标节点正好是 head 链表的第一个节点
        head = wp->next;       
    }

    /* 将脱离下来的节点插回 free_ 链表的头部，完成回收 */
    wp->next = free_;
    free_ = wp;

    /* 清理该节点残留的历史数据，避免脏数据干扰下一次复用 */
    memset(wp->expr, 0, sizeof(wp->expr));
    wp->old_val = 0;

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
