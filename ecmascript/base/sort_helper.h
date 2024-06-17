/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_BASE_SORT_HELPER_H
#define ECMASCRIPT_BASE_SORT_HELPER_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript::base {
struct run {
    int base;
    int len;
    run(int b, int l) : base(b), len(l) {
    }
};

class TimSort {
    TimSort(JSThread *thread, const JSHandle<TaggedArray> &elements, const JSHandle<JSTaggedValue> &fn)
        : thread_(thread), elements_(elements), fn_(fn),
          minGallop_(MIN_GALLOP) {}
    ~TimSort() {}

    JSThread *thread_ {nullptr};
    JSHandle<TaggedArray> elements_;
    JSHandle<JSTaggedValue> fn_;

    static const int MIN_MERGE = 32;
    static const int MIN_GALLOP = 7;
    static const int tempSize = 32;
    int minGallop_; // default to MIN_GALLOP
    JSHandle<TaggedArray> tmp_ {}; // temp storage for merges
    std::vector<run> pending_;

    static void BinarySort(JSThread *thread, JSHandle<TaggedArray> &array,
                           int lo, int hi, int start, JSHandle<JSTaggedValue> fn);
    static int CountRunAndMakeAscending(JSThread *thread, JSHandle<TaggedArray> &array, int lo, int h,
                                        const JSHandle<JSTaggedValue> &fn);
    static void ReverseRange(JSThread *thread, JSHandle<TaggedArray> &array, int from, int to);
    void MergeCollapse();
    void MergeForceCollapse();
    void MergeAt(int i);
    int GallopLeft(JSHandle<TaggedArray> &array, JSHandle<JSTaggedValue> key, int base, int len, int hint);
    int GallopRight(JSHandle<TaggedArray> &array, JSHandle<JSTaggedValue> key, int base, int len, int hint);
    void MergeLo(int base1, int len1, int base2, int len2);
    void MergeHi(int base1, int len1, int base2, int len2);
    void CopyArray(JSHandle<TaggedArray> &src, int srcPos,
                   JSHandle<TaggedArray> &dst, int dstPos, int length);

    JSHandle<TaggedArray> GetTempArray(int requestedSize)
    {
        int minSize = std::max(requestedSize, 32);
        if (!tmp_.IsEmpty()) {
            int currentSize = static_cast<int>(tmp_->GetLength());
            if (currentSize >= minSize) {
                return tmp_;
            }
        }
        tmp_ = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(minSize);
        return tmp_;
    }

    void PushRun(const int runBase, const int runLen)
    {
        pending_.push_back(run(runBase, runLen));
    }

    static int ComputeMinRunLength(int n)
    {
        int r = 0;
        while (n >= 2 * MIN_MERGE) { // 2 * MIN_MERGE : 64
            r |= (n & 1);
            n >>= 1;
        }
        return n + r;
    }

public:
    static void Sort(JSThread *thread, JSHandle<TaggedArray> &elements, const JSHandle<JSTaggedValue> &fn);
};
}
#endif