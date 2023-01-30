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

// === tests/cases/compiler/inferFromGenericFunctionReturnTypes2.ts ===
declare function AssertType(value:any, type:string):void;
type Mapper<T, U> = (x: T) => U;

declare function wrap<T, U>(cb: Mapper<T, U>): Mapper<T, U>;

declare function arrayize<T, U>(cb: Mapper<T, U>): Mapper<T, U[]>;

declare function combine<A, B, C>(f: (x: A) => B, g: (x: B) => C): (x: A) => C;

declare function foo(f: Mapper<string, number>): void;

let f1: Mapper<string, number> = s => s.length;
AssertType(f1, "Mapper<string, number>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let f2: Mapper<string, number> = wrap(s => s.length);
AssertType(f2, "Mapper<string, number>");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let f3: Mapper<string, number[]> = arrayize(wrap(s => s.length));
AssertType(f3, "Mapper<string, number[]>");
AssertType(arrayize(wrap(s => s.length)), "Mapper<string, number[]>");
AssertType(arrayize, "<T, U>(Mapper<T, U>) => Mapper<T, U[]>");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let f4: Mapper<string, boolean> = combine(wrap(s => s.length), wrap(n => n >= 10));
AssertType(f4, "Mapper<string, boolean>");
AssertType(combine(wrap(s => s.length), wrap(n => n >= 10)), "(string) => boolean");
AssertType(combine, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");
AssertType(wrap(n => n >= 10), "Mapper<number, boolean>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(n => n >= 10, "(number) => boolean");
AssertType(n, "number");
AssertType(n >= 10, "boolean");
AssertType(n, "number");
AssertType(10, "int");

foo(wrap(s => s.length));
AssertType(foo(wrap(s => s.length)), "void");
AssertType(foo, "(Mapper<string, number>) => void");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let a1 = ["a", "b"].map(s => s.length);
AssertType(a1, "number[]");
AssertType(["a", "b"].map(s => s.length), "number[]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let a2 = ["a", "b"].map(wrap(s => s.length));
AssertType(a2, "number[]");
AssertType(["a", "b"].map(wrap(s => s.length)), "number[]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let a3 = ["a", "b"].map(wrap(arrayize(s => s.length)));
AssertType(a3, "number[][]");
AssertType(["a", "b"].map(wrap(arrayize(s => s.length))), "number[][]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(wrap(arrayize(s => s.length)), "Mapper<string, number[]>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(arrayize(s => s.length), "Mapper<string, number[]>");
AssertType(arrayize, "<T, U>(Mapper<T, U>) => Mapper<T, U[]>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let a4 = ["a", "b"].map(combine(wrap(s => s.length), wrap(n => n > 10)));
AssertType(a4, "boolean[]");
AssertType(["a", "b"].map(combine(wrap(s => s.length), wrap(n => n > 10))), "boolean[]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(combine(wrap(s => s.length), wrap(n => n > 10)), "(string) => boolean");
AssertType(combine, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");
AssertType(wrap(n => n > 10), "Mapper<number, boolean>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(n => n > 10, "(number) => boolean");
AssertType(n, "number");
AssertType(n > 10, "boolean");
AssertType(n, "number");
AssertType(10, "int");

let a5 = ["a", "b"].map(combine(identity, wrap(s => s.length)));
AssertType(a5, "number[]");
AssertType(["a", "b"].map(combine(identity, wrap(s => s.length))), "number[]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(combine(identity, wrap(s => s.length)), "(string) => number");
AssertType(combine, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(identity, "<T>(T) => T");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let a6 = ["a", "b"].map(combine(wrap(s => s.length), identity));
AssertType(a6, "number[]");
AssertType(["a", "b"].map(combine(wrap(s => s.length), identity)), "number[]");
AssertType(["a", "b"].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(combine(wrap(s => s.length), identity), "(string) => number");
AssertType(combine, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(wrap(s => s.length), "Mapper<string, number>");
AssertType(wrap, "<T, U>(Mapper<T, U>) => Mapper<T, U>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");
AssertType(identity, "<T>(T) => T");

// This is a contrived class. We could do the same thing with Observables, etc.
class SetOf<A> {
  _store: A[];

  add(a: A) {
    this._store.push(a);
AssertType(this._store.push(a), "number");
AssertType(this._store.push, "(...A[]) => number");
AssertType(this._store, "A[]");
AssertType(this, "this");
AssertType(a, "A");
  }

  transform<B>(transformer: (a: SetOf<A>) => SetOf<B>): SetOf<B> {
AssertType(transformer(this), "SetOf<B>");
AssertType(transformer, "(SetOf<A>) => SetOf<B>");
AssertType(this, "this");
    return transformer(this);
  }

  forEach(fn: (a: A, index: number) => void) {
      this._store.forEach((a, i) => fn(a, i));
AssertType(this._store.forEach((a, i) => fn(a, i)), "void");
AssertType(this._store.forEach, "((A, number, A[]) => void, ?any) => void");
AssertType(this._store, "A[]");
AssertType(this, "this");
AssertType((a, i) => fn(a, i), "(A, number) => void");
AssertType(a, "A");
AssertType(i, "number");
AssertType(fn(a, i), "void");
AssertType(fn, "(A, number) => void");
AssertType(a, "A");
AssertType(i, "number");
  }
}

function compose<A, B, C, D, E>(
  fnA: (a: SetOf<A>) => SetOf<B>, 
  fnB: (b: SetOf<B>) => SetOf<C>, 
  fnC: (c: SetOf<C>) => SetOf<D>,
  fnD: (c: SetOf<D>) => SetOf<E>,
):(x: SetOf<A>) => SetOf<E>;
/* ... etc ... */
function compose<T>(...fns: ((x: T) => T)[]): (x: T) => T {
AssertType((x: T) => fns.reduce((prev, fn) => fn(prev), x), "(T) => T");
AssertType(x, "T");
AssertType(fns.reduce((prev, fn) => fn(prev), x), "T");
AssertType(fns.reduce, "{ (((T) => T, (T) => T, number, ((T) => T)[]) => (T) => T): (T) => T; (((T) => T, (T) => T, number, ((T) => T)[]) => (T) => T, (T) => T): (T) => T; <U>((U, (T) => T, number, ((T) => T)[]) => U, U): U; }");
AssertType((prev, fn) => fn(prev), "(T, (T) => T) => T");
AssertType(prev, "T");
AssertType(fn, "(T) => T");
AssertType(fn(prev), "T");
AssertType(fn, "(T) => T");
AssertType(prev, "T");
AssertType(x, "T");
  return (x: T) => fns.reduce((prev, fn) => fn(prev), x);
}

function map<A, B>(fn: (a: A) => B): (s: SetOf<A>) => SetOf<B> {
AssertType((a: SetOf<A>) => {    const b: SetOf<B> = new SetOf();    a.forEach(x => b.add(fn(x)));    return b;  }, "(SetOf<A>) => SetOf<B>");
AssertType(a, "SetOf<A>");
  return (a: SetOf<A>) => {

    const b: SetOf<B> = new SetOf();
AssertType(b, "SetOf<B>");
AssertType(new SetOf(), "SetOf<B>");
AssertType(SetOf, "typeof SetOf");

    a.forEach(x => b.add(fn(x)));
AssertType(a.forEach(x => b.add(fn(x))), "void");
AssertType(a.forEach, "((A, number) => void) => void");
AssertType(x => b.add(fn(x)), "(A) => void");
AssertType(x, "A");
AssertType(b.add(fn(x)), "void");
AssertType(b.add, "(B) => void");
AssertType(fn(x), "B");
AssertType(fn, "(A) => B");
AssertType(x, "A");

AssertType(b, "SetOf<B>");
    return b;
  }
}

function filter<A>(predicate: (a: A) => boolean): (s: SetOf<A>) => SetOf<A> {
AssertType((a: SetOf<A>) => {    const result = new SetOf<A>();    a.forEach(x => {      if (predicate(x)) result.add(x);    });   return result;  }, "(SetOf<A>) => SetOf<A>");
AssertType(a, "SetOf<A>");
  return (a: SetOf<A>) => {

    const result = new SetOf<A>();
AssertType(result, "SetOf<A>");
AssertType(new SetOf<A>(), "SetOf<A>");
AssertType(SetOf, "typeof SetOf");

    a.forEach(x => {
AssertType(a.forEach(x => {      if (predicate(x)) result.add(x);    }), "void");
AssertType(a.forEach, "((A, number) => void) => void");
AssertType(x => {      if (predicate(x)) result.add(x);    }, "(A) => void");
AssertType(x, "A");

      if (predicate(x)) result.add(x);
AssertType(predicate(x), "boolean");
AssertType(predicate, "(A) => boolean");
AssertType(x, "A");
AssertType(result.add(x), "void");
AssertType(result.add, "(A) => void");
AssertType(x, "A");

    });
AssertType(result, "SetOf<A>");
   return result;
  }
}

const testSet = new SetOf<number>();
AssertType(testSet, "SetOf<number>");
AssertType(new SetOf<number>(), "SetOf<number>");
AssertType(SetOf, "typeof SetOf");

testSet.add(1);
AssertType(testSet.add(1), "void");
AssertType(testSet.add, "(number) => void");
AssertType(1, "int");

testSet.add(2);
AssertType(testSet.add(2), "void");
AssertType(testSet.add, "(number) => void");
AssertType(2, "int");

testSet.add(3);
AssertType(testSet.add(3), "void");
AssertType(testSet.add, "(number) => void");
AssertType(3, "int");

const t1 = testSet.transform(
AssertType(t1, "SetOf<string>");
AssertType(testSet.transform(  compose(    filter(x => x % 1 === 0),    map(x => x + x),    map(x => x + '!!!'),    map(x => x.toUpperCase())  )), "SetOf<string>");
AssertType(testSet.transform, "<B>((SetOf<number>) => SetOf<B>) => SetOf<B>");

  compose(
AssertType(compose(    filter(x => x % 1 === 0),    map(x => x + x),    map(x => x + '!!!'),    map(x => x.toUpperCase())  ), "(SetOf<number>) => SetOf<string>");
AssertType(compose, "<A, B, C, D, E>((SetOf<A>) => SetOf<B>, (SetOf<B>) => SetOf<C>, (SetOf<C>) => SetOf<D>, (SetOf<D>) => SetOf<E>) => (SetOf<A>) => SetOf<E>");

    filter(x => x % 1 === 0),
AssertType(filter(x => x % 1 === 0), "(SetOf<number>) => SetOf<number>");
AssertType(filter, "<A>((A) => boolean) => (SetOf<A>) => SetOf<A>");
AssertType(x => x % 1 === 0, "(number) => boolean");
AssertType(x, "number");
AssertType(x % 1 === 0, "boolean");
AssertType(x % 1, "number");
AssertType(x, "number");
AssertType(1, "int");
AssertType(0, "int");

    map(x => x + x),
AssertType(map(x => x + x), "(SetOf<number>) => SetOf<number>");
AssertType(map, "<A, B>((A) => B) => (SetOf<A>) => SetOf<B>");
AssertType(x => x + x, "(number) => number");
AssertType(x, "number");
AssertType(x + x, "number");
AssertType(x, "number");
AssertType(x, "number");

    map(x => x + '!!!'),
AssertType(map(x => x + '!!!'), "(SetOf<number>) => SetOf<string>");
AssertType(map, "<A, B>((A) => B) => (SetOf<A>) => SetOf<B>");
AssertType(x => x + '!!!', "(number) => string");
AssertType(x, "number");
AssertType(x + '!!!', "string");
AssertType(x, "number");
AssertType('!!!', "string");

    map(x => x.toUpperCase())
AssertType(map(x => x.toUpperCase()), "(SetOf<string>) => SetOf<string>");
AssertType(map, "<A, B>((A) => B) => (SetOf<A>) => SetOf<B>");
AssertType(x => x.toUpperCase(), "(string) => string");
AssertType(x, "string");
AssertType(x.toUpperCase(), "string");
AssertType(x.toUpperCase, "() => string");

  )
)

declare function identity<T>(x: T): T;

const t2 = testSet.transform(
AssertType(t2, "SetOf<string>");
AssertType(testSet.transform(  compose(    filter(x => x % 1 === 0),    identity,    map(x => x + '!!!'),    map(x => x.toUpperCase())  )), "SetOf<string>");
AssertType(testSet.transform, "<B>((SetOf<number>) => SetOf<B>) => SetOf<B>");

  compose(
AssertType(compose(    filter(x => x % 1 === 0),    identity,    map(x => x + '!!!'),    map(x => x.toUpperCase())  ), "(SetOf<number>) => SetOf<string>");
AssertType(compose, "<A, B, C, D, E>((SetOf<A>) => SetOf<B>, (SetOf<B>) => SetOf<C>, (SetOf<C>) => SetOf<D>, (SetOf<D>) => SetOf<E>) => (SetOf<A>) => SetOf<E>");

    filter(x => x % 1 === 0),
AssertType(filter(x => x % 1 === 0), "(SetOf<number>) => SetOf<number>");
AssertType(filter, "<A>((A) => boolean) => (SetOf<A>) => SetOf<A>");
AssertType(x => x % 1 === 0, "(number) => boolean");
AssertType(x, "number");
AssertType(x % 1 === 0, "boolean");
AssertType(x % 1, "number");
AssertType(x, "number");
AssertType(1, "int");
AssertType(0, "int");

    identity,
AssertType(identity, "<T>(T) => T");

    map(x => x + '!!!'),
AssertType(map(x => x + '!!!'), "(SetOf<number>) => SetOf<string>");
AssertType(map, "<A, B>((A) => B) => (SetOf<A>) => SetOf<B>");
AssertType(x => x + '!!!', "(number) => string");
AssertType(x, "number");
AssertType(x + '!!!', "string");
AssertType(x, "number");
AssertType('!!!', "string");

    map(x => x.toUpperCase())
AssertType(map(x => x.toUpperCase()), "(SetOf<string>) => SetOf<string>");
AssertType(map, "<A, B>((A) => B) => (SetOf<A>) => SetOf<B>");
AssertType(x => x.toUpperCase(), "(string) => string");
AssertType(x, "string");
AssertType(x.toUpperCase(), "string");
AssertType(x.toUpperCase, "() => string");

  )
)


