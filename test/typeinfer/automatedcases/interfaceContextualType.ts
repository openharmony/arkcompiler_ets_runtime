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

// === tests/cases/compiler/interfaceContextualType.ts ===
declare function AssertType(value:any, type:string):void;
export interface IOptions {
    italic?: boolean;
    bold?: boolean;
}
export interface IMap {
    [s: string]: IOptions;
}

class Bug {
    public values: IMap;
    ok() {
        this.values = {};
AssertType(this.values = {}, "{}");
AssertType(this.values, "IMap");
AssertType(this, "this");
AssertType({}, "{}");

        this.values['comments'] = { italic: true };
AssertType(this.values['comments'] = { italic: true }, "{ italic: true; }");
AssertType(this.values['comments'], "IOptions");
AssertType(this.values, "IMap");
AssertType(this, "this");
AssertType('comments', "string");
AssertType({ italic: true }, "{ italic: true; }");
AssertType(italic, "boolean");
AssertType(true, "boolean");
    }
    shouldBeOK() {
        this.values = {
AssertType(this.values = {            comments: { italic: true }        }, "{ comments: { italic: true; }; }");
AssertType(this.values, "IMap");
AssertType(this, "this");
AssertType({            comments: { italic: true }        }, "{ comments: { italic: true; }; }");

            comments: { italic: true 
AssertType(comments, "{ italic: true; }");

AssertType({ italic: true }, "{ italic: true; }");

AssertType(italic, "boolean");

AssertType(true, "boolean");
}

        };
    }
}


