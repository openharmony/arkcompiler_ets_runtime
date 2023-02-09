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

// === tests/cases/conformance/controlFlow/controlFlowElementAccess2.ts ===
declare function AssertType(value:any, type:string):void;
declare const config: {
AssertType(config, "{ [string]: union; }");

    [key: string]: boolean | { prop: string };
AssertType(key, "string");
AssertType(prop, "string");

};

if (typeof config['works'] !== 'boolean') {
    config.works.prop = 'test'; // ok
AssertType(config.works.prop = 'test', "string");
AssertType(config.works.prop, "string");
AssertType(config.works, "{ prop: string; }");
AssertType('test', "string");

    config['works'].prop = 'test'; // error, config['works']: boolean | { 'prop': string 
AssertType(config['works'].prop = 'test', "string");

AssertType(config['works'].prop, "string");

AssertType(config['works'], "{ prop: string; }");

AssertType(config, "{ [string]: union; }");

AssertType('works', "string");

AssertType('test', "string");
}
}
if (typeof config.works !== 'boolean') {
    config['works'].prop = 'test'; // error, config['works']: boolean | { 'prop': string 
AssertType(config['works'].prop = 'test', "string");

AssertType(config['works'].prop, "string");

AssertType(config['works'], "{ prop: string; }");

AssertType(config, "{ [string]: union; }");

AssertType('works', "string");

AssertType('test', "string");
}

    config.works.prop = 'test'; // ok
AssertType(config.works.prop = 'test', "string");
AssertType(config.works.prop, "string");
AssertType(config.works, "{ prop: string; }");
AssertType('test', "string");
}


