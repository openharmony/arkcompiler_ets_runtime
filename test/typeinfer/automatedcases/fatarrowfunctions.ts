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

// === tests/cases/compiler/fatarrowfunctions.ts ===
declare function AssertType(value:any, type:string):void;
function foo(x:any) {
AssertType(x(), "any");
AssertType(x, "any");
	return x();
}


foo((x:number,y,z)=>{
AssertType(foo((x:number,y,z)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x:number,y,z)=>{return x+y+z;}, "(number, any, any) => any");
AssertType(x, "number");
AssertType(y, "any");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "number");
AssertType(y, "any");
AssertType(z, "any");
return x+y+z;});

foo((x,y,z)=>{
AssertType(foo((x,y,z)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y,z)=>{return x+y+z;}, "(any, any, any) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
return x+y+z;});

foo((x,y:number,z)=>{
AssertType(foo((x,y:number,z)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y:number,z)=>{return x+y+z;}, "(any, number, any) => any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "any");
return x+y+z;});

foo((x,y:number,z:number)=>{
AssertType(foo((x,y:number,z:number)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y:number,z:number)=>{return x+y+z;}, "(any, number, number) => any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "number");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "number");
return x+y+z;});

foo((x,y,z:number)=>{
AssertType(foo((x,y,z:number)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y,z:number)=>{return x+y+z;}, "(any, any, number) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "number");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "number");
return x+y+z;});

foo(()=>{
AssertType(foo(()=>{return 0;}), "any");
AssertType(foo, "(any) => any");
AssertType(()=>{return 0;}, "() => number");
AssertType(0, "int");
return 0;});

foo((x:number,y,z)=>x+y+z);
AssertType(foo((x:number,y,z)=>x+y+z), "any");
AssertType(foo, "(any) => any");
AssertType((x:number,y,z)=>x+y+z, "(number, any, any) => any");
AssertType(x, "number");
AssertType(y, "any");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "number");
AssertType(y, "any");
AssertType(z, "any");

foo((x,y,z)=>x+y+z);
AssertType(foo((x,y,z)=>x+y+z), "any");
AssertType(foo, "(any) => any");
AssertType((x,y,z)=>x+y+z, "(any, any, any) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

foo((x,y:number,z)=>{
AssertType(foo((x,y:number,z)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y:number,z)=>{return x+y+z;}, "(any, number, any) => any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "any");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "any");
return x+y+z;});

foo((x,y:number,z:number)=>{
AssertType(foo((x,y:number,z:number)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y:number,z:number)=>{return x+y+z;}, "(any, number, number) => any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "number");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "number");
AssertType(z, "number");
return x+y+z;});

foo((x,y,z:number)=>{
AssertType(foo((x,y,z:number)=>{return x+y+z;}), "any");
AssertType(foo, "(any) => any");
AssertType((x,y,z:number)=>{return x+y+z;}, "(any, any, number) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "number");
AssertType(x+y+z, "any");
AssertType(x+y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "number");
return x+y+z;});

foo(()=>{
AssertType(foo(()=>{return 0;}), "any");
AssertType(foo, "(any) => any");
AssertType(()=>{return 0;}, "() => number");
AssertType(0, "int");
return 0;});


foo(((x) => x));
AssertType(foo(((x) => x)), "any");
AssertType(foo, "(any) => any");
AssertType(((x) => x), "(any) => any");
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

foo(x => x*x);
AssertType(foo(x => x*x), "any");
AssertType(foo, "(any) => any");
AssertType(x => x*x, "(any) => number");
AssertType(x, "any");
AssertType(x*x, "number");
AssertType(x, "any");
AssertType(x, "any");

let y = x => x*x;
AssertType(y, "(any) => number");
AssertType(x => x*x, "(any) => number");
AssertType(x, "any");
AssertType(x*x, "number");
AssertType(x, "any");
AssertType(x, "any");

let z = (x:number) => x*x;
AssertType(z, "(number) => number");
AssertType((x:number) => x*x, "(number) => number");
AssertType(x, "number");
AssertType(x*x, "number");
AssertType(x, "number");
AssertType(x, "number");

let w = () => 3;
AssertType(w, "() => number");
AssertType(() => 3, "() => number");
AssertType(3, "int");

function ternaryTest(isWhile:boolean) {

                let f = isWhile ? function (n) { 
AssertType(f, "(any) => boolean");
AssertType(isWhile ? function (n) { return n > 0; } : function (n) { return n === 0; }, "(any) => boolean");
AssertType(isWhile, "boolean");
AssertType(function (n) { return n > 0; }, "(any) => boolean");
AssertType(n, "any");
AssertType(n > 0, "boolean");
AssertType(n, "any");
AssertType(0, "int");
AssertType(function (n) { return n === 0; }, "(any) => boolean");
AssertType(n, "any");
AssertType(n === 0, "boolean");
AssertType(n, "any");
AssertType(0, "int");
return n > 0; } : function (n) { return n === 0; };

}

declare function setTimeout(expression: any, msec?: number, language?: any): number;

let messenger = {
AssertType(messenger, "{ message: string; start: () => void; }");
AssertType({    message: "Hello World",    start: function() {        setTimeout(() => { this.message.toString(); }, 3000);    }}, "{ message: string; start: () => void; }");

    message: "Hello World",
AssertType(message, "string");
AssertType("Hello World", "string");

    start: function() {
AssertType(start, "() => void");
AssertType(function() {        setTimeout(() => { this.message.toString(); }, 3000);    }, "() => void");

        setTimeout(() => { this.message.toString(); }, 3000);
AssertType(setTimeout(() => { this.message.toString(); }, 3000), "number");
AssertType(setTimeout, "{ (TimerHandler, ?number, ...any[]): number; (any, ?number, ?any): number; }");
AssertType(() => { this.message.toString(); }, "() => void");
AssertType(this.message.toString(), "any");
AssertType(this.message.toString, "any");
AssertType(this.message, "any");
AssertType(this, "any");
AssertType(3000, "int");
    }
};


