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

let a = [1, 2, 3];
let b = [1, , 3];
let c = [1, 2, 'test', 1];
let d = [1, 2, 3, 4.23];
let e = new Array(3); // 3 is the size of Array

function foo(p) {
  p[1] = 1.1;
}

function foo1(p) {
  p[2] = 'test';
}

function foo2(p) {
  p[1] = 1;
}

for (let i = 0; i < 1000; i++) {
  foo(a);
  foo(b);
  foo(c);
  foo1(d);
  foo2(e);
}
c[6] = 1;
