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
    3: 30,
    b: 42,
    5: 50,
};

print(Object.entries(objectEntries1))

const objectEntries2 = {
    a: 'somestring',
    3: 30,
    b: 42,
    5: 50,
};
objectEntries2[1] = 10;
objectEntries2[4] = 40;
print(Object.entries(objectEntries2))

print(Object.entries("test"));

let obj4 = new Int8Array([-5, 10, 20, 30, 40, 50, 60.6]);
let obj5 = {
    a: 1,
    b: 2,
    c: 3,
};
Object.defineProperty(obj5, '23', {
    value: 31,
    enumerable: false,
})
Object.defineProperty(obj5, 'abc', {
    value: 31,
    enumerable: false,
})
let obj6 = {
    a: 1,
    b: 2,
    c: 3,
};
let obj7 = {
    1: 1,
    2: 2,
    3: 3,
};
let obj8 = {
    1: 1,
    2: 2,
    3: 3,
    a: 1,
    b: 2,
    c: 3,
};
let obj9 = {
    a: 1,
    b: 2,
    c: 3,
};
const a = Symbol('a');
const b = Symbol.for('b');
obj9[a] = 'aSymbol';
obj9[b] = 'bSymbol';
let obj10 = Object.create(
    {},
    {
        getBar: {
            value() {
                return 55;
            },
            writable: false,
            enumerable: false,
            configurable: false,
        },
    },
);
print(JSON.stringify(Object.entries(obj4)));
print(JSON.stringify(Object.entries(obj5)));
print(JSON.stringify(Object.entries(obj6)));
print(JSON.stringify(Object.entries(obj7)));
print(JSON.stringify(Object.entries(obj8)));
print(JSON.stringify(Object.entries(obj9)));
print(JSON.stringify(Object.entries(obj10)));

let arr =[];
arr["aaa"]="va";
arr["bbb"]="vb";
print(Object.entries(arr))
