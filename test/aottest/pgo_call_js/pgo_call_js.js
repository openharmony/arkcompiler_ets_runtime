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

function foo(a) {
    return a + 1;
}

function bar(a) {
    return a / 2;
}

function mol() {
    return "a";
}

function callFoo1(f, a) {
    return f(a);
}

function callFoo2(f, a) {
    return f(a);
}

function callFoo3(f, a) {
    return f(a);
}

for (let i = 0; i < 2; i++) {
    callFoo1(foo, i);
}

for (let i = 0; i < 2; i++) {
    callFoo2(foo, i);
}

for (let i = 0; i < 2; i++) {
    callFoo2(bar, i);
}

let ret = callFoo3(mol, 1);

callFoo2(bar, 1);   // bot callFoo2 and bar inlined
callFoo2(foo, 1);   // bot callFoo2 and foo inlined
callFoo1(foo, 1)
print(ret);
