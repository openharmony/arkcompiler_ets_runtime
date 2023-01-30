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

// === tests/cases/conformance/jsdoc/declarations/source.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @param {number} len
 */
export function Vec(len) {
    /**
     * @type {number[]}
     */
    this.storage = new Array(len);
AssertType(this.storage = new Array(len), "any[]");
AssertType(this.storage, "number[]");
AssertType(this, "this");
AssertType(new Array(len), "any[]");
AssertType(Array, "ArrayConstructor");
AssertType(len, "number");
}

Vec.prototype = {
AssertType(Vec.prototype, "{ dot(Vec): number; magnitude(): number; }");
AssertType({    /**     * @param {Vec} other     */    dot(other) {        if (other.storage.length !== this.storage.length) {            throw new Error(`Dot product only applicable for vectors of equal length`);        }        let sum = 0;        for (let i = 0; i < this.storage.length; i++) {            sum += (this.storage[i] * other.storage[i]);        }        return sum;    },    magnitude() {        let sum = 0;        for (let i = 0; i < this.storage.length; i++) {            sum += (this.storage[i] ** 2);        }        return Math.sqrt(sum);    }}, "{ dot(Vec): number; magnitude(): number; }");
AssertType(Vec.prototype = {    /**     * @param {Vec} other     */    dot(other) {        if (other.storage.length !== this.storage.length) {            throw new Error(`Dot product only applicable for vectors of equal length`);        }        let sum = 0;        for (let i = 0; i < this.storage.length; i++) {            sum += (this.storage[i] * other.storage[i]);        }        return sum;    },    magnitude() {        let sum = 0;        for (let i = 0; i < this.storage.length; i++) {            sum += (this.storage[i] ** 2);        }        return Math.sqrt(sum);    }}, "{ dot(Vec): number; magnitude(): number; }");

    /**
     * @param {Vec} other
     */
    dot(other) {
AssertType(dot, "(Vec) => number");
AssertType(other, "Vec");

        if (other.storage.length !== this.storage.length) {
AssertType(other.storage.length !== this.storage.length, "boolean");
AssertType(other.storage.length, "number");
AssertType(other.storage, "number[]");
AssertType(this.storage.length, "number");
AssertType(this.storage, "number[]");
AssertType(this, "this");

            throw new Error(`Dot product only applicable for vectors of equal length`);
AssertType(new Error(`Dot product only applicable for vectors of equal length`), "Error");
AssertType(Error, "ErrorConstructor");
AssertType(`Dot product only applicable for vectors of equal length`, "string");
        }
        let sum = 0;
AssertType(sum, "number");
AssertType(0, "int");

        for (let i = 0; i < this.storage.length; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < this.storage.length, "boolean");
AssertType(i, "number");
AssertType(this.storage.length, "number");
AssertType(this.storage, "number[]");
AssertType(this, "this");
AssertType(i++, "number");
AssertType(i, "number");

            sum += (this.storage[i] * other.storage[i]);
AssertType(sum += (this.storage[i] * other.storage[i]), "number");
AssertType(sum, "number");
AssertType((this.storage[i] * other.storage[i]), "number");
AssertType(this.storage[i] * other.storage[i], "number");
AssertType(this.storage[i], "number");
AssertType(this.storage, "number[]");
AssertType(this, "this");
AssertType(i, "number");
AssertType(other.storage[i], "number");
AssertType(other.storage, "number[]");
AssertType(i, "number");
        }
AssertType(sum, "number");
        return sum;

    },
    magnitude() {
AssertType(magnitude, "() => number");

        let sum = 0;
AssertType(sum, "number");
AssertType(0, "int");

        for (let i = 0; i < this.storage.length; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < this.storage.length, "boolean");
AssertType(i, "number");
AssertType(this.storage.length, "number");
AssertType(this.storage, "number[]");
AssertType(this, "this");
AssertType(i++, "number");
AssertType(i, "number");

            sum += (this.storage[i] ** 2);
AssertType(sum += (this.storage[i] ** 2), "number");
AssertType(sum, "number");
AssertType((this.storage[i] ** 2), "number");
AssertType(this.storage[i] ** 2, "number");
AssertType(this.storage[i], "number");
AssertType(this.storage, "number[]");
AssertType(this, "this");
AssertType(i, "number");
AssertType(2, "int");
        }
AssertType(Math.sqrt(sum), "number");
AssertType(Math.sqrt, "(number) => number");
AssertType(sum, "number");
        return Math.sqrt(sum);
    }
}

/**
 * @param {number} x
 * @param {number} y
 */
export function Point2D(x, y) {
    if (!(this instanceof Point2D)) {
AssertType(!(this instanceof Point2D), "boolean");
AssertType((this instanceof Point2D), "boolean");
AssertType(this instanceof Point2D, "boolean");
AssertType(this, "this");
AssertType(Point2D, "typeof Point2D");

AssertType(new Point2D(x, y), "Point2D");
AssertType(Point2D, "typeof Point2D");
AssertType(x, "number");
AssertType(y, "number");
        return new Point2D(x, y);
    }
    Vec.call(this, 2);
AssertType(Vec.call(this, 2), "any");
AssertType(Vec.call, "(Function, any, ...any[]) => any");
AssertType(this, "this");
AssertType(2, "int");

    this.x = x;
AssertType(this.x = x, "number");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(x, "number");

    this.y = y;
AssertType(this.y = y, "number");
AssertType(this.y, "number");
AssertType(this, "this");
AssertType(y, "number");
}

Point2D.prototype = {
AssertType(Point2D.prototype, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType({    __proto__: Vec,    get x() {        return this.storage[0];    },    /**     * @param {number} x     */    set x(x) {        this.storage[0] = x;    },    get y() {        return this.storage[1];    },    /**     * @param {number} y     */    set y(y) {        this.storage[1] = y;    }}, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType(Point2D.prototype = {    __proto__: Vec,    get x() {        return this.storage[0];    },    /**     * @param {number} x     */    set x(x) {        this.storage[0] = x;    },    get y() {        return this.storage[1];    },    /**     * @param {number} y     */    set y(y) {        this.storage[1] = y;    }}, "{ __proto__: typeof Vec; x: number; y: number; }");

    __proto__: Vec,
AssertType(__proto__, "typeof Vec");
AssertType(Vec, "typeof Vec");

    get x() {
AssertType(x, "number");

AssertType(this.storage[0], "any");
AssertType(this.storage, "any");
AssertType(this, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType(0, "int");
        return this.storage[0];

    },
    /**
     * @param {number} x
     */
    set x(x) {
AssertType(x, "number");
AssertType(x, "number");

        this.storage[0] = x;
AssertType(this.storage[0] = x, "number");
AssertType(this.storage[0], "any");
AssertType(this.storage, "any");
AssertType(this, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType(0, "int");
AssertType(x, "number");

    },
    get y() {
AssertType(y, "number");

AssertType(this.storage[1], "any");
AssertType(this.storage, "any");
AssertType(this, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType(1, "int");
        return this.storage[1];

    },
    /**
     * @param {number} y
     */
    set y(y) {
AssertType(y, "number");
AssertType(y, "number");

        this.storage[1] = y;
AssertType(this.storage[1] = y, "number");
AssertType(this.storage[1], "any");
AssertType(this.storage, "any");
AssertType(this, "{ __proto__: typeof Vec; x: number; y: number; }");
AssertType(1, "int");
AssertType(y, "number");
    }
};


