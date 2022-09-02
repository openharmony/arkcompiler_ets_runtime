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
let a:number = 1;
let b:number = 2;
let c:number = a - b;
print(c);

function foo(arg0:number, arg1:number) {
    print(arg0 - arg1);
}
foo(2.1, 2.1)

print(1 - 2);

function foo1(arg0:number, arg1:number) {
    return arg0 - arg1;
}
print(foo1(1, 2))

let d:number = 3;
let e:number = 2;
var f:number = 0;
if (d > e) {
    f = d + e;
} else {
    f = d - f;
}
print(f);
