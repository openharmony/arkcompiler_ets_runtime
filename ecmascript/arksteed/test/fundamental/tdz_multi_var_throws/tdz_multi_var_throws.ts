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

// Test: throw.undefinedifholewithname THROWS for multiple let variables accessed via closure before initialization.
// The inner function compute() accesses a, b, c via lexical env.
// Called before variables are initialized → first throw.undefinedifholewithname throws ReferenceError.

declare function print(arg: string): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_multi_var_throws(): void {
    function compute(): number {
        return a + b + c;  // three ldlexvar → first throw.undefinedifholewithname "a" → THROWS
    }
    ArkTools.arkSteedCompileSync(compute);

    try {
        compute();  // TDZ! a, b, c not yet initialized
    } catch (e) {
        print(e.name);
    }

    let a = 1;
    let b = 2;
    let c = 3;
}

tdz_multi_var_throws();
