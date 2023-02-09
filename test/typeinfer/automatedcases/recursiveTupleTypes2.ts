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

// === tests/cases/compiler/recursiveTupleTypes2.ts ===
declare function AssertType(value:any, type:string):void;
interface Tree1 {
    children: [Tree1, Tree2];
}

interface Tree2 {
    children: [Tree2, Tree2];
}

let tree1: Tree1;
AssertType(tree1, "Tree1");

let tree2: Tree2;
AssertType(tree2, "Tree2");

tree1 = tree2;
AssertType(tree1 = tree2, "Tree2");
AssertType(tree1, "Tree1");
AssertType(tree2, "Tree2");

tree2 = tree1;
AssertType(tree2 = tree1, "Tree1");
AssertType(tree2, "Tree2");
AssertType(tree1, "Tree1");


