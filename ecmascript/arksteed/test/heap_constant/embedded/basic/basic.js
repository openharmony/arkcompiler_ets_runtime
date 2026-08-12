/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! PARAMS --enable-force-gc=true
//! METHOD storeTransition
//! HAS HeapConstant
//! HAS TransitionHClassWithBarrier

function forceYoungGC() {
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("young", undefined, true);
        return;
    }
    ArkTools.gc();
}

function forceFullGC() {
    ArkTools.forceFullGC();
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("full", undefined, true);
    }
}

function loadConstant(value) {
    return "embedded-heap-constant:" + value;
}

function callLiteralMethod(value) {
    const holder = {
        add(delta) {
            return value + delta;
        }
    };
    return holder.add(2);
}

function makeTransitionBase() {
    return { base: 1 };
}

function storeTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

for (let i = 0; i < 20000; i++) {
    loadConstant(i);
    callLiteralMethod(i);
    storeTransition(makeTransitionBase(), { marker: i });
}

ArkTools.arkSteedCompileSync(loadConstant);
ArkTools.arkSteedCompileSync(callLiteralMethod);
ArkTools.arkSteedCompileSync(storeTransition);
ArkTools.waitJitCompileFinish(loadConstant);
ArkTools.waitJitCompileFinish(callLiteralMethod);
ArkTools.waitJitCompileFinish(storeTransition);

let ok = ArkTools.isAOTCompiled(loadConstant) &&
    ArkTools.isAOTCompiled(callLiteralMethod) &&
    ArkTools.isAOTCompiled(storeTransition);

function check(value) {
    const transitionBase = makeTransitionBase();
    const storedValue = { marker: value };
    return loadConstant(value) === "embedded-heap-constant:" + value &&
        callLiteralMethod(value) === value + 2 &&
        storeTransition(transitionBase, storedValue) === storedValue &&
        transitionBase.base === 1 && transitionBase.added.marker === value;
}

ok = ok && check(7);
forceYoungGC();
ok = ok && check(17);
forceFullGC();
ok = ok && check(27);
ArkTools.triggerSharedGC("shared_full");
ok = ok && check(37);
ArkTools.triggerSharedCC();
ok = ok && check(47);
ok = ok && loadConstant({}) === "embedded-heap-constant:[object Object]";

print(ok ? "PASS" : "FAIL");
