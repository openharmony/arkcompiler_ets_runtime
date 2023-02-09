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

// === tests/cases/compiler/genericTypeParameterEquivalence2.ts ===
declare function AssertType(value:any, type:string):void;
// compose :: (b->c) -> (a->b) -> (a->c)
function compose<A, B, C>(f: (b: B) => C, g: (a:A) => B): (a:A) => C {
AssertType(function (a:A) : C {        return f(g.apply(null, a));    }, "(A) => C");
AssertType(a, "A");
    return function (a:A) : C {

AssertType(f(g.apply(null, a)), "C");
AssertType(f, "(B) => C");
AssertType(g.apply(null, a), "any");
AssertType(g.apply, "(Function, any, ?any) => any");
AssertType(null, "null");
AssertType(a, "A");
        return f(g.apply(null, a));

    };
}

// forEach :: [a] -> (a -> ()) -> ()
function forEach<A>(list: A[], f: (a: A, n?: number) => void ): void {
    for (let i = 0; i < list.length; ++i) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < list.length, "boolean");
AssertType(i, "number");
AssertType(list.length, "number");
AssertType(++i, "number");
AssertType(i, "number");

        f(list[i], i);
AssertType(f(list[i], i), "void");
AssertType(f, "(A, ?number) => void");
AssertType(list[i], "A");
AssertType(list, "A[]");
AssertType(i, "number");
AssertType(i, "number");
    }
}

// filter :: (a->bool) -> [a] -> [a]
function filter<A>(f: (a: A) => boolean, ar: A[]): A[] {
    let ret = [];
AssertType(ret, "any[]");
AssertType([], "undefined[]");

    forEach(ar, (el) => {
AssertType(forEach(ar, (el) => {        if (f(el)) {            ret.push(el);        }    } ), "void");
AssertType(forEach, "<A>(A[], (A, ?number) => void) => void");
AssertType(ar, "A[]");
AssertType((el) => {        if (f(el)) {            ret.push(el);        }    }, "(A) => void");
AssertType(el, "A");

        if (f(el)) {
AssertType(f(el), "boolean");
AssertType(f, "(A) => boolean");
AssertType(el, "A");

            ret.push(el);
AssertType(ret.push(el), "number");
AssertType(ret.push, "(...any[]) => number");
AssertType(el, "A");
        }
    } );

AssertType(ret, "any[]");
    return ret;
}

// length :: [a] -> Num
function length2<A>(ar: A[]): number {
AssertType(ar.length, "number");
    return ar.length;
}

// curry1 :: ((a,b)->c) -> (a->(b->c))
function curry1<A, B, C>(f: (a: A, b: B) => C): (ax: A) => (bx: B) => C {
AssertType(function (ay: A) {        return function (by: B) {            return f(ay, by);        };    }, "(A) => (B) => C");
AssertType(ay, "A");
    return function (ay: A) {

AssertType(function (by: B) {            return f(ay, by);        }, "(B) => C");
AssertType(by, "B");
        return function (by: B) {

AssertType(f(ay, by), "C");
AssertType(f, "(A, B) => C");
AssertType(ay, "A");
AssertType(by, "B");
            return f(ay, by);

        };
    };
}

let cfilter = curry1(filter);
AssertType(cfilter, "<A>((A) => boolean) => (A[]) => A[]");
AssertType(curry1(filter), "<A>((A) => boolean) => (A[]) => A[]");
AssertType(curry1, "<A, B, C>((A, B) => C) => (A) => (B) => C");
AssertType(filter, "<A>((A) => boolean, A[]) => A[]");

// compose :: (b->c) -> (a->b) -> (a->c)
// length :: [a] -> Num
// cfilter :: {} -> {} -> [{}]
// pred :: a -> Bool 
// cfilter(pred) :: {} -> [{}]
// length2 :: [a] -> Num
// countWhere :: (a -> Bool) -> [a] -> Num

function countWhere_1<A>(pred: (a: A) => boolean): (a: A[]) => number {
AssertType(compose(length2, cfilter(pred)), "(A[]) => number");
AssertType(compose, "<A, B, C>((B) => C, (A) => B) => (A) => C");
AssertType(length2, "<A>(A[]) => number");
AssertType(cfilter(pred), "(A[]) => A[]");
AssertType(cfilter, "<A>((A) => boolean) => (A[]) => A[]");
AssertType(pred, "(A) => boolean");
    return compose(length2, cfilter(pred));
}

function countWhere_2<A>(pred: (a: A) => boolean): (a: A[]) => number {
    let where = cfilter(pred);
AssertType(where, "(A[]) => A[]");
AssertType(cfilter(pred), "(A[]) => A[]");
AssertType(cfilter, "<A>((A) => boolean) => (A[]) => A[]");
AssertType(pred, "(A) => boolean");

AssertType(compose(length2, where), "(A[]) => number");
AssertType(compose, "<A, B, C>((B) => C, (A) => B) => (A) => C");
AssertType(length2, "<A>(A[]) => number");
AssertType(where, "(A[]) => A[]");
    return compose(length2, where);
}

