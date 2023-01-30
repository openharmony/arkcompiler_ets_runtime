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

// === tests/cases/compiler/moduleAliasInterface.ts ===
declare function AssertType(value:any, type:string):void;

module _modes {
 export interface IMode {
  
 }
 
 export class Mode {
  
 }
}

// _modes. // produces an internal error - please implement in derived class

module editor {
 import modes = _modes;
 
 let i : modes.IMode;
  
 // If you just use p1:modes, the compiler accepts it - should be an error
 class Bug { 
  constructor(p1: modes.IMode, p2: modes.Mode) { }// should be an error on p2 - it's not exported
  public foo(p1:modes.IMode) {
   
  } 
 }
}

import modesOuter = _modes;
module editor2 {
 
 let i : modesOuter.IMode;
 
 class Bug {
     constructor(p1: modesOuter.IMode, p2: modesOuter.Mode) { }// no error here, since modesOuter is declared externally
  
 }
 
  module Foo { export class Bar{} }
 
  class Bug2 {
      constructor(p1: Foo.Bar, p2: modesOuter.Mode) { }
  }
}

module A1 {
    export interface A1I1 {}
    export class A1C1 {}
}

module B1 {
    import A1Alias1 = A1;
    
    let i : A1Alias1.A1I1;   
    let c : A1Alias1.A1C1;
}


