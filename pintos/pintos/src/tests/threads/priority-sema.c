/* Tests that the highest-priority thread waiting on a semaphore
   is the first to wake up. */
/*这里创建10个线程阻塞于P操作， 然后用一个循环V操作， 然后看结果：

 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-sema) begin
 7 (priority-sema) Thread priority 30 woke up.
 8 (priority-sema) Back in main thread.
 9 (priority-sema) Thread priority 29 woke up.
10 (priority-sema) Back in main thread.
11 (priority-sema) Thread priority 28 woke up.
12 (priority-sema) Back in main thread.
13 (priority-sema) Thread priority 27 woke up.
14 (priority-sema) Back in main thread.
15 (priority-sema) Thread priority 26 woke up.
16 (priority-sema) Back in main thread.
17 (priority-sema) Thread priority 25 woke up.
18 (priority-sema) Back in main thread.
19 (priority-sema) Thread priority 24 woke up.
20 (priority-sema) Back in main thread.
21 (priority-sema) Thread priority 23 woke up.
22 (priority-sema) Back in main thread.
23 (priority-sema) Thread priority 22 woke up.
24 (priority-sema) Back in main thread.
25 (priority-sema) Thread priority 21 woke up.
26 (priority-sema) Back in main thread.
27 (priority-sema) end
28 EOF
29 pass;
好， 也就是说V唤醒的时候也是优先级高的先唤醒， 换句话说， 信号量的等待队列是优先级队列。
*/
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func priority_sema_thread;
static struct semaphore sema;

void
test_priority_sema (void)
{
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  sema_init (&sema, 0);
  thread_set_priority (PRI_MIN);
  for (i = 0; i < 10; i++)
    {
      int priority = PRI_DEFAULT - (i + 3) % 10 - 1;
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, priority_sema_thread, NULL);
    }

  for (i = 0; i < 10; i++)
    {
      sema_up (&sema);
      msg ("Back in main thread.");
    }
}

static void
priority_sema_thread (void *aux UNUSED)
{
  sema_down (&sema);
  msg ("Thread %s woke up.", thread_name ());
}
