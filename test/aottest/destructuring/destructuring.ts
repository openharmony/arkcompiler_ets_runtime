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

declare function assert_equal(a: Object, b: Object):void;
function foo({x = 11, y = 22})
{
    return x + y;
}

assert_equal(foo({}), 33);

// normal
let [] = [];
let [c, d] = [1, 2];
let [e, , ,...f] = [1, 2, 3, 4, 5, 6];
assert_equal(c, 1); // 1
assert_equal(d, 2); // 2
assert_equal(e, 1); // 1
assert_equal(f, [4,5,6]); // 4,5,6

// destructuring more elements
const foo1 = ["one", "two"];
const [red, yellow, green, blue] = foo1;

assert_equal(red, "one"); // "one"
assert_equal(yellow, "two"); // "two"
assert_equal(green, undefined); // undefined
assert_equal(blue, undefined); //undefined

// swap
let a = 1;
let b = 3;
[a, b] = [b, a];
assert_equal(a, 3); // 3
assert_equal(b, 1); // 1
const arr = [1, 2, 3];
[arr[2], arr[1]] = [arr[1], arr[2]];
assert_equal(arr, [1, 3, 2]); // [1, 3, 2]
