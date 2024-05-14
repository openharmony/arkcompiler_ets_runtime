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
 * @tc.name:storeicbyvalue
 * @tc.desc:test storeicbyvalue
 * @tc.type: FUNC
 * @tc.require: issueI8EX0Z
 */

let obj = {
  "prop1" : "val1",
  "prop2" : "val2",
  "prop3" : "val3",
  "prop4" : "val4",
  "prop5" : "val5",
   1 : 2,
   2 : 3,
   3 : 4,
   5 : 6,
};

function ICTest() {
  delete obj["prop1"];

  for (let i = 0; i < 1024; i++) {
    obj[2] = 0x76543210;
  }

  delete obj[5];
}

ICTest();
ICTest();

print("test successful !!!");

let obj2 = {};
for(let i=0; i<100; i++){
    obj2[Uint16Array] = 1;
}
print("store obj by value successful!!!")