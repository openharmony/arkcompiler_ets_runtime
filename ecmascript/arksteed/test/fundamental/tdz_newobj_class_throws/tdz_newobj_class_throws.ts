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

// Test: throw.undefinedifholewithname THROWS for class constructor accessed via ldlexvar before class definition.
// The function tdz_newobj_class_throws loads Wallet via lexical env.
// Called before class Wallet is defined → throw.undefinedifholewithname "Wallet" throws ReferenceError.

declare function print(arg: string): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_newobj_class_throws(): void {
    let obj = new Wallet(5);  // ldlexvar Wallet → throw.undefinedifholewithname "Wallet" → THROWS
}

ArkTools.arkSteedCompileSync(tdz_newobj_class_throws);

try {
    tdz_newobj_class_throws();  // TDZ! Wallet not yet defined
} catch (e) {
    print(e.name);
}

class Wallet {
    val: number;
    constructor(v: number) {
        this.val = v;
    }
}
