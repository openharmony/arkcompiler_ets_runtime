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
 * @tc.name:getpropertybyindex
 * @tc.desc:test getpropertybyindex
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
const v2 = new Int16Array(11);
const t3 = v2.__proto__;
let flag1 = false;
try {
  delete t3?.[9];
} catch (e) {
  flag1 = true;
}
print(flag1);

let flag2 = true;
let result = [];
var arr = new Array(20)
for (let i = 0; i < 10; i++) {
  arr[i] = i;

}
let obj = new Int8Array(arr);
for (var i = 0; i < 10; i++) {
  result.push(obj[i] == i);
}
for (var i = 10; i < 20; i++) {
  result.push(obj[i] == 0);
}
for (let i = 0; i < result.length; i++) {
  if (!result[i]) {
    flag2 = false;
  }
}
print(flag2);