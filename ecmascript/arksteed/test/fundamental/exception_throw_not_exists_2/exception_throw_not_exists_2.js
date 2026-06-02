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

// Test: throw.notexists throw path.
// yield* with a custom iterator that has no throw method → throws TypeError.

let caught = 0;

// Custom iterable whose iterator only has next(), no throw() method
const iterable = {
    [Symbol.iterator]() {
        let i = 0;
        return {
            next() {
                if (i < 2) return { value: i++, done: false };
                return { value: undefined, done: true };
            }
            // No throw() method — intentional!
        };
    }
};

function* g() {
    yield* iterable;
}

function test() {
    const it = g();
    it.next();                   // { value: 0, done: false }
    try {
        it.throw(new Error());   // iterator has no throw → throw.notexists → THROWS
    } catch (e) {
        print(e.name);           // TypeError
        caught++;
    }
}

ArkTools.arkSteedCompileSync(test);
test();
print(caught);
