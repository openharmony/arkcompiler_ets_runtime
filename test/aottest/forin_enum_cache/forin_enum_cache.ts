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
 * @tc.name:forin_enum_cache
 * @tc.desc:test forin_enum_cache
 * @tc.type: FUNC
 * @tc.require: issueI89SMQ
 */

declare function print(str:any):string;

let grandparent = {}
let parent = {
    "c": undefined,
    "a": 1,
    "b": undefined,
}
let own = {
    "a": 1,
    "b": 1,
}
own.__proto__ = parent
parent.__proto__ = grandparent

// generate enum cache
print("===generate enum cache===")
for (let i in own) {
    print(i)
}
// use enum cache
print("===use enum cache===")
for (let i in own) {
    print(i)
}
// invalid enum cache and re-generate enum cache
print("===re-generate enum cache===")
grandparent['e'] = 1
for (let i in own) {
    print(i)
}
// change attribute
print("===change attribute===")
Object.defineProperty(grandparent, "e", {
    configurable:true,
    enumerable:false,
    value:"ggg",
    writable:true
})
for (let i in own) {
    print(i)
}
// delete property
print("===delete property===")
grandparent['f'] = 1
for (let i in own) {
    print(i)
    delete grandparent['f']
}