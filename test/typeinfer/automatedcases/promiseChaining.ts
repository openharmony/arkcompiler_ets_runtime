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

// === tests/cases/compiler/promiseChaining.ts ===
declare function AssertType(value:any, type:string):void;
class Chain<T> {
    constructor(public value: T) { }
    then<S>(cb: (x: T) => S): Chain<S> {
        let result = cb(this.value);
AssertType(result, "S");
AssertType(cb(this.value), "S");
AssertType(cb, "(T) => S");
AssertType(this.value, "T");
AssertType(this, "this");

        // should get a fresh type parameter which each then call
        let z = this.then(x => result)/*S*/.then(x => "abc")/*string*/.then(x => x.length)/*number*/; // No error
AssertType(z, "Chain<number>");
AssertType(this.then(x => result)/*S*/.then(x => "abc")/*string*/.then(x => x.length), "Chain<number>");
AssertType(this.then(x => result)/*S*/.then(x => "abc")/*string*/.then, "<S>((string) => S) => Chain<S>");
AssertType(this.then(x => result)/*S*/.then(x => "abc"), "Chain<string>");
AssertType(this.then(x => result)/*S*/.then, "<S>((S) => S) => Chain<S>");
AssertType(this.then(x => result), "Chain<S>");
AssertType(this.then, "<S>((T) => S) => Chain<S>");
AssertType(this, "this");
AssertType(x => result, "(T) => S");
AssertType(x, "T");
AssertType(result, "S");
AssertType(x => "abc", "(S) => string");
AssertType(x, "S");
AssertType("abc", "string");
AssertType(x => x.length, "(string) => number");
AssertType(x, "string");
AssertType(x.length, "number");

AssertType(new Chain(result), "Chain<S>");
AssertType(Chain, "typeof Chain");
AssertType(result, "S");
        return new Chain(result);
    }
}



