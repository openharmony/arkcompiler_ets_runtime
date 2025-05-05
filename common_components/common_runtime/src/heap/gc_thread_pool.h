/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ARK_COMMON_GC_THREAD_POOL_H
#define ARK_COMMON_GC_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <vector>

#include "common_components/common_runtime/src/base/log_file.h"
#include "common_components/common_runtime/src/heap/heap_work.h"

// thread pool implementation
namespace panda {
class GCThreadPool;

class GCThread {
public:
    // schedule priority for GC threads.
    // only superuser can lower priority.
    static constexpr int32_t GC_THREAD_PRIORITY = 0;            // default priority
    static constexpr int32_t GC_THREAD_STW_PRIORITY = 0;        // priority used in stw stage
    static constexpr int32_t GC_THREAD_CONCURRENT_PRIORITY = 0; // priority used in concurrent stage
    static constexpr int32_t PRIORITY_PROMOTE_STEP = -5;        // priority promote step

    GCThread(GCThreadPool* threadPool, const char* threadName, size_t threadId, size_t stackSize);
    ~GCThread();

    void SetPriority(int32_t prior) const;

    // get pthread of thread
    pthread_t GetThread() const { return pthread_; }

    // get thread id of thread
    pid_t GetTid() const { return tid_; }

    static void* WorkerFunc(void* param);
#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
    static void SetThreadPriority(pid_t tid, int32_t priority);
#endif
private:
    size_t id_;
    pthread_t pthread_;
    pid_t tid_;
    CString name_;
    GCThreadPool* pool_;
};

// manual
// new  (SetMaxActiveThreadNum(optional) addWork  startPool waitFinish)^. Exit delete
// if need to change MaxActiveThreadNum, should waitFinish or stop pool at first
class GCThreadPool {
public:
    // Constructor for thread pool, 1) Create threads, 2) wait all thread created & sleep
    // name is the thread pool name. thread name = Pool_$(poolname)_ThreadId_$(threadId)
    // maxThreadNum is the max thread number in pool.
    // prior is the priority of threads in pool.
    GCThreadPool(const char* name, int32_t maxThreadNum, int32_t prior);

    // Destructor for thread pool, 1) close pool 2) wait thread in pool to exit, 3) release resources of class
    ~GCThreadPool();

    // Set priority of each thread in pool.
    void SetPriority(int32_t prior) const;

    // Set max active thread number of pool, redundant thread hangup in sleep condition var.
    // notify more waiting thread get to work when pool is running.
    // Range [1 - maxThreadNum].
    void SetMaxActiveThreadNum(int32_t num);

    // Get max active thread number of pool.
    int32_t GetMaxActiveThreadNum() const { return maxActiveThreadCount_; }

    // Get max thread number of pool, default = maxThreadNum.
    int32_t GetMaxThreadNum() const { return maxThreadCount_; }

    int32_t GetWaitingThreadNumber() const { return waitingThreadNum_; }

    // Add new task to task queue , task should inherit from HeapWork.
    void AddWork(HeapWork* task);

    // Start thread pool, notify all sleep threads to get to work
    void Start();

    // Wait all task in task queue finished, if pool stopped, only wait until current executing task finish
    // after all task finished, stop pool
    // addToExecute indicate whether the caller thread execute task
    void WaitFinish();

    // used in none-parallel concurrent mark
    void DrainWorkQueue();

    // Notify & Wait all thread waiting for task to sleep
    void Stop();

    // Notify all thread in pool to exit , notify all waitFinish thread to return,nonblock ^.
    void Exit();

    // Remove all task in task queue
    void ClearAllWork();

    // Get work count in queue
    size_t GetWorkCount()
    {
        std::unique_lock<std::mutex> taskLock(poolMutex_);
        return workQueue_.size();
    }

    // Get all GCPoolThread in pool
    const std::vector<GCThread*>& GetThreads() const { return threads_; }

private:
    // thread default stack size 512 KB.
    static constexpr size_t DEFAULT_STACK_SIZE = (512 * 1024);
    int32_t priority_;

    CString name_;
    // pool stop or running state
    std::atomic<bool> running_;
    // is pool exit
    std::atomic<bool> exit_;
    // all task put in task queue
    std::queue<HeapWork*> workQueue_;

    // active thread 0 ..... maxActiveThreadNum .....maxThreadNum
    // max thread number in pool
    int32_t maxThreadCount_;

    // max active thread number, redundant thread hang up in threadSleepingCondVar
    int32_t maxActiveThreadCount_;

    // current active thread, when equals to zero, no thread running, all thread slept
    int32_t currActiveThreadNum_;

    // current watiing thread, when equals to currActiveThreadNum
    // no thread executing, all task finished
    int32_t waitingThreadNum_;

    // single lock
    std::mutex poolMutex_;

    // hangup when no task available
    std::condition_variable taskEmptyCV_;

    // hangup when too much active thread or pool stopped
    std::condition_variable threadSleepingCV_;

    // hangup when there is thread executing
    std::condition_variable allWorkDoneCV_;

    // hangup when there is thread active
    std::condition_variable allThreadStoppedCV_;

    // use for profiling
    std::vector<GCThread*> threads_;

    // is pool running or stopped
    bool IsRunning() const { return running_.load(std::memory_order_relaxed); }

    bool IsExited() const { return exit_.load(std::memory_order_relaxed); }

    friend class GCThread;
};

#ifndef NDEBUG
// Dump cpu clock time of parallel task
class ScopedCpuTime {
public:
    explicit ScopedCpuTime(GCThreadPool& threadPool, LogType type);
    ~ScopedCpuTime();

private:
    LogType logType_;
#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
    size_t threadCount_ = 0;
    std::vector<clockid_t> cid_;
    std::vector<struct timespec> workerStart_;
#endif
};
#endif
} // namespace panda

#endif // ARK_COMMON_GC_THREAD_POOL_H
