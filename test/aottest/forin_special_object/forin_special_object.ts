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
 * @tc.name:forin_specail_object
 * @tc.desc:test forin_specail_object
 * @tc.type: FUNC
 * @tc.require: issueI89SMQ
 */

declare function print(str:any):string;

var arr = new Array(10)
for (let i = 0; i < 5; i++) {
  arr[i] = i;
}
let parent = new Int8Array(arr);
let self = {
    2: "b",
    "a": {},
    8: []
}
self.__proto__ = parent
for (let i in self) {
    print(i)
}
print("===============")
for (let i in parent) {
    print(i)
}
print("===============")
const targetObj = {
    _secret: 'easily scared',
    test: "ss",
    eyeCount: 4
};

const proxy_has = new Proxy(targetObj, {
    has: (target, key) => {
        return key in target;
    }
})

for (const key in proxy_has) {
    print(key);
}