/* The main thread set its priority to PRI_MIN and creates 7 threads
   (thread 1..7) with priorities PRI_MIN + 3, 6, 9, 12, ...
   The main thread initializes 8 locks: lock 0..7 and acquires lock 0.

   When thread[i] starts, it first acquires lock[i] (unless i == 7.)
   Subsequently, thread[i] attempts to acquire lock[i-1], which is held by
   thread[i-1], except for lock[0], which is held by the main thread.
   Because the lock is held, thread[i] donates its priority to thread[i-1],
   which donates to thread[i-2], and so on until the main thread
   receives the donation.

   After threads[1..7] have been created and are blocked on locks[0..7],
   the main thread releases lock[0], unblocking thread[1], and being
   preempted by it.
   Thread[1] then completes acquiring lock[0], then releases lock[0],
   then releases lock[1], unblocking thread[2], etc.
   Thread[7] finally acquires & releases lock[7] and exits, allowing
   thread[6], then thread[5] etc. to run and exit until finally the
   main thread exits.

   In addition, interloper threads are created at priority levels
   p = PRI_MIN + 2, 5, 8, 11, ... which should not be run until the
   corresponding thread with priority p + 1 has finished.

   Written by Godmar Back <gback@cs.vt.edu> */
/*首先lock_pair是包含两个lock指针的结构体， 然后将当前线程优先级设为PRI_MIN， 然后这里有个locks数组， 容量为7, 然后lock_pairs数组用来装lock_pair， 容量也是7。

然后当前线程获取locks[0]这个锁， 接着跳到7次循环里， 每次循环thread_priority为PRI_MIN+i*3， 也就是3,6,9,12... 然后对应的lock_pairs[i]的first记录locks[i]的指针， second记录locks[i-1]指针，

然后创建线程， 优先级为thread_priority, 执行参数传的是&lock_pairs[i]， 注意这里由于优先级每次都底层， 所以每次循环都会抢占调用donor_thread_func， 然后分别获取lock_pairs[i]里装的锁， 然后每次循环先获取first, 即locks[i], 然后获取second, 由于second是前一个， 而前一个的拥有者一定是前一次循环创建的线程， 第一次拿得的是locks[0]， 最后一次循环first为NULL， second为locks[6]， 即最后一个线程不拥有锁， 但是阻塞于前一个创建的线程， 这里还会输出信息， 即创建的线程阻塞之后会输出当前线程的优先级msg, 当然这里必然是每一次都提升了的， 所以每次都是thread_priority。

然后每次循环最后还创建了1个线程， 优先级为thread_priority-1， 但是这里由于上一个线程创建和阻塞的过程中优先级捐献已经发生， 所以这里并不发生抢占， 只是创建出来了而已。

然后original_thread释放掉locks[0]， 释放掉这个之后thread1得到了唤醒， 输出信息， 释放掉这个锁， 然后输出当前优先级， 由于这个线程还是被后面最高优先级的线程说捐赠的， 所以每次往后优先级都是21, 然后释放掉first， 这里又触发下一个线程继续跑， 注意当后面的全部跑完的时候当前线程的优先级其实是不被捐赠的， 这里就变成了原来的优先级， 但是是所有线程都释放了之后才依次返回输出结束msg。

这个测试其实就是一个链式优先级捐赠， 本质测试的还是多层优先级捐赠逻辑的正确性。

需要注意的是一个逻辑： 释放掉一个锁之后， 如果当前线程不被捐赠即马上改为原来的优先级， 抢占式调度。*/
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define NESTING_DEPTH 8

struct lock_pair
  {
    struct lock *second;
    struct lock *first;
  };

static thread_func donor_thread_func;
static thread_func interloper_thread_func;

void
test_priority_donate_chain (void)
{
  int i;
  struct lock locks[NESTING_DEPTH - 1];
  struct lock_pair lock_pairs[NESTING_DEPTH];

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  thread_set_priority (PRI_MIN);

  for (i = 0; i < NESTING_DEPTH - 1; i++)
    lock_init (&locks[i]);

  lock_acquire (&locks[0]);
  msg ("%s got lock.", thread_name ());

  for (i = 1; i < NESTING_DEPTH; i++)
    {
      char name[16];
      int thread_priority;

      snprintf (name, sizeof name, "thread %d", i);
      thread_priority = PRI_MIN + i * 3;
      lock_pairs[i].first = i < NESTING_DEPTH - 1 ? locks + i: NULL;
      lock_pairs[i].second = locks + i - 1;

      thread_create (name, thread_priority, donor_thread_func, lock_pairs + i);
      msg ("%s should have priority %d.  Actual priority: %d.",
          thread_name (), thread_priority, thread_get_priority ());

      snprintf (name, sizeof name, "interloper %d", i);
      thread_create (name, thread_priority - 1, interloper_thread_func, NULL);
    }

  lock_release (&locks[0]);
  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
donor_thread_func (void *locks_)
{
  struct lock_pair *locks = locks_;

  if (locks->first)
    lock_acquire (locks->first);

  lock_acquire (locks->second);
  msg ("%s got lock", thread_name ());

  lock_release (locks->second);
  msg ("%s should have priority %d. Actual priority: %d",
        thread_name (), (NESTING_DEPTH - 1) * 3,
        thread_get_priority ());

  if (locks->first)
    lock_release (locks->first);

  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
interloper_thread_func (void *arg_ UNUSED)
{
  msg ("%s finished.", thread_name ());
}

// vim: sw=2
