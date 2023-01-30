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

// === tests/cases/conformance/parser/ecmascript5/parserSyntaxWalker.generated.ts ===
declare function AssertType(value:any, type:string):void;

//declare module "fs" {
//    export class File {
//        constructor(filename: string);
//        public ReadAllText(): string;
//    }
//    export interface IFile {
//        [index: number]: string;
//    }
//}

//import fs = module("fs");


//module TypeScriptAllInOne {
//    export class Program {
//        static Main(...args: string[]) {
//            try {
//                let bfs = new BasicFeatures();
//                let retValue: number = 0;

//                retValue = bfs.VARIABLES();
//                if (retValue != 0) {

//                    return 1;
//                }

//                retValue = bfs.STATEMENTS(4);
//                if (retValue != 0) {

//                    return 1;
//                }


//                retValue = bfs.TYPES();
//                if (retValue != 0) {

//                    return 1;
//                }

//                retValue = bfs.OPERATOR();
//                if (retValue != 0) {

//                    return 1;
//                }
//            }
//            catch (e) {
//                console.log(e);
//            }
//            finally {

//            }

//            console.log('Done');

//            return 0;

//        }
//    }

//    class BasicFeatures {
//        /// <summary>
//        /// Test letious of letiables. Including nullable,key world as letiable,special format
//        /// </summary>
//        /// <returns></returns>
//        public VARIABLES(): number {
//            let local = Number.MAX_VALUE;
//            let min = Number.MIN_VALUE;
//            let inf = Number.NEGATIVE_INFINITY;
//            let nan = Number.NaN;
//            let undef = undefined;

//            let п = local;
//            let м = local;

//            let local5 = <fs.File>null;
//            let local6 = local5 instanceof fs.File;

//            let hex = 0xBADC0DE, Hex = 0XDEADBEEF;
//            let float = 6.02e23, float2 = 6.02E-23
//            let char = 'c', \u0066 = '\u0066', hexchar = '\x42';
//            let quoted = '"', quoted2 = "'";
//            let reg = /\w*/;
//            let objLit = { "let": number = 42, equals: function (x) { return x["let"] === 42; }, toString: () => 'objLit{42}' };
//            let weekday = Weekdays.Monday;

//            let con = char + f + hexchar + float.toString() + float2.toString() + reg.toString() + objLit + weekday;

//            //
//            let any = 0;
//            let boolean = 0;
//            let declare = 0;
//            let constructor = 0;
//            let get = 0;
//            let implements = 0;
//            let interface = 0;
//            let let = 0;
//            let module = 0;
//            let number = 0;
//            let package = 0;
//            let private = 0;
//            let protected = 0;
//            let public = 0;
//            let set = 0;
//            let static = 0;
//            let string = 0;
//            let yield = 0;

//            let sum3 = any + boolean + declare + constructor + get + implements + interface + let + module + number + package + private + protected + public + set + static + string + yield;

//            return 0;
//        }

//        /// <summary>
//        /// Test different statements. Including if-else,swith,foreach,(un)checked,lock,using,try-catch-finally
//        /// </summary>
//        /// <param name="i"></param>
//        /// <returns></returns>
//        STATEMENTS(i: number): number {
//            let retVal = 0;
//            if (i == 1)
//                retVal = 1;
//            else
//                retVal = 0;
//            switch (i) {
//                case 2:
//                    retVal = 1;
//                    break;
//                case 3:
//                    retVal = 1;
//                    break;
//                default:
//                    break;
//            }

//            for (let x in { x: 0, y: 1 }) {
//            }

//            try {
//                throw null;
//            }
//            catch (Exception) {
//            }
//            finally {
//                try { }
//                catch (Exception) { }
//            }

//            return retVal;
//        }

//        /// <summary>
//        /// Test types in ts language. Including class,struct,interface,delegate,anonymous type
//        /// </summary>
//        /// <returns></returns>
//        public TYPES(): number {
//            let retVal = 0;
//            let c = new CLASS();
//            let xx: IF = c;
//            retVal += c.Property;
//            retVal += c.Member();
//            retVal += xx ^= Foo() ? 0 : 1;

//            //anonymous type
//            let anony = { a: new CLASS() };

//            retVal += anony.a.d();

//            return retVal;
//        }


//        ///// <summary>
//        ///// Test different operators
//        ///// </summary>
//        ///// <returns></returns>
//        public OPERATOR(): number {
//            let a: number[] = [1, 2, 3, 4,  implements , ];/*[] bug*/ // YES []
//            let i = a[1];/*[]*/
//            i = i + i - i * i / i % i & i | i ^ i;/*+ - * / % & | ^*/
//            let b = true && false || true ^ false;/*& | ^*/
//            b = !b;/*!*/
//            i = ~i;/*~i*/
//            b = i < (i -  continue ) && (i + 1) > i;/*< && >*/
//            let f = true ? 1 : 0;/*? :*/   // YES :
//            i++;/*++*/
//            i--;/*--*/
//            b = true && false || true;/*&& ||*/
//            i = i << 5;/*<<*/
//            i = i >> 5;/*>>*/
//            let j = i;
//            b = i == j && i != j && i <= j && i >= j;/*= == && != <= >=*/
//            i += <number>5.0;/*+=*/
//            i -= i;/*-=*/
//            i *= i;/**=*/
//            if (i == 0)
//                i++;
//            i /= i;/*/=*/
//            i %= i;/*%=*/
//            i &= i;/*&=*/
//            i |= i;/*|=*/
//            i ^= i;/*^=*/
//            i <<= i;/*<<=*/
//            i >>= i;/*>>=*/

//            if (i == 0 && !b && f == 1)
//                return 0;
//            else return 1;
//        }

//    }

//    interface IF {
//        Foo <!-- ): boolean;
//    }

//    class CLASS implements IF {

//        public d = () =>  '  return 0; };
//        public get Property() { return 0; }
//        public Member() {
//            return 0;
//        }
//        public Foo(): boolean {
//            let myEvent = () => { return 1; };
//            if (myEvent() == 1)
//                return true;
//            else
//                return false;
//        }
//    }


//    // todo: use these
//    class A {
//        public method1(val:number) {
//            return val;
//        }
//        public method2() {
//            return 2 * this.method1(2);
//        }
//    }

//    class B extends A {

//        public method2() {
//            return this.method1(2);
//        }
//    }

//    class Overloading {

//        private otherValue = 42;

//        constructor(private value: number, public name: string) { }
       
//        public Overloads(value: string);
//        public Overloads(value: string, ...rest: string[]) { }

//        public DefaultValue(value?: string = "Hello") { }
//    }
//}

//enum Weekdays {
//    Monday,
//    Tuesday,
//    Weekend,
//}

//enum Fruit {
//    Apple,
//    Pear
//}

//interface IDisposable {
//    Dispose(): void;
//}

//TypeScriptAllInOne.Program.Main();


