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

//! METHOD      compute
//! COUNT       BranchIfReferenceEqual  3
//! COUNT       CallCommonStub   GetStringFromConstPool  3
// Note: three guards survive because each guard's throw block emits
// GetStringFromConstPool (unknown side effect) which clears the env-slot CSE
// between reads, so every read yields a fresh value vertex.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_same_var_repeat_read(offset: number): number {
    let x = 7;

    function compute(): number {
        return x + x + x;  // three ldlexvar + three throw.undefinedifholewithname on one value
    }
    ArkTools.arkSteedCompileSync(compute);

    return compute() + offset;
}

print(tdz_same_var_repeat_read(0));
print(tdz_same_var_repeat_read(4));
