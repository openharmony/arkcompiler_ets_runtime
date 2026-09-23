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

// Test: throw.undefinedifholewithname THROWS for class reference via ldlexvar before class definition.
// The function tdz_class_ref_throws accesses Counter via lexical env.
// Called before class Counter is defined → throw.undefinedifholewithname "Counter" throws ReferenceError.

declare function print(arg: string): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_class_ref_throws(): void {
    Counter.count++;  // ldlexvar Counter → throw.undefinedifholewithname "Counter" → THROWS
}

ArkTools.arkSteedCompileSync(tdz_class_ref_throws);

try {
    tdz_class_ref_throws();  // TDZ! Counter not yet defined
} catch (e) {
    print(e.name);
}

class Counter {
    static count: number = 0;
}
