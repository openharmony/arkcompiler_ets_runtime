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

#include "ecmascript/base/sort_helper.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::base {
void TimSort::Sort(JSThread *thread, JSHandle<TaggedArray> &elements, const JSHandle<JSTaggedValue> &fn)
{
    uint32_t low = 0;
    uint32_t len = elements->GetLength();
    uint32_t nRemaining = len;
    if (nRemaining < 2) { // 2: means sorted.
        return;
    }
    if (nRemaining < MIN_MERGE) {
        int initRunLen = CountRunAndMakeAscending(thread, elements, low, len, fn);
        BinarySort(thread, elements, low, len, low + initRunLen, fn);
        return;
    }

    TimSort ts(thread, elements, fn);
    uint32_t minRunLength = static_cast<uint32_t>(ComputeMinRunLength(len));

    while (nRemaining != 0) {
        uint32_t runLen = static_cast<uint32_t>(CountRunAndMakeAscending(thread, elements, low, len, fn));
        if (runLen < minRunLength) {
            uint32_t force = std::min(nRemaining, minRunLength);
            BinarySort(thread, elements, low, low + force, low + runLen, fn);
            runLen = force;
        }

        ts.PushRun(low, runLen);
        ts.MergeCollapse();
        low += runLen;
        nRemaining -= runLen;
    }
    ASSERT(low = len);
    ts.MergeForceCollapse();
    ASSERT(ts.pending_.size() == 1);
}

int TimSort::CountRunAndMakeAscending(JSThread *thread, JSHandle<TaggedArray> &array,
                                      int lo, int hi, const JSHandle<JSTaggedValue> &fn)
{
    ASSERT(lo < hi);
    int runHi = lo + 1;
    if (runHi == hi) {
        return 1;
    }
    int runLength = 2;
    JSMutableHandle<JSTaggedValue> runHiValue(thread, array->Get(thread, runHi));
    JSMutableHandle<JSTaggedValue> previousValue(thread, array->Get(thread, runHi - 1));
    double order = ArrayHelper::SortCompare(thread, fn, runHiValue, previousValue);
    bool isDescending = order < 0 ? true : false;
    previousValue.Update(runHiValue.GetTaggedValue());
    for (int i = runHi + 1; i < hi; i++) {
        runHiValue.Update(array->Get(thread, i));
        order = ArrayHelper::SortCompare(thread, fn, runHiValue, previousValue);
        if (isDescending) {
            if (order >= 0) break;
        } else {
            if (order < 0) break;
        }
        previousValue.Update(runHiValue.GetTaggedValue());
        ++runLength;
    }
    if (isDescending) {
        ReverseRange(thread, array, lo, lo + runLength);
    }
    return runLength;
}

void TimSort::ReverseRange(JSThread *thread, JSHandle<TaggedArray> &array, int from, int to)
{
    int low = from;
    int high = to - 1;
    JSMutableHandle<JSTaggedValue> elementLow(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> elementHigh(thread, JSTaggedValue::Undefined());
    while (low < high) {
        elementLow.Update(array->Get(thread, low));
        elementHigh.Update(array->Get(thread, high));
        array->Set(thread, low++, elementHigh);
        array->Set(thread, high--, elementLow);
    }
}

void TimSort::BinarySort(JSThread *thread, JSHandle<TaggedArray> &array,
                         int lo, int hi, int start, JSHandle<JSTaggedValue> fn)
{
    ASSERT(lo <= start && start <= hi);
    if (start == lo) {
        start++;
    }
    JSMutableHandle<JSTaggedValue> midVal(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> tmpVal(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> pivotVal(thread, JSTaggedValue::Undefined());
    for (; start < hi; start++) {
        int left = lo;
        int right = start;
        pivotVal.Update(array->Get(thread, right));
        ASSERT(left <= right);
        while (left < right) {
            int mid = (left + right) >> 1;
            midVal.Update(array->Get(thread, mid));
            if (ArrayHelper::SortCompare(thread, fn, pivotVal, midVal) < 0) {
                right = mid;
            } else {
                left = mid + 1;
            }
        }
        ASSERT(left == right);

        for (int p = start; p > left; --p) {
            tmpVal.Update(array->Get(thread, p - 1));
            array->Set(thread, p, tmpVal);
        }
        array->Set(thread, left, pivotVal);
    }
}

void TimSort::MergeCollapse()
{
    while (pending_.size() > 1) {
        int n = static_cast<int>(pending_.size() - 2);
        if ((n > 0 && pending_[n - 1].len <= pending_[n].len + pending_[n + 1].len) ||
            (n > 1 && pending_[n - 2].len <= pending_[n - 1].len + pending_[n].len)) { //2: means minus 2
            if (pending_[n - 1].len < pending_[n + 1].len) {
                --n;
            }
            MergeAt(n);
        } else if (pending_[n].len <= pending_[n + 1].len) {
            MergeAt(n);
        } else {
            break;
        }
    }
}

void TimSort::MergeForceCollapse()
{
    while (pending_.size() > 1) {
        int n = static_cast<int>(pending_.size() - 2);
        if (n > 0 && pending_[n - 1].len < pending_[n + 1].len) {
            --n;
        }
        MergeAt(n);
    }
}

void TimSort::MergeAt(int i)
{
    const int stackSize = static_cast<int>(pending_.size());
    ASSERT(stackSize >= 2); // 2: stackSize
    ASSERT(i >= 0);
    ASSERT(i == stackSize - 2 || i == stackSize - 3);  // the 2nd-last and 3rd-last run.

    int base1 = pending_[i].base;
    int len1 = pending_[i].len;
    int base2 = pending_[i + 1].base;
    int len2 = pending_[i + 1].len;

    ASSERT(len1 > 0 && len2 > 0);
    ASSERT(base1 + len1 == base2);

    pending_[i].len = len1 + len2;
    if (i == stackSize - 3) { // 3: 3rd-last run
        pending_[i + 1] = pending_[i + 2]; // 2: means plus 2
    }
    pending_.pop_back();

    JSHandle<JSTaggedValue> key1(thread_, elements_->Get(thread_, base2));
    int k = GallopRight(elements_, key1, base1, len1, 0);
    ASSERT(k >= 0);
    base1 += k;
    len1 -= k;
    if (len1 == 0) {
        return;
    }
    JSHandle<JSTaggedValue> key2(thread_, elements_->Get(thread_, base1 + len1 - 1));
    len2 = GallopLeft(elements_, key2, base2, len2, len2 - 1);
    ASSERT(len2 >= 0);
    if (len2 == 0) {
        return;
    }
    // Merge remaining runs, using tmp array with min(len1, len2) elements_
    if (len1 <= len2) {
        MergeLo(base1, len1, base2, len2);
    } else {
        MergeHi(base1, len1, base2, len2);
    }
}

int TimSort::GallopLeft(JSHandle<TaggedArray> &array,
                        JSHandle<JSTaggedValue> key, int base, int len, int hint)
{
    ASSERT(len > 0 && hint >= 0 && hint < len);
    int lastOfs = 0;
    int ofs = 1;
    JSHandle<JSTaggedValue> baseHintElement(thread_, array->Get(thread_, base + hint));
    JSMutableHandle<JSTaggedValue> offsetElement(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mElement(thread_, JSTaggedValue::Undefined());
    double order = ArrayHelper::SortCompare(thread_, fn_, key, baseHintElement);
    if (order > 0) {
        int maxOfs = len - hint;
        while (ofs < maxOfs) {
            offsetElement.Update(array->Get(thread_, base + hint + ofs));
            order = ArrayHelper::SortCompare(thread_, fn_, key, offsetElement);
            if (order <= 0) break;

            lastOfs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0) {
                ofs = maxOfs;
            }
        }
        if (ofs > maxOfs) {
            ofs = maxOfs;
        }
        lastOfs += hint;
        ofs += hint;
    } else {
        int maxOfs = hint + 1;
        while (ofs < maxOfs) {
            offsetElement.Update(array->Get(thread_, base + hint - ofs));
            order = ArrayHelper::SortCompare(thread_, fn_, key, offsetElement);
            if (order > 0) break;

            lastOfs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0) {
                ofs = maxOfs;
            }
        }
        if (ofs > maxOfs) {
            ofs = maxOfs;
        }
        int tmp = lastOfs;
        lastOfs = hint - ofs;
        ofs = hint - tmp;
    }
    ASSERT(-1 <= lastOfs && lastOfs < ofs && ofs <= len);
    lastOfs++;
    while (lastOfs < ofs) {
        int m = lastOfs + ((ofs - lastOfs) >> 1);
        mElement.Update(array->Get(thread_, base + m));
        if (ArrayHelper::SortCompare(thread_, fn_, key, mElement) > 0) {
            lastOfs = m + 1;
        } else {
            ofs = m;
        }
    }
    ASSERT(lastOfs == ofs);
    return ofs;
}

int TimSort::GallopRight(JSHandle<TaggedArray> &array,
                         JSHandle<JSTaggedValue> key, int base, int len, int hint)
{
    ASSERT(len > 0 && hint >= 0 && hint < len);
    int lastOfs = 0;
    int ofs = 1;
    JSHandle<JSTaggedValue> baseHintElement(thread_, array->Get(thread_, base + hint));
    JSMutableHandle<JSTaggedValue> offsetElement(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mElement(thread_, JSTaggedValue::Undefined());
    double order = ArrayHelper::SortCompare(thread_, fn_, key, baseHintElement);
    if (order < 0) {
        int maxOfs = hint + 1;
        while (ofs < maxOfs) {
            offsetElement.Update(array->Get(thread_, base + hint - ofs));
            order = ArrayHelper::SortCompare(thread_, fn_, key, offsetElement);
            if (order >= 0) break;

            lastOfs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0) {
                ofs = maxOfs;
            }
        }
        if (ofs > maxOfs) {
            ofs = maxOfs;
        }
        int tmp = lastOfs;
        lastOfs = hint - ofs;
        ofs = hint - tmp;
    } else {
        int maxOfs = len - hint;
        while (ofs < maxOfs) {
            offsetElement.Update(array->Get(thread_, base + hint + ofs));
            order = ArrayHelper::SortCompare(thread_, fn_, key, offsetElement);
            if (order < 0) break;

            lastOfs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0) {
                ofs = maxOfs;
            }
        }
        if (ofs > maxOfs) {
            ofs = maxOfs;
        }
        lastOfs += hint;
        ofs += hint;
    }
    ASSERT(-1 <= lastOfs && lastOfs < ofs && ofs <= len);
    lastOfs++;
    while (lastOfs < ofs) {
        int m = lastOfs + ((ofs - lastOfs) >> 1);
        mElement.Update(array->Get(thread_, base + m));
        if (ArrayHelper::SortCompare(thread_, fn_, key, mElement) < 0) {
            ofs = m;
        } else {
            lastOfs = m + 1;
        }
    }
    return ofs;
}

void TimSort::MergeLo(int base1, int len1, int base2, int len2)
{
    ASSERT(len1 > 0 && len2 > 0 && base1 + len1 == base2);
    JSHandle<TaggedArray> workArray = elements_;
    JSHandle<TaggedArray> tmpArray = GetTempArray(len1);
    this->CopyArray(workArray, base1, tmpArray, 0, len1);

    JSMutableHandle<JSTaggedValue> tmpElement(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cmp1Element(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cmp2Element(thread_, JSTaggedValue::Undefined());

    int cursor1 = 0;
    int cursor2 = base2;
    int dest = base1;
    this->CopyArray(workArray, base1, tmpArray, cursor1, len1);

    tmpElement.Update(workArray->Get(thread_, cursor2++));
    workArray->Set(thread_, dest++, tmpElement);

    if (--len2 == 0) {
        this->CopyArray(tmpArray, cursor1, workArray, dest, len1);
        return;
    }
    if (len1 == 1) {
        this->CopyArray(workArray, cursor2, workArray, dest, len2);
        tmpElement.Update(tmpArray->Get(thread_, cursor1));
        workArray->Set(thread_, dest + len2, tmpElement);
        return;
    }
    int minGallop = minGallop_;
    while (true) {
        int count1 = 0;
        int count2 = 0;
        do {
            ASSERT(len1 > 1 && len2 > 0);
            cmp1Element.Update(workArray->Get(thread_, cursor2));
            cmp2Element.Update(tmpArray->Get(thread_, cursor1));

            if (ArrayHelper::SortCompare(thread_, fn_, cmp1Element, cmp2Element) < 0) {
                tmpElement.Update(workArray->Get(thread_, cursor2++));
                workArray->Set(thread_, dest++, tmpElement);
                count2++;
                count1 = 0;
                if (--len2 == 0) {
                    goto epilogue;
                }
            } else {
                tmpElement.Update(tmpArray->Get(thread_, cursor1++));
                workArray->Set(thread_, dest++, tmpElement);
                count1++;
                count2 = 0;
                if (--len1 == 1) {
                    goto epilogue;
                }
            }
        } while ((count1 | count2) < minGallop);

        do {
            ASSERT(len1 > 1 && len2 > 0);
            JSHandle<JSTaggedValue> cursorVal(thread_, workArray->Get(thread_, cursor2));
            count1 = GallopRight(tmpArray, cursorVal, cursor1, len1, 0);
            if (count1 != 0) {
                this->CopyArray(tmpArray, cursor1, workArray, dest, count1);
                dest += count1;
                cursor1 += count1;
                len1 -= count1;
                if (len1 <= 1) {
                    goto epilogue;
                }
            }
            tmpElement.Update(workArray->Get(thread_, cursor2++));
            workArray->Set(thread_, dest++, tmpElement);
            if (--len2 == 0) {
                goto epilogue;
            }
            JSHandle<JSTaggedValue> cursorVal2(thread_, tmpArray->Get(thread_, cursor1));
            count2 = GallopLeft(workArray, cursorVal2, cursor2, len2, 0);
            if (count2 != 0) {
                this->CopyArray(workArray, cursor2, workArray, dest, count2);
                dest += count2;
                cursor2 += count2;
                len2 -= count2;
                if (len2 == 0) {
                    goto epilogue;
                }
            }
            tmpElement.Update(tmpArray->Get(thread_, cursor1++));
            workArray->Set(thread_, dest++, tmpElement);
            if (--len1 == 1) {
                goto epilogue;
            }
            minGallop--;
        } while ((count1 >= MIN_GALLOP) | (count2 >= MIN_GALLOP));

        if (minGallop < 0) {
            minGallop = 0;
        }
        minGallop += 2; // 2: means plus 2
    }

    epilogue: // merge what is left from either cursor1 or cursor2

    minGallop_ = std::min(minGallop, 1);
    if (len1 == 1) {
        ASSERT(len2 > 0);
        this->CopyArray(workArray, cursor2, workArray, dest, len2);
        tmpElement.Update(tmpArray->Get(thread_, cursor1));
        workArray->Set(thread_, dest + len2, tmpElement);
    } else {
        ASSERT(len1 != 0);
        ASSERT(len2 == 0 && len1 > 1);
        this->CopyArray(tmpArray, cursor1, workArray, dest, len1);
    }
}

void TimSort::MergeHi(int base1, int len1, int base2, int len2)
{
    JSHandle<TaggedArray> workArray = elements_;
    JSHandle<TaggedArray> tmpArray = GetTempArray(len2);

    JSMutableHandle<JSTaggedValue> tmpElement(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cmp1Element(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cmp2Element(thread_, JSTaggedValue::Undefined());

    this->CopyArray(workArray, base2, tmpArray, 0, len2);
    int cursor1 = base1 + len1 - 1;
    int cursor2 =  len2 - 1;
    int dest = base2 + len2 - 1;

    tmpElement.Update(workArray->Get(thread_, cursor1--));
    workArray->Set(thread_, dest--, tmpElement);

    if (--len1 == 0) {
        this->CopyArray(tmpArray, 0, workArray, dest - (len2 - 1), len2);
        return;
    }
    if (len2 == 1) {
        dest -= len1;
        cursor1 -= len1;
        this->CopyArray(workArray, cursor1 + 1, workArray, dest + 1, len1);
        tmpElement.Update(tmpArray->Get(thread_, cursor2));
        workArray->Set(thread_, dest, tmpElement);
        return;
    }
    int minGallop = minGallop_;
    while (true) {
        int count1 = 0;
        int count2 = 0;
        do {
            ASSERT(len1 > 0 && len2 > 1);
            cmp1Element.Update(workArray->Get(thread_, cursor1));
            cmp2Element.Update(tmpArray->Get(thread_, cursor2));

            if (ArrayHelper::SortCompare(thread_, fn_, cmp2Element, cmp1Element) < 0) {
                tmpElement.Update(workArray->Get(thread_, cursor1--));
                workArray->Set(thread_, dest--, tmpElement);
                count1++;
                count2 = 0;
                if (--len1 == 0) {
                    goto epilogue;
                }
            } else {
                tmpElement.Update(tmpArray->Get(thread_, cursor2--));
                workArray->Set(thread_, dest--, tmpElement);
                count2++;
                count1 = 0;
                if (--len2 == 1) {
                    goto epilogue;
                }
            }
        } while ((count1 | count2) < minGallop);

        do {
            ASSERT(len1 > 0 && len2 > 1);
            JSHandle<JSTaggedValue> cursorVal(thread_, tmpArray->Get(thread_, cursor2));
            count1 = len1 - GallopRight(workArray, cursorVal, base1, len1, len1 - 1);
            if (count1 != 0) {
                dest -= count1;
                cursor1 -= count1;
                len1 -= count1;
                this->CopyArray(workArray, cursor1 + 1, workArray, dest + 1, count1);
                if (len1 == 0) {
                    goto epilogue;
                }
            }
            tmpElement.Update(tmpArray->Get(thread_, cursor2--));
            workArray->Set(thread_, dest--, tmpElement);
            if (--len2 == 1) {
                goto epilogue;
            }
            JSHandle<JSTaggedValue> cursorVal2(thread_, workArray->Get(thread_, cursor1));
            count2 = len2 - GallopLeft(tmpArray, cursorVal2, 0, len2, len2 - 1);
            if (count2 != 0) {
                dest -= count2;
                cursor2 -= count2;
                len2 -= count2;
                this->CopyArray(tmpArray, cursor2 + 1, workArray, dest + 1, count2);
                if (len2 <= 1) {
                    goto epilogue;
                }
            }
            tmpElement.Update(workArray->Get(thread_, cursor1--));
            workArray->Set(thread_, dest--, tmpElement);
            if (--len1 == 0) {
                goto epilogue;
            }
            minGallop--;
        } while ((count1 >= MIN_GALLOP) | (count2 >= MIN_GALLOP));

        if (minGallop < 0) {
            minGallop = 0;
        }
        minGallop += 2; // 2: means plus 2
    }

    epilogue: // merge what is left from either cursor1 or cursor2

    minGallop_ = std::min(minGallop, 1);
    if (len2 == 1) {
        ASSERT(len1 > 0);
        dest -= len1;
        cursor1 -= len1;
        this->CopyArray(workArray, cursor1 + 1, workArray, dest + 1, len1);
        tmpElement.Update(tmpArray->Get(thread_, cursor2));
        workArray->Set(thread_, dest, tmpElement);
    } else {
        ASSERT(len2 != 0);
        ASSERT(len1 == 0 && len2 > 1);
        this->CopyArray(tmpArray, 0, workArray, dest - (len2 - 1), len2);
    }
}

void TimSort::CopyArray(JSHandle<TaggedArray> &src, int srcPos,
                        JSHandle<TaggedArray> &dst, int dstPos, int length)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(srcPos >= 0);
    ASSERT(dstPos >= 0);
    ASSERT(static_cast<int64_t>(srcPos) <= static_cast<int64_t>(src->GetLength() - length));
    ASSERT(static_cast<int64_t>(dstPos) <= static_cast<int64_t>(dst->GetLength() - length));

    if (srcPos < dstPos) {
        int srcIdx = srcPos + length - 1;
        int dstIdx = dstPos + length - 1;
        while (srcIdx >= srcPos) {
            dst->Set(thread_, dstIdx--, src->Get(thread_, srcIdx--));
        }
    } else {
        int srcIdx = srcPos;
        int dstIdx = dstPos;
        int to = srcPos + length;
        while (srcIdx < to) {
            dst->Set(thread_, dstIdx++, src->Get(thread_, srcIdx++));
        }
    }
}
}