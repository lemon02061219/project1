/* Tests that cond_signal() wakes up the highest-priority thread
   waiting in cond_wait(). */
/*分析： cond_wait和cond_signal就是释放掉锁， 等待signal唤醒， 然后再重新获取锁。

这里的代码逻辑是： 创建10个线程， 每个线程调用的时候获取锁， 然后调用cond_wait把锁释放阻塞于cond_signal唤醒， 然后连续10次循环调用cond_signal。

来看输出：

 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-condvar) begin
 7 (priority-condvar) Thread priority 23 starting.
 8 (priority-condvar) Thread priority 22 starting.
 9 (priority-condvar) Thread priority 21 starting.
10 (priority-condvar) Thread priority 30 starting.
11 (priority-condvar) Thread priority 29 starting.
12 (priority-condvar) Thread priority 28 starting.
13 (priority-condvar) Thread priority 27 starting.
14 (priority-condvar) Thread priority 26 starting.
15 (priority-condvar) Thread priority 25 starting.
16 (priority-condvar) Thread priority 24 starting.
17 (priority-condvar) Signaling...
18 (priority-condvar) Thread priority 30 woke up.
19 (priority-condvar) Signaling...
20 (priority-condvar) Thread priority 29 woke up.
21 (priority-condvar) Signaling...
22 (priority-condvar) Thread priority 28 woke up.
23 (priority-condvar) Signaling...
24 (priority-condvar) Thread priority 27 woke up.
25 (priority-condvar) Signaling...
26 (priority-condvar) Thread priority 26 woke up.
27 (priority-condvar) Signaling...
28 (priority-condvar) Thread priority 25 woke up.
29 (priority-condvar) Signaling...
30 (priority-condvar) Thread priority 24 woke up.
31 (priority-condvar) Signaling...
32 (priority-condvar) Thread priority 23 woke up.
33 (priority-condvar) Signaling...
34 (priority-condvar) Thread priority 22 woke up.
35 (priority-condvar) Signaling...
36 (priority-condvar) Thread priority 21 woke up.
37 (priority-condvar) end
38 EOF
39 pass;

从结果来看， 这里要求的实质就是： condition的waiters队列是优先级队列*/
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func priority_condvar_thread;
static struct lock lock;
// 这里condition里面装的是一个waiters队列， 看一下con_wait和cond_signal函数
static struct condition condition;

void
test_priority_condvar (void)
{
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  lock_init (&lock);
  cond_init (&condition);

  thread_set_priority (PRI_MIN);
  for (i = 0; i < 10; i++)
    {
      int priority = PRI_DEFAULT - (i + 7) % 10 - 1;
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, priority_condvar_thread, NULL);
    }

  for (i = 0; i < 10; i++)
    {
      lock_acquire (&lock);
      msg ("Signaling...");
      cond_signal (&condition, &lock);
      lock_release (&lock);
    }
}

static void
priority_condvar_thread (void *aux UNUSED)
{
  msg ("Thread %s starting.", thread_name ());
  lock_acquire (&lock);
  cond_wait (&condition, &lock);
  msg ("Thread %s woke up.", thread_name ());
  lock_release (&lock);
}
