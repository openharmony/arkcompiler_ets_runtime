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

// === tests/cases/compiler/capturedLetConstInLoop9_ES6.ts ===
declare function AssertType(value:any, type:string):void;
for (let x = 0; x < 1; ++x) {
    let x;
AssertType(x, "any");

    (function() { 
AssertType((function() { return x }), "() => any");
AssertType(function() { return x }, "() => any");
AssertType(x, "any");
return x });
    {
        let x;
AssertType(x, "any");

        (function() { 
AssertType((function() { return x }), "() => any");
AssertType(function() { return x }, "() => any");
AssertType(x, "any");
return x });
    }

    try { }
    catch (e) {
AssertType(e, "any");

        let x;
AssertType(x, "any");

        (function() { 
AssertType((function() { return x }), "() => any");
AssertType(function() { return x }, "() => any");
AssertType(x, "any");
return x });
    }

    switch (x) {
AssertType(x, "any");

        case 1:
AssertType(1, "int");

            let x;
AssertType(x, "any");

            (function() { 
AssertType((function() { return x }), "() => any");
AssertType(function() { return x }, "() => any");
AssertType(x, "any");
return x });

           break;
    }
    
    while (1 == 1) {
AssertType(1 == 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        let x;
AssertType(x, "any");

        (function() { 
AssertType((function() { return x }), "() => any");
AssertType(function() { return x }, "() => any");
AssertType(x, "any");
return x });
    }
    
    class A {
        m() {
AssertType(x + 1, "any");
AssertType(x, "any");
AssertType(1, "int");
            return x + 1;
        }
    }
}

declare function use(a: any);

function foo() {
    l0:
AssertType(l0, "any");

    for (let a of []) {
AssertType(a, "any");
AssertType([], "undefined[]");
        
        if (a === 1) {
AssertType(a === 1, "boolean");
AssertType(a, "any");
AssertType(1, "int");

            break;
        }
        
        if (a === 2) {
AssertType(a === 2, "boolean");
AssertType(a, "any");
AssertType(2, "int");

            break l0;
AssertType(l0, "any");
        }
        
        for (let b of []) {
AssertType(b, "any");
AssertType([], "undefined[]");

            let [{x, y:z}] = [{x:1, y:2}];
AssertType(x, "number");
AssertType(y, "any");
AssertType(z, "number");
AssertType([{x:1, y:2}], "[{ x: number; y: number; }]");
AssertType({x:1, y:2}, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");

            if (b === 1) {
AssertType(b === 1, "boolean");
AssertType(b, "any");
AssertType(1, "int");

                break;
            }
            
            
            if (b === 2) {
AssertType(b === 2, "boolean");
AssertType(b, "any");
AssertType(2, "int");

                break l0;
AssertType(l0, "any");
            }
            
            l1:
AssertType(l1, "any");

            if (b === 3) {
AssertType(b === 3, "boolean");
AssertType(b, "any");
AssertType(3, "int");

                break l1;
AssertType(l1, "any");
            }
            
AssertType(50, "int");
            return 50;
        }

        for (let b of []) {
AssertType(b, "any");
AssertType([], "undefined[]");

            let [{x1, y:z1}] = [{x1:1, y:arguments.length}];
AssertType(x1, "number");
AssertType(y, "any");
AssertType(z1, "number");
AssertType([{x1:1, y:arguments.length}], "[{ x1: number; y: number; }]");
AssertType({x1:1, y:arguments.length}, "{ x1: number; y: number; }");
AssertType(x1, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(arguments.length, "number");

            if (b === 1) {
AssertType(b === 1, "boolean");
AssertType(b, "any");
AssertType(1, "int");

                break;
            }
            
            if (b === 2) {
AssertType(b === 2, "boolean");
AssertType(b, "any");
AssertType(2, "int");

                break l0;
AssertType(l0, "any");
            }
            () => b
AssertType(() => b, "() => any");
AssertType(b, "any");

AssertType(100, "int");
            return 100;
        }

        
        () => a;
AssertType(() => a, "() => any");
AssertType(a, "any");
    }
    
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "number");

    use(z);
AssertType(use(z), "any");
AssertType(use, "(any) => any");
AssertType(z, "number");

    use(x1);
AssertType(use(x1), "any");
AssertType(use, "(any) => any");
AssertType(x1, "number");

    use(z1);
AssertType(use(z1), "any");
AssertType(use, "(any) => any");
AssertType(z1, "number");
}

function foo2() {
    for (let x of []) {
AssertType(x, "any");
AssertType([], "undefined[]");

        if (x === 1) {
AssertType(x === 1, "boolean");
AssertType(x, "any");
AssertType(1, "int");

            break;
        }
        else if (x === 2) {
AssertType(x === 2, "boolean");
AssertType(x, "any");
AssertType(2, "int");

            continue;
        }
        
        while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

            if (x) {
AssertType(x, "any");

                break;
            }
            else {
                continue;
            }
        }
        
        switch(x) {
AssertType(x, "any");

            case 1: break;
AssertType(1, "int");

            case 2: continue;
AssertType(2, "int");
        }
        
        for (let y of []) {
AssertType(y, "any");
AssertType([], "undefined[]");

            switch(y) {
AssertType(y, "any");

                case 1: break;
AssertType(1, "int");

                case 2: continue;
AssertType(2, "int");
            }
        }
    }
}

class C {
    constructor(private N: number) { }
    foo() {
        for (let i = 0; i < 100; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 100, "boolean");
AssertType(i, "number");
AssertType(100, "int");
AssertType(i++, "number");
AssertType(i, "number");

            let f = () => this.N * i;
AssertType(f, "() => number");
AssertType(() => this.N * i, "() => number");
AssertType(this.N * i, "number");
AssertType(this.N, "number");
AssertType(this, "this");
AssertType(i, "number");
        }
    }
}

function foo3 () {
    let x = arguments.length;
AssertType(x, "number");
AssertType(arguments.length, "number");

    for (let y of []) {
AssertType(y, "any");
AssertType([], "undefined[]");

        let z = arguments.length;
AssertType(z, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return y + z + arguments.length; }), "() => any");
AssertType(function() { return y + z + arguments.length; }, "() => any");
AssertType(y + z + arguments.length, "any");
AssertType(y + z, "any");
AssertType(y, "any");
AssertType(z, "number");
AssertType(arguments.length, "number");
return y + z + arguments.length; });
    }
}

