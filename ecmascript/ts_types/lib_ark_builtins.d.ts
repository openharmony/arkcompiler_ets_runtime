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

declare class Function {
    constructor(...args: string[]): Function;

    apply(this: Function, thisArg: any, argArray?: any): any;

    bind(this: Function, thisArg: any, ...args: any[]): any;

    call(this: Function, thisArg: any, ...args: any[]): any;

    toString(): string;
}

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

declare class Object {
    constructor(arg?: any): Object;

    toString(): string;

    toLocaleString(): string;

    valueOf(): Object;

    hasOwnProperty(key: string | number | symbol): boolean;

    isPrototypeOf(o: Object): boolean;

    propertyIsEnumerable(key: string | number | symbol): boolean;

    static getPrototypeOf(o:any): Object | null;

    static getOwnPropertyDescriptor(o: any, key: string | number | symbol): PropertyDescriptor | undefined;

    static getOwnPropertyNames(o: any): string[];

    static create(o: Object | null, properties?: PropertyDescriptorMap): any;

    // fixme: defineProperty() defineProperties() seal() freeze() and preventExtensions() have template arguments.

    static isSealed(o: any): boolean;

    static isFrozen(o: any): boolean;

    static isExtensible(o: any): boolean;

    static keys(o: object): string[];

    static values(o: {}): any[];

    static entries(o: {}): [string, any][];

    static is(value1: any, value2: any): boolean;

    static getOwnPropertySymbols(o: any): symbol[];

    static setPrototypeOf(o: any, proto: Object | null): any;
}


declare class Number {
    constructor(arg?: any): Number;

    toString(radix?: number): string;

    toFixed(fractionDigits?: number): string;

    toExponential(fractionDigits?: number): string;

    toPrecision(precision?: number): string;

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

declare class Boolean {
    construnctor(arg?:any): Boolean;

    toString(): string;

    valueOf(): boolean;
}

declare class Set<T> {
    constructor(args: T[] | null): Set<T>;

    add(value: T): this;

    clear(): void;

    delete(value: T): boolean;

    forEach(callbackfn: (value: T, value2: T, set: Set<T>) => void, thisArg?: any): void;

    has(value: T): boolean;

    size: number;

    // fixme: add iterateable declaration entries(), values() and keys()
}

declare function print(arg:any, arg1?:any):string;
