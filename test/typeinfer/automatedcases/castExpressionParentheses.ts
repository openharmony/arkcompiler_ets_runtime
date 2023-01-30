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

// === tests/cases/compiler/castExpressionParentheses.ts ===
declare function AssertType(value:any, type:string):void;
declare let a;
AssertType(a, "any");

// parentheses should be omitted
// literals
(<any>{a:0});  
AssertType((<any>{a:0}), "any");
AssertType(<any>{a:0}, "any");
AssertType({a:0}, "{ a: number; }");
AssertType(a, "number");
AssertType(0, "int");

(<any>[1,3,]); 
AssertType((<any>[1,3,]), "any");
AssertType(<any>[1,3,], "any");
AssertType([1,3,], "number[]");
AssertType(1, "int");
AssertType(3, "int");

(<any>"string"); 
AssertType((<any>"string"), "any");
AssertType(<any>"string", "any");
AssertType("string", "string");

(<any>23.0); 
AssertType((<any>23.0), "any");
AssertType(<any>23.0, "any");
AssertType(23.0, "int");

(<any>1); 
AssertType((<any>1), "any");
AssertType(<any>1, "any");
AssertType(1, "int");

(<any>1.);
AssertType((<any>1.), "any");
AssertType(<any>1., "any");
AssertType(1., "int");

(<any>1.0);
AssertType((<any>1.0), "any");
AssertType(<any>1.0, "any");
AssertType(1.0, "int");

(<any>12e+34);
AssertType((<any>12e+34), "any");
AssertType(<any>12e+34, "any");
AssertType(12e+34, "int");

(<any>0xff);
AssertType((<any>0xff), "any");
AssertType(<any>0xff, "any");
AssertType(0xff, "int");

(<any>/regexp/g); 
AssertType((<any>/regexp/g), "any");
AssertType(<any>/regexp/g, "any");
AssertType(/regexp/g, "RegExp");

(<any>false); 
AssertType((<any>false), "any");
AssertType(<any>false, "any");
AssertType(false, "boolean");

(<any>true); 
AssertType((<any>true), "any");
AssertType(<any>true, "any");
AssertType(true, "boolean");

(<any>null); 
AssertType((<any>null), "any");
AssertType(<any>null, "any");
AssertType(null, "null");

// names and dotted names
(<any>this); 
AssertType((<any>this), "any");
AssertType(<any>this, "any");
AssertType(this, "typeof globalThis");

(<any>this.x); 
AssertType((<any>this.x), "any");
AssertType(<any>this.x, "any");
AssertType(this.x, "any");

(<any>(<any>a).x);
AssertType((<any>(<any>a).x), "any");
AssertType(<any>(<any>a).x, "any");
AssertType((<any>a).x, "any");

(<any><any>a);
AssertType((<any><any>a), "any");
AssertType(<any><any>a, "any");
AssertType(<any>a, "any");
AssertType(a, "any");

(<any>a[0]);
AssertType((<any>a[0]), "any");
AssertType(<any>a[0], "any");
AssertType(a[0], "any");
AssertType(a, "any");
AssertType(0, "int");

(<any>a.b["0"]);
AssertType((<any>a.b["0"]), "any");
AssertType(<any>a.b["0"], "any");
AssertType(a.b["0"], "any");
AssertType(a.b, "any");
AssertType("0", "string");

(<any>a()).x;
AssertType((<any>a()).x, "any");

(<any>1).foo;
AssertType((<any>1).foo, "any");

(<any>1.).foo;
AssertType((<any>1.).foo, "any");

(<any>1.0).foo;
AssertType((<any>1.0).foo, "any");

(<any>12e+34).foo;
AssertType((<any>12e+34).foo, "any");

(<any>0xff).foo;
AssertType((<any>0xff).foo, "any");

declare let A;
AssertType(A, "any");

// should keep the parentheses in emit
(<any>(1.0)); 
AssertType((<any>(1.0)), "any");
AssertType(<any>(1.0), "any");
AssertType((1.0), "int");
AssertType(1.0, "int");

(<any>new A).foo; 
AssertType((<any>new A).foo, "any");

(<any>typeof A).x; 
AssertType((<any>typeof A).x, "any");

(<any>-A).x; 
AssertType((<any>-A).x, "any");

new (<any>A());
AssertType(new (<any>A()), "any");
AssertType((<any>A()), "any");
AssertType(<any>A(), "any");
AssertType(A(), "any");
AssertType(A, "any");

(<Tany>()=> {})();
AssertType((<Tany>()=> {})(), "void");
AssertType((<Tany>()=> {}), "<Tany>() => void");
AssertType(<Tany>()=> {}, "<Tany>() => void");

(<any>function foo() { })();
AssertType((<any>function foo() { })(), "any");
AssertType((<any>function foo() { }), "any");
AssertType(<any>function foo() { }, "any");
AssertType(function foo() { }, "() => void");
AssertType(foo, "() => void");

(<any><number><any>-A).x; 
AssertType((<any><number><any>-A).x, "any");

// nested cast, should keep one pair of parenthese
(<any><number>(<any>-A)).x; 
AssertType((<any><number>(<any>-A)).x, "any");

// nested parenthesized expression, should keep one pair of parenthese
(<any>(A)) 
AssertType((<any>(A)), "any");
AssertType(<any>(A), "any");
AssertType((A), "any");
AssertType(A, "any");



