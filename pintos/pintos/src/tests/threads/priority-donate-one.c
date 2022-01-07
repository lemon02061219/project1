/* The main thread acquires a lock.  Then it creates two
   higher-priority threads that block acquiring the lock, causing
   them to donate their priorities to the main thread.  When the
   main thread releases the lock, the other threads should
   acquire it in priority order.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by Matt Franklin <startled@leland.stanford.edu>,
   Greg Hutchins <gmh@leland.stanford.edu>, Yu Ping Hu
   <yph@cs.stanford.edu>.  Modified by arens. */

/* 首先当前线程（称为original_thread）是一个优先级为PRI_DEFAULT的线程， 然后第4行创建了一个锁， 接着创建一个线程acquire1，优先级为PRI_DEFAULT+1, 传了一个参数为这个锁的函数过去（线程acquire1执行的时候会调用）。
我们之前实现的抢占式调度会让acquire1马上执行。
注意， 这里acquire1_thread_func阻塞了， msg这个时候并不会输出， 这时会继续执行original_thread, 然后输出msg， 输出当前线程应该的优先级和实际的优先级。

然后继续创建一个线程acquire2, 优先级为PRI_DEFAULT+2， 这里调用和acquire1一致， 然后original_thread继续输出msg。

好， 然后original_thread释放了这个锁（V操作）， 释放的过程会触发被锁着的线程acquire1, acquire2， 然后根据优先级调度， 先执行acquire2, 再acquire1, 最后再执行original_thread。

那么这里应该是acquire2, acquire1分别释放锁然后输出msg， 最后original_thread再输出msg。

好， 我们已经把这个测试程序分析完了， 我们来看它希望的输出：

 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-donate-one) begin
 7 (priority-donate-one) This thread should have priority 32.  Actual priority: 32.
 8 (priority-donate-one) This thread should have priority 33.  Actual priority: 33.
 9 (priority-donate-one) acquire2: got the lock
10 (priority-donate-one) acquire2: done
11 (priority-donate-one) acquire1: got the lock
12 (priority-donate-one) acquire1: done
13 (priority-donate-one) acquire2, acquire1 must already have finished, in that order.
14 (priority-donate-one) This should be the last line before finishing this test.
15 (priority-donate-one) end
16 EOF
17 pass;

输出行为和我们分析的一致， 来看7,8行， original_thread的优先级分别变成了PRI_DEFAULT+1和PRI_DEFAULT+2。

我们来根据这个结果分析一下优先级捐赠行为：

original_thread拥有的锁被acquire1获取之后， 因为acquire1线程被阻塞于这个锁， 那么acquire1的执行必须要original_thread继续执行释放这个锁， 从优先级的角度来说， original_thread的优先级应该提升到acquire1的优先级，

因为original_thread本身的执行包含了acquire1执行的阻塞， 所以此时acquire1对original_thread做了捐赠， 优先级提到PRI_DEFAULT+1， acquire2行为类似。

好， 支持priority-donate-one分析结束， 我们来分析一下实现：

具体行为肯定是被锁定在了锁的获取和释放上了， 我们的实现思路是：

在一个线程获取一个锁的时候， 如果拥有这个锁的线程优先级比自己低就提高它的优先级，然后在这个线程释放掉这个锁之后把原来拥有这个锁的线程改回原来的优先级。

 

好， 这里先不急着写代码， 继续来分析其他测试， 全部分析完了再写代码， 因为这些测试都是优先级捐献相关的行为， 我们全部分析完了再写就避免了走弯路了。
*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func acquire1_thread_func;
static thread_func acquire2_thread_func;

void
test_priority_donate_one (void)
{
  struct lock lock;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&lock);
  lock_acquire (&lock);
  thread_create ("acquire1", PRI_DEFAULT + 1, acquire1_thread_func, &lock);
  msg ("This thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 1, thread_get_priority ());  // 第1
  thread_create ("acquire2", PRI_DEFAULT + 2, acquire2_thread_func, &lock);
  msg ("This thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());  // 第2
  lock_release (&lock);
  msg ("acquire2, acquire1 must already have finished, in that order.");  // 第7
  msg ("This should be the last line before finishing this test."); // 第8
}

static void
acquire1_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("acquire1: got the lock"); // 第5
  lock_release (lock);
  msg ("acquire1: done"); // 第6
}

static void
acquire2_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("acquire2: got the lock"); // 第3
  lock_release (lock);
  msg ("acquire2: done"); // 第4
}
