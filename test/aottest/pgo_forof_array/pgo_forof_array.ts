/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

// @ts-nocheck
declare function print(arg:any):string;

let r = 0;
function foo(obj) {
    for (let value of obj) {
        r += value
    }
    for (let value of obj) {
        r += value
    }
}
let obj1 = new Array(1, 2, 3)
for (let i = 0; i < 50; i++) {
    foo(obj1)
}
print(r)

// for-of manully
let b = obj1[Symbol.iterator]
let c = b.apply(obj1, [1])
for (let i = 0; i < 10; i++) {
    let m = c.next(100)
    if (m.done) {
        break
    }
    print(m.value)
}

function testThrow() {
  let x = 1;
  for (const x of [x]) {}
}

try {
    testThrow();
} catch (e) {}

