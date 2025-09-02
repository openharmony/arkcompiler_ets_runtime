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
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include "ecmascript/base/number_helper.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/heap_marker.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda::ecmascript;
using ErrorType = base::ErrorType;

class HeapDumpTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm_, thread_, scope_);
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
};

class HeapDumpTestHelper {
public:
    explicit HeapDumpTestHelper(EcmaVM *vm) : instance(vm) {}

    ~HeapDumpTestHelper()
    {
        HeapProfilerInterface::Destroy(instance);
    }

    size_t GenerateSnapShot(const std::string &filePath)
    {
        // first generate this file of filePath if not exist,
        // so the function `realpath` of FileStream can not failed on arm/arm64.
        fstream outputString(filePath, std::ios::out);
        outputString.close();
        outputString.clear();
        FileStream stream(filePath.c_str());
        HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = DumpFormat::JSON;
        heapProfile->DumpHeapSnapshot(&stream, dumpOption);
        return heapProfile->GetIdCount();
    }

    bool GenerateRawHeapSnashot(const std::string &filePath, DumpSnapShotOption &dumpOption,
                                Progress *progress = nullptr, std::function<void(uint8_t)> callback = [] (uint8_t) {})
    {
        HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
        dumpOption.dumpFormat = DumpFormat::BINARY;
        dumpOption.isDumpOOM = true;
        dumpOption.isFullGC = false;
        fstream outputString(filePath, std::ios::out);
        outputString.close();
        outputString.clear();
        int fd = open(filePath.c_str(), O_RDWR | O_CREAT);
        FileDescriptorStream stream(fd);
        auto ret = heapProfile->DumpHeapSnapshot(&stream, dumpOption, progress, callback);
        stream.EndOfStream();
        return ret;
    }

    bool DecodeRawHeapObjectTableV1(std::string &filePath, CSet<JSTaggedType> &result)
    {
        uint64_t fileSize = rawheap_translate::FileReader::GetFileSize(filePath);
        rawheap_translate::FileReader file;
        rawheap_translate::RawHeapTranslateV1 translate(nullptr);

        if (!file.Initialize(filePath) || !translate.Parse(file, fileSize)) {
            return false;
        }

        for (auto &it : translate.nodesMap_) {
            result.insert(it.first);
        }
        GTEST_LOG_(INFO) << "DecodeRawHeapObjectTable count " << translate.nodesMap_.size();
        return true;
    }

    bool CheckHashInRawheap(JSThread *thread, const std::vector<Reference> &refs, const std::string &filePath)
    {
        uint64_t fileSize = rawheap_translate::FileReader::GetFileSize(filePath);
        rawheap_translate::FileReader file;
        rawheap_translate::MetaParser metaParser;

        if (!file.Initialize(filePath) || !rawheap_translate::RawHeap::ParseMetaData(file, &metaParser) ||
            !file.CheckAndGetHeaderAt(fileSize - sizeof(uint64_t), 0)) {
            return false;
        }

        rawheap_translate::RawHeapTranslateV1 translate(&metaParser);
        if (!translate.Parse(file, file.GetHeaderLeft()) || !translate.Translate()) {
            return false;
        }

        int checkCnt = 0;
        for (auto const &ref : refs) {
            if (!ref.value_.IsJSObject()) {
                continue;
            }

            uint32_t hash = static_cast<uint32_t>(JSObject::Cast(ref.value_)->GetHash(thread));
            if (hash == 0) {
                continue;
            }

            if (translate.hashSet_.find(hash) == translate.hashSet_.end()) {
                std::cout << "CheckHashInRawheap, missed object hash in rawheap." << std::endl;
                return false;
            }

            ++checkCnt;
        }

        if (checkCnt == 0) {
            std::cout << "CheckHashInRawheap, no JSObject." << std::endl;
            return false;
        }
        return true;
    }

    bool DecodeRawHeapObjectTableV2(std::string &filePath, CSet<uint32_t> &result)
    {
        uint64_t fileSize = rawheap_translate::FileReader::GetFileSize(filePath);
        rawheap_translate::FileReader file;
        rawheap_translate::RawHeapTranslateV2 translate(nullptr);

        if (!file.Initialize(filePath) || !translate.Parse(file, fileSize)) {
            return false;
        }

        for (auto &it : translate.nodesMap_) {
            result.insert(it.first);
        }
        GTEST_LOG_(INFO) << "DecodeRawHeapObjectTable count " << translate.nodesMap_.size();
        return true;
    }

    bool AddMetaDataJsonToRawheap(const std::string &rawFilePath)
    {
        uint32_t rawheapSize = rawheap_translate::FileReader::GetFileSize(rawFilePath);
        uint32_t metadataSize = rawheap_translate::FileReader::GetFileSize(HPROF_TEST_METADATA_JSON_DIR"metadata.json");
        if (rawheapSize == 0 || metadataSize == 0) {
            std::cout << "rawheapSize=" << rawheapSize << ", metadataSize=" << metadataSize << std::endl;
            return false;
        }

        std::ofstream rawheapFile(rawFilePath, std::ios::app | std::ios::binary);
        std::ifstream metadataFile(HPROF_TEST_METADATA_JSON_DIR"metadata.json", std::ios::binary);
        if (!rawheapFile.is_open() || !metadataFile.is_open()) {
            return false;
        }

        uint32_t bufSize = metadataSize + sizeof(uint64_t);
        char *buf = new char[bufSize];
        if (!metadataFile.read(buf, metadataSize)) {
            delete[] buf;
            return false;
        }

        *reinterpret_cast<uint32_t *>(buf + bufSize - sizeof(uint64_t)) = rawheapSize;
        *reinterpret_cast<uint32_t *>(buf + bufSize - sizeof(uint32_t)) = metadataSize;
        rawheapFile.write(buf, bufSize);

        rawheapFile.close();
        metadataFile.close();
        delete[] buf;
        return true;
    }

    bool DecodeRawheap(const std::string &rawFilePath, const std::string &outputPath)
    {
        return rawheap_translate::RawHeap::TranslateRawheap(rawFilePath, outputPath);
    }

    bool MatchHeapObjectReferencesV1(const std::vector<Reference> &refs, const CSet<JSTaggedType> &dumpObjects)
    {
        for (auto &ref : refs) {
            if (ref.value_.IsHeapObject() && dumpObjects.find(ref.value_.GetRawData()) == dumpObjects.end()) {
                return false;
            }
        }
        return true;
    }

    bool MatchHeapObjectReferencesV2(const std::vector<Reference> &refs, const CSet<uint32_t> &dumpObjects)
    {
        for (auto &ref : refs) {
            if (ref.value_.IsHeapObject() &&
                dumpObjects.find(static_cast<uint32_t>(ref.value_.GetRawData())) == dumpObjects.end()) {
                return false;
            }
        }
        return true;
    }

    bool MatchHeapDumpString(const std::string &filePath, std::string targetStr)
    {
        std::string line;
        std::ifstream inputStream(filePath);
        std::size_t lineNum = 0;
        while (getline(inputStream, line)) {
            lineNum = line.find(targetStr);
            if (lineNum != line.npos) {
                return true;
            }
        }
        GTEST_LOG_(ERROR) << "file: " << filePath.c_str() << ", target:" << targetStr.c_str()
                          << ", line:" << std::to_string(lineNum) <<"not found";
        return false;  // Lost the Line
    }

    bool ReadRawHeapSectionInfo(std::string &filePath, std::vector<uint32_t> &section)
    {
        uint64_t fileSize = rawheap_translate::FileReader::GetFileSize(filePath);
        rawheap_translate::FileReader file;
        if (!file.Initialize(filePath) || !file.CheckAndGetHeaderAt(file.GetFileSize() - sizeof(uint64_t), 0)) {
            return false;
        }
        if (!file.CheckAndGetHeaderAt(fileSize - sizeof(uint64_t), sizeof(uint32_t))) {
            GTEST_LOG_(ERROR) << "sections header error!";
            return false;
        }

        uint32_t sectionHeaderOffset = fileSize - (file.GetHeaderLeft() + 2) * sizeof(uint32_t);
        section.resize(file.GetHeaderLeft());
        if (!file.Seek(sectionHeaderOffset) || !file.ReadArray(section, file.GetHeaderLeft())) {
            GTEST_LOG_(ERROR) << "read sections error!";
            return false;
        }

        return true;
    }

    JSHandle<JSTypedArray> CreateNumberTypedArray(JSType jsType)
    {
        JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> handleTagValFunc = env->GetInt8ArrayFunction();
        switch (jsType) {
            case JSType::JS_INT8_ARRAY:
                break;
            case JSType::JS_UINT8_ARRAY:
                handleTagValFunc = env->GetUint8ArrayFunction();
                break;
            case JSType::JS_UINT8_CLAMPED_ARRAY:
                handleTagValFunc = env->GetUint8ClampedArrayFunction();
                break;
            case JSType::JS_INT16_ARRAY:
                handleTagValFunc = env->GetInt16ArrayFunction();
                break;
            case JSType::JS_UINT16_ARRAY:
                handleTagValFunc = env->GetUint16ArrayFunction();
                break;
            case JSType::JS_INT32_ARRAY:
                handleTagValFunc = env->GetInt32ArrayFunction();
                break;
            case JSType::JS_UINT32_ARRAY:
                handleTagValFunc = env->GetUint32ArrayFunction();
                break;
            case JSType::JS_FLOAT32_ARRAY:
                handleTagValFunc = env->GetFloat32ArrayFunction();
                break;
            case JSType::JS_FLOAT64_ARRAY:
                handleTagValFunc = env->GetFloat64ArrayFunction();
                break;
            case JSType::JS_BIGINT64_ARRAY:
                handleTagValFunc = env->GetBigInt64ArrayFunction();
                break;
            case JSType::JS_BIGUINT64_ARRAY:
                handleTagValFunc = env->GetBigUint64ArrayFunction();
                break;
            default:
                ASSERT_PRINT(false, "wrong jsType used in CreateNumberTypedArray function");
                break;
        }
        return JSHandle<JSTypedArray>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(handleTagValFunc), handleTagValFunc));
    }

    JSHandle<JSSharedTypedArray> CreateNumberSharedTypedArray(JSType jsType)
    {
        JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> handleTagValFunc = env->GetSharedInt8ArrayFunction();
        switch (jsType) {
            case JSType::JS_SHARED_INT8_ARRAY:
                break;
            case JSType::JS_SHARED_UINT8_ARRAY:
                handleTagValFunc = env->GetSharedUint8ArrayFunction();
                break;
            case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
                handleTagValFunc = env->GetSharedUint8ClampedArrayFunction();
                break;
            case JSType::JS_SHARED_INT16_ARRAY:
                handleTagValFunc = env->GetSharedInt16ArrayFunction();
                break;
            case JSType::JS_SHARED_UINT16_ARRAY:
                handleTagValFunc = env->GetSharedUint16ArrayFunction();
                break;
            case JSType::JS_SHARED_INT32_ARRAY:
                handleTagValFunc = env->GetSharedInt32ArrayFunction();
                break;
            case JSType::JS_SHARED_UINT32_ARRAY:
                handleTagValFunc = env->GetSharedUint32ArrayFunction();
                break;
            case JSType::JS_SHARED_FLOAT32_ARRAY:
                handleTagValFunc = env->GetSharedFloat32ArrayFunction();
                break;
            case JSType::JS_SHARED_FLOAT64_ARRAY:
                handleTagValFunc = env->GetSharedFloat64ArrayFunction();
                break;
            case JSType::JS_SHARED_BIGINT64_ARRAY:
                handleTagValFunc = env->GetSharedBigInt64ArrayFunction();
                break;
            case JSType::JS_SHARED_BIGUINT64_ARRAY:
                handleTagValFunc = env->GetSharedBigUint64ArrayFunction();
                break;
            default:
                ASSERT_PRINT(false, "wrong jsType used in CreateNumberTypedArray function");
                break;
        }
        return JSHandle<JSSharedTypedArray>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(handleTagValFunc), handleTagValFunc));
    }

    JSHandle<TaggedArray> NewSharedTypedArray()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSThread *thread = instance->GetJSThread();
        JSHandle<TaggedArray> array(factory->NewTaggedArray(11)); // 11: elements
        // 0: JS_SHARED_INT8_ARRAY
        array->Set(thread, 0, CreateNumberSharedTypedArray(JSType::JS_SHARED_INT8_ARRAY));
        // 1: JS_SHARED_UINT8_ARRAY
        array->Set(thread, 1, CreateNumberSharedTypedArray(JSType::JS_SHARED_UINT8_ARRAY));
        // 2: JS_SHARED_UINT8_CLAMPED_ARRAY
        array->Set(thread, 2, CreateNumberSharedTypedArray(JSType::JS_SHARED_UINT8_CLAMPED_ARRAY));
        // 3: JS_SHARED_INT16_ARRAY
        array->Set(thread, 3, CreateNumberSharedTypedArray(JSType::JS_SHARED_INT16_ARRAY));
        // 4: JS_SHARED_UINT16_ARRAY
        array->Set(thread, 4, CreateNumberSharedTypedArray(JSType::JS_SHARED_UINT16_ARRAY));
        // 5: JS_SHARED_INT32_ARRAY
        array->Set(thread, 5, CreateNumberSharedTypedArray(JSType::JS_SHARED_INT32_ARRAY));
        // 6: JS_SHARED_UINT32_ARRAY
        array->Set(thread, 6, CreateNumberSharedTypedArray(JSType::JS_SHARED_UINT32_ARRAY));
        // 7: JS_SHARED_FLOAT32_ARRAY
        array->Set(thread, 7, CreateNumberSharedTypedArray(JSType::JS_SHARED_FLOAT32_ARRAY));
        // 8: JS_SHARED_FLOAT64_ARRAY
        array->Set(thread, 8, CreateNumberSharedTypedArray(JSType::JS_SHARED_FLOAT64_ARRAY));
        // 9: JS_SHARED_BIGINT64_ARRAY
        array->Set(thread, 9, CreateNumberSharedTypedArray(JSType::JS_SHARED_BIGINT64_ARRAY));
        // 10: JS_SHARED_BIGUINT64_ARRAY
        array->Set(thread, 10, CreateNumberSharedTypedArray(JSType::JS_SHARED_BIGUINT64_ARRAY));
        return array;
    }

    JSHandle<JSObject> NewObject(uint32_t size, JSType type, JSHandle<JSTaggedValue> proto)
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSHClass> hclass = factory->NewEcmaHClass(size, type, proto);
        return factory->NewJSObjectWithInit(hclass);
    }

    JSHandle<JSObject> NewSObject(uint32_t size, JSType type, JSHandle<JSTaggedValue> proto)
    {
        ObjectFactory *factory = instance->GetFactory();
        auto emptySLayout = instance->GetJSThread()->GlobalConstants()->GetHandledEmptySLayoutInfo();
        JSHandle<JSHClass> hclass = factory->NewSEcmaHClass(size, 0, type, proto, emptySLayout);
        return factory->NewJSObjectWithInit(hclass);
    }

    // JS_SET
    JSHandle<JSSet> NewJSSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsSetObject = NewObject(JSSet::SIZE, JSType::JS_SET, proto);
        JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(jsSetObject);
        JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread));
        jsSet->SetLinkedSet(thread, linkedSet);
        return jsSet;
    }

    // JS_SHARED_SET
    JSHandle<JSSharedSet> NewJSSharedSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetSFunctionPrototype();
        JSHandle<JSObject> jsSSetObject = NewSObject(JSSharedSet::SIZE, JSType::JS_SHARED_SET, proto);
        JSHandle<JSSharedSet> jsSSet = JSHandle<JSSharedSet>::Cast(jsSSetObject);
        JSHandle<LinkedHashSet> linkedSet(
            LinkedHashSet::Create(thread, LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED));
        jsSSet->SetLinkedSet(thread, linkedSet);
        jsSSet->SetModRecord(0);
        return jsSSet;
    }

    // JS_MAP
    JSHandle<JSMap> NewJSMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsMapObject = NewObject(JSMap::SIZE, JSType::JS_MAP, proto);
        JSHandle<JSMap> jsMap = JSHandle<JSMap>::Cast(jsMapObject);
        JSHandle<LinkedHashMap> linkedMap(LinkedHashMap::Create(thread));
        jsMap->SetLinkedMap(thread, linkedMap);
        return jsMap;
    }

    // JS_SHARED_MAP
    JSHandle<JSSharedMap> NewJSSharedMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetSFunctionPrototype();
        JSHandle<JSObject> jsSMapObject = NewSObject(JSSharedMap::SIZE, JSType::JS_SHARED_MAP, proto);
        JSHandle<JSSharedMap> jsSMap = JSHandle<JSSharedMap>::Cast(jsSMapObject);
        JSHandle<LinkedHashMap> linkedMap(
            LinkedHashMap::Create(thread, LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED));
        jsSMap->SetLinkedMap(thread, linkedMap);
        jsSMap->SetModRecord(0);
        return jsSMap;
    }

    //JS_WEAK_SET
    JSHandle<JSWeakSet> NewJSWeakSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsWeakSetObject = NewObject(JSWeakSet::SIZE, JSType::JS_WEAK_SET, proto);
        JSHandle<JSWeakSet> jsWeakSet = JSHandle<JSWeakSet>::Cast(jsWeakSetObject);
        JSHandle<LinkedHashSet> weakLinkedSet(LinkedHashSet::Create(thread));
        jsWeakSet->SetLinkedSet(thread, weakLinkedSet);
        return jsWeakSet;
    }

    //JS_WEAK_MAP
    JSHandle<JSWeakMap> NewJSWeakMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsWeakMapObject = NewObject(JSWeakMap::SIZE, JSType::JS_WEAK_MAP, proto);
        JSHandle<JSWeakMap> jsWeakMap = JSHandle<JSWeakMap>::Cast(jsWeakMapObject);
        JSHandle<LinkedHashMap> weakLinkedMap(LinkedHashMap::Create(thread));
        jsWeakMap->SetLinkedMap(thread, weakLinkedMap);
        return jsWeakMap;
    }

    // JS_PROXY
    JSHandle<JSProxy> NewJSProxy()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSFunction *newTarget = instance->GetGlobalEnv()->GetObjectFunction().GetObject<JSFunction>();
        JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
        JSHandle<JSObject> jsObject =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(newTargetHandle), newTargetHandle);
        JSHandle<JSTaggedValue> emptyObj(thread, jsObject.GetTaggedValue());
        return factory->NewJSProxy(emptyObj, emptyObj);
    }

    // JS_FORIN_ITERATOR
    JSHandle<JSForInIterator> NewJSForInIterator()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> arrayEmpty(thread, factory->NewJSArray().GetTaggedValue());
        JSHandle<JSTaggedValue> keys(thread, factory->EmptyArray().GetTaggedValue());
        JSHandle<JSTaggedValue> hclass(thread, JSTaggedValue::Undefined());
        return factory->NewJSForinIterator(arrayEmpty, keys, hclass, static_cast<uint32_t>(EnumCacheKind::NONE));
    }

    // JS_REG_EXP_ITERATOR
    JSHandle<JSRegExpIterator> NewJSRegExpIterator()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<EcmaString> emptyString = factory->GetEmptyString();
        JSHandle<JSTaggedValue> jsRegExp(NewObject(JSRegExp::SIZE, JSType::JS_REG_EXP, proto));
        return factory->NewJSRegExpIterator(jsRegExp, emptyString, false, false);
    }

    // PROMISE_ITERATOR_RECORD
    JSHandle<PromiseIteratorRecord> NewPromiseIteratorRecord()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSFunction *newTarget = instance->GetGlobalEnv()->GetObjectFunction().GetObject<JSFunction>();
        JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
        JSHandle<JSObject> jsObject =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(newTargetHandle), newTargetHandle);
        JSHandle<JSTaggedValue> emptyObj(thread, jsObject.GetTaggedValue());
        return factory->NewPromiseIteratorRecord(emptyObj, false);
    }

    // JS_API_ARRAY_LIST
    JSHandle<JSAPIArrayList> NewJSAPIArrayList()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAPIArrayListObject = NewObject(JSAPIArrayList::SIZE, JSType::JS_API_ARRAY_LIST, proto);
        JSHandle<JSAPIArrayList> jsAPIArrayList = JSHandle<JSAPIArrayList>::Cast(jsAPIArrayListObject);
        JSThread *thread = instance->GetJSThread();
        jsAPIArrayList->SetLength(thread, JSTaggedValue(0));
        return jsAPIArrayList;
    }

    // JS_API_HASH_MAP
    JSHandle<JSAPIHashMap> NewJSAPIHashMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPIHashMapObject = NewObject(JSAPIHashMap::SIZE, JSType::JS_API_HASH_MAP, proto);
        JSHandle<JSAPIHashMap> jsAPIHashMap = JSHandle<JSAPIHashMap>::Cast(jsAPIHashMapObject);
        jsAPIHashMap->SetTable(thread, TaggedHashArray::Create(thread));
        jsAPIHashMap->SetSize(0);
        return jsAPIHashMap;
    }

    // JS_API_HASH_SET
    JSHandle<JSAPIHashSet> NewJSAPIHashSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPIHashSetObject = NewObject(JSAPIHashSet::SIZE, JSType::JS_API_HASH_SET, proto);
        JSHandle<JSAPIHashSet> jsAPIHashSet = JSHandle<JSAPIHashSet>::Cast(jsAPIHashSetObject);
        jsAPIHashSet->SetTable(thread, TaggedHashArray::Create(thread));
        jsAPIHashSet->SetSize(0);
        return jsAPIHashSet;
    }

    // JS_API_LIGHT_WEIGHT_MAP
    JSHandle<JSAPILightWeightMap> NewJSAPILightWeightMap()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPILightWeightMapObject =
            NewObject(JSAPILightWeightMap::SIZE, JSType::JS_API_LIGHT_WEIGHT_MAP, proto);
        JSHandle<JSAPILightWeightMap> jsAPILightWeightMap =
            JSHandle<JSAPILightWeightMap>::Cast(jsAPILightWeightMapObject);
        JSHandle<JSTaggedValue> hashArray =
            JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
        JSHandle<JSTaggedValue> keyArray =
            JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
        JSHandle<JSTaggedValue> valueArray =
            JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
        jsAPILightWeightMap->SetHashes(thread, hashArray);
        jsAPILightWeightMap->SetKeys(thread, keyArray);
        jsAPILightWeightMap->SetValues(thread, valueArray);
        jsAPILightWeightMap->SetLength(0);
        return jsAPILightWeightMap;
    }

    // JS_API_LIGHT_WEIGHT_SET
    JSHandle<JSAPILightWeightSet> NewJSAPILightWeightSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPILightWeightSetObject =
            NewObject(JSAPILightWeightSet::SIZE, JSType::JS_API_LIGHT_WEIGHT_SET, proto);
        JSHandle<JSAPILightWeightSet> jsAPILightWeightSet =
            JSHandle<JSAPILightWeightSet>::Cast(jsAPILightWeightSetObject);
        JSHandle<TaggedArray> hashes =
            JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
        JSHandle<TaggedArray> values =
            JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
        jsAPILightWeightSet->SetHashes(thread, hashes);
        jsAPILightWeightSet->SetValues(thread, values);
        jsAPILightWeightSet->SetLength(0);
        return jsAPILightWeightSet;
    }

    // JS_API_TREE_MAP
    JSHandle<JSAPITreeMap> NewJSAPITreeMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPITreeMapObject = NewObject(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
        JSHandle<JSAPITreeMap> jsAPITreeMap = JSHandle<JSAPITreeMap>::Cast(jsAPITreeMapObject);
        JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
        jsAPITreeMap->SetTreeMap(thread, treeMap);
        return jsAPITreeMap;
    }

    // JS_API_TREE_SET
    JSHandle<JSAPITreeSet> NewJSAPITreeSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPITreeSetObject = NewObject(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
        JSHandle<JSAPITreeSet> jsAPITreeSet = JSHandle<JSAPITreeSet>::Cast(jsAPITreeSetObject);
        JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
        jsAPITreeSet->SetTreeSet(thread, treeSet);
        return jsAPITreeSet;
    }

    // JS_API_QUEUE
    JSHandle<JSAPIQueue> NewJSAPIQueue()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAPIQueueObject = NewObject(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, proto);
        JSHandle<JSAPIQueue> jsAPIQueue = JSHandle<JSAPIQueue>::Cast(jsAPIQueueObject);
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIQueue::DEFAULT_CAPACITY_LENGTH);
        jsAPIQueue->SetLength(thread, JSTaggedValue(0));
        jsAPIQueue->SetFront(0);
        jsAPIQueue->SetTail(0);
        jsAPIQueue->SetElements(thread, newElements);
        return jsAPIQueue;
    }
    // JS_API_DEQUE
    JSHandle<JSAPIDeque> NewJSAPIDeque()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAPIDequeObject = NewObject(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, proto);
        JSHandle<JSAPIDeque> jsAPIDeque = JSHandle<JSAPIDeque>::Cast(jsAPIDequeObject);
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
        jsAPIDeque->SetFirst(0);
        jsAPIDeque->SetLast(0);
        jsAPIDeque->SetElements(instance->GetJSThread(), newElements);
        return jsAPIDeque;
    }
    // JS_API_STACK
    JSHandle<JSAPIStack> NewJSAPIStack()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAPIStackObject = NewObject(JSAPIStack::SIZE, JSType::JS_API_STACK, proto);
        JSHandle<JSAPIStack> jsAPIStack = JSHandle<JSAPIStack>::Cast(jsAPIStackObject);
        jsAPIStack->SetTop(0);
        return jsAPIStack;
    }

    // JS_API_PLAIN_ARRAY
    JSHandle<JSAPIPlainArray> NewJSAPIPlainArray()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPIPlainArrayObject = NewObject(JSAPIPlainArray::SIZE, JSType::JS_API_PLAIN_ARRAY, proto);
        JSHandle<JSAPIPlainArray> jsAPIPlainArray = JSHandle<JSAPIPlainArray>::Cast(jsAPIPlainArrayObject);
        JSHandle<TaggedArray> keys =
                JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
        JSHandle<TaggedArray> values =
                JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
        jsAPIPlainArray->SetKeys(thread, keys);
        jsAPIPlainArray->SetValues(thread, values);
        jsAPIPlainArray->SetLength(0);
        return jsAPIPlainArray;
    }

    // JS_API_LIST
    JSHandle<JSAPIList> NewJSAPIList()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPIListObject = NewObject(JSAPIList::SIZE, JSType::JS_API_LIST, proto);
        JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(jsAPIListObject);
        JSHandle<JSTaggedValue> taggedSingleList(thread, TaggedSingleList::Create(thread));
        jsAPIList->SetSingleList(thread, taggedSingleList);
        return jsAPIList;
    }

    // JS_API_LINKED_LIST
    JSHandle<JSAPILinkedList> NewJSAPILinkedList()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSObject> jsAPILinkedListObject = NewObject(JSAPILinkedList::SIZE, JSType::JS_API_LINKED_LIST, proto);
        JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(jsAPILinkedListObject);
        JSHandle<JSTaggedValue> linkedlist(thread, TaggedDoubleList::Create(thread));
        jsAPILinkedList->SetDoubleList(thread, linkedlist);
        return jsAPILinkedList;
    }

    // JS_API_VECTOR
    JSHandle<JSAPIVector> NewJSAPIVector()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAPIVectorObject = NewObject(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
        JSHandle<JSAPIVector> jsAPIVector = JSHandle<JSAPIVector>::Cast(jsAPIVectorObject);
        jsAPIVector->SetLength(0);
        return jsAPIVector;
    }

    // JS_WEAK_REF
    JSHandle<JSWeakRef> NewJSWeakRef()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsWeakRefObject = NewObject(JSWeakRef::SIZE, JSType::JS_WEAK_REF, proto);
        JSHandle<JSWeakRef> jsWeakRef = JSHandle<JSWeakRef>::Cast(jsWeakRefObject);
        return jsWeakRef;
    }

    // JS_FINALIZATION_REGISTRY
    JSHandle<JSFinalizationRegistry> NewJSFinalizationRegistry()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsFinalizationRegistryObject =
            NewObject(JSFinalizationRegistry::SIZE, JSType::JS_FINALIZATION_REGISTRY, proto);
        JSHandle<JSFinalizationRegistry> jsFinalizationRegistry =
            JSHandle<JSFinalizationRegistry>::Cast(jsFinalizationRegistryObject);
        return jsFinalizationRegistry;
    }

    // JS_ASYNC_FROM_SYNC_ITERATOR
    JSHandle<JSAsyncFromSyncIterator> NewJSAsyncFromSyncIterator()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsAsyncFromSyncIteratorObject =
            NewObject(JSAsyncFromSyncIterator::SIZE, JSType::JS_ASYNC_FROM_SYNC_ITERATOR, proto);
        JSHandle<JSAsyncFromSyncIterator> jsAsyncFromSyncIterator =
            JSHandle<JSAsyncFromSyncIterator>::Cast(jsAsyncFromSyncIteratorObject);
        return jsAsyncFromSyncIterator;
    }

    // JS_API_LINKED_LIST_ITERATOR
    JSHandle<JSAPILinkedListIterator> NewJSAPILinkedListIterator()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSAPILinkedList> jsAPILinkedList = NewJSAPILinkedList();
        JSHandle<JSAPILinkedListIterator> jsAPILinkedListIter =
            factory->NewJSAPILinkedListIterator(jsAPILinkedList);
        return jsAPILinkedListIter;
    }

    // JS_API_LIST_ITERATOR
    JSHandle<JSAPIListIterator> NewJSAPIListIterator()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSAPIList> jsAPIList = NewJSAPIList();
        JSHandle<JSAPIListIterator> jsAPIListIter = factory->NewJSAPIListIterator(jsAPIList);
        return jsAPIListIter;
    }

    // JS_SHARED_ARRAY_ITERATOR
    JSHandle<JSSharedArrayIterator> NewJSSharedArrayIterator()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSSharedArrayIterator> sharedArrayIter = factory->NewJSSharedArrayIterator(
            JSHandle<JSObject>::Cast(factory->NewJSSArray()), IterationKind::KEY);
        return sharedArrayIter;
    }

    // BYTE_ARRAY
    JSHandle<ByteArray> NewByteArray()
    {
        uint8_t value = 255; // 255 : test case
        int byteArrayLength = 10; // 10: arrayLength
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<ByteArray> byteArray = factory->NewByteArray(byteArrayLength, sizeof(value));
        return byteArray;
    }

    // RB_TREENODE
    JSHandle<RBTreeNode> NewRBTreeNode()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSThread *thread = instance->GetJSThread();
        std::string k("testKey");
        std::string v("testValue");
        JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(k).GetTaggedValue());
        JSHandle<JSTaggedValue> value(thread, factory->NewFromStdString(v).GetTaggedValue());
        int hash = TaggedNode::Hash(thread, factory->NewFromStdString(k).GetTaggedValue());
        JSHandle<RBTreeNode> newNode = factory->NewTreeNode(hash, key, value);
        return newNode;
    }

    // CLASS_LITERAL
    JSHandle<ClassLiteral> NewClassLiteral()
    {
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> classLiteralObject = NewObject(ClassLiteral::SIZE, JSType::CLASS_LITERAL, proto);
        JSHandle<ClassLiteral> classLiteral = JSHandle<ClassLiteral>::Cast(classLiteralObject);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<TaggedArray> emptyArray = factory->EmptyArray();

        classLiteral->SetArray(instance->GetJSThread(), emptyArray);
        classLiteral->SetIsAOTUsed(false);
        return classLiteral;
    }

    // ASYNC_ITERATOR_RECORD
    JSHandle<AsyncIteratorRecord> NewAsyncIteratorRecord()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsObject = NewObject(JSObject::SIZE, JSType::JS_OBJECT, proto);
        JSHandle<JSTaggedValue> emptyObj(jsObject);
        JSHandle<JSTaggedValue> emptyMethod(jsObject);
        JSHandle<AsyncIteratorRecord> asyncIteratorRecord =
            factory->NewAsyncIteratorRecord(emptyObj, emptyMethod, false);
        return asyncIteratorRecord;
    }

    // PROFILE_TYPE_INFO_CELL
    JSHandle<ProfileTypeInfoCell> NewProfileTypeInfoCell()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<ProfileTypeInfoCell> newProfileTypeInfoCell =
            instance->GetFactory()->NewProfileTypeInfoCell(handleUndefined);
        return newProfileTypeInfoCell;
    }
private:
    EcmaVM *instance {nullptr};
};

class MockHeapProfiler : public HeapProfilerInterface {
public:
    NO_MOVE_SEMANTIC(MockHeapProfiler);
    NO_COPY_SEMANTIC(MockHeapProfiler);
    explicit MockHeapProfiler(HeapProfilerInterface *profiler) : profiler_(profiler) {}
    ~MockHeapProfiler() override
    {
        allocEvtObj_.clear();
    };

    void AllocationEvent(TaggedObject *address, size_t size) override
    {
        allocEvtObj_.emplace(address, true);
        profiler_->AllocationEvent(address, size);
    }

    void MoveEvent(uintptr_t address, TaggedObject *forwardAddress, size_t size) override
    {
        return profiler_->MoveEvent(address, forwardAddress, size);
    }

    bool DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress = nullptr,
                          std::function<void(uint8_t)> callback = [] (uint8_t) {}) override
    {
        return profiler_->DumpHeapSnapshot(stream, dumpOption, progress, callback);
    }

    void DumpHeapSnapshotForOOM(const DumpSnapShotOption &dumpOption, bool fromSharedGC = false) override
    {
        profiler_->DumpHeapSnapshotForOOM(dumpOption, fromSharedGC);
    }

    bool StartHeapTracking(double timeInterval, bool isVmMode = true, Stream *stream = nullptr,
                           bool traceAllocation = false, bool newThread = true) override
    {
        return profiler_->StartHeapTracking(timeInterval, isVmMode, stream, traceAllocation, newThread);
    }

    bool StopHeapTracking(Stream *stream, Progress *progress = nullptr, bool newThread = true) override
    {
        return profiler_->StopHeapTracking(stream, progress, newThread);
    }

    bool UpdateHeapTracking(Stream *stream) override
    {
        return profiler_->UpdateHeapTracking(stream);
    }

    bool StartHeapSampling(uint64_t samplingInterval, int stackDepth = 128) override
    {
        return profiler_->StartHeapSampling(samplingInterval, stackDepth);
    }

    void StopHeapSampling() override
    {
        profiler_->StopHeapSampling();
    }

    const struct SamplingInfo *GetAllocationProfile() override
    {
        return profiler_->GetAllocationProfile();
    }

    size_t GetIdCount() override
    {
        return profiler_->GetIdCount();
    }

    std::unordered_map<TaggedObject *, bool> allocEvtObj_;
    HeapProfilerInterface *profiler_ {nullptr};
};

template <class Callback>
class TestAllocationEventRootVisitor final : public RootVisitor {
public:
    explicit TestAllocationEventRootVisitor(Callback &cb) : cb_(cb) {}
    ~TestAllocationEventRootVisitor() = default;
    void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
    {
        JSTaggedValue value((slot).GetTaggedType());
        if (!value.IsHeapObject()) {
            return;
        }
        TaggedObject *root = value.GetTaggedObject();
        cb_(root);
    }
    void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
    {
        for (ObjectSlot slot = start; slot < end; slot++) {
            VisitRoot(type, slot);
        }
    }
    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
        [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
private:
    Callback &cb_;
};

HWTEST_F_L0(HeapDumpTest, TestAllocationEvent)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"heapdump.abc";
    const std::string jsFileName = HPROF_TEST_JS_FILES_DIR"heapdump.js";
    MockHeapProfiler mockHeapProfiler(ecmaVm_->GetOrNewHeapProfile());
    ecmaVm_->SetHeapProfile(&mockHeapProfiler);

    std::unordered_map<TaggedObject *, bool> ObjBeforeExecute;
    std::unordered_map<TaggedObject *, bool> *ObjMap = &ObjBeforeExecute;
    auto heap = ecmaVm_->GetHeap();
    ASSERT_NE(heap, nullptr);
    auto countCb = [&ObjMap](TaggedObject *obj) {
        ObjMap->emplace(obj, true);
    };
    heap->IterateOverObjects(countCb);

    TestAllocationEventRootVisitor testAllocationEventRootVisitor(countCb);
    ecmaVm_->Iterate(testAllocationEventRootVisitor);
    thread_->Iterate(testAllocationEventRootVisitor);

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump");
    EXPECT_TRUE(result);

    std::unordered_map<TaggedObject *, bool> ObjAfterExecute;
    ObjMap = &ObjAfterExecute;
    heap->IterateOverObjects(countCb);
    ecmaVm_->Iterate(testAllocationEventRootVisitor);
    thread_->Iterate(testAllocationEventRootVisitor);
    ecmaVm_->SetHeapProfile(mockHeapProfiler.profiler_);

    std::unordered_map<std::string, int> noTraceObjCheck =
       {{"TaggedArray", 1}, {"AsyncFunction", 2}, {"LexicalEnv", 2}, {"Array", 3}, {"Function", 7}, {"Map", 1},
       {"Object", 1}, {"Uint8 Clamped Array", 1}, {"Uint32 Array", 1}, {"Float32 Array", 1}, {"Int32 Array", 1},
       {"Int16 Array", 1}, {"BigUint64 Array", 1}, {"Uint8 Array", 1}, {"Float64 Array", 1}, {"ByteArray", 11},
       {"Int8 Array", 1}, {"BigInt64 Array", 1}, {"Uint16 Array", 1}, {"ArrayIterator", 1}};
    bool pass = true;
    std::unordered_map<std::string, int> noTraceObj;
    for (auto o = ObjAfterExecute.begin(); o != ObjAfterExecute.end(); o++) {
        if (ObjBeforeExecute.find(o->first) != ObjBeforeExecute.end()) {
            continue;
        }
        if (mockHeapProfiler.allocEvtObj_.find(o->first) != mockHeapProfiler.allocEvtObj_.end()) {
            continue;
        }
        auto objType = o->first->GetClass()->GetObjectType();
        std::string typeName = ConvertToStdString(JSHClass::DumpJSType(objType));
        if (noTraceObjCheck.size() == 0) {
            LOG_ECMA(ERROR) << "Object not traced, Addr=" << o->first << ", TypeName=" << typeName;
            pass = false;
        }
        if (noTraceObj.find(typeName) == noTraceObj.end()) {
            noTraceObj.emplace(typeName, 0);
        }
        noTraceObj[typeName] += 1;
    }
    for (auto o = noTraceObj.begin(); o != noTraceObj.end(); o++) {
        if (noTraceObjCheck.find(o->first) == noTraceObjCheck.end()) {
            LOG_ECMA(ERROR) << "Object not traced, TypeName=" << o->first << ", count=" << o->second;
            pass = false;
        }
    }
    ASSERT_TRUE(pass);
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpFunctionUrl)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"heapdump.abc";

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump");
    EXPECT_TRUE(result);

    HeapDumpTestHelper tester(ecmaVm_);
    tester.GenerateSnapShot("testFunctionUrl.heapsnapshot");

    // match function url
    std::string line;
    std::ifstream inputStream("testFunctionUrl.heapsnapshot");
    bool strMatched = false;
    bool funcTempMatched = false;
    while (getline(inputStream, line)) {
        if (strMatched && funcTempMatched) {
            break;
        }
        if (line.find("heapdump.js#greet(line:98)") != std::string::npos) {
            strMatched = true;
            continue;
        }
        if (line == "\"ArkInternalFunctionTemplate\",") {
            funcTempMatched = true;
            continue;
        }
    }
    ASSERT_TRUE(strMatched);
    ASSERT_TRUE(funcTempMatched);
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpFunctionAndObject)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"heapdump1.abc";

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump1");
    EXPECT_TRUE(result);
    ecmaVm_->SetBundleName("com.example.testFunctionAndObject");
    HeapDumpTestHelper tester(ecmaVm_);
    tester.GenerateSnapShot("testFunctionAndObject.heapsnapshot");

    // match function url
    std::string line;
    std::ifstream inputStream("testFunctionAndObject.heapsnapshot");
    bool strMatched = false;
    bool strMatched1 = false;
    bool strMatched2 = false;
    bool strMatched3 = false;
    bool strMatched4 = false;
    bool strMatched5 = false;
    bool strMatched6 = false;
    bool strMatched7 = false;
    while (getline(inputStream, line)) {
        if (strMatched && strMatched1 && strMatched2 && strMatched3 &&
            strMatched4 && strMatched5 && strMatched6 && strMatched7) {
            break;
        }
        if (line.find("heapdump1.js#anonymous(line:19)") != std::string::npos) {
            strMatched = true;
            continue;
        }
        if (line.find("heapdump1.js#test(line:25)") != std::string::npos) {
            strMatched1 = true;
            continue;
        }
        if (line.find("heapdump1.js#ObjectFun(line:30)") != std::string::npos) {
            strMatched2 = true;
            continue;
        }
        if (line.find("heapdump1.js#Person(line:35)") != std::string::npos) {
            strMatched3 = true;
            continue;
        }
        if (line.find("heapdump1.js#greet(line:40)") != std::string::npos) {
            strMatched4 = true;
            continue;
        }
        if (line.find("heapdump1.js#testObject(line:51)") != std::string::npos) {
            strMatched5 = true;
            continue;
        }
        if (line.find("heapdump1.js#Student(line:62)") != std::string::npos) {
            strMatched6 = true;
            continue;
        }
        if (line.find("heapdump1.js#getMessage(line:67)") != std::string::npos) {
            strMatched7 = true;
            continue;
        }
    }
    ASSERT_TRUE(strMatched);
    ASSERT_TRUE(strMatched1);
    ASSERT_TRUE(strMatched2);
    ASSERT_TRUE(strMatched3);
    ASSERT_TRUE(strMatched4);
    ASSERT_TRUE(strMatched5);
    ASSERT_TRUE(strMatched6);
    ASSERT_TRUE(strMatched7);
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName1)
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // TAGGED_ARRAY
    factory->NewTaggedArray(10);
    // LEXICAL_ENV
    factory->NewLexicalEnv(10);
    // CONSTANT_POOL
    factory->NewConstantPool(10);
    // PROFILE_TYPE_INFO
    factory->NewProfileTypeInfo(10);
    // TAGGED_DICTIONARY
    factory->NewDictionaryArray(10);
    // AOT_LITERAL_INFO
    factory->NewAOTLiteralInfo(10);
    // VTABLE
    factory->NewVTable(10);
    // COW_TAGGED_ARRAY
    factory->NewCOWTaggedArray(10);
    // HCLASS
    JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
    factory->NewEcmaHClass(JSHClass::SIZE, JSType::HCLASS, proto);
    // LINKED_NODE
    JSHandle<LinkedNode> linkedNode(thread_, JSTaggedValue::Hole());
    factory->NewLinkedNode(1, JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole()),
        JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole()), linkedNode);
    // JS_NATIVE_POINTER
    auto newData = ecmaVm_->GetNativeAreaAllocator()->AllocateBuffer(8);
    factory->NewJSNativePointer(newData);

    tester.GenerateSnapShot("testGenerateNodeName_1.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalArray["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"LexicalEnv["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalConstantPool["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalProfileTypeInfo["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalDict["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalAOTLiteralInfo["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalVTable["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalCOWArray["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"HiddenClass(NonMovable)"));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"LinkedNode\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"JSNativePointer\""));
    // Test Not Found
    ASSERT_TRUE(!tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "*#@failed case"));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName2)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_ERROR
    JSHandle<EcmaString> handleMessage(thread_, EcmaStringAccessor::CreateEmptyString(ecmaVm_));
    factory->NewJSError(ErrorType::ERROR, handleMessage);
    // JS_EVAL_ERROR
    factory->NewJSError(ErrorType::EVAL_ERROR, handleMessage);
    // JS_RANGE_ERROR
    factory->NewJSError(ErrorType::RANGE_ERROR, handleMessage);
    // JS_TYPE_ERROR
    factory->NewJSError(ErrorType::TYPE_ERROR, handleMessage);
    // JS_AGGREGATE_ERROR
    factory->NewJSAggregateError();
    // JS_REFERENCE_ERROR
    factory->NewJSError(ErrorType::REFERENCE_ERROR, handleMessage);
    // JS_URI_ERROR
    factory->NewJSError(ErrorType::URI_ERROR, handleMessage);
    // JS_SYNTAX_ERROR
    factory->NewJSError(ErrorType::SYNTAX_ERROR, handleMessage);
    // JS_OOM_ERROR
    factory->NewJSError(ErrorType::OOM_ERROR, handleMessage);
    // JS_TERMINATION_ERROR
    factory->NewJSError(ErrorType::TERMINATION_ERROR, handleMessage);

    tester.GenerateSnapShot("testGenerateNodeName_2.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Eval Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Range Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Type Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Aggregate Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Reference Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Uri Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Syntax Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"OutOfMemory Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Termination Error\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName3)
{
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_INT8_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT8_ARRAY);
    // JS_UINT8_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT8_ARRAY);
    // JS_UINT8_CLAMPED_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT8_CLAMPED_ARRAY);
    // JS_INT16_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT16_ARRAY);
    // JS_UINT16_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT16_ARRAY);
    // JS_INT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT32_ARRAY);
    // JS_UINT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT32_ARRAY);
    // JS_FLOAT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_FLOAT32_ARRAY);
    // JS_FLOAT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_FLOAT64_ARRAY);
    // JS_BIGINT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_BIGINT64_ARRAY);
    // JS_BIGUINT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_BIGUINT64_ARRAY);

    tester.GenerateSnapShot("testGenerateNodeName_3.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint8 Clamped Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Float32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Float64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"BigInt64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"BigUint64 Array\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName4)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    // JS_SET
    tester.NewJSSet();
    // JS_SHARED_SET
    tester.NewJSSharedSet();
    // JS_MAP
    tester.NewJSMap();
    // JS_SHARED_MAP
    tester.NewJSSharedMap();
    // JS_WEAK_SET
    tester.NewJSWeakSet();
    // JS_WEAK_MAP
    tester.NewJSWeakMap();
    // JS_ARRAY
    factory->NewJSArray();
    // JS_TYPED_ARRAY
    tester.NewObject(JSTypedArray::SIZE, JSType::JS_TYPED_ARRAY, proto);
    tester.GenerateSnapShot("testGenerateNodeName_4.heapsnapshot");

    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Set\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Map\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"JSArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Typed Array\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName5)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    // JS_REG_EXP
    tester.NewObject(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    // JS_DATE
    tester.NewObject(JSDate::SIZE, JSType::JS_DATE, proto);
    // JS_ARGUMENTS
    factory->NewJSArguments();
    // JS_PROXY
    tester.NewJSProxy();
    // JS_PRIMITIVE_REF
    tester.NewObject(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, proto);
    // JS_DATA_VIEW
    factory->NewJSDataView(factory->NewJSArrayBuffer(10), 5, 5);

    tester.GenerateSnapShot("testGenerateNodeName_5.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Regexp\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Date\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Arguments\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Proxy\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Primitive\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"DataView\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName6)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_FORIN_ITERATOR
    tester.NewJSForInIterator();
    // JS_MAP_ITERATOR
    factory->NewJSMapIterator(tester.NewJSMap(), IterationKind::KEY);
    // JS_SHARED_MAP_ITERATOR
    factory->NewJSMapIterator(tester.NewJSSharedMap(), IterationKind::KEY);
    // JS_SET_ITERATOR
    factory->NewJSSetIterator(tester.NewJSSet(), IterationKind::KEY);
    // JS_SHARED_SET_ITERATOR
    factory->NewJSSetIterator(tester.NewJSSharedSet(), IterationKind::KEY);
    // JS_REG_EXP_ITERATOR
    tester.NewJSRegExpIterator();
    // JS_ARRAY_ITERATOR
    factory->NewJSArrayIterator(JSHandle<JSObject>::Cast(factory->NewJSArray()), IterationKind::KEY);
    // JS_STRING_ITERATOR
    JSStringIterator::CreateStringIterator(thread_, factory->GetEmptyString());

    tester.GenerateSnapShot("testGenerateNodeName_6.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"ForinInterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"MapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SharedMapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SharedSetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"RegExpIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"ArrayIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"StringIterator\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName7)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    // JS_ARRAY_BUFFER
    factory->NewJSArrayBuffer(10);
    // JS_SHARED_ARRAY_BUFFER
    factory->NewJSSharedArrayBuffer(10);
    // PROMISE_REACTIONS
    factory->NewPromiseReaction();
    // PROMISE_CAPABILITY
    factory->NewPromiseCapability();
    // PROMISE_ITERATOR_RECORD
    tester.NewPromiseIteratorRecord();
    // PROMISE_RECORD
    factory->NewPromiseRecord();
    // RESOLVING_FUNCTIONS_RECORD
    factory->NewResolvingFunctionsRecord();
    // JS_PROMISE
    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    tester.NewObject(JSPromise::SIZE, JSType::JS_PROMISE, proto);
    // ASYNC_GENERATOR_REQUEST
    factory->NewAsyncGeneratorRequest();

    tester.GenerateSnapShot("testGenerateNodeName_7.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"ArrayBuffer\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"SharedArrayBuffer\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseReaction\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseCapability\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseIteratorRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"ResolvingFunctionsRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"Promise\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"AsyncGeneratorRequest\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName8)
{
    auto factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    // JS_API_ARRAY_LIST
    auto jsAPIArrayList = tester.NewJSAPIArrayList();
    // JS_API_ARRAYLIST_ITERATOR
    factory->NewJSAPIArrayListIterator(jsAPIArrayList);
    // JS_API_HASH_MAP
    auto jsAPIHashMap = tester.NewJSAPIHashMap();
    // JS_API_HASHMAP_ITERATOR
    factory->NewJSAPIHashMapIterator(jsAPIHashMap, IterationKind::KEY);
    // JS_API_HASH_SET
    auto jsAPIHashSet = tester.NewJSAPIHashSet();
    // JS_API_HASHSET_ITERATOR
    factory->NewJSAPIHashSetIterator(jsAPIHashSet, IterationKind::KEY);
    // JS_API_LIGHT_WEIGHT_MAP
    auto jsAPILightWeightMap = tester.NewJSAPILightWeightMap();
    // JS_API_LIGHT_WEIGHT_MAP_ITERATOR
    factory->NewJSAPILightWeightMapIterator(jsAPILightWeightMap, IterationKind::KEY);
    // JS_API_LIGHT_WEIGHT_SET
    auto jsAPILightWeightSet = tester.NewJSAPILightWeightSet();
    // JS_API_LIGHT_WEIGHT_SET_ITERATOR
    factory->NewJSAPILightWeightSetIterator(jsAPILightWeightSet, IterationKind::KEY);

    tester.GenerateSnapShot("testGenerateNodeName_8.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"ArrayList\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"ArrayListIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"HashMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"HashSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"HashMapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"HashSetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"LightWeightMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"LightWeightMapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"LightWeightSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_8.heapsnapshot", "\"LightWeightSetIterator\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName9)
{
    auto factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    // JS_API_TREE_MAP
    auto jsAPITreeMap = tester.NewJSAPITreeMap();
    // JS_API_TREEMAP_ITERATOR
    factory->NewJSAPITreeMapIterator(jsAPITreeMap, IterationKind::KEY);
    // JS_API_TREE_SET
    auto jsAPITreeSet = tester.NewJSAPITreeSet();
    // JS_API_TREESET_ITERATOR
    factory->NewJSAPITreeSetIterator(jsAPITreeSet, IterationKind::KEY);
    // JS_API_VECTOR
    auto jsAPIVector = tester.NewJSAPIVector();
    // JS_API_VECTOR_ITERATOR
    factory->NewJSAPIVectorIterator(jsAPIVector);
    // JS_API_QUEUE
    auto jsAPIQueue = tester.NewJSAPIQueue();
    // JS_API_QUEUE_ITERATOR
    factory->NewJSAPIQueueIterator(jsAPIQueue);
    // JS_API_DEQUE
    auto jsAPIDeque = tester.NewJSAPIDeque();
    // JS_API_DEQUE_ITERATOR
    factory->NewJSAPIDequeIterator(jsAPIDeque);
    // JS_API_STACK
    auto jsAPIStack = tester.NewJSAPIStack();
    // JS_API_STACK_ITERATOR
    factory->NewJSAPIStackIterator(jsAPIStack);
    // JS_API_LIST
    tester.NewJSAPIList();
    // JS_API_LINKED_LIST
    tester.NewJSAPILinkedList();
    // JS_API_PLAIN_ARRAY
    auto jsAPIPlainArray = tester.NewJSAPIPlainArray();
    // JS_API_PLAIN_ARRAY_ITERATOR
    factory->NewJSAPIPlainArrayIterator(jsAPIPlainArray, IterationKind::KEY);

    tester.GenerateSnapShot("testGenerateNodeName_9.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"TreeMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"TreeMapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"TreeSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"TreeSetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"Vector\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"VectorIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"Queue\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"QueueIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"Deque\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"DequeIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"Stack\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"StackIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"List\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"LinkedList\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"PlainArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_9.heapsnapshot", "\"PlainArrayIterator\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName10)
{
    auto factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    // JS_WEAK_REF
    tester.NewJSWeakRef();
    // JS_FINALIZATION_REGISTRY
    tester.NewJSFinalizationRegistry();
    // JS_ASYNC_FROM_SYNC_ITERATOR
    tester.NewJSAsyncFromSyncIterator();
    // JS_API_LINKED_LIST_ITERATOR
    tester.NewJSAPILinkedListIterator();
    // JS_API_LIST_ITERATOR
    tester.NewJSAPIListIterator();
    // JS_SHARED_ARRAY_ITERATOR
    tester.NewJSSharedArrayIterator();
    // JS_SHARED_INT8_ARRAY
    // JS_SHARED_UINT8_ARRAY
    // JS_SHARED_UINT8_CLAMPED_ARRAY
    // JS_SHARED_INT16_ARRAY
    // JS_SHARED_UINT16_ARRAY
    // JS_SHARED_INT32_ARRAY
    // JS_SHARED_UINT32_ARRAY
    // JS_SHARED_FLOAT32_ARRAY
    // JS_SHARED_FLOAT64_ARRAY
    // JS_SHARED_BIGINT64_ARRAY
    // JS_SHARED_BIGUINT64_ARRAY
    tester.NewSharedTypedArray();
    // BYTE_ARRAY
    tester.NewByteArray();
    // RB_TREENODE
    tester.NewRBTreeNode();
    // ENUM_CACHE
    factory->NewEnumCache();
    // CLASS_LITERAL
    factory->NewClassLiteral();
    // ASYNC_ITERATOR_RECORD
    tester.NewAsyncIteratorRecord();
    // PROFILE_TYPE_INFO_CELL
    tester.NewProfileTypeInfoCell();
    // CELL_RECORD
    factory->NewCellRecord();
    // NATIVE_MODULE_FAILURE_INFO
    factory->NewNativeModuleFailureInfo();
    // MUTANT_TAGGED_ARRAY
    factory->NewMutantTaggedArray(10); // 10: arrayLength
    // COW_MUTANT_TAGGED_ARRAY
    factory->NewCOWMutantTaggedArray(10); // 10: arrayLength
    // EXTRA_PROFILE_TYPE_INFO
    factory->NewExtraProfileTypeInfo();

    tester.GenerateSnapShot("testGenerateNodeName_10.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"WeakRef\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"JSFinalizationRegistry\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"AsyncFromSyncIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"LinkedListIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"ListIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"SharedArrayIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"ByteArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"RBTreeNode\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"EnumCache\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"ClassLiteral\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"AsyncIteratorRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"ProfileTypeInfoCell\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"CellRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"NativeModuleFailureInfo\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"MutantTaggedArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"COWMutantTaggedArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"ExtraProfileTypeInfo\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Int8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Uint8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Uint8 Clamped Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Int16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Uint16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Int32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Uint32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Float32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared Float64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared BigInt64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_10.heapsnapshot", "\"Shared BigUint64 Array\""));
}

#ifndef PANDA_TARGET_ARM32
#define CREATE_OBJECT_AND_ADD_REFS(generator, typeName, refs)           \
    {                                                                   \
        auto obj = generator->New##typeName();                          \
        refs.emplace_back(#typeName, obj.GetTaggedValue());             \
        obj.GetTaggedValue().DumpForSnapshot(thread, refs);             \
    }

#define CREATE_ARRAY_AND_ADD_REFS(generator, typeName, length, refs)    \
    {                                                                   \
        auto obj = generator->New##typeName(length);                    \
        refs.emplace_back(#typeName, obj.GetTaggedValue());             \
        obj.GetTaggedValue().DumpForSnapshot(thread, refs);             \
    }

void CreateObjectsForBinaryDump(JSThread *thread, ObjectFactory *factory, HeapDumpTestHelper *tester,
                                std::vector<Reference> &refs)
{
    CREATE_OBJECT_AND_ADD_REFS(factory, PromiseRecord, refs)
    CREATE_OBJECT_AND_ADD_REFS(factory, PromiseReaction, refs)
    CREATE_OBJECT_AND_ADD_REFS(factory, PromiseCapability, refs)
    CREATE_OBJECT_AND_ADD_REFS(factory, ResolvingFunctionsRecord, refs)
    CREATE_OBJECT_AND_ADD_REFS(factory, AsyncGeneratorRequest, refs)
    CREATE_OBJECT_AND_ADD_REFS(tester, PromiseIteratorRecord, refs)
    CREATE_OBJECT_AND_ADD_REFS(tester, JSWeakSet, refs)
    CREATE_OBJECT_AND_ADD_REFS(tester, JSWeakMap, refs)
    CREATE_ARRAY_AND_ADD_REFS(factory, JSArrayBuffer, 10, refs)
    CREATE_ARRAY_AND_ADD_REFS(factory, JSSharedArrayBuffer, 10, refs)
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpBinaryDumpV0)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    std::string rawHeapPath("test_binary_dump_v0.rawheap");
    DumpSnapShotOption dumpOption;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption));

    FILE* file = std::fopen(rawHeapPath.c_str(), "rb+");
    ASSERT_TRUE(file != nullptr);

    char version[sizeof(uint64_t)];
    *reinterpret_cast<uint64_t *>(version) = 0;
    std::fseek(file, 0, SEEK_SET);
    std::fwrite(version, sizeof(char), sizeof(version), file);
    std::fclose(file);

    std::string heapsnapshotPath("test_binary_dump_v0.heapsnapshot");
    ASSERT_TRUE(tester.AddMetaDataJsonToRawheap(rawHeapPath));
    ASSERT_TRUE(tester.DecodeRawheap(rawHeapPath, heapsnapshotPath));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpBinaryDumpV1)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    std::string rawHeapPath("test_binary_dump_v1.rawheap");
    DumpSnapShotOption dumpOption;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption));

    CSet<JSTaggedType> dumpObjects;
    ASSERT_TRUE(tester.DecodeRawHeapObjectTableV1(rawHeapPath, dumpObjects));

    ASSERT_TRUE(tester.MatchHeapObjectReferencesV1(vec, dumpObjects));

    std::string heapsnapshotPath("test_binary_dump_v1.heapsnapshot");
    ASSERT_TRUE(tester.AddMetaDataJsonToRawheap(rawHeapPath));
    ASSERT_TRUE(tester.DecodeRawheap(rawHeapPath, heapsnapshotPath));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpBinaryDumpV2)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    std::string rawHeapPath("test_binary_dump_v2.rawheap");
    Runtime::GetInstance()->SetRawHeapDumpCropLevel(RawHeapDumpCropLevel::LEVEL_V2);
    DumpSnapShotOption dumpOption;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption));

    CSet<uint32_t> dumpObjects;
    ASSERT_TRUE(tester.DecodeRawHeapObjectTableV2(rawHeapPath, dumpObjects));

    ASSERT_TRUE(tester.MatchHeapObjectReferencesV2(vec, dumpObjects));
    Runtime::GetInstance()->SetRawHeapDumpCropLevel(RawHeapDumpCropLevel::DEFAULT);

    std::string heapsnapshotPath("test_binary_dump_v2.heapsnapshot");
    ASSERT_TRUE(tester.AddMetaDataJsonToRawheap(rawHeapPath));
    ASSERT_TRUE(tester.DecodeRawheap(rawHeapPath, heapsnapshotPath));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpBinaryDumpByForkWithCallback)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    bool status = true;
    std::string rawHeapPath("test_binary_dump_by_fork_with_callback.raw");
    auto cb = [&status](uint8_t retCode) {
        if (retCode != static_cast<uint8_t>(DumpHeapSnapshotStatus::SUCCESS)) {
            status = false;
        }
    };
    DumpSnapShotOption dumpOption;
    dumpOption.isSync = false;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption, nullptr, cb));
    ASSERT_TRUE(status);
}

HWTEST_F_L0(HeapDumpTest, TestGenerateHashInRawheap)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    for (auto ref : vec) {
        if (ref.value_.IsJSObject()) {
            int32_t hash = base::RandomGenerator::GenerateIdentityHash();
            JSHandle<ECMAObject> ecmaObj(thread_, ref.value_);
            ECMAObject::SetHash(thread_, hash, ecmaObj);
        }
    }

    std::string rawHeapPath("test_binary_dump_for_mixed_node_id.rawheap");
    DumpSnapShotOption dumpOption;
    dumpOption.isSync = false;
    dumpOption.isJSLeakWatcher = true;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption));
    ASSERT_TRUE(tester.AddMetaDataJsonToRawheap(rawHeapPath));
    ASSERT_TRUE(tester.CheckHashInRawheap(thread_, vec, rawHeapPath));
}

HWTEST_F_L0(HeapDumpTest, TestDecodeRawheapAddrTableItemSizeMin)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    std::vector<Reference> vec;
    CreateObjectsForBinaryDump(thread_, factory, &tester, vec);

    std::string rawHeapPath("test_binary_dump_addrtable_size_min.rawheap");
    DumpSnapShotOption dumpOption;
    ASSERT_TRUE(tester.GenerateRawHeapSnashot(rawHeapPath, dumpOption));

    std::vector<uint32_t> sections;
    ASSERT_TRUE(tester.ReadRawHeapSectionInfo(rawHeapPath, sections));

    FILE* fd = std::fopen(rawHeapPath.c_str(), "rb+");
    ASSERT_TRUE(fd != nullptr);
    uint32_t testUnitSize = sizeof(uint64_t) - 1;
    std::fseek(fd, sections[4] + sizeof(uint32_t), SEEK_SET);
    std::fwrite(reinterpret_cast<char *>(&testUnitSize), sizeof(testUnitSize), sizeof(testUnitSize), fd);
    std::fclose(fd);

    std::string heapsnapshotPath("test_binary_dump_addrtable_size_min.heapsnapshot");
    ASSERT_TRUE(tester.AddMetaDataJsonToRawheap(rawHeapPath));
    ASSERT_FALSE(tester.DecodeRawheap(rawHeapPath, heapsnapshotPath));
}
#endif

HWTEST_F_L0(HeapDumpTest, TestSharedFullGCInHeapDump)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    // JS_SET
    tester.NewJSSet();
    // JS_SHARED_SET
    tester.NewJSSharedSet();
    // JS_MAP
    tester.NewJSMap();
    // JS_SHARED_MAP
    tester.NewJSSharedMap();
    // JS_WEAK_SET
    tester.NewJSWeakSet();
    // JS_WEAK_MAP
    tester.NewJSWeakMap();
    // JS_ARRAY
    factory->NewJSArray();
    // JS_TYPED_ARRAY
    tester.NewObject(JSTypedArray::SIZE, JSType::JS_TYPED_ARRAY, proto);
    tester.GenerateSnapShot("testGenerateNodeName_4.heapsnapshot");

    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Set\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Map\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"JSArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Typed Array\""));

    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread_);

    tester.GenerateSnapShot("testGenerateNodeName_4.heapsnapshot");

    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Set\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Map\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"JSArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Typed Array\""));
}

HWTEST_F_L0(HeapDumpTest, TestMarkObjects)
{
    HeapDumpTestHelper tester(ecmaVm_);
    JSHandle<JSMap> jsMap = tester.NewJSMap();
    JSHandle<JSSet> jsSet = tester.NewJSSet();
    JSHandle<JSAPIArrayList> jsAPIArrayList = tester.NewJSAPIArrayList();
    [[maybe_unused]] JSHandle<JSAPIDeque> jsAPIDeque = tester.NewJSAPIDeque();
    [[maybe_unused]] JSHandle<JSSharedMap> jsSharedMap = tester.NewJSSharedMap();
    [[maybe_unused]] JSHandle<JSWeakMap> jsWeakMap = tester.NewJSWeakMap();

    HeapMarker marker {};
    marker.Mark(jsMap.GetTaggedType());
    marker.Mark(jsSet.GetTaggedType());
    marker.Mark(jsAPIArrayList.GetTaggedType());

    ASSERT_TRUE(marker.IsMarked(jsMap.GetTaggedType()));
    ASSERT_FALSE(marker.IsMarked(jsSharedMap.GetTaggedType()));
}

HWTEST_F_L0(HeapDumpTest, TestIterateMarkedObjects)
{
    HeapDumpTestHelper tester(ecmaVm_);
    JSHandle<JSMap> jsMap = tester.NewJSMap();
    JSHandle<JSSet> jsSet = tester.NewJSSet();
    JSHandle<JSAPIArrayList> jsAPIArrayList = tester.NewJSAPIArrayList();
    [[maybe_unused]] JSHandle<JSAPIDeque> jsAPIDeque = tester.NewJSAPIDeque();
    [[maybe_unused]] JSHandle<JSSharedMap> jsSharedMap = tester.NewJSSharedMap();
    [[maybe_unused]] JSHandle<JSWeakMap> jsWeakMap = tester.NewJSWeakMap();

    HeapMarker marker {};
    marker.Mark(jsMap.GetTaggedType());
    marker.Mark(jsSet.GetTaggedType());
    marker.Mark(jsAPIArrayList.GetTaggedType());

    std::unordered_set<uintptr_t> markedObjects;
    markedObjects.emplace(jsMap.GetTaggedType());
    markedObjects.emplace(jsSet.GetTaggedType());
    markedObjects.emplace(jsAPIArrayList.GetTaggedType());
    std::function<void(JSTaggedType addr)> callback = [markedObjects](JSTaggedType addr) {
        ASSERT_TRUE(markedObjects.find(addr) != markedObjects.end());
    };
    marker.IterateMarked(callback);
}

HWTEST_F_L0(HeapDumpTest, TestRemoveUnmarkedObjects)
{
    HeapDumpTestHelper tester(ecmaVm_);
    JSHandle<JSMap> jsMap = tester.NewJSMap();
    JSHandle<JSSet> jsSet = tester.NewJSSet();
    JSHandle<JSAPIArrayList> jsAPIArrayList = tester.NewJSAPIArrayList();
    JSHandle<JSAPIDeque> jsAPIDeque = tester.NewJSAPIDeque();
    JSHandle<JSSharedMap> jsSharedMap = tester.NewJSSharedMap();
    JSHandle<JSWeakMap> jsWeakMap = tester.NewJSWeakMap();

    EntryIdMap entryIdMap {};
    entryIdMap.InsertId(jsMap.GetTaggedType(), entryIdMap.GetNextId());
    entryIdMap.InsertId(jsSet.GetTaggedType(), entryIdMap.GetNextId());
    entryIdMap.InsertId(jsAPIArrayList.GetTaggedType(), entryIdMap.GetNextId());
    entryIdMap.InsertId(jsAPIDeque.GetTaggedType(), entryIdMap.GetNextId());
    entryIdMap.InsertId(jsSharedMap.GetTaggedType(), entryIdMap.GetNextId());
    entryIdMap.InsertId(jsWeakMap.GetTaggedType(), entryIdMap.GetNextId());
    ASSERT_EQ(entryIdMap.GetIdMap()->size(), 6);

    HeapMarker marker {};
    marker.Mark(jsMap.GetTaggedType());
    marker.Mark(jsSet.GetTaggedType());
    marker.Mark(jsAPIArrayList.GetTaggedType());
    marker.Mark(jsAPIDeque.GetTaggedType());

    entryIdMap.RemoveUnmarkedObjects(marker);
    ASSERT_EQ(entryIdMap.GetIdMap()->size(), 4);
}
}
