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

class ProbeCtor {
    value: any;

    constructor(value: any) {
        this.value = value;
    }
}

function storeProbe(obj: any, value: any) {
    obj.value = value;
    return obj.value;
}

function instanceProbe(obj: any) {
    return obj instanceof ProbeCtor ? 1 : 0;
}

let obj = new ProbeCtor({ payload: 0 });
let inst = new ProbeCtor({ payload: 1 });

for (let i = 0; i < 20000; i++) {
    storeProbe(obj, inst);
    instanceProbe(inst);
}

print(ArkTools.getICState(storeProbe, 0, 1));
print(ArkTools.isStableHClass(obj));
print(ArkTools.isStableHClass(ProbeCtor));

ArkTools.arkSteedCompileSync(storeProbe);
ArkTools.arkSteedCompileSync(instanceProbe);
ArkTools.waitJitCompileFinish(storeProbe);
ArkTools.waitJitCompileFinish(instanceProbe);
print(ArkTools.isAOTCompiled(storeProbe));
print(ArkTools.isAOTCompiled(instanceProbe));

print(storeProbe(obj, inst) === inst ? 42 : 0);
print(instanceProbe(inst));
