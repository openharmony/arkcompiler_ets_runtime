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

// === tests/cases/compiler/arrayLiteralContextualType.ts ===
declare function AssertType(value:any, type:string):void;
interface IAnimal {
    name: string;
}

class Giraffe {
    name = "Giraffe";
    neckLength = "3m";
}

class Elephant {
    name = "Elephant";
    trunkDiameter = "20cm";
}

function foo(animals: IAnimal[]) { }
function bar(animals: { [n: number]: IAnimal }) { }

foo([
AssertType(foo([    new Giraffe(),    new Elephant()]), "void");
AssertType(foo, "(IAnimal[]) => void");
AssertType([    new Giraffe(),    new Elephant()], "(union)[]");

    new Giraffe(),
AssertType(new Giraffe(), "Giraffe");
AssertType(Giraffe, "typeof Giraffe");

    new Elephant()
AssertType(new Elephant(), "Elephant");
AssertType(Elephant, "typeof Elephant");

]); // Legal because of the contextual type IAnimal provided by the parameter
bar([
AssertType(bar([    new Giraffe(),    new Elephant()]), "void");
AssertType(bar, "({ [number]: IAnimal; }) => void");
AssertType([    new Giraffe(),    new Elephant()], "(union)[]");

    new Giraffe(),
AssertType(new Giraffe(), "Giraffe");
AssertType(Giraffe, "typeof Giraffe");

    new Elephant()
AssertType(new Elephant(), "Elephant");
AssertType(Elephant, "typeof Elephant");

]); // Legal because of the contextual type IAnimal provided by the parameter

let arr = [new Giraffe(), new Elephant()];
AssertType(arr, "(union)[]");
AssertType([new Giraffe(), new Elephant()], "(union)[]");
AssertType(new Giraffe(), "Giraffe");
AssertType(Giraffe, "typeof Giraffe");
AssertType(new Elephant(), "Elephant");
AssertType(Elephant, "typeof Elephant");

foo(arr); // ok because arr is Array<Giraffe|Elephant> not {}[]
AssertType(foo(arr), "void");
AssertType(foo, "(IAnimal[]) => void");
AssertType(arr, "(union)[]");

bar(arr); // ok because arr is Array<Giraffe|Elephant> not {}[]
AssertType(bar(arr), "void");
AssertType(bar, "({ [number]: IAnimal; }) => void");
AssertType(arr, "(union)[]");


