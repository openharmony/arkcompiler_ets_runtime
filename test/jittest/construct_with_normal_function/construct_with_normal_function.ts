/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

function f0() {
}

function F1() {
    if (!new.target) {
        throw 'must be called with new';
    }
    this.h = f0;
    this.c = f0;
}

for (let v25 = 0; v25 < 10; v25++) {
    for (let v37 = 0; v37 < 10; v37++) {
        Reflect.construct(F1, []);
    }
}

print("execute successful.");

function Base() {
    if (!new.target) { throw 'must be called with new'; }
    this.c = 1;
}

function test() {
    class Son extends Base {
        #c = Int32Array;
    }
    new Son()
}

for (let i = 0; i < 4000; i++) {
    test()
}
print("execute successful.")
