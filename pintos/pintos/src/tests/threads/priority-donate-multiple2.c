/* The main thread acquires locks A and B, then it creates three
   higher-priority threads.  The first two of these threads block
   acquiring one of the locks and thus donate their priority to
   the main thread.  The main thread releases the locks in turn
   and relinquishes its donated priorities, allowing the third thread
   to run.

   In this test, the main thread releases the locks in a different
   order compared to priority-donate-multiple.c.

   Written by Godmar Back <gback@cs.vt.edu>.
   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by Matt Franklin <startled@leland.stanford.edu>,
   Greg Hutchins <gmh@leland.stanford.edu>, Yu Ping Hu
   <yph@cs.stanford.edu>.  Modified by arens. */

/*有了之前的分析这里简单说一下： original_thread拥有2个锁， 然后创建PRI_DEFAULT+3的线程a去拿a这个锁， PRI_DEFAULT+5的线程b去拿b这个锁， 中间创建了一个PRI_DEFAULT+1的c线程， 但是因为创建的时候当前线程的优先级已经被a线程捐赠了所以抢占调度并没有发生。 然后分别释放掉a和b， 释放a的时候线程a被唤醒， 但是优先级依然不如当前线程， 此时当前线程优先级仍然被b捐赠着， 优先级最高继续执行， 然后释放掉b， 释放掉b之后，original_thread的优先级降到初始，应该最后被调用， 线程b抢占调度， 然后线程a， 再是线程c， 最后才original_thread输出msg。

于是就有了以下的输出断言：

复制代码
 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-donate-multiple2) begin
 7 (priority-donate-multiple2) Main thread should have priority 34.  Actual priority: 34.
 8 (priority-donate-multiple2) Main thread should have priority 36.  Actual priority: 36.
 9 (priority-donate-multiple2) Main thread should have priority 36.  Actual priority: 36.
10 (priority-donate-multiple2) Thread b acquired lock b.
11 (priority-donate-multiple2) Thread b finished.
12 (priority-donate-multiple2) Thread a acquired lock a.
13 (priority-donate-multiple2) Thread a finished.
14 (priority-donate-multiple2) Thread c finished.
15 (priority-donate-multiple2) Threads b, a, c should have just finished, in that order.
16 (priority-donate-multiple2) Main thread should have priority 31.  Actual priority: 31.
17 (priority-donate-multiple2) end
18 EOF
19 pass;
这里依然测试的是多锁情况下优先级逻辑的正确性*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func a_thread_func;
static thread_func b_thread_func;
static thread_func c_thread_func;

void
test_priority_donate_multiple2 (void)
{
  struct lock a, b;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&a);
  lock_init (&b);

  lock_acquire (&a);
  lock_acquire (&b);

  thread_create ("a", PRI_DEFAULT + 3, a_thread_func, &a);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 3, thread_get_priority ());  //第1

  thread_create ("c", PRI_DEFAULT + 1, c_thread_func, NULL);

  thread_create ("b", PRI_DEFAULT + 5, b_thread_func, &b);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 5, thread_get_priority ());  //第2

  lock_release (&a);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 5, thread_get_priority ());  //第3

  lock_release (&b);
  msg ("Threads b, a, c should have just finished, in that order.");  //第9
  msg ("Main thread should have priority %d.  Actual priority: %d.",  //第10
       PRI_DEFAULT, thread_get_priority ());
}

static void
a_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("Thread a acquired lock a.");  //第6
  lock_release (lock);
  msg ("Thread a finished.");  //第7
}

static void
b_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("Thread b acquired lock b.");  //第4
  lock_release (lock);
  msg ("Thread b finished.");  //第5
}

static void
c_thread_func (void *a_ UNUSED)
{
  msg ("Thread c finished.");  //第8
}
