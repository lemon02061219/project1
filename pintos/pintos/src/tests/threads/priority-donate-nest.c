/* Low-priority main thread L acquires lock A.  Medium-priority
   thread M then acquires lock B then blocks on acquiring lock A.
   High-priority thread H then blocks on acquiring lock B.  Thus,
   thread H donates its priority to M, which in turn donates it
   to thread L.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by Matt Franklin <startled@leland.stanford.edu>,
   Greg Hutchins <gmh@leland.stanford.edu>, Yu Ping Hu
   <yph@cs.stanford.edu>.  Modified by arens. */

/*注意， original_thread只获取了锁a， 它并不拥有锁b。

这里创建了一个locks的结构体装着2个锁a和b， 然后创建medium线程， 优先级为PRI_DEFAULT+1, 把这个locks作为线程medium执行函数的参数。

然后抢占调用medium_thread_func， 此时这个函数获取b这个锁， 此时medium成为锁b的拥有者， 并不阻塞， 继续执行， 然后medium在获取锁a的时候阻塞了。

此时original_thread继续跑， 它的优先级被medium提到了PRI_DEFAULT+1, 输出优先级Msg。

然后创建优先级为PRI_DEFAULT+2的high线程， 抢占调用high_thread_func， 然后这里high拿到了b这个锁， 而b的拥有者是medium， 阻塞， 注意， 这里medium被high捐赠了， 优先级到PRI_DEFAULT+2, 此时original_thread也应该一样提到同样优先级。

然后original_thread输出一下优先级msg之后释放掉锁a， 释放出发了medium_thread_func抢占调用， 输出此时优先级为PRI_DEFAULT+2， 然后medium释放掉a, 释放掉b， 释放b的时候被high_thread_func抢占， high输出完之后medium继续run， 输出两句之后再到original_thread， 输出两句msg完事。

按照这个逻辑， 它的希望输出也是和我们分析的过程一样：

 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-donate-nest) begin
 7 (priority-donate-nest) Low thread should have priority 32.  Actual priority: 32.
 8 (priority-donate-nest) Low thread should have priority 33.  Actual priority: 33.
 9 (priority-donate-nest) Medium thread should have priority 33.  Actual priority: 33.
10 (priority-donate-nest) Medium thread got the lock.
11 (priority-donate-nest) High thread got the lock.
12 (priority-donate-nest) High thread finished.
13 (priority-donate-nest) High thread should have just finished.
14 (priority-donate-nest) Middle thread finished.
15 (priority-donate-nest) Medium thread should just have finished.
16 (priority-donate-nest) Low thread should have priority 31.  Actual priority: 31.
17 (priority-donate-nest) end
18 EOF
19 pass;

这个测试是一个优先级嵌套问题， 重点在于medium拥有的锁被low阻塞， 在这个前提下high再去获取medium的说阻塞的话， 优先级提升具有连环效应， 就是medium被提升了， 此时它被锁捆绑的low线程应该跟着一起提升
从实现的角度来说， 我们线程又需要加一个数据结构， 我们需要获取这个线程被锁于哪个线程*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct locks
  {
    struct lock *a;
    struct lock *b;
  };

static thread_func medium_thread_func;
static thread_func high_thread_func;

void
test_priority_donate_nest (void)
{
  struct lock a, b;
  struct locks locks;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&a);
  lock_init (&b);

  lock_acquire (&a);

  locks.a = &a;
  locks.b = &b;
  thread_create ("medium", PRI_DEFAULT + 1, medium_thread_func, &locks);
  thread_yield ();
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 1, thread_get_priority ());  //第1

  thread_create ("high", PRI_DEFAULT + 2, high_thread_func, &b);
  thread_yield ();
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());  //第2

  lock_release (&a);
  thread_yield ();
  msg ("Medium thread should just have finished.");   //第9
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT, thread_get_priority ());    //第10
}

static void
medium_thread_func (void *locks_)
{
  struct locks *locks = locks_;

  lock_acquire (locks->b);
  lock_acquire (locks->a);

  msg ("Medium thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());  //第3
  msg ("Medium thread got the lock.");  //第4

  lock_release (locks->a);
  thread_yield ();

  lock_release (locks->b);
  thread_yield ();

  msg ("High thread should have just finished.");   //第7
  msg ("Middle thread finished.");    //第8
}

static void
high_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("High thread got the lock.");  //第5
  lock_release (lock);
  msg ("High thread finished.");  //第6
}