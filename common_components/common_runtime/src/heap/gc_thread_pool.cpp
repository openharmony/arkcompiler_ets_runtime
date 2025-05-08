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
#include "common_components/common_runtime/src/heap/gc_thread_pool.h"

#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
#include <sys/resource.h>
#endif
#include <sched.h>

#include "common_components/common_runtime/src/base/sys_call.h"
#include "common_components/common_runtime/src/mutator/mutator_manager.h"
#include "common_components/log/log.h"
#include "securec.h"

// thread pool implementation
namespace panda {
GCThread::GCThread(GCThreadPool* threadPool, const char* threadName, size_t threadId, size_t stackSize)
    : id_(threadId), tid_(-1), name_(threadName), pool_(threadPool)
{
    pthread_attr_t attr;
    CHECK_CALL(pthread_attr_init, (&attr), "");
    CHECK_CALL(pthread_attr_setstacksize, (&attr, stackSize), stackSize);
    CHECK_CALL(pthread_create, (&pthread_, nullptr, &WorkerFunc, this), "GCPoolThread init");
#ifdef __WIN64
    CHECK_CALL(pthread_setname_np, (pthread, threadName), "GCPoolThread SetName");
#endif
    CHECK_CALL(pthread_attr_destroy, (&attr), "GCPoolThread init");
}

GCThread::~GCThread()
{
    CHECK_CALL(pthread_join, (pthread_, nullptr), "thread deinit");
    pool_ = nullptr;
}

#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
void GCThread::SetPriority(int32_t priority) const { SetThreadPriority(tid_, priority); }

void GCThread::SetThreadPriority(pid_t tid, int32_t priority)
{
    errno = 0;
    int ret = ::setpriority(static_cast<int>(PRIO_PROCESS), static_cast<uint32_t>(tid), priority);
    // strerror_r does not work as expected: outputs nothing for __amd64__
    LOGE_IF(UNLIKELY_CC(ret != 0 && errno != 0))
        << "::setpriority(tid " << tid << ", priority " << priority << ") failed: " << ::strerror(errno);
}
#endif

void* GCThread::WorkerFunc(void* param)
{
    // set current thread as a gc thread.
    ThreadLocal::SetThreadType(ThreadType::GC_THREAD);

    GCThread* thread = reinterpret_cast<GCThread*>(param);
    GCThreadPool* pool = thread->pool_;

#ifdef __APPLE__
    CHECK_CALL(pthread_setname_np, (thread->name.Str()), "GCPoolThread SetName");
#elif defined(__linux__) || defined(PANDA_TARGET_OHOS)
    CHECK_CALL(prctl, (PR_SET_NAME, thread->name_.Str()), "GCPoolThread SetName");
#endif

#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
    thread->tid_ = panda::GetTid();
    SetThreadPriority(thread->tid_, pool->priority_);
#endif

    while (!pool->IsExited()) {
        HeapWork* task = nullptr;
        {
            std::unique_lock<std::mutex> poolLock(pool->poolMutex_);
            // hang up in threadSleepingCondVar when pool stopped or to many active thread
            while (((pool->currActiveThreadNum_ > pool->maxActiveThreadCount_) || !pool->IsRunning()) &&
                   !pool->IsExited()) {
                // currActiveThreadNum start at maxThreadNum, dec before thread hangup in sleeping state
                --(pool->currActiveThreadNum_);
                if (pool->currActiveThreadNum_ == 0) {
                    // all thread sleeping, pool in stop state, notify wait stop thread
                    pool->allThreadStoppedCV_.notify_all();
                }
                pool->threadSleepingCV_.wait(poolLock);
                ++(pool->currActiveThreadNum_);
            }
            // if no task available thread hung up in taskEmptyCondVar
            while (pool->workQueue_.empty() && pool->IsRunning() && !pool->IsExited()) {
                // currExecuteThreadNum start at 0, inc before thread wait for task
                ++(pool->waitingThreadNum_);
                if (pool->waitingThreadNum_ == pool->maxActiveThreadCount_) {
                    // all task is done, notify wait finish thread
                    pool->allWorkDoneCV_.notify_all();
                }
                pool->taskEmptyCV_.wait(poolLock);
                --(pool->waitingThreadNum_);
            }
            if (!pool->workQueue_.empty() && pool->IsRunning() && !pool->IsExited()) {
                task = pool->workQueue_.front();
                pool->workQueue_.pop();
            }
        }
        if (task != nullptr) {
            task->Execute(thread->id_);
            delete task;
        }
    }
    {
        std::unique_lock<std::mutex> poolLock(pool->poolMutex_);
        --(pool->currActiveThreadNum_);
        if (pool->currActiveThreadNum_ == 0) {
            // all thread sleeping, pool in stop state, notify wait stop thread
            pool->allThreadStoppedCV_.notify_all();
        }
    }
    return nullptr;
}

const int MAX_NAME_LEN = 256;

GCThreadPool::GCThreadPool(const char* poolName, int32_t threadNum, int32_t prior)
    : priority_(prior), name_(poolName), running_(false), exit_(false), maxThreadCount_(threadNum),
      maxActiveThreadCount_(threadNum), currActiveThreadNum_(maxThreadCount_), waitingThreadNum_(0)
{
    // init and start thread
    char threadName[MAX_NAME_LEN];
    for (int32_t i = 0; i < maxThreadCount_; ++i) {
        // threadID 0 is main thread, sub threadID start at 1
        errno_t ret = snprintf_s(threadName, MAX_NAME_LEN, (MAX_NAME_LEN - 1), "%s-pool-t%d", poolName, (i + 1));
        LOGE_IF(ret < 0) << "snprintf_s name = " << name_.Str() << " threadId" << (i + 1)
                         << " in GCThreadPool::GCThreadPool return " << ret << " rather than 0.";
        // default Sleeping
        GCThread* threadItem = new (std::nothrow) GCThread(this, threadName, (i + 1), DEFAULT_STACK_SIZE);
        LOGF_CHECK(threadItem != nullptr) << "new GCPoolThread failed";
        threads_.push_back(threadItem);
    }
    // pool init in stop state
    Stop();
    LOG_COMMON(DEBUG) << "GCThreadPool init";
}

void GCThreadPool::Exit()
{
    std::unique_lock<std::mutex> lockGuard(poolMutex_);
    // Set the exit flag for the pool to initiate shutdown.
    exit_.store(true, std::memory_order_relaxed);

    // Send a notification to all waiting threads to exit.
    taskEmptyCV_.notify_all();
    // Send a notification to all stopped threads to terminate.
    threadSleepingCV_.notify_all();

    // Send a notification to all WaitFinish threads to return control.
    allWorkDoneCV_.notify_all();
    // Send a notification to all WaitStop threads to return control.
    allThreadStoppedCV_.notify_all();
    LOG_COMMON(DEBUG) << "GCThreadPool Exit";
}

GCThreadPool::~GCThreadPool()
{
    // wait until threads exit
    for (auto thread : threads_) {
        delete thread;
    }
    threads_.clear();
    ClearAllWork();
}

#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
void GCThreadPool::SetPriority(int32_t prior) const
{
    for (auto thread : threads_) {
        thread->SetPriority(prior);
    }
}
#endif

void GCThreadPool::SetMaxActiveThreadNum(int32_t num)
{
    std::unique_lock<std::mutex> poolLock(poolMutex_);
    int32_t prevThreadCount = maxActiveThreadCount_;
    if (num > maxThreadCount_) {
        maxActiveThreadCount_ = maxThreadCount_;
    } else if (num > 0) {
        maxActiveThreadCount_ = num;
    } else {
        LOG_COMMON(ERROR) << "The input value for SetMaxActiveThreadNum is invalid.";
        return;
    }
    // Activate additional threads to handle tasks when the thread pool is running.
    if ((maxActiveThreadCount_ > prevThreadCount) && (waitingThreadNum_ > 0) && IsRunning()) {
        threadSleepingCV_.notify_all();
    }
}

void GCThreadPool::AddWork(HeapWork* task)
{
    LOGF_CHECK(task != nullptr) << "Failed to add a null task; ensure tasks are properly initialized before adding.";
    std::unique_lock<std::mutex> lockGuard(poolMutex_);
    workQueue_.push(task);
    // Avoid notifications when the pool is inactive
    // Send a notification if there is an active thread waiting for a task to ensure timely processing.
    if ((waitingThreadNum_ > 0) && IsRunning()) {
        taskEmptyCV_.notify_one();
    }
}

void GCThreadPool::Start()
{
    // notify all sleeping threads get to work
    std::unique_lock<std::mutex> poolLock(poolMutex_);
    running_.store(true, std::memory_order_relaxed);
    threadSleepingCV_.notify_all();
}

void GCThreadPool::DrainWorkQueue()
{
    ASSERT_LOGF(!IsRunning(), "The thread pool is not running");
    HeapWork* currentTask{ nullptr };
    do {
        currentTask = nullptr;
        {
            std::unique_lock<std::mutex> lockGuard(poolMutex_);
            if (!workQueue_.empty()) {
                currentTask = workQueue_.front();
                workQueue_.pop();
            }
        }
        if (currentTask == nullptr) {
            continue;
        } else {
            currentTask->Execute(0);
            delete currentTask;
        }
    } while (currentTask != nullptr);
}

void GCThreadPool::WaitFinish()
{
    HeapWork* currentTask{ nullptr };
    do {
        currentTask = nullptr;
        {
            std::unique_lock<std::mutex> lockGuard(poolMutex_);
            if (!workQueue_.empty() && IsRunning() && !IsExited()) {
                currentTask = workQueue_.front();
                workQueue_.pop();
            }
        }
        if (currentTask == nullptr) {
            continue;
        } else {
            currentTask->Execute(0);
            delete currentTask;
        }
    } while (currentTask != nullptr);

    // wait all task execute finish
    // waitingThreadNum == maxActiveThreadNum indicate all work done
    // no need to wait when pool stopped or exited
    {
        std::unique_lock<std::mutex> poolLock(poolMutex_);
        while ((waitingThreadNum_ != maxActiveThreadCount_) && IsRunning() && !IsExited()) {
            allWorkDoneCV_.wait(poolLock);
        }
    }
    // let all thread sleeping for next start
    // if threads not in sleeping mode, next start signal may be missed
    Stop();
    // clean up task in GC thread, thread pool might receive "exit" in stop thread
    DrainWorkQueue();
}

void GCThreadPool::Stop()
{
    // Send a stop signal to all threads and wait for them to reach the stopped state.
    std::unique_lock<std::mutex> lockGuard(poolMutex_);
    running_.store(false, std::memory_order_relaxed);
    taskEmptyCV_.notify_all();
    while (currActiveThreadNum_ != 0) {
        allThreadStoppedCV_.wait(lockGuard);
    }
}

void GCThreadPool::ClearAllWork()
{
    std::unique_lock<std::mutex> poolLock(poolMutex_);
    while (!workQueue_.empty()) {
        HeapWork* task = workQueue_.front();
        workQueue_.pop();
        delete task;
    }
}

#ifndef NDEBUG
ScopedCpuTime::ScopedCpuTime(GCThreadPool& threadPool, LogType type) : logType_(type)
{
    if (ENABLE_LOG(logType_)) {
#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
        threadCount_ = threadPool.GetMaxThreadNum() + 1;
        cid_.resize(threadCount_);
        workerStart_.resize(threadCount_);

        pthread_getcpuclockid(pthread_self(), &cid_[0]);
        clock_gettime(cid_[0], &workerStart_[0]);
        size_t index = 1;
        for (auto worker : threadPool.GetThreads()) {
            pthread_t thread = worker->GetThread();
            pthread_getcpuclockid(thread, &cid_[index]);
            clock_gettime(cid_[index], &workerStart_[index]);
            ++index;
        }
#else
        VLOG(logType, "ScopedCpuTime is not supported in windows yet.");
#endif
    }
}

ScopedCpuTime::~ScopedCpuTime()
{
    if (ENABLE_LOG(logType_)) {
#if defined(__linux__) || defined(PANDA_TARGET_OHOS)
        struct timespec workerEnd[threadCount_];
        uint64_t workerCpuTime[threadCount_];
        for (size_t i = 0; i < threadCount_; ++i) {
            clock_gettime(cid_[i], &workerEnd[i]);
            workerCpuTime[i] =
                static_cast<uint64_t>(workerEnd[i].tv_sec - workerStart_[i].tv_sec) * panda::SECOND_TO_NANO_SECOND +
                static_cast<uint64_t>((workerEnd[i].tv_nsec - workerStart_[i].tv_nsec));
        }
        for (size_t i = 0; i < threadCount_; ++i) {
            VLOG(logType_, "worker %zu cputime: %lu,\t", i, workerCpuTime[i]);
        }
#endif
    }
}
#endif // DEBUG
} // namespace panda
