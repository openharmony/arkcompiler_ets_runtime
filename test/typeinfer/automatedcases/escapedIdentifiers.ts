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

// === tests/cases/compiler/escapedIdentifiers.ts ===
declare function AssertType(value:any, type:string):void;
/*
    0 .. \u0030
    9 .. \u0039

    A .. \u0041
    Z .. \u005a

    a .. \u0061
    z .. \u00za
*/

// let decl
let \u0061 = 1;
AssertType(\u0061, "number");
AssertType(1, "int");

a ++;
AssertType(a ++, "number");
AssertType(a, "number");

\u0061 ++;
AssertType(\u0061 ++, "number");
AssertType(\u0061, "number");

let b = 1;
AssertType(b, "number");
AssertType(1, "int");

b ++;
AssertType(b ++, "number");
AssertType(b, "number");

\u0062 ++;
AssertType(\u0062 ++, "number");
AssertType(\u0062, "number");

// modules
module moduleType1 { 
    export let baz1: number;
}
module moduleType\u0032 { 
    export let baz2: number;
}

moduleType1.baz1 = 3;
AssertType(moduleType1.baz1 = 3, "int");
AssertType(moduleType1.baz1, "number");
AssertType(3, "int");

moduleType\u0031.baz1 = 3;
AssertType(moduleType\u0031.baz1 = 3, "int");
AssertType(moduleType\u0031.baz1, "number");
AssertType(3, "int");

moduleType2.baz2 = 3;
AssertType(moduleType2.baz2 = 3, "int");
AssertType(moduleType2.baz2, "number");
AssertType(3, "int");

moduleType\u0032.baz2 = 3;
AssertType(moduleType\u0032.baz2 = 3, "int");
AssertType(moduleType\u0032.baz2, "number");
AssertType(3, "int");

// classes

class classType1 { 
    public foo1: number;
}
class classType\u0032 { 
    public foo2: number;
}

let classType1Object1 = new classType1();
AssertType(classType1Object1, "classType1");
AssertType(new classType1(), "classType1");
AssertType(classType1, "typeof classType1");

classType1Object1.foo1 = 2;
AssertType(classType1Object1.foo1 = 2, "int");
AssertType(classType1Object1.foo1, "number");
AssertType(2, "int");

let classType1Object2 = new classType\u0031();
AssertType(classType1Object2, "classType1");
AssertType(new classType\u0031(), "classType1");
AssertType(classType\u0031, "typeof classType1");

classType1Object2.foo1 = 2;
AssertType(classType1Object2.foo1 = 2, "int");
AssertType(classType1Object2.foo1, "number");
AssertType(2, "int");

let classType2Object1 = new classType2();
AssertType(classType2Object1, "classType\u0032");
AssertType(new classType2(), "classType\u0032");
AssertType(classType2, "typeof classType\u0032");

classType2Object1.foo2 = 2;
AssertType(classType2Object1.foo2 = 2, "int");
AssertType(classType2Object1.foo2, "number");
AssertType(2, "int");

let classType2Object2 = new classType\u0032();
AssertType(classType2Object2, "classType\u0032");
AssertType(new classType\u0032(), "classType\u0032");
AssertType(classType\u0032, "typeof classType\u0032");

classType2Object2.foo2 = 2;
AssertType(classType2Object2.foo2 = 2, "int");
AssertType(classType2Object2.foo2, "number");
AssertType(2, "int");

// interfaces
interface interfaceType1 { 
    bar1: number;
}
interface interfaceType\u0032 { 
    bar2: number;
}

let interfaceType1Object1 = <interfaceType1>{ bar1: 0 };
AssertType(interfaceType1Object1, "interfaceType1");
AssertType(<interfaceType1>{ bar1: 0 }, "interfaceType1");
AssertType({ bar1: 0 }, "{ bar1: number; }");
AssertType(bar1, "number");
AssertType(0, "int");

interfaceType1Object1.bar1 = 2;
AssertType(interfaceType1Object1.bar1 = 2, "int");
AssertType(interfaceType1Object1.bar1, "number");
AssertType(2, "int");

let interfaceType1Object2 = <interfaceType\u0031>{ bar1: 0 };
AssertType(interfaceType1Object2, "interfaceType1");
AssertType(<interfaceType\u0031>{ bar1: 0 }, "interfaceType1");
AssertType({ bar1: 0 }, "{ bar1: number; }");
AssertType(bar1, "number");
AssertType(0, "int");

interfaceType1Object2.bar1 = 2;
AssertType(interfaceType1Object2.bar1 = 2, "int");
AssertType(interfaceType1Object2.bar1, "number");
AssertType(2, "int");

let interfaceType2Object1 = <interfaceType2>{ bar2: 0 };
AssertType(interfaceType2Object1, "interfaceType\u0032");
AssertType(<interfaceType2>{ bar2: 0 }, "interfaceType\u0032");
AssertType({ bar2: 0 }, "{ bar2: number; }");
AssertType(bar2, "number");
AssertType(0, "int");

interfaceType2Object1.bar2 = 2;
AssertType(interfaceType2Object1.bar2 = 2, "int");
AssertType(interfaceType2Object1.bar2, "number");
AssertType(2, "int");

let interfaceType2Object2 = <interfaceType\u0032>{ bar2: 0 };
AssertType(interfaceType2Object2, "interfaceType\u0032");
AssertType(<interfaceType\u0032>{ bar2: 0 }, "interfaceType\u0032");
AssertType({ bar2: 0 }, "{ bar2: number; }");
AssertType(bar2, "number");
AssertType(0, "int");

interfaceType2Object2.bar2 = 2;
AssertType(interfaceType2Object2.bar2 = 2, "int");
AssertType(interfaceType2Object2.bar2, "number");
AssertType(2, "int");


// arguments
class testClass { 
    public func(arg1: number, arg\u0032: string, arg\u0033: boolean, arg4: number) { 
        arg\u0031 = 1;
AssertType(arg\u0031 = 1, "int");
AssertType(arg\u0031, "number");
AssertType(1, "int");

        arg2 = 'string';
AssertType(arg2 = 'string', "string");
AssertType(arg2, "string");
AssertType('string', "string");

        arg\u0033 = true;
AssertType(arg\u0033 = true, "boolean");
AssertType(arg\u0033, "boolean");
AssertType(true, "boolean");

        arg4 = 2;
AssertType(arg4 = 2, "int");
AssertType(arg4, "number");
AssertType(2, "int");
    }
}

// constructors
class constructorTestClass { 
    constructor (public arg1: number,public arg\u0032: string,public arg\u0033: boolean,public arg4: number) { 
    }
}
let constructorTestObject = new constructorTestClass(1, 'string', true, 2);
AssertType(constructorTestObject, "constructorTestClass");
AssertType(new constructorTestClass(1, 'string', true, 2), "constructorTestClass");
AssertType(constructorTestClass, "typeof constructorTestClass");
AssertType(1, "int");
AssertType('string', "string");
AssertType(true, "boolean");
AssertType(2, "int");

constructorTestObject.arg\u0031 = 1;
AssertType(constructorTestObject.arg\u0031 = 1, "int");
AssertType(constructorTestObject.arg\u0031, "number");
AssertType(1, "int");

constructorTestObject.arg2 = 'string';
AssertType(constructorTestObject.arg2 = 'string', "string");
AssertType(constructorTestObject.arg2, "string");
AssertType('string', "string");

constructorTestObject.arg\u0033 = true;
AssertType(constructorTestObject.arg\u0033 = true, "boolean");
AssertType(constructorTestObject.arg\u0033, "boolean");
AssertType(true, "boolean");

constructorTestObject.arg4 = 2;
AssertType(constructorTestObject.arg4 = 2, "int");
AssertType(constructorTestObject.arg4, "number");
AssertType(2, "int");

// Lables

l\u0061bel1: 
    while (false)
    {  
       while(false)
AssertType(false, "boolean");

           continue label1;  // it will go to next iteration of outer loop 
AssertType(label1, "any");
    } 

label2: 
    while (false)
    {  
       while(false)
AssertType(false, "boolean");

           continue l\u0061bel2;  // it will go to next iteration of outer loop 
AssertType(l\u0061bel2, "any");
    } 

label3: 
    while (false)
    {  
       while(false)
AssertType(false, "boolean");

           continue label3;  // it will go to next iteration of outer loop 
AssertType(label3, "any");
    } 

l\u0061bel4: 
    while (false)
    {  
       while(false)
AssertType(false, "boolean");

           continue l\u0061bel4;  // it will go to next iteration of outer loop 
AssertType(l\u0061bel4, "any");
    } 

