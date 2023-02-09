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

// === tests/cases/compiler/inferenceDoesntCompareAgainstUninstantiatedTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
class ClassA<TEntityClass>  {
    constructor(private entity?: TEntityClass, public settings?: SettingsInterface<TEntityClass>) {

    }
}
export interface ValueInterface<TValueClass> {
    func?: (row: TValueClass) => any;
    value?: string;
}
export interface SettingsInterface<TClass> {
    values?: (row: TClass) => ValueInterface<TClass>[],
}
class ConcreteClass {
    theName = 'myClass';
}

let thisGetsTheFalseError = new ClassA(new ConcreteClass(), {
AssertType(thisGetsTheFalseError, "ClassA<ConcreteClass>");
AssertType(new ClassA(new ConcreteClass(), {    values: o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ]}), "ClassA<ConcreteClass>");
AssertType(ClassA, "typeof ClassA");
AssertType(new ConcreteClass(), "ConcreteClass");
AssertType(ConcreteClass, "typeof ConcreteClass");
AssertType({    values: o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ]}, "{ values: (ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]; }");

    values: o => [
AssertType(values, "(ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]");
AssertType(o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ], "(ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]");
AssertType(o, "ConcreteClass");
AssertType([        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ], "{ value: string; func: (ConcreteClass) => string; }[]");
        {
AssertType({            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }, "{ value: string; func: (ConcreteClass) => string; }");

            value: o.theName,
AssertType(value, "string");
AssertType(o.theName, "string");

            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'
AssertType(func, "(ConcreteClass) => string");
AssertType(x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj', "(ConcreteClass) => string");
AssertType(x, "ConcreteClass");
AssertType('asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj', "string");
        }
    ]
});

let thisIsOk = new ClassA<ConcreteClass>(new ConcreteClass(), {
AssertType(thisIsOk, "ClassA<ConcreteClass>");
AssertType(new ClassA<ConcreteClass>(new ConcreteClass(), {    values: o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ]}), "ClassA<ConcreteClass>");
AssertType(ClassA, "typeof ClassA");
AssertType(new ConcreteClass(), "ConcreteClass");
AssertType(ConcreteClass, "typeof ConcreteClass");
AssertType({    values: o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ]}, "{ values: (ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]; }");

    values: o => [
AssertType(values, "(ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]");
AssertType(o => [        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ], "(ConcreteClass) => { value: string; func: (ConcreteClass) => string; }[]");
AssertType(o, "ConcreteClass");
AssertType([        {            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }    ], "{ value: string; func: (ConcreteClass) => string; }[]");
        {
AssertType({            value: o.theName,            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'        }, "{ value: string; func: (ConcreteClass) => string; }");

            value: o.theName,
AssertType(value, "string");
AssertType(o.theName, "string");

            func: x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj'
AssertType(func, "(ConcreteClass) => string");
AssertType(x => 'asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj', "(ConcreteClass) => string");
AssertType(x, "ConcreteClass");
AssertType('asdfkjhgfdfghjkjhgfdfghjklkjhgfdfghjklkjhgfghj', "string");
        }
    ]
});

