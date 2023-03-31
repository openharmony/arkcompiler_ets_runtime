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

// The following test case once exposed a bug: deopt does not restore vregs correctly in a loop when the number of 
// declare args is greater than the number of actual args.

// @ts-nocheck
declare function print(arg:any):string;

class C {
    x:number = 1;
    y:number;
    z:number;

    set(x: C, y?:number, z?: number, w?: number) {
        this.x = x.x;  // occur deopt
        print(x);
    }
}

let c1 = new C();
C.prototype.bar = 1;
let c2 = new C();
c2.set(c1);
