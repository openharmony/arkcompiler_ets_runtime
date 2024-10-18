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
declare function print(arg:any, arg1?:any):string;

class cpu {
    public mode: number = 1;
    constructor() {
        this.mode = 4;
    }
    public static add(a: string):number {
        let i : number = 3;
        let b : string = a.substring(3);
        print(b);
        let d : number = b.charCodeAt(4);
        print(a[i]);
        print(b[i]);
        return d;
    }
}

function foo(a: string):number {
	let b : string = a.substring(1);
    let d : number = b.charCodeAt(1);
    return d;
}

function foo1(a: string):number {
    let i : number = 2;
	let b : string = a.substring(1);
    let d : number = b.charCodeAt(1);
    print(a[i]);
    print(b[i]);
    return d;
}
var a : string = "12345678901234567890"
var b : string = a.substring(1);
print(foo(b));
print(foo1(b));
print(cpu.add(b));

//this is a case from a fuzz test
let v0 = 2 >= 1;
let v1 = "hello"
let v2 = v1.substring(v0, v1);
print(v2)

