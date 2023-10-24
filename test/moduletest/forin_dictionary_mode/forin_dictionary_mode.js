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
 * @tc.name:forin_dictionary_mode
 * @tc.desc:test forin_dictionary_mode
 * @tc.type: FUNC
 * @tc.require: issueI84DMO
 */

let parent = {
    "c": undefined,
    "a": 1,
    "b": undefined,
    1: 2
}
delete parent.a

let own = {
    "a": 1,
    "b": 1,
    1: 2,
}
delete own.b
own.__proto__ = parent

for (let i in parent) {
    print(i)
}
print("=============")
for (let i in own) {
    print(i)
}
