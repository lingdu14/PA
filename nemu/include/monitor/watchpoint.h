#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[32];
  uint32_t val;

  /* TODO: Add more members if necessary */


} WP;

/* 初始化监视点池，在系统启动时调用一次 */
void init_wp_pool(void);

/* 创建一个新的监视点，返回其指针 */
WP* new_wp(void);

/* 释放指定编号的监视点，成功返回 true，失败返回 false */
bool free_wp(int N);

/* 打印当前所有活动的监视点（编号、表达式、当前值） */
void show_wp(void);

bool wp_changed();

#endif
