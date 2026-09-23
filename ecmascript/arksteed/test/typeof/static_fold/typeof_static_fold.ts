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

//! METHOD      typeof_static_fold
//! HAS_NOT     CallCommonStub TypeOf
// Test: typeof on compile-time constants is folded; no TypeOf stub call remains.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function typeof_static_fold() {
    let undef;
    let numberInt = 42;
    let numberDouble = 3.5;
    let booleanTrue = true;
    let booleanFalse = false;
    let nullObject = null;

    print(typeof undef === "undefined" ? 1 : 0);
    print(typeof numberInt === "number" ? 1 : 0);
    print(typeof numberDouble === "number" ? 1 : 0);
    print(typeof booleanTrue === "boolean" ? 1 : 0);
    print(typeof booleanFalse === "boolean" ? 1 : 0);
    print(typeof nullObject === "object" ? 1 : 0);
}

ArkTools.arkSteedCompileSync(typeof_static_fold);

typeof_static_fold();
