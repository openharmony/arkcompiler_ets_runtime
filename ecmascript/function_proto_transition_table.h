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

#ifndef ECMASCRIPT_CLASS_LIKE_FUNCTION_TRANSITION_MAP_H
#define ECMASCRIPT_CLASS_LIKE_FUNCTION_TRANSITION_MAP_H

#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
class FunctionProtoTransitionTable {
public:
    explicit FunctionProtoTransitionTable(const JSThread *thread);
    virtual ~FunctionProtoTransitionTable();

    void Iterate(const RootVisitor &v);

    void InsertTransitionItem(const JSThread *thread,
                              JSHandle<JSTaggedValue> ihc,
                              JSHandle<JSTaggedValue> baseIhc,
                              JSHandle<JSTaggedValue> transIhc,
                              JSHandle<JSTaggedValue> transPhc);

    std::pair<JSTaggedValue, JSTaggedValue> FindTransitionByHClass(const JSThread *thread,
                                                                   JSHandle<JSTaggedValue> ihc,
                                                                   JSHandle<JSTaggedValue> baseIhc) const;

    bool TryInsertFakeParentItem(JSTaggedType child, JSTaggedType parent);
    JSTaggedType GetFakeParent(JSTaggedType child) const;

    JSTaggedValue GetProtoTransitionTable() const
    {
        return protoTransitionTable_;
    }

    void UpdateProtoTransitionTable(const JSThread *thread, JSHandle<PointerToIndexDictionary> map);

    class ProtoArray : public TaggedArray {
    public:
        CAST_CHECK(ProtoArray, IsTaggedArray);

        static JSHandle<ProtoArray> Create(const JSThread *thread);
        void SetEntry(const JSThread *thread, uint32_t index, JSTaggedValue key, JSTaggedValue transIhc,
                      JSTaggedValue transPhc);
        JSTaggedValue GetKey(uint32_t index) const;
        std::pair<JSTaggedValue, JSTaggedValue> GetTransition(uint32_t index) const;
        int32_t FindEntry(JSTaggedValue key) const;
        std::pair<JSTaggedValue, JSTaggedValue> FindTransition(JSTaggedValue key) const;
        static bool GetEntry(JSHandle<JSHClass> hclass);

    private:
        static constexpr uint32_t ENTRY_NUMBER = 2;
        static constexpr uint32_t INIT_ENTRY_INDEX = 0;
        static constexpr uint32_t CLONED_ENTRY_INDEX = 1;

        static constexpr uint32_t ENTRY_SIZE = 3;
        static constexpr uint32_t KEY_INDEX = 0;
        static constexpr uint32_t TRANS_IHC_INDEX = 1;
        static constexpr uint32_t TRANS_PHC_INDEX = 2;
    };

private:
    static constexpr uint32_t DEFAULT_FIRST_LEVEL_SIZE = 2;
    // second level table:
    // <PointerToIndexDictionary(n):
    //      [ihc,
    //          <ProtoArray(2):
    //              [base.ihc,
    //                  (trans_ihc, trans_phc)
    //              ]
    //              [base.ihc(cloned),
    //                  (trans_ihc, trans_phc)
    //              ]
    //          >
    //      ]
    //      ...
    //  >
    JSTaggedValue protoTransitionTable_ {JSTaggedValue::Hole()};
    // hclasses after set prototype maps to hclasses before set prototype
    CUnorderedMap<JSTaggedType, JSTaggedType> fakeParentMap_ {};
    // reverse fakeParentMap_
    CUnorderedMap<JSTaggedType, JSTaggedType> fakeChildMap_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_CLASS_LIKE_FUNCTION_TRANSITION_MAP_H