/* The main thread acquires a lock.  Then it creates a
   higher-priority thread that blocks acquiring the lock, causing
   it to donate their priorities to the main thread.  The main
   thread attempts to lower its priority, which should not take
   effect until the donation is released. */
/*这里当前线程有一个锁， 然后创建acquire抢占式获取了这个锁阻塞， 然后此时original_thread优先级为PRI_DEFAULT+10， 然后这里调用thread_set_priority， 此时当前线程的优先级应该没有改变， 但是它以后如果恢复优先级时候其实是有改变的， 就是说， 我们如果用original_priority来记录他的话， 如果这个线程处于被捐赠状态的话则直接修改original_priority来完成逻辑， 此时函数过后优先级还是PRI_DEFAULT+10， 然后释放掉锁， acquire抢占输出和释放， 然后original_thread的优先级应该变成了PRI_DEFAULT-10。

Acceptable output:
  begin
  Main thread should have priority 41.  Actual priority: 41.
  Lowering base priority...
  Main thread should have priority 41.  Actual priority: 41.
  acquire: got the lock
  acquire: done
  acquire must already have finished.
  Main thread should have priority 21.  Actual priority: 21.
  end

这里测试的逻辑是当修改一个被捐赠的线程优先级的时候的行为正确性。*/
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func acquire_thread_func;

void
test_priority_donate_lower (void)
{
  struct lock lock;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&lock);
  lock_acquire (&lock);
  thread_create ("acquire", PRI_DEFAULT + 10, acquire_thread_func, &lock);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 10, thread_get_priority ());  //第1

  msg ("Lowering base priority...");  //第2
  thread_set_priority (PRI_DEFAULT - 10);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 10, thread_get_priority ());  //第3
  lock_release (&lock);
  msg ("acquire must already have finished.");  //第6
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT - 10, thread_get_priority ());  //第7
}

static void
acquire_thread_func (void *lock_)
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("acquire: got the lock");  //第4
  lock_release (lock);
  msg ("acquire: done");  //第5
}
