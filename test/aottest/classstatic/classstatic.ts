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
    public static add(a: number, b: number): number {
        print(a);
        cpu.sub(a, b);
        var out : number = a + b;
        return out;
    }
    get kind() { 
        print(this.mode);
        return this.mode;
    }
    public static sub(a: number, b: number): number {
        print(b);
        var out : number = a + b;
        return out;
    }
    
    static Constant = 1;
    static Curve = 2;
    static TwoC = 3;
    static TwoCC = 4;
}
class cpu1 {
    public mode: number = 1;
    constructor() {
        this.mode = 4;
    }
    get kind() { 
        print(this.mode);
        return this.mode;
    }
    static Constant = 1;
    static Curve = 2;
    static TwoC = 3;
    static TwoCC = 4;
}
function foo():number {
	return cpu.Curve;
}
print(foo());
cpu.add(1, 3);
var systems: cpu = new cpu();
print(systems.kind);
print(cpu1.TwoC);