/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/arrayBestCommonTypes.ts ===
declare function AssertType(value:any, type:string):void;
module EmptyTypes {
    interface iface { }
    class base implements iface { }
    class base2 implements iface { }
    class derived extends base { }


    class f {
        public voidIfAny(x: boolean, y?: boolean): number;
        public voidIfAny(x: string, y?: boolean): number;
        public voidIfAny(x: number, y?: boolean): number;
        public voidIfAny(x: any, y = false): any { 
AssertType(null, "null");
return null; 
}

        public x() {
            <number>(this.voidIfAny([4, 2][0]));
AssertType(<number>(this.voidIfAny([4, 2][0])), "number");
AssertType((this.voidIfAny([4, 2][0])), "number");
AssertType(this.voidIfAny([4, 2][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([4, 2][0], "number");
AssertType([4, 2], "number[]");
AssertType(4, "int");
AssertType(2, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([4, 2, undefined][0]));
AssertType(<number>(this.voidIfAny([4, 2, undefined][0])), "number");
AssertType((this.voidIfAny([4, 2, undefined][0])), "number");
AssertType(this.voidIfAny([4, 2, undefined][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([4, 2, undefined][0], "number");
AssertType([4, 2, undefined], "number[]");
AssertType(4, "int");
AssertType(2, "int");
AssertType(undefined, "undefined");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, 2, 4][0]));
AssertType(<number>(this.voidIfAny([undefined, 2, 4][0])), "number");
AssertType((this.voidIfAny([undefined, 2, 4][0])), "number");
AssertType(this.voidIfAny([undefined, 2, 4][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, 2, 4][0], "number");
AssertType([undefined, 2, 4], "number[]");
AssertType(undefined, "undefined");
AssertType(2, "int");
AssertType(4, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([null, 2, 4][0]));
AssertType(<number>(this.voidIfAny([null, 2, 4][0])), "number");
AssertType((this.voidIfAny([null, 2, 4][0])), "number");
AssertType(this.voidIfAny([null, 2, 4][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([null, 2, 4][0], "number");
AssertType([null, 2, 4], "number[]");
AssertType(null, "null");
AssertType(2, "int");
AssertType(4, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([2, 4, null][0]));
AssertType(<number>(this.voidIfAny([2, 4, null][0])), "number");
AssertType((this.voidIfAny([2, 4, null][0])), "number");
AssertType(this.voidIfAny([2, 4, null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([2, 4, null][0], "number");
AssertType([2, 4, null], "number[]");
AssertType(2, "int");
AssertType(4, "int");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, 4, null][0]));
AssertType(<number>(this.voidIfAny([undefined, 4, null][0])), "number");
AssertType((this.voidIfAny([undefined, 4, null][0])), "number");
AssertType(this.voidIfAny([undefined, 4, null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, 4, null][0], "number");
AssertType([undefined, 4, null], "number[]");
AssertType(undefined, "undefined");
AssertType(4, "int");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny(['', "q"][0]));
AssertType(<number>(this.voidIfAny(['', "q"][0])), "number");
AssertType((this.voidIfAny(['', "q"][0])), "number");
AssertType(this.voidIfAny(['', "q"][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(['', "q"][0], "string");
AssertType(['', "q"], "string[]");
AssertType('', "string");
AssertType("q", "string");
AssertType(0, "int");

            <number>(this.voidIfAny(['', "q", undefined][0]));
AssertType(<number>(this.voidIfAny(['', "q", undefined][0])), "number");
AssertType((this.voidIfAny(['', "q", undefined][0])), "number");
AssertType(this.voidIfAny(['', "q", undefined][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(['', "q", undefined][0], "string");
AssertType(['', "q", undefined], "string[]");
AssertType('', "string");
AssertType("q", "string");
AssertType(undefined, "undefined");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, "q", ''][0]));
AssertType(<number>(this.voidIfAny([undefined, "q", ''][0])), "number");
AssertType((this.voidIfAny([undefined, "q", ''][0])), "number");
AssertType(this.voidIfAny([undefined, "q", ''][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, "q", ''][0], "string");
AssertType([undefined, "q", ''], "string[]");
AssertType(undefined, "undefined");
AssertType("q", "string");
AssertType('', "string");
AssertType(0, "int");

            <number>(this.voidIfAny([null, "q", ''][0]));
AssertType(<number>(this.voidIfAny([null, "q", ''][0])), "number");
AssertType((this.voidIfAny([null, "q", ''][0])), "number");
AssertType(this.voidIfAny([null, "q", ''][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([null, "q", ''][0], "string");
AssertType([null, "q", ''], "string[]");
AssertType(null, "null");
AssertType("q", "string");
AssertType('', "string");
AssertType(0, "int");

            <number>(this.voidIfAny(["q", '', null][0]));
AssertType(<number>(this.voidIfAny(["q", '', null][0])), "number");
AssertType((this.voidIfAny(["q", '', null][0])), "number");
AssertType(this.voidIfAny(["q", '', null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(["q", '', null][0], "string");
AssertType(["q", '', null], "string[]");
AssertType("q", "string");
AssertType('', "string");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, '', null][0]));
AssertType(<number>(this.voidIfAny([undefined, '', null][0])), "number");
AssertType((this.voidIfAny([undefined, '', null][0])), "number");
AssertType(this.voidIfAny([undefined, '', null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, '', null][0], "string");
AssertType([undefined, '', null], "string[]");
AssertType(undefined, "undefined");
AssertType('', "string");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([[3, 4], [null]][0][0]));
AssertType(<number>(this.voidIfAny([[3, 4], [null]][0][0])), "number");
AssertType((this.voidIfAny([[3, 4], [null]][0][0])), "number");
AssertType(this.voidIfAny([[3, 4], [null]][0][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([[3, 4], [null]][0][0], "number");
AssertType([[3, 4], [null]][0], "number[]");
AssertType([[3, 4], [null]], "number[][]");
AssertType([3, 4], "number[]");
AssertType(3, "int");
AssertType(4, "int");
AssertType([null], "null[]");
AssertType(null, "null");
AssertType(0, "int");
AssertType(0, "int");


            let t1: { x: number; y: base; }[] = [{ x: 7, y: new derived() }, { x: 5, y: new base() }];
AssertType(t1, "{ x: number; y: base; }[]");
AssertType(x, "number");
AssertType(y, "base");
AssertType([{ x: 7, y: new derived() }, { x: 5, y: new base() }], "{ x: number; y: derived; }[]");
AssertType({ x: 7, y: new derived() }, "{ x: number; y: derived; }");
AssertType(x, "number");
AssertType(7, "int");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");
AssertType({ x: 5, y: new base() }, "{ x: number; y: base; }");
AssertType(x, "number");
AssertType(5, "int");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let t2: { x: boolean; y: base; }[] = [{ x: true, y: new derived() }, { x: false, y: new base() }];
AssertType(t2, "{ x: boolean; y: base; }[]");
AssertType(x, "boolean");
AssertType(y, "base");
AssertType([{ x: true, y: new derived() }, { x: false, y: new base() }], "(union)[]");
AssertType({ x: true, y: new derived() }, "{ x: true; y: derived; }");
AssertType(x, "boolean");
AssertType(true, "boolean");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");
AssertType({ x: false, y: new base() }, "{ x: false; y: base; }");
AssertType(x, "boolean");
AssertType(false, "boolean");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let t3: { x: string; y: base; }[] = [{ x: undefined, y: new base() }, { x: '', y: new derived() }];
AssertType(t3, "{ x: string; y: base; }[]");
AssertType(x, "string");
AssertType(y, "base");
AssertType([{ x: undefined, y: new base() }, { x: '', y: new derived() }], "{ x: string; y: derived; }[]");
AssertType({ x: undefined, y: new base() }, "{ x: undefined; y: base; }");
AssertType(x, "undefined");
AssertType(undefined, "undefined");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");
AssertType({ x: '', y: new derived() }, "{ x: string; y: derived; }");
AssertType(x, "string");
AssertType('', "string");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");

            let anyObj: any = null;
AssertType(anyObj, "any");
AssertType(null, "null");

            // Order matters here so test all the letiants
            let a1 = [{ x: 0, y: 'a' }, { x: 'a', y: 'a' }, { x: anyObj, y: 'a' }];
AssertType(a1, "{ x: any; y: string; }[]");
AssertType([{ x: 0, y: 'a' }, { x: 'a', y: 'a' }, { x: anyObj, y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");

            let a2 = [{ x: anyObj, y: 'a' }, { x: 0, y: 'a' }, { x: 'a', y: 'a' }];
AssertType(a2, "{ x: any; y: string; }[]");
AssertType([{ x: anyObj, y: 'a' }, { x: 0, y: 'a' }, { x: 'a', y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");

            let a3 = [{ x: 0, y: 'a' }, { x: anyObj, y: 'a' }, { x: 'a', y: 'a' }];
AssertType(a3, "{ x: any; y: string; }[]");
AssertType([{ x: 0, y: 'a' }, { x: anyObj, y: 'a' }, { x: 'a', y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");

            let ifaceObj: iface = null;
AssertType(ifaceObj, "iface");
AssertType(null, "null");

            let baseObj = new base();
AssertType(baseObj, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let base2Obj = new base2();
AssertType(base2Obj, "base2");
AssertType(new base2(), "base2");
AssertType(base2, "typeof base2");

            let b1 = [baseObj, base2Obj, ifaceObj];
AssertType(b1, "iface[]");
AssertType([baseObj, base2Obj, ifaceObj], "iface[]");
AssertType(baseObj, "base");
AssertType(base2Obj, "base2");
AssertType(ifaceObj, "iface");

            let b2 = [base2Obj, baseObj, ifaceObj];
AssertType(b2, "iface[]");
AssertType([base2Obj, baseObj, ifaceObj], "iface[]");
AssertType(base2Obj, "base2");
AssertType(baseObj, "base");
AssertType(ifaceObj, "iface");

            let b3 = [baseObj, ifaceObj, base2Obj];
AssertType(b3, "iface[]");
AssertType([baseObj, ifaceObj, base2Obj], "iface[]");
AssertType(baseObj, "base");
AssertType(ifaceObj, "iface");
AssertType(base2Obj, "base2");

            let b4 = [ifaceObj, baseObj, base2Obj];
AssertType(b4, "iface[]");
AssertType([ifaceObj, baseObj, base2Obj], "iface[]");
AssertType(ifaceObj, "iface");
AssertType(baseObj, "base");
AssertType(base2Obj, "base2");
        }
    }
}

module NonEmptyTypes {
    interface iface { x: string; }
    class base implements iface { x: string; y: string; }
    class base2 implements iface { x: string; z: string; }
    class derived extends base { a: string; }


    class f {
        public voidIfAny(x: boolean, y?: boolean): number;
        public voidIfAny(x: string, y?: boolean): number;
        public voidIfAny(x: number, y?: boolean): number;
        public voidIfAny(x: any, y = false): any { 
AssertType(null, "null");
return null; 
}

        public x() {
            <number>(this.voidIfAny([4, 2][0]));
AssertType(<number>(this.voidIfAny([4, 2][0])), "number");
AssertType((this.voidIfAny([4, 2][0])), "number");
AssertType(this.voidIfAny([4, 2][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([4, 2][0], "number");
AssertType([4, 2], "number[]");
AssertType(4, "int");
AssertType(2, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([4, 2, undefined][0]));
AssertType(<number>(this.voidIfAny([4, 2, undefined][0])), "number");
AssertType((this.voidIfAny([4, 2, undefined][0])), "number");
AssertType(this.voidIfAny([4, 2, undefined][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([4, 2, undefined][0], "number");
AssertType([4, 2, undefined], "number[]");
AssertType(4, "int");
AssertType(2, "int");
AssertType(undefined, "undefined");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, 2, 4][0]));
AssertType(<number>(this.voidIfAny([undefined, 2, 4][0])), "number");
AssertType((this.voidIfAny([undefined, 2, 4][0])), "number");
AssertType(this.voidIfAny([undefined, 2, 4][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, 2, 4][0], "number");
AssertType([undefined, 2, 4], "number[]");
AssertType(undefined, "undefined");
AssertType(2, "int");
AssertType(4, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([null, 2, 4][0]));
AssertType(<number>(this.voidIfAny([null, 2, 4][0])), "number");
AssertType((this.voidIfAny([null, 2, 4][0])), "number");
AssertType(this.voidIfAny([null, 2, 4][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([null, 2, 4][0], "number");
AssertType([null, 2, 4], "number[]");
AssertType(null, "null");
AssertType(2, "int");
AssertType(4, "int");
AssertType(0, "int");

            <number>(this.voidIfAny([2, 4, null][0]));
AssertType(<number>(this.voidIfAny([2, 4, null][0])), "number");
AssertType((this.voidIfAny([2, 4, null][0])), "number");
AssertType(this.voidIfAny([2, 4, null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([2, 4, null][0], "number");
AssertType([2, 4, null], "number[]");
AssertType(2, "int");
AssertType(4, "int");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, 4, null][0]));
AssertType(<number>(this.voidIfAny([undefined, 4, null][0])), "number");
AssertType((this.voidIfAny([undefined, 4, null][0])), "number");
AssertType(this.voidIfAny([undefined, 4, null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, 4, null][0], "number");
AssertType([undefined, 4, null], "number[]");
AssertType(undefined, "undefined");
AssertType(4, "int");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny(['', "q"][0]));
AssertType(<number>(this.voidIfAny(['', "q"][0])), "number");
AssertType((this.voidIfAny(['', "q"][0])), "number");
AssertType(this.voidIfAny(['', "q"][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(['', "q"][0], "string");
AssertType(['', "q"], "string[]");
AssertType('', "string");
AssertType("q", "string");
AssertType(0, "int");

            <number>(this.voidIfAny(['', "q", undefined][0]));
AssertType(<number>(this.voidIfAny(['', "q", undefined][0])), "number");
AssertType((this.voidIfAny(['', "q", undefined][0])), "number");
AssertType(this.voidIfAny(['', "q", undefined][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(['', "q", undefined][0], "string");
AssertType(['', "q", undefined], "string[]");
AssertType('', "string");
AssertType("q", "string");
AssertType(undefined, "undefined");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, "q", ''][0]));
AssertType(<number>(this.voidIfAny([undefined, "q", ''][0])), "number");
AssertType((this.voidIfAny([undefined, "q", ''][0])), "number");
AssertType(this.voidIfAny([undefined, "q", ''][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, "q", ''][0], "string");
AssertType([undefined, "q", ''], "string[]");
AssertType(undefined, "undefined");
AssertType("q", "string");
AssertType('', "string");
AssertType(0, "int");

            <number>(this.voidIfAny([null, "q", ''][0]));
AssertType(<number>(this.voidIfAny([null, "q", ''][0])), "number");
AssertType((this.voidIfAny([null, "q", ''][0])), "number");
AssertType(this.voidIfAny([null, "q", ''][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([null, "q", ''][0], "string");
AssertType([null, "q", ''], "string[]");
AssertType(null, "null");
AssertType("q", "string");
AssertType('', "string");
AssertType(0, "int");

            <number>(this.voidIfAny(["q", '', null][0]));
AssertType(<number>(this.voidIfAny(["q", '', null][0])), "number");
AssertType((this.voidIfAny(["q", '', null][0])), "number");
AssertType(this.voidIfAny(["q", '', null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType(["q", '', null][0], "string");
AssertType(["q", '', null], "string[]");
AssertType("q", "string");
AssertType('', "string");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([undefined, '', null][0]));
AssertType(<number>(this.voidIfAny([undefined, '', null][0])), "number");
AssertType((this.voidIfAny([undefined, '', null][0])), "number");
AssertType(this.voidIfAny([undefined, '', null][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([undefined, '', null][0], "string");
AssertType([undefined, '', null], "string[]");
AssertType(undefined, "undefined");
AssertType('', "string");
AssertType(null, "null");
AssertType(0, "int");

            <number>(this.voidIfAny([[3, 4], [null]][0][0]));
AssertType(<number>(this.voidIfAny([[3, 4], [null]][0][0])), "number");
AssertType((this.voidIfAny([[3, 4], [null]][0][0])), "number");
AssertType(this.voidIfAny([[3, 4], [null]][0][0]), "number");
AssertType(this.voidIfAny, "{ (boolean, ?boolean): number; (string, ?boolean): number; (number, ?boolean): number; }");
AssertType(this, "this");
AssertType([[3, 4], [null]][0][0], "number");
AssertType([[3, 4], [null]][0], "number[]");
AssertType([[3, 4], [null]], "number[][]");
AssertType([3, 4], "number[]");
AssertType(3, "int");
AssertType(4, "int");
AssertType([null], "null[]");
AssertType(null, "null");
AssertType(0, "int");
AssertType(0, "int");


            let t1: { x: number; y: base; }[] = [{ x: 7, y: new derived() }, { x: 5, y: new base() }];
AssertType(t1, "{ x: number; y: base; }[]");
AssertType(x, "number");
AssertType(y, "base");
AssertType([{ x: 7, y: new derived() }, { x: 5, y: new base() }], "{ x: number; y: base; }[]");
AssertType({ x: 7, y: new derived() }, "{ x: number; y: derived; }");
AssertType(x, "number");
AssertType(7, "int");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");
AssertType({ x: 5, y: new base() }, "{ x: number; y: base; }");
AssertType(x, "number");
AssertType(5, "int");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let t2: { x: boolean; y: base; }[] = [{ x: true, y: new derived() }, { x: false, y: new base() }];
AssertType(t2, "{ x: boolean; y: base; }[]");
AssertType(x, "boolean");
AssertType(y, "base");
AssertType([{ x: true, y: new derived() }, { x: false, y: new base() }], "(union)[]");
AssertType({ x: true, y: new derived() }, "{ x: true; y: derived; }");
AssertType(x, "boolean");
AssertType(true, "boolean");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");
AssertType({ x: false, y: new base() }, "{ x: false; y: base; }");
AssertType(x, "boolean");
AssertType(false, "boolean");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let t3: { x: string; y: base; }[] = [{ x: undefined, y: new base() }, { x: '', y: new derived() }];
AssertType(t3, "{ x: string; y: base; }[]");
AssertType(x, "string");
AssertType(y, "base");
AssertType([{ x: undefined, y: new base() }, { x: '', y: new derived() }], "(union)[]");
AssertType({ x: undefined, y: new base() }, "{ x: undefined; y: base; }");
AssertType(x, "undefined");
AssertType(undefined, "undefined");
AssertType(y, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");
AssertType({ x: '', y: new derived() }, "{ x: string; y: derived; }");
AssertType(x, "string");
AssertType('', "string");
AssertType(y, "derived");
AssertType(new derived(), "derived");
AssertType(derived, "typeof derived");

            let anyObj: any = null;
AssertType(anyObj, "any");
AssertType(null, "null");

            // Order matters here so test all the letiants
            let a1 = [{ x: 0, y: 'a' }, { x: 'a', y: 'a' }, { x: anyObj, y: 'a' }];
AssertType(a1, "{ x: any; y: string; }[]");
AssertType([{ x: 0, y: 'a' }, { x: 'a', y: 'a' }, { x: anyObj, y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");

            let a2 = [{ x: anyObj, y: 'a' }, { x: 0, y: 'a' }, { x: 'a', y: 'a' }];
AssertType(a2, "{ x: any; y: string; }[]");
AssertType([{ x: anyObj, y: 'a' }, { x: 0, y: 'a' }, { x: 'a', y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");

            let a3 = [{ x: 0, y: 'a' }, { x: anyObj, y: 'a' }, { x: 'a', y: 'a' }];
AssertType(a3, "{ x: any; y: string; }[]");
AssertType([{ x: 0, y: 'a' }, { x: anyObj, y: 'a' }, { x: 'a', y: 'a' }], "{ x: any; y: string; }[]");
AssertType({ x: 0, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: anyObj, y: 'a' }, "{ x: any; y: string; }");
AssertType(x, "any");
AssertType(anyObj, "any");
AssertType(y, "string");
AssertType('a', "string");
AssertType({ x: 'a', y: 'a' }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType('a', "string");
AssertType(y, "string");
AssertType('a', "string");

            let ifaceObj: iface = null;
AssertType(ifaceObj, "iface");
AssertType(null, "null");

            let baseObj = new base();
AssertType(baseObj, "base");
AssertType(new base(), "base");
AssertType(base, "typeof base");

            let base2Obj = new base2();
AssertType(base2Obj, "base2");
AssertType(new base2(), "base2");
AssertType(base2, "typeof base2");

            let b1 = [baseObj, base2Obj, ifaceObj];
AssertType(b1, "iface[]");
AssertType([baseObj, base2Obj, ifaceObj], "iface[]");
AssertType(baseObj, "base");
AssertType(base2Obj, "base2");
AssertType(ifaceObj, "iface");

            let b2 = [base2Obj, baseObj, ifaceObj];
AssertType(b2, "iface[]");
AssertType([base2Obj, baseObj, ifaceObj], "iface[]");
AssertType(base2Obj, "base2");
AssertType(baseObj, "base");
AssertType(ifaceObj, "iface");

            let b3 = [baseObj, ifaceObj, base2Obj];
AssertType(b3, "iface[]");
AssertType([baseObj, ifaceObj, base2Obj], "iface[]");
AssertType(baseObj, "base");
AssertType(ifaceObj, "iface");
AssertType(base2Obj, "base2");

            let b4 = [ifaceObj, baseObj, base2Obj];
AssertType(b4, "iface[]");
AssertType([ifaceObj, baseObj, base2Obj], "iface[]");
AssertType(ifaceObj, "iface");
AssertType(baseObj, "base");
AssertType(base2Obj, "base2");
        }
    }
}



