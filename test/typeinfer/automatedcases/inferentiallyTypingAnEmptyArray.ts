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

// === tests/cases/compiler/inferentiallyTypingAnEmptyArray.ts ===
declare function AssertType(value:any, type:string):void;
// April 2014, Section 4.6:
//      In the absence of a contextual type, the type of an array literal is C[], where C is the
//      Undefined type(section 3.2.6) if the array literal is empty, or the best common type of
//      the element expressions if the array literal is not empty.
//      When an array literal is contextually typed(section 4.19) by an object type containing a
//      numeric index signature of type T, each element expression is contextually typed by T and
//      the type of the array literal is the best common type of T and the types of the element
//      expressions.
//
// While the spec does not say it, an inferential type causes an empty array literal to have
// the undefined[] type. In other words, the first clause from the excerpt above applies even
// though there is a "contextual type" present. This is the intention, even though the spec
// seems to imply the contrary.
// Therefore, the following access to bar should not cause an error because we infer
// the undefined[] type.
declare function foo<T>(arr: T[]): T;
foo([]).bar;
AssertType(foo([]).bar, "any");


