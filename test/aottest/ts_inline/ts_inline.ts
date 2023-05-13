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

declare function print(arg:any):string;

function foo(arg1 : number, arg2 : number) : number {
    let res = arg1 + arg2;
    if (res > 0) {
        return res;
    } else {
        return 0;
    }
}

function bar(arg1 : number, arg2 : number) : number {
    return arg1 * arg2;
}

function calculate(arg1 : number, arg2 : number) : number {
    let a1 = arg1 + arg2;
    if (a1 > 0) {
        a1 = a1 * 2;
    } else {
        a1++;
    }
    let a2 = arg1 * arg2;
    if (a2 > 0) {
        a2 = a2 / 2;
    } else {
        a2++;
    }
    let a3 = a1 + a2;
    return a3;
}

print(foo(1, 2));
print(foo(1, -2));

// inline doesn't support try-catch
print(bar(7, 8));
try {
    print(bar(7, 8));
} catch (e) {
    print(e);
}

// large function no inline
print(calculate(4, 2));