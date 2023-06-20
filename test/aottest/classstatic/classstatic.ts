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
    static test1 = 1;
    static test2 = 2;
    static test3 = 3;
    static test4 = 4;
    static test5 = 1;
    static test6 = 2;
    static test7 = 3;
    static test8 = 4;
    static test9 = 1;
    static test10 = 2;
    static test11 = 3;
    static test12 = 4;
    static test13 = 1;
    static test14 = 2;
    static test14 = 3;
    static test15 = 4;
    static test16 = 1;
    static test17 = 2;
    static test18 = 3;
    static test19 = 4;
    static test20 = 1;
    static test21 = 2;
    static test22 = 3;
    static test23 = 4;
    static test24 = 1;
    static test25 = 2;
    static test26 = 3;
    static test27 = 4;
    static test28 = 2;
    static test29 = 3;
    static test30 = 1;
    static test31 = 2;
    static test32 = 3;
    static test33 = 4;
    static test34 = 1;
    static test35 = 2;
    static test36 = 3;
    static test37 = 4;
    static test38 = 2;
    static test39 = 3;
    static test40 = 1;
    static test41 = 2;
    static test42 = 3;
    static test43 = 4;
    static test44 = 1;
    static test45 = 2;
    static test46 = 3;
    static test47 = 4;
    static test48 = 2;
    static test49 = 3;
    static test50 = 1;
    static test51 = 2;
    static test52 = 3;
    static test53 = 4;
    static test54 = 1;
    static test55 = 2;
    static test56 = 3;
    static test57 = 4;
    static test58 = 2;
    static test59 = 3;
    static test60 = 1;
    static test61 = 2;
    static test62 = 3;
    static test63 = 4;
    static test64 = 1;
    static test65 = 2;
    static test66 = 3;
    static test67 = 4;
    static test68 = 2;
    static test69 = 3;
    static test70 = 1;
    static test71 = 2;
    static test72 = 3;
    static test73 = 4;
    static test74 = 1;
    static test75 = 2;
    static test76 = 3;
    static test77 = 4;
    static test78 = 2;
    static test79 = 3;
    static test80 = 1;
    static test81 = 2;
    static test82 = 3;
    static test83 = 4;
    static test84 = 1;
    static test85 = 2;
    static test86 = 3;
    static test87 = 4;
    static test88 = 2;
    static test89 = 3;
    static test90 = 1;
    static test91 = 2;
    static test92 = 3;
    static test93 = 4;
    static test94 = 1;
    static test95 = 2;
    static test96 = 3;
    static test97 = 4;
    static test98 = 2;
    static test99 = 3;
    static test100 = 2;
    static test101 = 3;
    static test102 = 4;
    static test103 = 1;
    static test104 = 2;
    static test104 = 3;
    static test105 = 4;
    static test106 = 1;
    static test107 = 2;
    static test108 = 3;
    static test109 = 4;
    static test110 = 2;
    static test111 = 3;
    static test112 = 4;
    static test113 = 1;
    static test114 = 2;
    static test114 = 3;
    static test115 = 4;
    static test116 = 1;
    static test117 = 2;
    static test118 = 3;
    static test119 = 4;
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
cpu.sub(1, 4);
print(cpu1.TwoC);
print(cpu1.length);

try {
    cpu1.length = 4;
} catch (e) {
    print(e)
}