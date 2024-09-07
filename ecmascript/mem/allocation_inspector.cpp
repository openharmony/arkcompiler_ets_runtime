/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "allocation_inspector.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"

namespace panda::ecmascript {
void AllocationInspector::Step([[maybe_unused]] Address object, [[maybe_unused]] size_t size)
{
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    ASSERT(!heap_->GetJSThread()->GetGcState());
    if (object) {
        profiler_->ImplementSampling(object, size);
    }
#endif
}

// Sampling follows a Poisson distribution.
// According to the relationship between Poisson distribution and exponential probability distribution,
// the stepSize follows the exponential probability distribution with parameter λ = 1/rate where rate
// is average bytes between samples.
// Let random be a uniformly distributed random number between 0 and 1, beacuse Inverse transform sampling
// can generate exponential probability distribution with a uniform distribution, so nextSample = (- ln random) / λ.
size_t AllocationInspector::GetNextStepSize()
{
    double random = base::RandomGenerator::NextDouble();
    double stepSize = -std::log(random) * rate_;
    return stepSize < TaggedObject::TaggedObjectSize()
        ? TaggedObject::TaggedObjectSize()
        : (stepSize > INT_MAX ? INT_MAX : static_cast<size_t>(stepSize));
}

void AllocationCounter::AddAllocationInspector(AllocationInspector *inspector)
{
    ASSERT(inspector != nullptr);
    size_t stepSize = inspector->GetNextStepSize();
    size_t nextCounter = currentCounter_ + stepSize;
    inspector_ = inspector;

    ASSERT(currentCounter_ == nextCounter_);
    nextCounter_ = nextCounter;
}

void AllocationCounter::ClearAllocationInspector()
{
    inspector_ = nullptr;
    currentCounter_ = 0;
    nextCounter_ = 0;
}

void AllocationCounter::AdvanceAllocationInspector(size_t allocated)
{
    if (!IsActive()) {
        return;
    }
    ASSERT(allocated < nextCounter_ - currentCounter_);
    currentCounter_ += allocated;
}

void AllocationCounter::InvokeAllocationInspector(Address object, size_t objectSize, size_t alignedObjectSize)
{
    if (!IsActive()) {
        return;
    }

    ASSERT(alignedObjectSize >= nextCounter_ - currentCounter_);
    ASSERT(object != 0);
    {
        DISALLOW_GARBAGE_COLLECTION;
        inspector_->Step(object, objectSize);
    }
    size_t nextStepSize = inspector_->GetNextStepSize();
    // because next allocate or advance can add currentCounter_ to real allocated size,
    // so need add alignedObjectSize here.
    nextCounter_ = currentCounter_ + alignedObjectSize + nextStepSize;
}
} // namespace panda::ecmascript