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

//! METHOD      typeof_hclass_fold
//! HAS_NOT     CallCommonStub TypeOf
// Test: typeof on receivers whose HClasses are recorded by mono IC guards is
// folded: fn.x records a callable HClass ("function"), obj.x records a plain
// object HClass ("object"), and s[0] records the LineString HClass ("string").

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

let fnWithX: any = function () { return 1; };
fnWithX.x = 2;

let objWithX: any = { x: 10 };

function typeof_hclass_fold(fn: any, obj: any, s: any): number {
    let functionField = fn.x;
    let objectField = obj.x;
    let stringChar = s[0];

    let functionOk = typeof fn === "function" ? 1 : 0;
    let objectOk = typeof obj === "object" ? 1 : 0;
    let stringOk = typeof s === "string" ? 1 : 0;
    return functionOk + objectOk + stringOk;
}

typeof_hclass_fold(fnWithX, objWithX, "abc");
typeof_hclass_fold(fnWithX, objWithX, "abc");
typeof_hclass_fold(fnWithX, objWithX, "abc");
typeof_hclass_fold(fnWithX, objWithX, "abc");

ArkTools.arkSteedCompileSync(typeof_hclass_fold);

print(typeof_hclass_fold(fnWithX, objWithX, "abc"));
