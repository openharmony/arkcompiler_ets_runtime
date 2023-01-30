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

// === tests/cases/compiler/getSetEnumerable.ts ===
declare function AssertType(value:any, type:string):void;
class GetSetEnumerableClassGet {
    get prop() { 
AssertType(true, "boolean");
return true;
}
}

class GetSetEnumerableClassSet {
    set prop(value: boolean) { }
}

class GetSetEnumerableClassGetSet {
    get prop() { 
AssertType(true, "boolean");
return true;
}

    set prop(value: boolean) { }
}

const GetSetEnumerableObjectGet = {
AssertType(GetSetEnumerableObjectGet, "{ readonly prop: boolean; }");
AssertType({    get prop() { return true; }}, "{ readonly prop: boolean; }");

    get prop() { 
AssertType(prop, "boolean");
return true; 

AssertType(true, "boolean");
}

};

const GetSetEnumerableObjectSet = {
AssertType(GetSetEnumerableObjectSet, "{ prop: boolean; }");
AssertType({    set prop(value: boolean) { }}, "{ prop: boolean; }");

    set prop(value: boolean) { 
AssertType(prop, "boolean");

AssertType(value, "boolean");
}

};

const GetSetEnumerableObjectGetSet = {
AssertType(GetSetEnumerableObjectGetSet, "{ prop: boolean; }");
AssertType({    get prop() { return true; },    set prop(value: boolean) { }}, "{ prop: boolean; }");

    get prop() { 
AssertType(prop, "boolean");
AssertType(true, "boolean");
return true; },

    set prop(value: boolean) { 
AssertType(prop, "boolean");

AssertType(value, "boolean");
}

};


