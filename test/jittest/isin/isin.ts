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

declare function print(str: number):string;

function isin_test1(): boolean {
    var A = {a:1, b:2};
    return 'a' in A;
}

let ret = isin_test1();
ArkTools.jitCompileAsync(isin_test1);
let res = ArkTools.waitJitCompileFinish(isin_test1);
print(res);
ret = isin_test1();
print(ret);


function isin_test2(): boolean {
    let obj2 = {"5": 5};
    let obj = Object.create(obj2);
    obj[1] = 1;
    obj.abc = 2;

    return 5 in obj;
}

ret = isin_test2();
ArkTools.jitCompileAsync(isin_test2);
res = ArkTools.waitJitCompileFinish(isin_test2);
print(res);
ret = isin_test2();
print(ret);


function isin_test3(): boolean {
    let obj2 = {"5": 5};
    let obj = Object.create(obj2);
    obj[1] = 1;
    obj.abc = 2;

    return "abc" in obj;
}

ret = isin_test3();
ArkTools.jitCompileAsync(isin_test3);
res = ArkTools.waitJitCompileFinish(isin_test3);
print(res);
ret = isin_test3();
print(ret);


function isin_test4(): boolean {
    var str = new String("hello world");
    return '1' in str;
}

ret = isin_test4();
ArkTools.jitCompileAsync(isin_test4);
res = ArkTools.waitJitCompileFinish(isin_test4);
print(res);
ret = isin_test4();
print(ret);

function isin_test5(): boolean {
    var str = new String("hello world");
    return '11' in str;
}

ret = isin_test5();
ArkTools.jitCompileAsync(isin_test5);
res = ArkTools.waitJitCompileFinish(isin_test5);
print(res);
ret = isin_test5();
print(ret);

