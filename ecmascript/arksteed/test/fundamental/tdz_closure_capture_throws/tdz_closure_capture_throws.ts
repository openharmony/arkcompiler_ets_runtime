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

// Test: throw.undefinedifholewithname THROWS for closure-captured let accessed before initialization.
// The inner function add() accesses sum via lexical env.
// Called before sum is initialized → throw.undefinedifholewithname "sum" throws ReferenceError.

declare function print(arg: string): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_closure_capture_throws(): void {
    function add(x: number): void {
        sum += x;   // ldlexvar sum → throw.undefinedifholewithname "sum" → THROWS
    }
    ArkTools.arkSteedCompileSync(add);

    try {
        add(1);  // TDZ! sum not yet initialized
    } catch (e) {
        print(e.name);
    }

    let sum = 0;
    sum = 1;
}

tdz_closure_capture_throws();
