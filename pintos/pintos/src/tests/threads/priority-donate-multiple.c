/* The main thread acquires locks A and B, then it creates two
   higher-priority threads.  Each of these threads blocks
   acquiring one of the locks and thus donate their priority to
   the main thread.  The main thread releases the locks in turn
   and relinquishes its donated priorities.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by Matt Franklin <startled@leland.stanford.edu>,
   Greg Hutchins <gmh@leland.stanford.edu>, Yu Ping Hu
   <yph@cs.stanford.edu>.  Modified by arens. */

/*original_thread是优先级为PRI_DEFAULT的线程， 然后创建2个锁， 接着创建优先级为PRI_DEFAULT+1的线程a， 把锁a丢给这个线程的执行函数。

这时候线程a抢占式地调用a_thread_func， 获取了a这个锁， 阻塞。

然后original_thread输出线程优先级的msg。

然后再创建一个线程优先级为PRI_DEFAULT+2的线程b， 和a一样做同样的操作。

好， 然后original_thread释放掉了锁b， 此时线程b被唤醒， 抢占式执行b_thread_func。

然后original再输出msg， a同上， 此时我们来看一下测试希望的输出是什么：

复制代码
 1 # -*- perl -*-
 2 use strict;
 3 use warnings;
 4 use tests::tests;
 5 check_expected ([<<'EOF']);
 6 (priority-donate-multiple) begin
 7 (priority-donate-multiple) Main thread should have priority 32.  Actual priority: 32.
 8 (priority-donate-multiple) Main thread should have priority 33.  Actual priority: 33.
 9 (priority-donate-multiple) Thread b acquired lock b.
10 (priority-donate-multiple) Thread b finished.
11 (priority-donate-multiple) Thread b should have just finished.
12 (priority-donate-multiple) Main thread should have priority 32.  Actual priority: 32.
13 (priority-donate-multiple) Thread a acquired lock a.
14 (priority-donate-multiple) Thread a finished.
15 (priority-donate-multiple) Thread a should have just finished.
16 (priority-donate-multiple) Main thread should have priority 31.  Actual priority: 31.
17 (priority-donate-multiple) end
18 EOF
19 pass;
复制代码
好， 这里输出和我们的分析依然是一致的。 重点在于original_thread的优先级变化， 第一次输出是正常的， priority-donate-one已经测试了这个逻辑。

这里特别的是original_thread拥有两把锁分别给a, b两个线程占有了。

后面是释放了b之后， original_thread的优先级恢复到32, 即当前线程的优先级还是被a的优先级所捐赠着的，最后释放了a之后才回到原来的优先级。

这里测试的行为实际是： 多锁情况下优先级逻辑的正确性。

那么我们对应的实现思路是： 释放一个锁的时候， 将该锁的拥有者改为该线程被捐赠的第二优先级，若没有其余捐赠者， 则恢复原始优先级。 

那么我们的线程必然需要一个数据结构来记录所有对这个线程有捐赠行为的线程。*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func a_thread_func;
static thread_func b_thread_func;

void
test_priority_donate_multiple (void)
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

  thread_create ("a", PRI_DEFAULT + 1, a_thread_func, &a);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 1, thread_get_priority ());  //第1

  thread_create ("b", PRI_DEFAULT + 2, b_thread_func, &b);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());  //第2

  lock_release (&b);
  msg ("Thread b should have just finished.");  //第5
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 1, thread_get_priority ());  //第6

  lock_release (&a);
  msg ("Thread a should have just finished.");  //第9
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT, thread_get_priority ());  //第10
}

static void
a_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("Thread a acquired lock a.");  //第7
  lock_release (lock);
  msg ("Thread a finished.");  //第8
}

static void
b_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("Thread b acquired lock b.");  //第3
  lock_release (lock);
  msg ("Thread b finished.");  //第4
}
