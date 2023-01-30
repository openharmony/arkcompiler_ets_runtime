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

// === tests/cases/compiler/specializedInheritedConstructors1.ts ===
declare function AssertType(value:any, type:string):void;
interface ViewOptions<TModel> {
    model: TModel;
}

class View<TModel> {
    constructor(options: ViewOptions<TModel>) { }
    model: TModel;
}

class Model { }
class MyView extends View<Model> { }

let m: ViewOptions<Model> = { model: new Model() };
AssertType(m, "ViewOptions<Model>");
AssertType({ model: new Model() }, "{ model: Model; }");
AssertType(model, "Model");
AssertType(new Model(), "Model");
AssertType(Model, "typeof Model");

let aView = new View({ model: new Model() }); 
AssertType(aView, "View<Model>");
AssertType(new View({ model: new Model() }), "View<Model>");
AssertType(View, "typeof View");
AssertType({ model: new Model() }, "{ model: Model; }");
AssertType(model, "Model");
AssertType(new Model(), "Model");
AssertType(Model, "typeof Model");

let aView2 = new View(m); 
AssertType(aView2, "View<Model>");
AssertType(new View(m), "View<Model>");
AssertType(View, "typeof View");
AssertType(m, "ViewOptions<Model>");

let myView = new MyView(m); // was error
AssertType(myView, "MyView");
AssertType(new MyView(m), "MyView");
AssertType(MyView, "typeof MyView");
AssertType(m, "ViewOptions<Model>");


