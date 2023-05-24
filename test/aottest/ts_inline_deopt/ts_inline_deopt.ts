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

// @ts-nocheck
declare function print(arg:any):string;
class A {
    n: number = 2
}

class B {
    constructor(i: string) {
        if (i == null) {
            return
        }
        print(i)  // do not print
    }
}

function foo() {
    A.prototype.o = 1
    let a = new A()
    let b = a.n // deopt
    // new common object
    let c = new B()

    print(b)   // 2
}

let arr = new Array(2)
arr[0] = 1
foo()
let str = new String("hello wrold")
arr[1] = str
print(arr.length)
print(arr[0])
print(arr[1])
