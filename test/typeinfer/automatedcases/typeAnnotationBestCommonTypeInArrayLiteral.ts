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

// === tests/cases/compiler/typeAnnotationBestCommonTypeInArrayLiteral.ts ===
declare function AssertType(value:any, type:string):void;
interface IMenuItem {
    id: string;
    type: string;
    link?: string;
    classes?: string;
    text?: string;
    icon?: string;
}
let menuData: IMenuItem[] = [
AssertType(menuData, "IMenuItem[]");
AssertType([    {        "id": "ourLogo",        "type": "image",        "link": "",        "icon": "modules/menu/logo.svg"    }, {        "id": "productName",        "type": "default",        "link": "",        "text": "Product Name"    }], "(union)[]");
    {
AssertType({        "id": "ourLogo",        "type": "image",        "link": "",        "icon": "modules/menu/logo.svg"    }, "{ id: string; type: string; link: string; icon: string; }");

        "id": "ourLogo",
AssertType("id", "string");
AssertType("ourLogo", "string");

        "type": "image",
AssertType("type", "string");
AssertType("image", "string");

        "link": "",
AssertType("link", "string");
AssertType("", "string");

        "icon": "modules/menu/logo.svg"
AssertType("icon", "string");
AssertType("modules/menu/logo.svg", "string");

    }, {
AssertType({        "id": "productName",        "type": "default",        "link": "",        "text": "Product Name"    }, "{ id: string; type: string; link: string; text: string; }");

        "id": "productName",
AssertType("id", "string");
AssertType("productName", "string");

        "type": "default",
AssertType("type", "string");
AssertType("default", "string");

        "link": "",
AssertType("link", "string");
AssertType("", "string");

        "text": "Product Name"
AssertType("text", "string");
AssertType("Product Name", "string");
    }
];


