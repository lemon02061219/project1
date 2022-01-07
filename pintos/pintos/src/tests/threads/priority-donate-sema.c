/* Low priority thread L acquires a lock, then blocks downing a
   semaphore.  Medium priority thread M then blocks waiting on
   the same semaphore.  Next, high priority thread H attempts to
   acquire the lock, donating its priority to L.

   Next, the main thread ups the semaphore, waking up L.  L
   releases the lock, which wakes up H.  H "up"s the semaphore,
   waking up M.  H terminates, then M, then L, and finally the
   main thread.

   Written by Godmar Back <gback@cs.vt.edu>. */

/*lock_and_sema是包含一个锁和一个信号量的结构体， 初始化信号量为0, 然后创建PRI_DEFAULT+1的线程low, 获取ls内的锁成为拥有者， 然后sema_down（P）阻塞。 

然后创建PRI_DEFAULT+3的线程med， 这里也直接调用sema_down(P)阻塞了。

最后创建PRI_DEFAULT+5的线程high, 这里获取锁， 阻塞。

然后回到original_thread， 调用V操作， 此时唤醒了l_thread_func， 因为low被high捐献了优先级高于med， 然后l_thread_func跑， 释放掉了锁。

此时直接触发h_thread_func， 输出， 然后V操作， 释放掉锁， 由于优先级最高所以V操作之后不会被抢占， 这个函数跑完之后m_thread_func开始跑（被V唤醒）， 然后输出一句完事， 再到l_thread_func中输出最后一句回到original_thread。

这里包含了信号量和锁混合触发， 实际上还是信号量在起作用， 因为锁是由信号量实现的。

所以输出是这样的：
 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-donate-sema) begin
 7 (priority-donate-sema) Thread L acquired lock.
 8 (priority-donate-sema) Thread L downed semaphore.
 9 (priority-donate-sema) Thread H acquired lock.
10 (priority-donate-sema) Thread H finished.
11 (priority-donate-sema) Thread M finished.
12 (priority-donate-sema) Thread L finished.
13 (priority-donate-sema) Main thread finished.
14 (priority-donate-sema) end
15 EOF
16 pass;*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct lock_and_sema
  {
    struct lock lock;
    struct semaphore sema;
  };

static thread_func l_thread_func;
static thread_func m_thread_func;
static thread_func h_thread_func;

void
test_priority_donate_sema (void)
{
  struct lock_and_sema ls;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&ls.lock);
  sema_init (&ls.sema, 0);
  thread_create ("low", PRI_DEFAULT + 1, l_thread_func, &ls);
  thread_create ("med", PRI_DEFAULT + 3, m_thread_func, &ls);
  thread_create ("high", PRI_DEFAULT + 5, h_thread_func, &ls);
  sema_up (&ls.sema);
  msg ("Main thread finished.");  //第7
}

static void
l_thread_func (void *ls_)
{
  struct lock_and_sema *ls = ls_;

  lock_acquire (&ls->lock);
  msg ("Thread L acquired lock.");  //第1
  sema_down (&ls->sema);
  msg ("Thread L downed semaphore.");  //第2
  lock_release (&ls->lock);
  msg ("Thread L finished.");  //第6
}

static void
m_thread_func (void *ls_)
{
  struct lock_and_sema *ls = ls_;

  sema_down (&ls->sema);
  msg ("Thread M finished.");  //第5
}

static void
h_thread_func (void *ls_)
{
  struct lock_and_sema *ls = ls_;

  lock_acquire (&ls->lock);
  msg ("Thread H acquired lock.");  //第3

  sema_up (&ls->sema);
  lock_release (&ls->lock);
  msg ("Thread H finished.");  //第4
}
