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

//! METHOD      typeof_known_type
//! HAS_NOT     CallCommonStub TypeOf
// Test: typeof on values whose types are recorded by upstream operations is
// folded: the int arithmetic result is known as NUMBER, the comparison result
// is known as BOOLEAN, and the string concatenation result is known as STRING.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function typeof_known_type(a: number, b: string): number {
    let numberValue = a + 1;
    let booleanValue = a === 1;
    let stringValue = b + b;

    let numberOk = typeof numberValue === "number" ? 1 : 0;
    let booleanOk = typeof booleanValue === "boolean" ? 1 : 0;
    let stringOk = typeof stringValue === "string" ? 1 : 0;
    return numberOk + booleanOk + stringOk;
}

typeof_known_type(1, "a");
typeof_known_type(2, "b");

ArkTools.arkSteedCompileSync(typeof_known_type);

print(typeof_known_type(3, "c"));
