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
// Note: see tdz_same_var_repeat_read - guard throw blocks break the env-slot
// CSE between argument reads, so each read keeps its own guard.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_repeat_read_call_args(): number {
    let x = 3;

    function add3(a: number, b: number, c: number): number {
        return a + b + c;
    }

    function compute(): number {
        return add3(x, x, x);  // three hole guards on the same value
    }
    ArkTools.arkSteedCompileSync(compute);

    return compute();
}

print(tdz_repeat_read_call_args());
