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

// === tests/cases/compiler/staticAnonymousTypeNotReferencingTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
// This test case is a condensed version of Angular 2's ListWrapper. Prior to #7448
// this would cause the compiler to run out of memory.

function outer<T>(x: T) {
  class Inner {
    static y: T = x;
  }
AssertType(Inner, "typeof Inner");
  return Inner;
}
let y: number = outer(5).y;
AssertType(y, "number");
AssertType(outer(5).y, "number");

class ListWrapper2 {
  static clone<T>(dit: typeof ListWrapper2, array: T[]): T[] { 
AssertType(array.slice(0), "T[]");
return array.slice(0); 

AssertType(array.slice, "(?number, ?number) => T[]");

AssertType(0, "int");
}

  static reversed<T>(dit: typeof ListWrapper2, array: T[]): T[] {
    let a = ListWrapper2.clone(dit, array);
AssertType(a, "T[]");
AssertType(ListWrapper2.clone(dit, array), "T[]");
AssertType(ListWrapper2.clone, "<T>(typeof ListWrapper2, T[]) => T[]");
AssertType(dit, "typeof ListWrapper2");
AssertType(array, "T[]");

AssertType(a, "T[]");
    return a;
  }
}
namespace tessst {
    /**
     * Iterates through 'array' by index and performs the callback on each element of array until the callback
     * 
returns a truthy value, then returns that value.
     * If no such value is found, the callback is applied to each element of array and undefined is 
returned.
     */
    export function funkyFor<T, U>(array: T[], callback: (element: T, index: number) => U): U {
        if (array) {
AssertType(array, "T[]");

            for (let i = 0, len = array.length; i < len; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(len, "number");
AssertType(array.length, "number");
AssertType(i < len, "boolean");
AssertType(i, "number");
AssertType(len, "number");
AssertType(i++, "number");
AssertType(i, "number");

                const result = callback(array[i], i);
AssertType(result, "U");
AssertType(callback(array[i], i), "U");
AssertType(callback, "(T, number) => U");
AssertType(array[i], "T");
AssertType(array, "T[]");
AssertType(i, "number");
AssertType(i, "number");

                if (result) {
AssertType(result, "U");

AssertType(result, "U");
                    return result;
                }
            }
        }
AssertType(undefined, "undefined");
        return undefined;
    }
}
interface Scanner {
  scanRange<T>(start: number, length: number, callback: () => T): T;
}
class ListWrapper {
  // JS has no way to express a statically fixed size list, but dart does so we
  // keep both methods.
  static createFixedSize(dit: typeof ListWrapper, size: number): any[] { 
AssertType(new Array(size), "any[]");
return new Array(size); 

AssertType(Array, "ArrayConstructor");

AssertType(size, "number");
}

  static createGrowableSize(dit: typeof ListWrapper, size: number): any[] { 
AssertType(new Array(size), "any[]");
return new Array(size); 

AssertType(Array, "ArrayConstructor");

AssertType(size, "number");
}

  static clone<T>(dit: typeof ListWrapper, array: T[]): T[] { 
AssertType(array.slice(0), "T[]");
return array.slice(0); 

AssertType(array.slice, "(?number, ?number) => T[]");

AssertType(0, "int");
}

  static forEachWithIndex<T>(dit: typeof ListWrapper, array: T[], fn: (t: T, n: number) => void) {
    for (let i = 0; i < array.length; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < array.length, "boolean");
AssertType(i, "number");
AssertType(array.length, "number");
AssertType(i++, "number");
AssertType(i, "number");

      fn(array[i], i);
AssertType(fn(array[i], i), "void");
AssertType(fn, "(T, number) => void");
AssertType(array[i], "T");
AssertType(array, "T[]");
AssertType(i, "number");
AssertType(i, "number");
    }
  }
  static first<T>(dit: typeof ListWrapper, array: T[]): T {
    if (!array) 
AssertType(!array, "boolean");
AssertType(array, "T[]");
AssertType(null, "null");
return null;

AssertType(array[0], "T");
AssertType(array, "T[]");
AssertType(0, "int");
    return array[0];
  }
  static last<T>(dit: typeof ListWrapper, array: T[]): T {
    if (!array || array.length == 0) 
AssertType(!array || array.length == 0, "boolean");
AssertType(!array, "boolean");
AssertType(array, "T[]");
AssertType(array.length == 0, "boolean");
AssertType(array.length, "number");
AssertType(0, "int");
AssertType(null, "null");
return null;

AssertType(array[array.length - 1], "T");
AssertType(array, "T[]");
AssertType(array.length - 1, "number");
AssertType(array.length, "number");
AssertType(1, "int");
    return array[array.length - 1];
  }
  static indexOf<T>(dit: typeof ListWrapper, array: T[], value: T, startIndex: number = 0): number {
AssertType(array.indexOf(value, startIndex), "number");
AssertType(array.indexOf, "(T, ?number) => number");
AssertType(value, "T");
AssertType(startIndex, "number");
    return array.indexOf(value, startIndex);
  }
  static contains<T>(dit: typeof ListWrapper, list: T[], el: T): boolean { 
AssertType(list.indexOf(el) !== -1, "boolean");
return list.indexOf(el) !== -1; 

AssertType(list.indexOf(el), "number");

AssertType(list.indexOf, "(T, ?number) => number");

AssertType(el, "T");

AssertType(-1, "int");

AssertType(1, "int");
}

  static reversed<T>(dit: typeof ListWrapper, array: T[]): T[] {
    let a = ListWrapper.clone(dit, array);
AssertType(a, "T[]");
AssertType(ListWrapper.clone(dit, array), "T[]");
AssertType(ListWrapper.clone, "<T>(typeof ListWrapper, T[]) => T[]");
AssertType(dit, "typeof ListWrapper");
AssertType(array, "T[]");

    let scanner: Scanner;
AssertType(scanner, "Scanner");

    scanner.scanRange(3, 5, () => {  });
AssertType(scanner.scanRange(3, 5, () => {  }), "void");
AssertType(scanner.scanRange, "<T>(number, number, () => T) => T");
AssertType(3, "int");
AssertType(5, "int");
AssertType(() => {  }, "() => void");

AssertType(tessst.funkyFor(array, t => t.toString()) ? a.reverse() : a, "T[]");
AssertType(tessst.funkyFor(array, t => t.toString()), "string");
AssertType(tessst.funkyFor, "<T, U>(T[], (T, number) => U) => U");
AssertType(array, "T[]");
AssertType(t => t.toString(), "(T) => string");
AssertType(t, "T");
AssertType(t.toString(), "string");
AssertType(t.toString, "() => string");
AssertType(a.reverse(), "T[]");
AssertType(a.reverse, "() => T[]");
AssertType(a, "T[]");
    return tessst.funkyFor(array, t => t.toString()) ? a.reverse() : a;
  }
  static concat(dit: typeof ListWrapper, a: any[], b: any[]): any[] { 
AssertType(a.concat(b), "any[]");
return a.concat(b); 

AssertType(a.concat, "{ (...ConcatArray<any>[]): any[]; (...any[]): any[]; }");

AssertType(b, "any[]");
}

  static insert<T>(dit: typeof ListWrapper, list: T[], index: number, value: T) { list.splice(index, 0, value); 
AssertType(list.splice(index, 0, value), "T[]");

AssertType(list.splice, "{ (number, ?number): T[]; (number, number, ...T[]): T[]; }");

AssertType(index, "number");

AssertType(0, "int");

AssertType(value, "T");
}

  static removeAt<T>(dit: typeof ListWrapper, list: T[], index: number): T {
    let res = list[index];
AssertType(res, "T");
AssertType(list[index], "T");
AssertType(list, "T[]");
AssertType(index, "number");

    list.splice(index, 1);
AssertType(list.splice(index, 1), "T[]");
AssertType(list.splice, "{ (number, ?number): T[]; (number, number, ...T[]): T[]; }");
AssertType(index, "number");
AssertType(1, "int");

AssertType(res, "T");
    return res;
  }
  static removeAll<T>(dit: typeof ListWrapper, list: T[], items: T[]) {
    for (let i = 0; i < items.length; ++i) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < items.length, "boolean");
AssertType(i, "number");
AssertType(items.length, "number");
AssertType(++i, "number");
AssertType(i, "number");

      let index = list.indexOf(items[i]);
AssertType(index, "number");
AssertType(list.indexOf(items[i]), "number");
AssertType(list.indexOf, "(T, ?number) => number");
AssertType(items[i], "T");
AssertType(items, "T[]");
AssertType(i, "number");

      list.splice(index, 1);
AssertType(list.splice(index, 1), "T[]");
AssertType(list.splice, "{ (number, ?number): T[]; (number, number, ...T[]): T[]; }");
AssertType(index, "number");
AssertType(1, "int");
    }
  }
  static remove<T>(dit: typeof ListWrapper, list: T[], el: T): boolean {
    let index = list.indexOf(el);
AssertType(index, "number");
AssertType(list.indexOf(el), "number");
AssertType(list.indexOf, "(T, ?number) => number");
AssertType(el, "T");

    if (index > -1) {
AssertType(index > -1, "boolean");
AssertType(index, "number");
AssertType(-1, "int");
AssertType(1, "int");

      list.splice(index, 1);
AssertType(list.splice(index, 1), "T[]");
AssertType(list.splice, "{ (number, ?number): T[]; (number, number, ...T[]): T[]; }");
AssertType(index, "number");
AssertType(1, "int");

AssertType(true, "boolean");
      return true;
    }
AssertType(false, "boolean");
    return false;
  }
  static clear(dit: typeof ListWrapper, list: any[]) { list.length = 0; 
AssertType(list.length = 0, "int");

AssertType(list.length, "number");

AssertType(0, "int");
}

  static isEmpty(dit: typeof ListWrapper, list: any[]): boolean { 
AssertType(list.length == 0, "boolean");
return list.length == 0; 

AssertType(list.length, "number");

AssertType(0, "int");
}

  static fill(dit: typeof ListWrapper, list: any[], value: any, start: number = 0, end: number = null) {
    list.fill(value, start, end === null ? list.length : end);
AssertType(list.fill(value, start, end === null ? list.length : end), "void");
AssertType(list.fill, "(any, number, number) => void");
AssertType(value, "any");
AssertType(start, "number");
AssertType(end === null ? list.length : end, "number");
AssertType(end === null, "boolean");
AssertType(end, "number");
AssertType(null, "null");
AssertType(list.length, "number");
AssertType(end, "number");
  }
  static equals(dit: typeof ListWrapper, a: any[], b: any[]): boolean {
    if (a.length != b.length) 
AssertType(a.length != b.length, "boolean");
AssertType(a.length, "number");
AssertType(b.length, "number");
AssertType(false, "boolean");
return false;

    for (let i = 0; i < a.length; ++i) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < a.length, "boolean");
AssertType(i, "number");
AssertType(a.length, "number");
AssertType(++i, "number");
AssertType(i, "number");

      if (a[i] !== b[i]) 
AssertType(a[i] !== b[i], "boolean");
AssertType(a[i], "any");
AssertType(a, "any[]");
AssertType(i, "number");
AssertType(b[i], "any");
AssertType(b, "any[]");
AssertType(i, "number");
AssertType(false, "boolean");
return false;
    }
AssertType(true, "boolean");
    return true;
  }
  static slice<T>(dit: typeof ListWrapper, l: T[], from: number = 0, to: number = null): T[] {
AssertType(l.slice(from, to === null ? undefined : to), "T[]");
AssertType(l.slice, "(?number, ?number) => T[]");
AssertType(from, "number");
AssertType(to === null ? undefined : to, "number");
AssertType(to === null, "boolean");
AssertType(to, "number");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(to, "number");
    return l.slice(from, to === null ? undefined : to);
  }
  static splice<T>(dit: typeof ListWrapper, l: T[], from: number, length: number): T[] { 
AssertType(l.splice(from, length), "T[]");
return l.splice(from, length); 

AssertType(l.splice, "{ (number, ?number): T[]; (number, number, ...T[]): T[]; }");

AssertType(from, "number");

AssertType(length, "number");
}

  static sort<T>(dit: typeof ListWrapper, l: T[], compareFn?: (a: T, b: T) => number) {
    if (isPresent(compareFn)) {
AssertType(isPresent(compareFn), "boolean");
AssertType(isPresent, "<T>(?(T, T) => number) => boolean");
AssertType(compareFn, "(T, T) => number");

      l.sort(compareFn);
AssertType(l.sort(compareFn), "T[]");
AssertType(l.sort, "(?(T, T) => number) => T[]");
AssertType(compareFn, "(T, T) => number");

    } else {
      l.sort();
AssertType(l.sort(), "T[]");
AssertType(l.sort, "(?(T, T) => number) => T[]");
    }
  }
  static toString<T>(dit: typeof ListWrapper, l: T[]): string { 
AssertType(l.toString(), "string");
return l.toString(); 

AssertType(l.toString, "() => string");
}

  static toJSON<T>(dit: typeof ListWrapper, l: T[]): string { 
AssertType(JSON.stringify(l), "string");
return JSON.stringify(l); 

AssertType(JSON.stringify, "{ (any, ?(any, string, any) => any, ?union): string; (any, ?(union)[], ?union): string; }");

AssertType(l, "T[]");
}

  static maximum<T>(dit: typeof ListWrapper, list: T[], predicate: (t: T) => number): T {
    if (list.length == 0) {
AssertType(list.length == 0, "boolean");
AssertType(list.length, "number");
AssertType(0, "int");

AssertType(null, "null");
      return null;
    }
    let solution: T = null;
AssertType(solution, "T");
AssertType(null, "null");

    let maxValue = -Infinity;
AssertType(maxValue, "number");
AssertType(-Infinity, "number");
AssertType(Infinity, "number");

    for (let index = 0; index < list.length; index++) {
AssertType(index, "number");
AssertType(0, "int");
AssertType(index < list.length, "boolean");
AssertType(index, "number");
AssertType(list.length, "number");
AssertType(index++, "number");
AssertType(index, "number");

      let candidate = list[index];
AssertType(candidate, "T");
AssertType(list[index], "T");
AssertType(list, "T[]");
AssertType(index, "number");

      if (isBlank(candidate)) {
AssertType(isBlank(candidate), "boolean");
AssertType(isBlank, "(any) => boolean");
AssertType(candidate, "T");

        continue;
      }
      let candidateValue = predicate(candidate);
AssertType(candidateValue, "number");
AssertType(predicate(candidate), "number");
AssertType(predicate, "(T) => number");
AssertType(candidate, "T");

      if (candidateValue > maxValue) {
AssertType(candidateValue > maxValue, "boolean");
AssertType(candidateValue, "number");
AssertType(maxValue, "number");

        solution = candidate;
AssertType(solution = candidate, "T");
AssertType(solution, "T");
AssertType(candidate, "T");

        maxValue = candidateValue;
AssertType(maxValue = candidateValue, "number");
AssertType(maxValue, "number");
AssertType(candidateValue, "number");
      }
    }
AssertType(solution, "T");
    return solution;
  }
}
let cloned = ListWrapper.clone(ListWrapper, [1,2,3,4]);
AssertType(cloned, "number[]");
AssertType(ListWrapper.clone(ListWrapper, [1,2,3,4]), "number[]");
AssertType(ListWrapper.clone, "<T>(typeof ListWrapper, T[]) => T[]");
AssertType(ListWrapper, "typeof ListWrapper");
AssertType([1,2,3,4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

declare function isBlank(x: any): boolean;
declare function isPresent<T>(compareFn?: (a: T, b: T) => number): boolean;
interface Array<T> {
	fill(value: any, start: number, end: number): void;
}

