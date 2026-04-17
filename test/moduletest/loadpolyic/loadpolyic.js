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

let obj1 = {
    a: 1,
    b: 2,
};

let obj2 = {
    c: 3,
    d: 4,
    e: 5,
    f: 6
}

obj2.__proto__ = obj1;

function test(obj) {
    let a = obj.a;
}

// generate ic
for (let i = 0; i < 200; i++) {
    test(obj2);
}

// change proto
obj1.m = 10;

for (let i = 0; i < 10; ++i) {
    test(obj2);
}

print(ArkTools.getICState(test, 0, 0)) // expected poly state