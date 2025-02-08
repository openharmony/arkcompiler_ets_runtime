/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

var A = {a:1, b:2};
print('a' in A);
print('b' in A);
print('c' in A);

let key1 = {
    toString: function() {
        return "3";
    } 
}

let key2 = {
    valueOf: function() {
        return "4";
    },
    toString: null
}

let obj = {1: 1, "abc": 2, [key1]: 3, [key2]: 4};

let obj2 = {"5": 5};
obj.__proto__ = obj2;

print(1 in obj)
print("abc" in obj)
print("3" in obj)
print(5 in obj)
print(6 in obj)
print([key1] in obj)
print([key2] in obj)

let obj3 = {"abcdef": "abcdef"};
print("ab" + "cd" + "ef" in obj3);

var str = new String("hello world");
print('1' in str);
print('11' in str);