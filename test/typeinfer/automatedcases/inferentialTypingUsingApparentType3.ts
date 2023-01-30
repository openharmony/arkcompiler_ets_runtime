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

// === tests/cases/compiler/inferentialTypingUsingApparentType3.ts ===
declare function AssertType(value:any, type:string):void;
interface Field<T> {
    clean(input: T): T
}

class CharField implements Field<string> {
    clean(input: string) {
AssertType("Yup", "string");
        return "Yup";
    }
}

class NumberField implements Field<number> {
    clean(input: number) {
AssertType(123, "int");
        return 123;
    }
}

class ObjectField<A, T extends { [name: string]: Field<any> }> {
    constructor(public fields: T) { }
}

let person = new ObjectField({
AssertType(person, "ObjectField<unknown, { id: NumberField; name: CharField; }>");
AssertType(new ObjectField({    id: new NumberField(),    name: new CharField()}), "ObjectField<unknown, { id: NumberField; name: CharField; }>");
AssertType(ObjectField, "typeof ObjectField");
AssertType({    id: new NumberField(),    name: new CharField()}, "{ id: NumberField; name: CharField; }");

    id: new NumberField(),
AssertType(id, "NumberField");
AssertType(new NumberField(), "NumberField");
AssertType(NumberField, "typeof NumberField");

    name: new CharField()
AssertType(name, "CharField");
AssertType(new CharField(), "CharField");
AssertType(CharField, "typeof CharField");

});

person.fields.id;
AssertType(person.fields.id, "NumberField");


