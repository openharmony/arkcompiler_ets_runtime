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

declare function print(arg:any):string;
function foo({x = 11, y = 22})
{
    return x + y;
}

print(foo({}));

// normal
let [] = [];
let [c, d] = [1, 2];
let [e, , ,...f] = [1, 2, 3, 4, 5, 6];
print(c); // 1
print(d); // 2
print(e); // 1
print(f); // 4,5,6

// destructuring more elements
const foo1 = ["one", "two"];
const [red, yellow, green, blue] = foo1;

print(red); // "one"
print(yellow); // "two"
print(green); // undefined
print(blue); //undefined

// swap
let a = 1;
let b = 3;
[a, b] = [b, a];
print(a); // 3
print(b); // 1
const arr = [1, 2, 3];
[arr[2], arr[1]] = [arr[1], arr[2]];
print(arr); // [1, 3, 2]

let x, y;
function fn() {
  for ([...[x, y]] of [[null]]) {
    print(x);
  }
}
fn();
