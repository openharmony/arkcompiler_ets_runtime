/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_H
#define ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_H

#include "ecmascript/compiler/aot_snapshot/snapshot_global_data.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_runtime_options.h"

namespace panda::ecmascript::kungfu {
class AOTSnapshot {
public:
    explicit AOTSnapshot(EcmaVM *vm)
        : vm_(vm), factory_(vm->GetFactory()), thread_(vm_->GetJSThread()) {}

    void Iterate(RootVisitor &v)
    {
        snapshotData_.Iterate(v);
    }

    JSTaggedValue GetSnapshotData() const
    {
        return snapshotData_.GetData();
    }

    void StoreSymbolInfo(JSHandle<TaggedArray> info)
    {
        snapshotData_.StoreSymbolInfo(info);
    }

    JSTaggedValue GetSymbolInfo() const
    {
        return snapshotData_.GetSymbolInfo();
    }

    void StoreHClassInfo(JSHandle<TaggedArray> info)
    {
        snapshotData_.StoreHClassInfo(info);
    }

    void StoreArrayInfo(JSHandle<TaggedArray> info)
    {
        snapshotData_.StoreArrayInfo(info);
    }

    void StoreProtoTransTableInfo(JSHandle<JSTaggedValue> info)
    {
        snapshotData_.StoreProtoTransTableInfo(info);
    }

    JSTaggedValue GetArrayInfo()
    {
        return snapshotData_.GetArrayInfo();
    }

    void StoreConstantIndexInfo(JSHandle<TaggedArray> info)
    {
        snapshotData_.StoreConstantIndexInfo(info);
    }

    void InitSnapshot(uint32_t compileFilesCount);

    void PUBLIC_API StoreConstantPoolInfo(BytecodeInfoCollector *bcInfoCollector);

    void ResolveSnapshotData(const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap)
    {
        snapshotData_.ResolveSnapshotData(thread_, methodToEntryIndexMap);
    }

private:
    JSHandle<ConstantPool> NewSnapshotConstantPool(uint32_t cacheSize);

    void GenerateSnapshotConstantPools(
        const CMap<int32_t, JSTaggedValue> &allConstantPools, const CString &fileName, uint32_t fileIndex);

    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSThread *thread_ {nullptr};
    SnapshotGlobalData snapshotData_ {};
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_H
