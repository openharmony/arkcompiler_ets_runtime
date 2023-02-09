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

// === tests/cases/compiler/overloadResolutionOverNonCTObjectLit.ts ===
declare function AssertType(value:any, type:string):void;
module Bugs {
                export interface IToken {
                                startIndex:number;
                                type:string;
                                bracket:number;
                }
                
                export interface IState {
                }

                export interface IStateToken extends IToken {
                                state: IState;
                                length: number;
                }
                
                function bug3() {
                                let tokens:IToken[]= [];
AssertType(tokens, "IToken[]");
AssertType([], "undefined[]");

                                tokens.push({ startIndex: 1, type: '', bracket: 3 });
AssertType(tokens.push({ startIndex: 1, type: '', bracket: 3 }), "number");
AssertType(tokens.push, "(...IToken[]) => number");
AssertType({ startIndex: 1, type: '', bracket: 3 }, "{ startIndex: number; type: string; bracket: number; }");
AssertType(startIndex, "number");
AssertType(1, "int");
AssertType(type, "string");
AssertType('', "string");
AssertType(bracket, "number");
AssertType(3, "int");

                                tokens.push(<IToken>({ startIndex: 1, type: '', bracket: 3, state: null, length: 10 }));
AssertType(tokens.push(<IToken>({ startIndex: 1, type: '', bracket: 3, state: null, length: 10 })), "number");
AssertType(tokens.push, "(...IToken[]) => number");
AssertType(<IToken>({ startIndex: 1, type: '', bracket: 3, state: null, length: 10 }), "IToken");
AssertType(({ startIndex: 1, type: '', bracket: 3, state: null, length: 10 }), "{ startIndex: number; type: string; bracket: number; state: null; length: number; }");
AssertType({ startIndex: 1, type: '', bracket: 3, state: null, length: 10 }, "{ startIndex: number; type: string; bracket: number; state: null; length: number; }");
AssertType(startIndex, "number");
AssertType(1, "int");
AssertType(type, "string");
AssertType('', "string");
AssertType(bracket, "number");
AssertType(3, "int");
AssertType(state, "null");
AssertType(null, "null");
AssertType(length, "number");
AssertType(10, "int");
                }
}

