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

// === tests/cases/compiler/anyAsReturnTypeForNewOnCall.ts ===
declare function AssertType(value:any, type:string):void;
function Point(x, y) {

 this.x = x;
AssertType(this.x = x, "any");
AssertType(this.x, "any");
AssertType(this, "any");
AssertType(x, "any");

 this.y = y;
AssertType(this.y = y, "any");
AssertType(this.y, "any");
AssertType(this, "any");
AssertType(y, "any");

}

let o = new Point(3, 4);
AssertType(o, "any");
AssertType(new Point(3, 4), "any");
AssertType(Point, "(any, any) => void");
AssertType(3, "int");
AssertType(4, "int");

let xx = o.x;
AssertType(xx, "any");
AssertType(o.x, "any");

 


