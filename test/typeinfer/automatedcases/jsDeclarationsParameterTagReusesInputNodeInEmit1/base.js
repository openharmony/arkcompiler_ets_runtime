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

// === tests/cases/conformance/jsdoc/declarations/base.js ===
declare function AssertType(value:any, type:string):void;
class Base {
    constructor() {}
}

const BaseFactory = () => {
AssertType(BaseFactory, "{ (): Base; Base: typeof Base; }");
AssertType(() => {    return new Base();}, "{ (): Base; Base: typeof Base; }");

AssertType(new Base(), "Base");
AssertType(Base, "typeof Base");
    return new Base();

};

BaseFactory.Base = Base;
AssertType(BaseFactory.Base = Base, "typeof Base");
AssertType(BaseFactory.Base, "typeof Base");
AssertType(Base, "typeof Base");

module.exports = BaseFactory;
AssertType(module.exports = BaseFactory, "{ (): Base; Base: typeof Base; }");
AssertType(module.exports, "{ (): Base; Base: typeof Base; }");
AssertType(BaseFactory, "{ (): Base; Base: typeof Base; }");


