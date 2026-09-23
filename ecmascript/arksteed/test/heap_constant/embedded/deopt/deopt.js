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
//! METHOD deoptWithHeapConstant
//! HAS HeapConstant
//! HAS DeoptIfHClassMismatch

class TrainingReceiver {
    constructor(value) {
        this.value = value;
    }
}

class DeoptReceiver {
    constructor(value) {
        this.padding = 1;
        this.value = value;
    }
}

function deoptWithHeapConstant(receiver) {
    const prefix = "heap-constant-deopt:";
    const value = receiver.value;
    return prefix + value;
}

let training = new TrainingReceiver(0);
for (let i = 0; i < 32; i++) {
    training.value = i;
    deoptWithHeapConstant(training);
}

let ok = ArkTools.arkSteedCompileSync(deoptWithHeapConstant);
ArkTools.waitJitCompileFinish(deoptWithHeapConstant);
ok = ok && ArkTools.isAOTCompiled(deoptWithHeapConstant);
ok = ok && deoptWithHeapConstant(training) === "heap-constant-deopt:31";

// Keep the HeapConstant live in the eager-deopt frame across moving GCs, then
// miss the profiled receiver HClass so execution resumes in the interpreter.
ArkTools.forceFullGC();
let miss = new DeoptReceiver(42);
ok = ok && deoptWithHeapConstant(miss) === "heap-constant-deopt:42";

print(ok ? "PASS" : "FAIL");
