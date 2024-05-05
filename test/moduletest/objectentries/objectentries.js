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

/*
 * @tc.name:objectcloneproperties
 * @tc.desc:test object clone properties
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let obj = {
    a: "something",
    42: 42,
    c: "string",
    d: undefined
}
print(Object.keys(obj));

let obj2 = { 
    a: "aa",
    get b() { 
        this.e = "ee";
        Object.defineProperty(obj, "c", {
            value: "c",
            enumerable: false
        });
        return "bb";
    },
    c: "cc",
    123: "123"
};
for (const [key, value] of Object.entries(obj)) {
    print(key + "," + value);
}
print("e," + obj.e);

var obj3 = {
    a: 4294967295,
    b: -1,
    c: 2147483647,
    d: 2147483648,
    e: -2147483648,
    f: -2147483649
}
for (const [key, value] of Object.entries(obj3)) {
    print(key + "," + value);
}

const objectEntries1 = {
    a: 'somestring',
    3:30,
    b: 42,
    5:50,
  };

  print(Object.entries(objectEntries1))

  const objectEntries2 = {
    a: 'somestring',
    3:30,
    b: 42,
    5:50,
  };
  objectEntries2[1] = 10;
  objectEntries2[4] = 40;
  print(Object.entries(objectEntries2))

  print(Object.entries("test"));
