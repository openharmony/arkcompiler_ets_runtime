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

/*
 * @tc.name:forin_empty_prototype
 * @tc.desc:test forin_empty_prototype
 * @tc.type: FUNC
 * @tc.require: issueI89SMQ
 */

declare function print(str:any):string;

// no elements
let fast = {"a":1}
fast.b = "a"
for (let i in fast) {
    print(i)
}

// use enum cache
for (let i in fast) {
    print(i)
}

fast.c = 1
// invalidate enum cache
for (let i in fast) {
    print(i)
}

// has elements
let slow = {"s":222}
slow[2] = "aa"
slow[4] = 1
for (let i in slow) {
    print(i)
}