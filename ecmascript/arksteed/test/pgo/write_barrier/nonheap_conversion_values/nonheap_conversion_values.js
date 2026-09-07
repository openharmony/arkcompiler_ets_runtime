/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD updateDouble
//! HAS F64ToTaggedDouble
//! HAS StoreEnvSlot
//! HAS_NOT SetValueWithBarrier

//! METHOD updateLogical
//! HAS CheckedNonNegativeI32ToTaggedInt
//! HAS StoreEnvSlot
//! HAS_NOT SetValueWithBarrier

//! METHOD storeDoubleField
//! HAS F64ToTaggedDouble
//! HAS StoreTaggedField
//! HAS_NOT StoreTaggedFieldWithBarrier

//! METHOD storeLogicalField
//! HAS CheckedNonNegativeI32ToTaggedInt
//! HAS StoreTaggedField
//! HAS_NOT StoreTaggedFieldWithBarrier

function createUpdater()
{
    let capturedDouble = 0.0;
    let capturedLogical = 0;

    function updateDouble(value)
    {
        capturedDouble = -value;
        return capturedDouble;
    }

    function updateLogical(value, shift)
    {
        capturedLogical = value >>> shift;
        return capturedLogical;
    }

    return { updateDouble, updateLogical };
}

function storeDoubleField(holder, value)
{
    holder.tagged = -value;
    return holder.tagged;
}

function storeLogicalField(holder, value, shift)
{
    holder.tagged = value >>> shift;
    return holder.tagged;
}

const updater = createUpdater();
const holder = { tagged: { marker: 1 } };

for (let i = 0; i < 1000; i++) {
    updater.updateDouble(1.25 + i);
    updater.updateLogical(i, 1);
    storeDoubleField(holder, 2.5 + i);
    storeLogicalField(holder, i, 1);
}

ArkTools.arkSteedCompileSync(updater.updateDouble);
ArkTools.arkSteedCompileSync(updater.updateLogical);
ArkTools.arkSteedCompileSync(storeDoubleField);
ArkTools.arkSteedCompileSync(storeLogicalField);
ArkTools.waitJitCompileFinish(updater.updateDouble);
ArkTools.waitJitCompileFinish(updater.updateLogical);
ArkTools.waitJitCompileFinish(storeDoubleField);
ArkTools.waitJitCompileFinish(storeLogicalField);

let ok = ArkTools.isAOTCompiled(updater.updateDouble);
ok = ok && ArkTools.isAOTCompiled(updater.updateLogical);
ok = ok && ArkTools.isAOTCompiled(storeDoubleField);
ok = ok && ArkTools.isAOTCompiled(storeLogicalField);

ok = ok && updater.updateDouble(2.5) === -2.5;
ok = ok && updater.updateLogical(-2, 1) === 2147483647;
holder.tagged = { marker: 1 };
ok = ok && storeDoubleField(holder, 3.5) === -3.5;
holder.tagged = { marker: 1 };
ok = ok && storeLogicalField(holder, -2, 1) === 2147483647;

print(ok ? "PASS" : "FAIL");
