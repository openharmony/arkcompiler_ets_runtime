/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"

#include <future>

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;
using namespace panda::panda_file;

namespace panda::test {
class PGOProfilerTestOne : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        GTEST_LOG_(INFO) << "SetUpTestSuite";
    }
    static void TearDownTestSuite()
    {
        GTEST_LOG_(INFO) << "TearDownTestSuite";
    }
    void SetUp() override {}
    void TearDown() override {}

protected:
    void ParseRelatedPandaFileMethods(
        panda::ecmascript::pgo::PGOProfilerDecoder& loader,
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>>& methodIdInAp)
    {
        std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
        ASSERT_TRUE(loader.LoadFull(abcFilePool));
        for (const auto& recordInfo: loader.GetRecordDetailInfos().GetRecordInfos()) {
            auto recordProfile = recordInfo.first;
            ASSERT_EQ(recordProfile.GetKind(), ProfileType::Kind::RecordClassId);
            if (recordProfile.IsNone()) {
                continue;
            }
            LOG_ECMA(ERROR) << "recordProfile: " << recordProfile.GetTypeString();
            const auto* recordName = loader.GetRecordDetailInfos().GetRecordPool()->GetName(recordProfile);
            ASSERT(recordName != nullptr);
            const auto abcNormalizedDesc =
                JSPandaFile::GetNormalizedFileDesc(abcFilePool->GetEntry(recordProfile.GetAbcId())->GetData());
            if (abcNormalizedDesc.empty()) {
                continue;
            }

            const auto* info = recordInfo.second;
            for (const auto& method: info->GetMethodInfos()) {
                // add ap entry info
                methodIdInAp[abcNormalizedDesc.c_str()][recordName].emplace_back(method.first);
            }
        };
    }

    void CheckApMethods(
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>>& methodIdInAp)
    {
        for (auto abcIter = methodIdInAp.begin(); abcIter != methodIdInAp.end();) {
            std::string fileName(abcIter->first.c_str());
            auto lastDirToken = fileName.find_last_of('/');
            if (lastDirToken != std::string::npos) {
                fileName = fileName.substr(lastDirToken + 1);
            }
            std::unordered_map<std::string, std::vector<PGOMethodId>>& recordMethodList = abcIter->second;
            CheckApMethodsInApFiles(fileName, recordMethodList);
            if (recordMethodList.empty()) {
                abcIter = methodIdInAp.erase(abcIter);
            } else {
                abcIter++;
            }
        }
        ASSERT_TRUE(methodIdInAp.empty());
    }

    std::shared_ptr<JSPandaFile> CreateJSPandaFile(const std::string& filename,
                                                   std::vector<MethodLiteral*>& methodLiterals)
    {
        std::string targetAbcPath = std::string(TARGET_ABC_PATH) + filename.c_str();
        auto pfPtr = panda_file::OpenPandaFileOrZip(targetAbcPath, panda_file::File::READ_WRITE);
        JSPandaFileManager* pfManager = JSPandaFileManager::GetInstance();
        auto pf = pfManager->NewJSPandaFile(pfPtr.release(), filename.c_str());

        const File* file = pf->GetPandaFile();
        auto classes = pf->GetClasses();

        for (size_t i = 0; i < classes.Size(); i++) {
            panda_file::File::EntityId classId(classes[i]);
            if (!classId.IsValid() || pf->IsExternal(classId)) {
                continue;
            }
            ClassDataAccessor cda(*file, classId);
            cda.EnumerateMethods([pf, &methodLiterals](panda_file::MethodDataAccessor& mda) {
                auto* methodLiteral = new MethodLiteral(mda.GetMethodId());
                methodLiteral->Initialize(pf.get());
                pf->SetMethodLiteralToMap(methodLiteral);
                methodLiterals.push_back(methodLiteral);
            });
        }
        return pf;
    }

    void CheckApMethodsInApFiles(const std::string& fileName,
                                 std::unordered_map<std::string, std::vector<PGOMethodId>>& recordMethodList)
    {
        std::vector<MethodLiteral*> methodLiterals {};
        auto pf = CreateJSPandaFile(fileName, methodLiterals);

        for (auto& methodLiteral: methodLiterals) {
            auto methodName = MethodLiteral::GetRecordName(pf.get(), methodLiteral->GetMethodId());
            auto recordEntry = recordMethodList.find(methodName.c_str());
            if (recordEntry == recordMethodList.end()) {
                continue;
            }
            for (size_t index = 0; index < recordEntry->second.size(); ++index) {
                if (!(recordEntry->second.at(index) == methodLiteral->GetMethodId())) {
                    continue;
                }
                recordEntry->second.erase(recordEntry->second.begin() + index);
                if (recordEntry->second.empty()) {
                    recordEntry = recordMethodList.erase(recordEntry);
                }
                break;
            }
        }
    }

    static constexpr uint32_t threshhold = 0;
};

HWTEST_F_L0(PGOProfilerTestOne, Worker)
{
    mkdir("ark-profiler-worker/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetLogLevel(LOG_LEVEL::INFO);
    option.SetProfileDir("ark-profiler-worker/");
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    std::string targetAbcPath = std::string(TARGET_ABC_PATH) + "truck.abc";
    auto result = JSNApi::Execute(vm, targetAbcPath, "truck", false);
    EXPECT_TRUE(result);

    std::thread workerThread([vm]() {
        RuntimeOption workerOption;
        workerOption.SetEnableProfile(true);
        workerOption.SetLogLevel(LOG_LEVEL::INFO);
        workerOption.SetProfileDir("ark-profiler-worker/");
        workerOption.SetIsWorker();
        EcmaVM* workerVm = JSNApi::CreateJSVM(workerOption);
        ASSERT_TRUE(workerVm != nullptr) << "Cannot create Worker Runtime";

        JSNApi::AddWorker(vm, workerVm);

        std::string targetAbcPath = std::string(TARGET_ABC_PATH) + "call_test.abc";
        auto result = JSNApi::Execute(workerVm, targetAbcPath, "call_test", false);
        EXPECT_TRUE(result);

        auto hasDeleted = JSNApi::DeleteWorker(vm, workerVm);
        EXPECT_TRUE(hasDeleted);
        JSNApi::DestroyJSVM(workerVm);
    });
    workerThread.join();
    JSNApi::DestroyJSVM(vm);

    PGOProfilerDecoder loader("ark-profiler-worker/modules.ap", threshhold);
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>> methodIdInAp;
    ParseRelatedPandaFileMethods(loader, methodIdInAp);
    ASSERT_EQ(methodIdInAp.size(), 3);
    CheckApMethods(methodIdInAp);

    unlink("ark-profiler-worker/modules.ap");
    rmdir("ark-profiler-worker/");
}

HWTEST_F_L0(PGOProfilerTestOne, ForceDump)
{
    mkdir("ark-profiler-worker/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetLogLevel(LOG_LEVEL::INFO);
    option.SetProfileDir("ark-profiler-worker/");
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    std::string targetAbcPath = std::string(TARGET_ABC_PATH) + "truck.abc";
    auto result = JSNApi::Execute(vm, targetAbcPath, "truck", false);
    EXPECT_TRUE(result);

    std::thread workerThread([vm]() {
        RuntimeOption workerOption;
        workerOption.SetEnableProfile(true);
        workerOption.SetLogLevel(LOG_LEVEL::INFO);
        workerOption.SetProfileDir("ark-profiler-worker/");
        workerOption.SetIsWorker();
        EcmaVM* workerVm = JSNApi::CreateJSVM(workerOption);
        ASSERT_TRUE(workerVm != nullptr) << "Cannot create Worker Runtime";

        JSNApi::AddWorker(vm, workerVm);

        std::string targetAbcPath = std::string(TARGET_ABC_PATH) + "call_test.abc";
        auto result = JSNApi::Execute(workerVm, targetAbcPath, "call_test", false);
        EXPECT_TRUE(result);

        auto hasDeleted = JSNApi::DeleteWorker(vm, workerVm);
        EXPECT_TRUE(hasDeleted);
        JSNApi::DestroyJSVM(workerVm);
    });

    auto manager = PGOProfilerManager::GetInstance();
    auto state = manager->GetPGOState();
    manager->ForceDump();
    while (!state->IsStop()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    PGOProfilerDecoder loader("ark-profiler-worker/modules.ap", threshhold);
    std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(loader.LoadFull(abcFilePool));

    workerThread.join();
    JSNApi::DestroyJSVM(vm);
    unlink("ark-profiler-worker/modules.ap");
    rmdir("ark-profiler-worker/");
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    state->SetStartIfStop();
    EXPECT_TRUE(state->IsStart());

    std::thread thread1([state]() {
        state->SuspendByGC();
        state->SetStartIfPauseByGC();
    });

    std::thread thread2([state]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state->SetStopIfStartAndNotify();
    });

    thread1.join();
    thread2.join();

    EXPECT_TRUE(state->IsStart());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread2)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    manager->GetPGOState()->SetStartIfStop();
    EXPECT_TRUE(manager->GetPGOState()->IsStart());

    std::thread thread1([state]() {
        state->SuspendByGC();
        state->SetStartIfPauseByGC();
    });

    std::thread thread2([state]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state->SetStopIfStartAndNotify();
    });

    std::thread thread3([state]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        state->SetStopIfStartAndNotify();
    });

    thread1.join();
    thread2.join();
    thread3.join();

    EXPECT_TRUE(manager->GetPGOState()->IsStop());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread3)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    manager->GetPGOState()->SetStartIfStop();
    EXPECT_TRUE(manager->GetPGOState()->IsStart());

    std::thread thread1([state]() { state->SetStopIfStartAndNotify(); });

    std::thread thread2([state]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state->SuspendByGC();
    });

    thread1.join();
    thread2.join();

    EXPECT_TRUE(manager->GetPGOState()->IsStop());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread4)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    manager->GetPGOState()->SetStartIfStop();
    EXPECT_TRUE(manager->GetPGOState()->IsStart());

    std::thread thread1([state]() { state->SuspendByGC(); });

    std::thread thread2([state]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state->SetStopIfStartAndNotify();
    });

    thread1.join();
    thread2.join();

    EXPECT_TRUE(manager->GetPGOState()->IsGcWaiting());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread5)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    state->SetStartIfStop();
    EXPECT_TRUE(state->IsStart());

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    std::thread thread1([state, &mtx, &cv, &ready]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&ready]() { return ready; });
        }
        state->SuspendByGC();
    });

    std::thread thread2([state, &mtx, &cv, &ready]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&ready]() { return ready; });
        }
        state->SetStopIfStartAndNotify();
    });

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_all();
    thread1.join();
    thread2.join();

    EXPECT_TRUE(manager->GetPGOState()->IsGcWaiting() || manager->GetPGOState()->IsStop());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiGCShouldWait)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);

    std::mutex mtx;
    std::condition_variable c23;
    bool t23 = false;

    std::thread thread2([state, &mtx, &c23, &t23]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            c23.wait(lock, [&t23]() { return t23; });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!state->SetPauseIfStartByGC()) {
            state->SetStartIfPauseByGC();
        }
    });

    std::thread thread3([state, &mtx, &c23, &t23]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            c23.wait(lock, [&t23]() { return t23; });
        }
        if (!state->SetPauseIfStartByGC()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            state->SetStartIfPauseByGC();
        }
    });

    state->SetStartIfStop();
    EXPECT_TRUE(state->IsStart());
    {
        std::lock_guard<std::mutex> lock(mtx);
        t23 = true;
    }
    c23.notify_all();

    thread2.join();
    thread3.join();

    EXPECT_TRUE(state->IsGcWaiting());
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread6)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    EcmaVM* vm = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";

    auto manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager != nullptr);
    auto state = manager->GetPGOState();
    ASSERT_TRUE(state != nullptr);
    EXPECT_TRUE(state->IsStop());

    state->SuspendByGC();
    EXPECT_EQ(state->GetGcCount(), 1);
    EXPECT_TRUE(state->IsStop());

    state->SetStartIfStopAndDispatchDumpTask(nullptr);
    EXPECT_TRUE(state->IsStop());

    std::thread thread([state]() { state->SuspendByGC(); });
    thread.join();
    EXPECT_EQ(state->GetGcCount(), 2);
    EXPECT_TRUE(state->IsStop());

    state->ResumeByGC(nullptr);
    EXPECT_EQ(state->GetGcCount(), 1);
    EXPECT_TRUE(state->IsStop());

    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(PGOProfilerTestOne, PGOStateMultiThread7)
{
    auto state = std::make_shared<PGOState>();
    EXPECT_TRUE(state->IsStop());
    state->SetStartIfStop();
    EXPECT_TRUE(state->IsStart());

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    std::future<void> future1 = std::async(std::launch::async, [state, &mtx, &cv, &ready]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&ready]() { return ready; });
        }
        state->WaitDumpIfStart();
    });

    std::future<void> future2 = std::async(std::launch::async, [state, &mtx, &cv, &ready]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&ready]() { return ready; });
        }
        state->WaitDumpIfStart();
    });

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    state->SetStopIfStartAndNotify();

    auto status1 = future1.wait_for(std::chrono::seconds(2));
    auto status2 = future2.wait_for(std::chrono::seconds(2));
    ASSERT_NE(status1, std::future_status::timeout) << "thread 1 may freeze";
    ASSERT_NE(status2, std::future_status::timeout) << "thread 2 may freeze";
}
} // namespace panda::test
