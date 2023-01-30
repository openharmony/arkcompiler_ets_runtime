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

// === tests/cases/compiler/typeInferenceReturnTypeCallback.ts ===
declare function AssertType(value:any, type:string):void;
interface IList<A> {
    map<B>(f: (t: A) => B): IList<B>;
}

class Nil<C> implements IList<C>{
    map<D>(f: (t: C) => D): IList<D> {
AssertType(null, "null");
        return null;
    }
}

class Cons<T> implements IList<T>{
    map<U>(f: (t: T) => U): IList<U> {
AssertType(this.foldRight(new Nil<U>(), (t, acc) => {            return new Cons<U>();        }), "Nil<U>");
AssertType(this.foldRight, "<E>(E, (T, E) => E) => E");
AssertType(this, "this");
AssertType(new Nil<U>(), "Nil<U>");
AssertType(Nil, "typeof Nil");
AssertType((t, acc) => {            return new Cons<U>();        }, "(T, Nil<U>) => Cons<U>");
AssertType(t, "T");
AssertType(acc, "Nil<U>");
        return this.foldRight(new Nil<U>(), (t, acc) => {

AssertType(new Cons<U>(), "Cons<U>");
AssertType(Cons, "typeof Cons");
            return new Cons<U>();

        });
    }

    foldRight<E>(z: E, f: (t: T, acc: E) => E): E {
AssertType(null, "null");
        return null;
    }
}

