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

// Test: throw.undefinedifholewithname THROWS for module-level let accessed before initialization.
// The function tdz_module_let_throws accesses module-scoped counter via lexical env.
// Called before counter is initialized → throw.undefinedifholewithname "counter" throws ReferenceError.

declare function print(arg: string): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_module_let_throws(): void {
    counter++;   // ldlexvar counter → throw.undefinedifholewithname "counter" → THROWS
}

ArkTools.arkSteedCompileSync(tdz_module_let_throws);

try {
    tdz_module_let_throws();  // TDZ! counter not yet initialized
} catch (e) {
    print(e.name);
}

let counter = 0;
