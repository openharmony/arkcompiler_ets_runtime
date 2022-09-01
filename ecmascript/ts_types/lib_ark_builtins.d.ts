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

interface Iterator {
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

interface RegExp {
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

    // fixme: Front-end not supported
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

    // fixme: Front-end not supported
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

    add(value: any): this;

    clear(): void;

    delete(value: any): boolean;

    entries(): IterableIterator;

    forEach(callbackfn: (value: any, value2: any, set: Set) => void, thisArg?: any): void;

    has(value: any): boolean;

    values(): IterableIterator;

    // fixme: Front-end not supported
    // 1. [Symbol.species]: SetConstructor;
    // 2. [Symbol.iterator](): IterableIterator<T>;
    // 3. [Symbol.toStringTag]: string;
}

declare class WeakSet extends object {
    constructor(values?: readonly any[] | null): WeakSet;

    constructor(iterable: Iterable): WeakSet;

    add(value: any): this;

    delete(value: any): boolean;

    has(value: any): boolean;

    // fixme: Front-end not supported
    // 1. [Symbol.toStringTag]: string;
}

declare class Array extends Object {
    length: number;

    constructor(arrayLength?: number): any[];

    constructor(...items: any[]): any[];

    concat(...items: ConcatArray[]): any[];

    copyWithin(target: number, start: number, end?: number): this;

    entries(): IterableIterator;

    every(predicate: (value: any, index: number, array: any[]) => unknown, thisArg?: any): boolean;

    fill(value: any, start?: number, end?: number): this;

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

    sort(compareFn?: (a: any, b: any) => number): this;

    splice(start: number, deleteCount: number, ...items: any[]): any[];

    toLocaleString(): string;

    toString(): string;

    unshift(...items: any[]): number;

    values(): IterableIterator;

    includes(searchElement: any, fromIndex?: number): boolean;

    flat(
        this: any,
        depth?: any
    ): any[]

    flatMap(
        callback: (this: any, value: any, index: number, array: any[]) => any,
        thisArg?: any
    ): any[]

    static from<T, U>(arrayLike: ArrayLike<T>, mapfn: (v: T, k: number) => U, thisArg?: any): U[];

    static from<T, U>(iterable: Iterable<T> | ArrayLike<T>, mapfn: (v: T, k: number) => U, thisArg?: any): U[];

    static isArray(arg: any): arg is any[];

    static of<T>(...items: T[]): T[];

    // fixme: Front-end not supported
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

    // fixme: Front-end not supported
    // 1. [Symbol.species]: SharedArrayBuffer;
    // 2. [Symbol.toStringTag]: "SharedArrayBuffer";
}

declare class String extends Object {
    length: number;

    constructor(value?: any): String;

    charAt(pos: number): string;

    charCodeAt(index: number): number;

    concat(...strings: string[]): string;

    endsWith(searchString: string, endPosition?: number): boolean;

    includes(searchString: string, position?: number): boolean;

    indexOf(searchString: string, position?: number): number;

    lastIndexOf(searchString: string, position?: number): number;

    localeCompare(that: string): number;

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

    // fixme: Front-end not supported
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

    // fixme: Front-end not supported
    // 1. [Symbol.toPrimitive](hint: string): symbol;
    // 2. [Symbol.toStringTag]: string;
}


declare class WeakRef extends object {
    constructor(target: any): WeakRef;

    deref(): any | undefined;

    // fixme: Front-end not supported
    // 1. [Symbol.toStringTag]: "WeakRef";
}

declare class Uint8ClampedArray { }

declare class Uint8Array { }

declare class TypedArray { }

declare class Int8Array { }

declare class Uint16Array { }

declare class Uint32Array { }

declare class Int16Array { }

declare class Int32Array { }

declare class Float32Array { }

declare class Float64Array { }

declare class BigInt64Array { }

declare class BigUint64Array { }

declare class DataView {}

declare class Atomics {}

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

// globalThis, ArkPrivate, undefined, Promise, Proxy, GeneratorFunction
// namespace Intl, Reflect