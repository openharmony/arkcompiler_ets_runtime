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

// Test: throw.notexists no-throw path.
// yield* with a generator that has a throw method → throw.notexists is skipped.

let cnt = 0;
let throwCaught = false;

function* inner() {
    try {
        yield 1;
    } catch (e) {
        throwCaught = true;     // inner generator received the throw
    }
}

function* outer() {
    yield* inner();
}

function test() {
    const it = outer();
    it.next();                   // { value: 1, done: false }
    try {
        it.throw(new Error());   // inner has throw → throw.notexists skipped
    } catch (e) {
        // inner catches throw but doesn't re-yield, so outer completes
    }
    cnt++;
}

ArkTools.arkSteedCompileSync(test);
test();
print(cnt);
print(throwCaught);
