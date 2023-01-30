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

// === tests/cases/compiler/methodContainingLocalFunction.ts ===
declare function AssertType(value:any, type:string):void;
// The first case here (BugExhibition<T>) caused a crash. Try with different permutations of features.
class BugExhibition<T> {
    public exhibitBug() {
        function localFunction() { 
AssertType(localFunction, "() => void");
}

        let x: { (): void; };
AssertType(x, "() => void");

        x = localFunction;
AssertType(x = localFunction, "() => void");
AssertType(x, "() => void");
AssertType(localFunction, "() => void");
    }
}

class BugExhibition2<T> {
    private static get exhibitBug() {
        function localFunction() { 
AssertType(localFunction, "() => void");
}

        let x: { (): void; };
AssertType(x, "() => void");

        x = localFunction;
AssertType(x = localFunction, "() => void");
AssertType(x, "() => void");
AssertType(localFunction, "() => void");

AssertType(null, "null");
        return null;
    }
}

class BugExhibition3<T> {
    public exhibitBug() {
        function localGenericFunction<U>(u?: U) { 
AssertType(localGenericFunction, "<U>(?U) => void");

AssertType(u, "U");
}

        let x: { (): void; };
AssertType(x, "() => void");

        x = localGenericFunction;
AssertType(x = localGenericFunction, "<U>(?U) => void");
AssertType(x, "() => void");
AssertType(localGenericFunction, "<U>(?U) => void");
    }
}

class C {
    exhibit() {
        let funcExpr = <U>(u?: U) => { };
AssertType(funcExpr, "<U>(?U) => void");
AssertType(<U>(u?: U) => { }, "<U>(?U) => void");
AssertType(u, "U");

        let x: { (): void; };
AssertType(x, "() => void");

        x = funcExpr;
AssertType(x = funcExpr, "<U>(?U) => void");
AssertType(x, "() => void");
AssertType(funcExpr, "<U>(?U) => void");
    }
}

module M {
    export function exhibitBug() {
        function localFunction() { 
AssertType(localFunction, "() => void");
}

        let x: { (): void; };
AssertType(x, "() => void");

        x = localFunction;
AssertType(x = localFunction, "() => void");
AssertType(x, "() => void");
AssertType(localFunction, "() => void");
    }
}

enum E {
    A = (() => {
        function localFunction() { 
AssertType(localFunction, "() => void");
}

        let x: { (): void; };
AssertType(x, "() => void");

        x = localFunction;
AssertType(x = localFunction, "() => void");
AssertType(x, "() => void");
AssertType(localFunction, "() => void");

AssertType(0, "int");
        return 0;

    })()
}

