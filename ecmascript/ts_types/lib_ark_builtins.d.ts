/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 */

interface PropertyDescriptor {
    configurable?: boolean;
    enumerable?: boolean;
    value?: any;
    writable?: boolean;
    get?(): any;
    set?(v: any): void;
}

interface PropertyDescriptorMap {
    [s: string]: PropertyDescriptor;
}

interface IteratorYieldResult {
    done?: false;
    value: any;
}

interface IteratorReturnResult {
    done: true;
    value: any;
}

declare class Iterator {
    next(...args: [] | [any]): IteratorYieldResult | IteratorReturnResult;
    return?(value?: any): IteratorYieldResult | IteratorReturnResult;
    throw?(e?: any): IteratorYieldResult | IteratorReturnResult;
}

interface Iterable {
    [Symbol.iterator](): Iterator;
}

interface IterableIterator extends Iterator {
    [Symbol.iterator](): IterableIterator;
}

interface ArrayLike<T> {
    readonly length: number;
    readonly [n: number]: T;
}

interface ConcatArray<T> {
    readonly length: number;
    readonly [n: number]: T;
    join(separator?: string): string;
    slice(start?: number, end?: number): T[];
}

interface ArrayBufferTypes {
    ArrayBuffer: ArrayBuffer;
}

declare class RegExpMatchArray extends Array {
    index?: number;
    input?: string;
}

declare class RegExp {
    readonly flags: string;

    readonly sticky: boolean;

    readonly unicode: boolean;
}

interface ArrayLike {
    readonly length: number;
    readonly [n: number]: any;
}

declare class Object {
    constructor(arg?: any): Object;

    toString(): string;

    toLocaleString(): string;

    valueOf(): Object;

    hasOwnProperty(key: string | number | symbol): boolean;

    isPrototypeOf(o: Object): boolean;

    propertyIsEnumerable(key: string | number | symbol): boolean;

    static assign(target: object, ...sources: any[]): any;

    static create(o: Object | null, properties?: PropertyDescriptorMap): any;

    static defineProperties(o: any, properties: PropertyDescriptorMap & ThisType<any>): any;

    static defineProperty(o: any, p: PropertyKey, attributes: PropertyDescriptor & ThisType<any>): any;

    static freeze(f: any): any;

    static getOwnPropertyDescriptor(o: any, key: string | number | symbol): PropertyDescriptor | undefined;

    static getOwnPropertyNames(o: any): string[];

    static getOwnPropertySymbols(o: any): symbol[];

    static getPrototypeOf(o: any): Object | null;

    static is(value1: any, value2: any): boolean;

    static isExtensible(o: any): boolean;

    static isFrozen(o: any): boolean;

    static isSealed(o: any): boolean;

    static keys(o: object): string[];

    static values(o: {}): any[];

    static preventExtensions(o: any): any;

    static seal(o: any): any;

    static setPrototypeOf(o: any, proto: Object | null): any;

    static entries(o: {}): [string, any][];

    static fromEntries(entries: Iterable): any;
}

declare class Function extends Object {
    prototype: any;

    length: number;

    constructor(...args: string[]): Function;

    apply(this: Function, thisArg: any, argArray?: any): any;

    bind(this: Function, thisArg: any, ...args: any[]): any;

    call(this: Function, thisArg: any, ...args: any[]): any;

    // [Symbol.hasInstance](value: any): boolean;
}

declare class Error extends Object {
    name: string;

    message: string;

    stack?: string;

    constructor(message?: string): Error;
}

declare class RangeError extends Error {
    constructor(message?: string): RangeError;
}

declare class SyntaxError extends Error {
    constructor(message?: string): SyntaxError;
}

declare class TypeError extends Error {
    constructor(message?: string): TypeError;
}

declare class ReferenceError extends Error {
    constructor(message?: string): ReferenceError;
}

declare class URIError extends Error {
    constructor(message?: string): URIError;
}

declare class EvalError extends Error {
    constructor(message?: string): EvalError;
}

declare class Boolean extends Object {
    constructor(arg: number): Boolean;

    valueOf(): boolean;
}

declare class Date extends Object {
    constructor(): Date;

    constructor(value: number | string | Date): Date;

    constructor(year: number, month: number, date?: number, hours?: number, minutes?: number, seconds?: number, ms?: number): Date;

    getDate(): number;

    getDay(): number;

    getFullYear(): number;

    getHours(): number;

    getMilliseconds(): number;

    getMinutes(): number;

    getMonth(): number;

    getSeconds(): number;

    getTime(): number;

    getTimezoneOffset(): number;

    getUTCDate(): number;

    getDay(): number;

    getUTCFullYear(): number;

    getUTCHours(): number;

    getUTCMilliseconds(): number;

    getUTCMinutes(): number;

    getUTCMonth(): number;

    getUTCSeconds(): number;

    setDate(date: number): number;

    setFullYear(year: number, month?: number, date?: number): number;

    setHours(hours: number, min?: number, sec?: number, ms?: number): number;

    setMilliseconds(ms: number): number;

    setMinutes(min: number, sec?: number, ms?: number): number;

    setMonth(month: number, date?: number): number;

    setSeconds(sec: number, ms?: number): number;

    setTime(time: number): number;

    setUTCDate(date: number): number;

    setUTCFullYear(year: number, month?: number, date?: number): number;

    setUTCHours(hours: number, min?: number, sec?: number, ms?: number): number;

    setUTCMilliseconds(ms: number): number;

    setUTCMinutes(min: number, sec?: number, ms?: number): number;

    setUTCMonth(month: number, date?: number): number;

    setUTCSeconds(sec: number, ms?: number): number;

    toDateString(): string;

    toISOString(): string;

    toJSON(key?: any): string;

    toLocaleDateString(): string;

    toLocaleString(): string;

    toLocaleTimeString(): string;

    toString(): string;

    toTimeString(): string;

    toUTCString(): string;

    valueOf(): number;

    static parse(s: string): number;

    static UTC(year: number, month: number, date?: number, hours?: number, minutes?: number, seconds?: number, ms?: number): number;

    static now(): number;

    // 1. [Symbol.toPrimitive](hint: "default"): string;
    // 2. [Symbol.toPrimitive](hint: "string"): string;
    // 3. [Symbol.toPrimitive](hint: "number"): number;
    // 4. [Symbol.toPrimitive](hint: string): string | number;
}

declare class Math extends Object {
    static E: number;

    static LN10: number;

    static LN2: number;

    static LOG10E: number;

    static LOG2E: number;

    static PI: number;

    static SQRT1_2: number;

    static SQRT2: number;

    static abs(x: number): number;

    static acos(x: number): number;

    static acosh(x: number): number;

    static asin(x: number): number;

    static asinh(x: number): number;

    static atan(x: number): number;

    static atanh(x: number): number;

    static atan2(y: number, x: number): number;

    static cbrt(x: number): number;

    static ceil(x: number): number;

    static clz32(x: number): number;

    static cos(x: number): number;

    static cosh(x: number): number;

    static exp(x: number): number;

    static expm1(x: number): number;

    static floor(x: number): number;

    static fround(x: number): number;

    static hypot(...values: number[]): number;

    static imul(x: number, y: number): number;

    static log(x: number): number;

    static log1p(x: number): number;

    static log10(x: number): number;

    static log2(x: number): number;

    static max(...values: number[]): number;

    static min(...values: number[]): number;

    static pow(x: number, y: number): number;

    static random(): number;

    static round(x: number): number;

    static sign(x: number): number;

    static sin(x: number): number;

    static sinh(x: number): number;

    static sqrt(x: number): number;

    static tan(x: number): number;

    static tanh(x: number): number;

    static trunc(x: number): number;
}

declare class JSON extends Object {
    static parse(text: string, reviver?: (this: any, key: string, value: any) => any): any;

    static stringify(value: any, replacer?: (this: any, key: string, value: any) => any, space?: string | number): string;

    static stringify(value: any, replacer?: (number | string)[] | null, space?: string | number): string;
}

declare class Number extends Object {
    constructor(arg?: any): Number;

    toExponential(fractionDigits?: number): string;

    toFixed(fractionDigits?: number): string;

    toLocaleString(locales?: string | string[], options?: Intl.NumberFormatOptions): string;

    toPrecision(precision?: number): string;

    toString(radix?: number): string;

    valueOf(): number;

    static isFinite(num: any): boolean;

    static isInteger(num: any): boolean;

    static isNaN(num: any): boolean;

    static isSafeInteger(num: any): boolean;

    static parseFloat(str: string): number;

    static parseInt(str: string, radix?: number): number;

    static MAX_VALUE: number;

    static MIN_VALUE: number;

    static NaN: number;

    static NEGATIVE_INFINITY: number;

    static POSITIVE_INFINITY: number;

    static MAX_SAFE_INTEGER: number;

    static MIN_SAFE_INTEGER: number;

    static EPSILON: number;
}

declare class Set extends Object {
    size: number;

    constructor(values?: any[] | null): Set;

    constructor(iterable?: Iterable | null): Set;

    add(value: any): Set;

    clear(): void;

    delete(value: any): boolean;

    entries(): IterableIterator;

    forEach(callbackfn: (value: any, value2: any, set: Set) => void, thisArg?: any): void;

    has(value: any): boolean;

    values(): IterableIterator;

    // 1. [Symbol.species]: SetConstructor;
    // 2. [Symbol.iterator](): IterableIterator<T>;
    // 3. [Symbol.toStringTag]: string;
}

declare class WeakSet extends Object {
    constructor(values?: readonly any[] | null): WeakSet;

    constructor(iterable: Iterable): WeakSet;

    add(value: any): WeakSet;

    delete(value: any): boolean;

    has(value: any): boolean;

    // 1. [Symbol.toStringTag]: string;
}

declare class Array extends Object {
    length: number;

    constructor(arrayLength?: number): any[];

    constructor(...items: any[]): any[];

    concat(...items: ConcatArray[]): any[];

    copyWithin(target: number, start: number, end?: number): Array;

    entries(): IterableIterator;

    every(predicate: (value: any, index: number, array: any[]) => unknown, thisArg?: any): boolean;

    fill(value: any, start?: number, end?: number): Array;

    filter(predicate: (value: any, index: number, array: any[]) => unknown, thisArg?: any): any[];

    find(predicate: (value: any, index: number, obj: any[]) => unknown, thisArg?: any): any | undefined;

    findIndex(predicate: (value: any, index: number, obj: any[]) => unknown, thisArg?: any): number;

    forEach(callbackfn: (value: any, index: number, array: any[]) => void, thisArg?: any): void;

    indexOf(searchElement: any, fromIndex?: number): number;

    join(separator?: string): string;

    keys(): IterableIterator;

    lastIndexOf(searchElement: any, fromIndex?: number): number;

    map(callbackfn: (value: any, index: number, array: any[]) => any, thisArg?: any): any[];

    pop(): any | undefined;

    push(...items: any[]): number;

    reduce(callbackfn: (previousValue: any, currentValue: any, currentIndex: number, array: any[]) => any, initialValue?: any): any;

    reduceRight(callbackfn: (previousValue: any, currentValue: any, currentIndex: number, array: any[]) => any, initialValue?: any): any;

    reverse(): any[];

    shift(): any | undefined;

    slice(start?: number, end?: number): any[];

    some(predicate: (value: any, index: number, array: any[]) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: any, b: any) => number): Array;

    splice(start: number, deleteCount: number, ...items: any[]): any[];

    toLocaleString(): string;

    toString(): string;

    unshift(...items: any[]): number;

    values(): IterableIterator;

    includes(searchElement: any, fromIndex?: number): boolean;

    flat(
        this: any,
        depth?: any
    ): any[];

    flatMap(
        callback: (this: any, value: any, index: number, array: any[]) => any,
        thisArg?: any
    ): any[];

    static from<T, U>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => U, thisArg?: any): U[];

    static from<T, U>(iterable: Iterable<T> | ArrayLike<T>, mapfn: (v: T, k: number) => U, thisArg?: any): U[];

    static isArray(arg: any): arg is any[];

    static of<T>(...items: T[]): T[];

    // 1. [Symbol.species]: ArrayConstructor;
    // 2. [n: number]: T;
    // 3. [Symbol.unscopables]()
}

declare class ArrayBuffer extends Object {
    byteLength: number;

    constructor(byteLength: number): ArrayBuffer;

    slice(begin: number, end?: number): ArrayBuffer;

    static isView(arg: any): boolean;

    // fixme: Front-end not supported
    // 1. [Symbol.species]: ArrayBufferConstructor;
}


declare class SharedArrayBuffer extends Object {
    constructor(byteLength: number): SharedArrayBuffer;

    byteLength: number;

    slice(begin: number, end?: number): SharedArrayBuffer;

    // 1. [Symbol.species]: SharedArrayBuffer;
    // 2. [Symbol.toStringTag]: "SharedArrayBuffer";
}

declare class String extends Object {
    length: number;

    constructor(value?: any): String;

    at(index: number): string;

    charAt(pos: number): string;

    charCodeAt(index: number): number;

    concat(...strings: string[]): string;

    endsWith(searchString: string, endPosition?: number): boolean;

    includes(searchString: string, position?: number): boolean;

    indexOf(searchString: string, position?: number): number;

    lastIndexOf(searchString: string, position?: number): number;

    localeCompare(that: string, locale?: string, options?: any): number;

    match(regexp: string | RegExp): RegExpMatchArray | null;

    matchAll(regexp: RegExp): IterableIterator;

    normalize(form?: string): string;

    padStart(maxLength: number, fillString?: string): string;

    padEnd(maxLength: number, fillString?: string): string;

    repeat(count: number): string;

    replace(searchValue: string | RegExp, replaceValue: string): string;

    replaceAll(searchValue: string | RegExp, replaceValue: string): string;

    search(regexp: string | RegExp): number;

    slice(start?: number, end?: number): string;

    split(separator: string | RegExp, limit?: number): string[];

    startsWith(searchString: string, position?: number): boolean;

    substring(start: number, end?: number): string;

    toLocaleLowerCase(locales?: string | string[]): string;

    toLocaleUpperCase(locales?: string | string[]): string;

    toLowerCase(): string;

    toString(): string;

    toUpperCase(): string;

    trim(): string;

    trimStart(): string;

    trimEnd(): string;

    trimLeft(): string;

    trimRight(): string;

    valueOf(): string;

    substr(from: number, length?: number): string;

    static fromCharCode(...codes: number[]): string;

    static fromCodePoint(...codePoints: number[]): string;

    static raw(template: { raw: readonly string[] | ArrayLike<string> }, ...substitutions: any[]): string;

    // 1. [index: number]: string;
}

declare class Symbol extends Object {
    description: string | undefined;

    static for(key: string): symbol;

    static keyFor(sym: symbol): string | undefined;

    toString(): string;

    valueOf(): symbol;

    static iterator: symbol;

    static prototype: Symbol;

    static hasInstance: symbol;

    static isConcatSpreadable: symbol;

    static match: symbol;

    static replace: symbol;

    static search: symbol;

    static species: symbol;

    static split: symbol;

    static toPrimitive: symbol;

    static toStringTag: symbol;

    static unscopables: symbol;

    static asyncIterator: symbol;

    static matchAll: symbol;

    // 1. [Symbol.toPrimitive](hint: string): symbol;
    // 2. [Symbol.toStringTag]: string;
}


declare class WeakRef extends Object {
    constructor(target: any): WeakRef;

    deref(): any | undefined;

    // 1. [Symbol.toStringTag]: "WeakRef";
}

declare class Uint8ClampedArray extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Uint8ClampedArray;

    constructor(length: number): Uint8ClampedArray;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Uint8ClampedArray;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Uint8ClampedArray;

    constructor(elements: Iterable<number>): Uint8ClampedArray;

    constructor(): Uint8ClampedArray;

    static of(...items: number[]): Uint8ClampedArray;

    static from(arrayLike: ArrayLike<number>): Uint8ClampedArray;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Uint8ClampedArray;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Uint8ClampedArray;

    copyWithin(target: number, start: number, end?: number): Uint8ClampedArray;

    every(predicate: (value: number, index: number, array: Uint8ClampedArray) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Uint8ClampedArray;

    filter(predicate: (value: number, index: number, array: Uint8ClampedArray) => any, thisArg?: any): Uint8ClampedArray;

    find(predicate: (value: number, index: number, obj: Uint8ClampedArray) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Uint8ClampedArray) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Uint8ClampedArray) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Uint8ClampedArray) => number, thisArg?: any): Uint8ClampedArray;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint8ClampedArray) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint8ClampedArray) => U, initialValue?: U): U;

    reverse(): Uint8ClampedArray;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Uint8ClampedArray;

    some(predicate: (value: number, index: number, array: Uint8ClampedArray) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Uint8ClampedArray;

    subarray(begin?: number, end?: number): Uint8ClampedArray;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Uint8ClampedArray;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Uint8ClampedArray";
}

declare class Uint8Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Uint8Array;

    constructor(length: number): Uint8Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Uint8Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Uint8Array;

    constructor(elements: Iterable<number>): Uint8Array;

    constructor(): Uint8Array;

    static of(...items: number[]): Uint8Array;

    static from(arrayLike: ArrayLike<number>): Uint8Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Uint8Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Uint8Array;

    copyWithin(target: number, start: number, end?: number): Uint8Array;

    every(predicate: (value: number, index: number, array: Uint8Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Uint8Array;

    filter(predicate: (value: number, index: number, array: Uint8Array) => any, thisArg?: any): Uint8Array;

    find(predicate: (value: number, index: number, obj: Uint8Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Uint8Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Uint8Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Uint8Array) => number, thisArg?: any): Uint8Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint8Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint8Array) => U, initialValue?: U): U;

    reverse(): Uint8Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Uint8Array;

    some(predicate: (value: number, index: number, array: Uint8Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Uint8Array;

    subarray(begin?: number, end?: number): Uint8Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Uint8Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Uint8Array";
}

declare class Int8Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Int8Array;

    constructor(length: number): Int8Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Int8Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Int8Array;

    constructor(elements: Iterable<number>): Int8Array;

    constructor(): Int8Array;

    static of(...items: number[]): Int8Array;

    static from(arrayLike: ArrayLike<number>): Int8Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Int8Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Int8Array;

    copyWithin(target: number, start: number, end?: number): Int8Array;

    every(predicate: (value: number, index: number, array: Int8Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Int8Array;

    filter(predicate: (value: number, index: number, array: Int8Array) => any, thisArg?: any): Int8Array;

    find(predicate: (value: number, index: number, obj: Int8Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Int8Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Int8Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Int8Array) => number, thisArg?: any): Int8Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int8Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int8Array) => U, initialValue?: U): U;

    reverse(): Int8Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Int8Array;

    some(predicate: (value: number, index: number, array: Int8Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Int8Array;

    subarray(begin?: number, end?: number): Int8Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Int8Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Int8Array";
}

declare class Uint16Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Uint16Array;

    constructor(length: number): Uint16Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Uint16Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Uint16Array;

    constructor(elements: Iterable<number>): Uint16Array;

    constructor(): Uint16Array;

    static of(...items: number[]): Uint16Array;

    static from(arrayLike: ArrayLike<number>): Uint16Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Uint16Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Uint16Array;

    copyWithin(target: number, start: number, end?: number): Uint16Array;

    every(predicate: (value: number, index: number, array: Uint16Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Uint16Array;

    filter(predicate: (value: number, index: number, array: Uint16Array) => any, thisArg?: any): Uint16Array;

    find(predicate: (value: number, index: number, obj: Uint16Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Uint16Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Uint16Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Uint16Array) => number, thisArg?: any): Uint16Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint16Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint16Array) => U, initialValue?: U): U;

    reverse(): Uint16Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Uint16Array;

    some(predicate: (value: number, index: number, array: Uint16Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Uint16Array;

    subarray(begin?: number, end?: number): Uint16Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Uint16Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Uint16Array";
}

declare class Uint32Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Uint32Array;

    constructor(length: number): Uint32Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Uint32Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Uint32Array;

    constructor(elements: Iterable<number>): Uint32Array;

    constructor(): Uint32Array;

    static of(...items: number[]): Uint32Array;

    static from(arrayLike: ArrayLike<number>): Uint32Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Uint32Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Uint32Array;

    copyWithin(target: number, start: number, end?: number): Uint32Array;

    every(predicate: (value: number, index: number, array: Uint32Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Uint32Array;

    filter(predicate: (value: number, index: number, array: Uint32Array) => any, thisArg?: any): Uint32Array;

    find(predicate: (value: number, index: number, obj: Uint32Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Uint32Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Uint32Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Uint32Array) => number, thisArg?: any): Uint32Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint32Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Uint32Array) => U, initialValue?: U): U;

    reverse(): Uint32Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Uint32Array;

    some(predicate: (value: number, index: number, array: Uint32Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Uint32Array;

    subarray(begin?: number, end?: number): Uint32Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Uint32Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Uint32Array";
}

declare class Int16Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Int16Array;

    constructor(length: number): Int16Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Int16Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Int16Array;

    constructor(elements: Iterable<number>): Int16Array;

    constructor(): Int16Array;

    static of(...items: number[]): Int16Array;

    static from(arrayLike: ArrayLike<number>): Int16Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Int16Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Int16Array;

    copyWithin(target: number, start: number, end?: number): Int16Array;

    every(predicate: (value: number, index: number, array: Int16Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Int16Array;

    filter(predicate: (value: number, index: number, array: Int16Array) => any, thisArg?: any): Int16Array;

    find(predicate: (value: number, index: number, obj: Int16Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Int16Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Int16Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Int16Array) => number, thisArg?: any): Int16Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int16Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int16Array) => U, initialValue?: U): U;

    reverse(): Int16Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Int16Array;

    some(predicate: (value: number, index: number, array: Int16Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Int16Array;

    subarray(begin?: number, end?: number): Int16Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Int16Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Int16Array";
}

declare class Int32Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Int32Array;

    constructor(length: number): Int32Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Int32Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Int32Array;

    constructor(elements: Iterable<number>): Int32Array;

    constructor(): Int32Array;

    static of(...items: number[]): Int32Array;

    static from(arrayLike: ArrayLike<number>): Int32Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Int32Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Int32Array;

    copyWithin(target: number, start: number, end?: number): Int32Array;

    every(predicate: (value: number, index: number, array: Int32Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Int32Array;

    filter(predicate: (value: number, index: number, array: Int32Array) => any, thisArg?: any): Int32Array;

    find(predicate: (value: number, index: number, obj: Int32Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Int32Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Int32Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Int32Array) => number, thisArg?: any): Int32Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int32Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Int32Array) => U, initialValue?: U): U;

    reverse(): Int32Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Int32Array;

    some(predicate: (value: number, index: number, array: Int32Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Int32Array;

    subarray(begin?: number, end?: number): Int32Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Int32Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Int32Array";
}

declare class Float32Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Float32Array;

    constructor(length: number): Float32Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Float32Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Float32Array;

    constructor(elements: Iterable<number>): Float32Array;

    constructor(): Float32Array;

    static of(...items: number[]): Float32Array;

    static from(arrayLike: ArrayLike<number>): Float32Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Float32Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Float32Array;

    copyWithin(target: number, start: number, end?: number): Float32Array;

    every(predicate: (value: number, index: number, array: Float32Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Float32Array;

    filter(predicate: (value: number, index: number, array: Float32Array) => any, thisArg?: any): Float32Array;

    find(predicate: (value: number, index: number, obj: Float32Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Float32Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Float32Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Float32Array) => number, thisArg?: any): Float32Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Float32Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Float32Array) => U, initialValue?: U): U;

    reverse(): Float32Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Float32Array;

    some(predicate: (value: number, index: number, array: Float32Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Float32Array;

    subarray(begin?: number, end?: number): Float32Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Float32Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Float32Array";
}

declare class Float64Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: Float64Array;

    constructor(length: number): Float64Array;

    constructor(array: ArrayLike<number> | ArrayBufferLike): Float64Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): Float64Array;

    constructor(elements: Iterable<number>): Float64Array;

    constructor(): Float64Array;

    static of(...items: number[]): Float64Array;

    static from(arrayLike: ArrayLike<number>): Float64Array;

    static from<T>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => number, thisArg?: any): Float64Array;

    static from(arrayLike: Iterable<number>, mapfn?: (v: number, k: number) => number, thisArg?: any): Float64Array;

    copyWithin(target: number, start: number, end?: number): Float64Array;

    every(predicate: (value: number, index: number, array: Float64Array) => unknown, thisArg?: any): boolean;

    fill(value: number, start?: number, end?: number): Float64Array;

    filter(predicate: (value: number, index: number, array: Float64Array) => any, thisArg?: any): Float64Array;

    find(predicate: (value: number, index: number, obj: Float64Array) => boolean, thisArg?: any): number | undefined;

    findIndex(predicate: (value: number, index: number, obj: Float64Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: number, index: number, array: Float64Array) => void, thisArg?: any): void;

    indexOf(searchElement: number, fromIndex?: number): number;

    join(separator?: string): string;

    lastIndexOf(searchElement: number, fromIndex?: number): number;

    map(callbackfn: (value: number, index: number, array: Float64Array) => number, thisArg?: any): Float64Array;

    reduce<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Float64Array) => U, initialValue?: U): U;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: number, currentIndex: number, array: Float64Array) => U, initialValue?: U): U;

    reverse(): Float64Array;

    set(array: ArrayLike<number>, offset?: number): void;

    slice(start?: number, end?: number): Float64Array;

    some(predicate: (value: number, index: number, array: Float64Array) => unknown, thisArg?: any): boolean;

    sort(compareFn?: (a: number, b: number) => number): Float64Array;

    subarray(begin?: number, end?: number): Float64Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): Float64Array;

    entries(): IterableIterator<[number, number]>;

    keys(): IterableIterator<number>;

    values(): IterableIterator<number>;

    includes(searchElement: number, fromIndex?: number): boolean;

    //1.[index: number]: number;
    //2.[Symbol.iterator](): IterableIterator<number>;
    //3.[Symbol.toStringTag]: "Float64Array";
}

declare class BigInt64Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: BigInt64Array;

    constructor(length?: number): BigInt64Array;

    constructor(array: Iterable<bigint>): BigInt64Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): BigInt64Array;

    static of(...items: bigint[]): BigInt64Array;

    static from(arrayLike: ArrayLike<bigint>): BigInt64Array;

    static from<U>(arrayLike: ArrayLike<U>, mapfn: (v: U, k: number) => bigint, thisArg?: any): BigInt64Array;

    copyWithin(target: number, start: number, end?: number): BigInt64Array;

    entries(): IterableIterator<[number, bigint]>;

    every(predicate: (value: bigint, index: number, array: BigInt64Array) => boolean, thisArg?: any): boolean;

    fill(value: bigint, start?: number, end?: number): BigInt64Array;

    filter(predicate: (value: bigint, index: number, array: BigInt64Array) => any, thisArg?: any): BigInt64Array;

    find(predicate: (value: bigint, index: number, array: BigInt64Array) => boolean, thisArg?: any): bigint | undefined;

    findIndex(predicate: (value: bigint, index: number, array: BigInt64Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: bigint, index: number, array: BigInt64Array) => void, thisArg?: any): void;

    includes(searchElement: bigint, fromIndex?: number): boolean;

    indexOf(searchElement: bigint, fromIndex?: number): number;

    join(separator?: string): string;

    keys(): IterableIterator<number>;

    lastIndexOf(searchElement: bigint, fromIndex?: number): number;

    map(callbackfn: (value: bigint, index: number, array: BigInt64Array) => bigint, thisArg?: any): BigInt64Array;

    reduce(callbackfn: (previousValue: bigint, currentValue: bigint, currentIndex: number, array: BigInt64Array) => bigint): bigint;

    reduce<U>(callbackfn: (previousValue: U, currentValue: bigint, currentIndex: number, array: BigInt64Array) => U, initialValue: U): U;

    reduceRight(callbackfn: (previousValue: bigint, currentValue: bigint, currentIndex: number, array: BigInt64Array) => bigint): bigint;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: bigint, currentIndex: number, array: BigInt64Array) => U, initialValue: U): U;

    reverse(): BigInt64Array;

    set(array: ArrayLike<bigint>, offset?: number): void;

    slice(start?: number, end?: number): BigInt64Array;

    some(predicate: (value: bigint, index: number, array: BigInt64Array) => boolean, thisArg?: any): boolean;

    sort(compareFn?: (a: bigint, b: bigint) => number | bigint): BigInt64Array;

    subarray(begin?: number, end?: number): BigInt64Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): BigInt64Array;

    values(): IterableIterator<bigint>;

    //1.[Symbol.iterator](): IterableIterator<bigint>;
    //2.[Symbol.toStringTag]: "BigInt64Array";
    //3.[index: number]: bigint;
}

declare class BigUint64Array extends Object {
    BYTES_PER_ELEMENT: number;

    buffer: ArrayBufferLike;

    byteLength: number;

    byteOffset: number;

    length: number;

    prototype: BigUint64Array;

    constructor(length?: number): BigUint64Array;

    constructor(array: Iterable<bigint>): BigUint64Array;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, length?: number): BigUint64Array;

    static of(...items: bigint[]): BigUint64Array;

    static from(arrayLike: ArrayLike<bigint>): BigUint64Array;

    static from<U>(arrayLike: ArrayLike<U>, mapfn: (v: U, k: number) => bigint, thisArg?: any): BigUint64Array;

    copyWithin(target: number, start: number, end?: number): BigUint64Array;

    entries(): IterableIterator<[number, bigint]>;

    every(predicate: (value: bigint, index: number, array: BigUint64Array) => boolean, thisArg?: any): boolean;

    fill(value: bigint, start?: number, end?: number): BigUint64Array;

    filter(predicate: (value: bigint, index: number, array: BigUint64Array) => any, thisArg?: any): BigUint64Array;

    find(predicate: (value: bigint, index: number, array: BigUint64Array) => boolean, thisArg?: any): bigint | undefined;

    findIndex(predicate: (value: bigint, index: number, array: BigUint64Array) => boolean, thisArg?: any): number;

    forEach(callbackfn: (value: bigint, index: number, array: BigUint64Array) => void, thisArg?: any): void;

    includes(searchElement: bigint, fromIndex?: number): boolean;

    indexOf(searchElement: bigint, fromIndex?: number): number;

    join(separator?: string): string;

    keys(): IterableIterator<number>;

    lastIndexOf(searchElement: bigint, fromIndex?: number): number;

    map(callbackfn: (value: bigint, index: number, array: BigUint64Array) => bigint, thisArg?: any): BigUint64Array;

    reduce(callbackfn: (previousValue: bigint, currentValue: bigint, currentIndex: number, array: BigUint64Array) => bigint): bigint;

    reduce<U>(callbackfn: (previousValue: U, currentValue: bigint, currentIndex: number, array: BigUint64Array) => U, initialValue: U): U;

    reduceRight(callbackfn: (previousValue: bigint, currentValue: bigint, currentIndex: number, array: BigUint64Array) => bigint): bigint;

    reduceRight<U>(callbackfn: (previousValue: U, currentValue: bigint, currentIndex: number, array: BigUint64Array) => U, initialValue: U): U;

    reverse(): BigUint64Array;

    set(array: ArrayLike<bigint>, offset?: number): void;

    slice(start?: number, end?: number): BigUint64Array;

    some(predicate: (value: bigint, index: number, array: BigUint64Array) => boolean, thisArg?: any): boolean;

    sort(compareFn?: (a: bigint, b: bigint) => number | bigint): BigUint64Array;

    subarray(begin?: number, end?: number): BigUint64Array;

    toLocaleString(): string;

    toString(): string;

    valueOf(): BigUint64Array;

    values(): IterableIterator<bigint>;

    //1.[Symbol.iterator](): IterableIterator<bigint>;
    //2.[Symbol.toStringTag]: "BigUint64Array";
    //3.[index: number]: bigint;
}

declare class DataView extends Object {
    buffer: ArrayBuffer;

    byteLength: number;

    byteOffset: number;

    prototype: DataView;

    constructor(buffer: ArrayBufferLike, byteOffset?: number, byteLength?: number): DataView;

    constructor(buffer: ArrayBuffer, byteOffset?: number, byteLength?: number): DataView;

    getFloat32(byteOffset: number, littleEndian?: boolean): number;

    getFloat64(byteOffset: number, littleEndian?: boolean): number;

    getInt8(byteOffset: number): number;

    getInt16(byteOffset: number, littleEndian?: boolean): number;

    getInt32(byteOffset: number, littleEndian?: boolean): number;

    getUint8(byteOffset: number): number;

    getUint16(byteOffset: number, littleEndian?: boolean): number;

    getUint32(byteOffset: number, littleEndian?: boolean): number;

    getBigInt64(byteOffset: number, littleEndian?: boolean): bigint;

    getBigUint64(byteOffset: number, littleEndian?: boolean): bigint;

    setFloat32(byteOffset: number, value: number, littleEndian?: boolean): void;

    setFloat64(byteOffset: number, value: number, littleEndian?: boolean): void;

    setInt8(byteOffset: number, value: number): void;

    setInt16(byteOffset: number, value: number, littleEndian?: boolean): void;

    setInt32(byteOffset: number, value: number, littleEndian?: boolean): void;

    setUint8(byteOffset: number, value: number): void;

    setUint16(byteOffset: number, value: number, littleEndian?: boolean): void;

    setUint32(byteOffset: number, value: number, littleEndian?: boolean): void;

    setBigInt64(byteOffset: number, value: bigint, littleEndian?: boolean): void;

    setBigUint64(byteOffset: number, value: bigint, littleEndian?: boolean): void;

    //1.[Symbol.toStringTag]: string;
}

declare class Atomics extends Object {
    add(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    and(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    compareExchange(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array,
        index: number, expectedValue: number, replacementValue: number): number;

    exchange(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    isLockFree(size: number): boolean;

    load(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array,
        index: number): number;

    or(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    store(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    sub(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    wait(typedArray: Int32Array, index: number, value: number, timeout?: number): 'ok' | 'not-equal' | 'timed-out';

    notify(typedArray: Int32Array, index: number, count?: number): number;

    xor(typedArray: Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array, index: number,
        value: number): number;

    //1.[Symbol.toStringTag]: "Atomics";
}

declare class Map extends Object {
    prototype: Map;

    size: number;

    constructor(): Map;

    constructor(entries?: readonly (readonly [any, any])[] | null): Map;

    constructor(iterable: Iterable<readonly [any, any]>): Map;

    clear(): void;

    delete(key: any): boolean;

    forEach(callbackfn: (value: any, key: any, map: Map<any, any>) => void, thisArg?: any): void;

    get(key: any): any;

    has(key: any): boolean;

    set(key: any, value: any): Map;

    entries(): IterableIterator<[any, any]>;

    keys(): IterableIterator<any>;

    values(): IterableIterator<any>;

    //1.[Symbol.iterator](): IterableIterator<[K, V]>;
    //2.[Symbol.species]: MapConstructor;
    //3.[Symbol.toStringTag]: string;
    //4.[index: string]: T;
}

declare class Promise extends Object {

    constructor(executor: (resolve: (value: any) => void, reject: (reason?: any) => void) => void): Promise;

    static all(values: Iterable<any>): Promise;

    static race(values: Iterable<any>): Promise;

    static reject(reason?: any): Promise;

    static resolve(): Promise;

    static resolve(value: any): Promise;

    then(onfulfilled?: ((value: any) => any) | undefined | null, onrejected?: ((reason: any) => any) | undefined | null): Promise;

    catch(onrejected?: ((reason: any) => any) | undefined | null): Promise;

    inally(onfinally?: (() => void) | undefined | null): Promise;
}

declare function parseFloat(string: string): number;

declare function parseInt(string: string, radix?: number): number;

declare function eval(x: string): any;

declare function isFinite(number: number): boolean;

declare function print(arg: any, arg1?: any): string;

declare function decodeURI(encodedURI: string): string;

declare function decodeURIComponent(encodedURIComponent: string): string;

declare function isNaN(number: number): boolean;

declare function encodeURI(uri: string): string;

declare function encodeURIComponent(uriComponent: string | number | boolean): string;

// globalThis, ArkPrivate
// namespace, undefined
// Intl, Reflect, TypedArray, Proxy
