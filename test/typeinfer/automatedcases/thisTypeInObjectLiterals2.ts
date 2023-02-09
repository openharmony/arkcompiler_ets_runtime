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

// === tests/cases/conformance/types/thisType/thisTypeInObjectLiterals2.ts ===
declare function AssertType(value:any, type:string):void;
// In methods of an object literal with no contextual type, 'this' has the type
// of the object literal.

let obj1 = {
AssertType(obj1, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");
AssertType({    a: 1,    f() {        return this.a;    },    b: "hello",    c: {        g() {            this.g();        }    },    get d() {        return this.a;    },    get e() {        return this.b;    },    set e(value) {        this.b = value;    }}, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");

    a: 1,
AssertType(a, "number");
AssertType(1, "int");

    f() {
AssertType(f, "() => number");

AssertType(this.a, "number");
AssertType(this, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");
        return this.a;

    },
    b: "hello",
AssertType(b, "string");
AssertType("hello", "string");

    c: {
AssertType(c, "{ g(): void; }");
AssertType({        g() {            this.g();        }    }, "{ g(): void; }");

        g() {
AssertType(g, "() => void");

            this.g();
AssertType(this.g(), "void");
AssertType(this.g, "() => void");
AssertType(this, "{ g(): void; }");
        }
    },
    get d() {
AssertType(d, "number");

AssertType(this.a, "number");
AssertType(this, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");
        return this.a;

    },
    get e() {
AssertType(e, "string");

AssertType(this.b, "string");
AssertType(this, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");
        return this.b;

    },
    set e(value) {
AssertType(e, "string");
AssertType(value, "string");

        this.b = value;
AssertType(this.b = value, "string");
AssertType(this.b, "string");
AssertType(this, "{ a: number; f(): number; b: string; c: { g(): void; }; readonly d: number; e: string; }");
AssertType(value, "string");
    }
};

// In methods of an object literal with a contextual type, 'this' has the
// contextual type.

type Point = {
    x: number;
    y: number;
    z?: number;
    moveBy(dx: number, dy: number, dz?: number): void;
}

let p1: Point = {
AssertType(p1, "Point");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
};

let p2: Point | null = {
AssertType(p2, "union");
AssertType(null, "null");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
};

let p3: Point | undefined = {
AssertType(p3, "union");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
};

let p4: Point | null | undefined = {
AssertType(p4, "union");
AssertType(null, "null");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
};

declare function f1(p: Point): void;

f1({
AssertType(f1({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}), "void");
AssertType(f1, "(Point) => void");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
});

declare function f2(p: Point | null | undefined): void;

f2({
AssertType(f2({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}), "void");
AssertType(f2, "(union) => void");
AssertType({    x: 10,    y: 20,    moveBy(dx, dy, dz) {        this.x += dx;        this.y += dy;        if (this.z && dz) {            this.z += dz;        }    }}, "{ x: number; y: number; moveBy(number, number, union): void; }");

    x: 10,
AssertType(x, "number");
AssertType(10, "int");

    y: 20,
AssertType(y, "number");
AssertType(20, "int");

    moveBy(dx, dy, dz) {
AssertType(moveBy, "(number, number, union) => void");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(dz, "union");

        this.x += dx;
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(dx, "number");

        this.y += dy;
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "Point");
AssertType(dy, "number");

        if (this.z && dz) {
AssertType(this.z && dz, "union");
AssertType(this.z, "union");
AssertType(this, "Point");
AssertType(dz, "union");

            this.z += dz;
AssertType(this.z += dz, "number");
AssertType(this.z, "number");
AssertType(this, "Point");
AssertType(dz, "number");
        }
    }
});

// In methods of an object literal with a contextual type that includes some
// ThisType<T>, 'this' is of type T.

type ObjectDescriptor<D, M> = {
    data?: D;
    methods?: M & ThisType<D & M>;  // Type of 'this' in methods is D & M
}

declare function makeObject<D, M>(desc: ObjectDescriptor<D, M>): D & M;

let x1 = makeObject({
AssertType(x1, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(makeObject({    data: { x: 0, y: 0 },    methods: {        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }}), "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(makeObject, "<D, M>(ObjectDescriptor<D, M>) => D & M");
AssertType({    data: { x: 0, y: 0 },    methods: {        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }}, "{ data: { x: number; y: number; }; methods: { moveBy(number, number): void; }; }");

    data: { x: 0, y: 0 },
AssertType(data, "{ x: number; y: number; }");
AssertType({ x: 0, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(0, "int");

    methods: {
AssertType(methods, "{ moveBy(number, number): void; }");
AssertType({        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }, "{ moveBy(number, number): void; }");

        moveBy(dx: number, dy: number) {
AssertType(moveBy, "(number, number) => void");
AssertType(dx, "number");
AssertType(dy, "number");

            this.x += dx;  // Strongly typed this
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(dx, "number");

            this.y += dy;  // Strongly typed this
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(dy, "number");
        }
    }
});

// In methods contained in an object literal with a contextual type that includes
// some ThisType<T>, 'this' is of type T.

type ObjectDescriptor2<D, M> = ThisType<D & M> & {
    data?: D;
    methods?: M;
}

declare function makeObject2<D, M>(desc: ObjectDescriptor<D, M>): D & M;

let x2 = makeObject2({
AssertType(x2, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(makeObject2({    data: { x: 0, y: 0 },    methods: {        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }}), "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(makeObject2, "<D, M>(ObjectDescriptor<D, M>) => D & M");
AssertType({    data: { x: 0, y: 0 },    methods: {        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }}, "{ data: { x: number; y: number; }; methods: { moveBy(number, number): void; }; }");

    data: { x: 0, y: 0 },
AssertType(data, "{ x: number; y: number; }");
AssertType({ x: 0, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(0, "int");

    methods: {
AssertType(methods, "{ moveBy(number, number): void; }");
AssertType({        moveBy(dx: number, dy: number) {            this.x += dx;  // Strongly typed this            this.y += dy;  // Strongly typed this        }    }, "{ moveBy(number, number): void; }");

        moveBy(dx: number, dy: number) {
AssertType(moveBy, "(number, number) => void");
AssertType(dx, "number");
AssertType(dy, "number");

            this.x += dx;  // Strongly typed this
AssertType(this.x += dx, "number");
AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(dx, "number");

            this.y += dy;  // Strongly typed this
AssertType(this.y += dy, "number");
AssertType(this.y, "number");
AssertType(this, "{ x: number; y: number; } & { moveBy(number, number): void; }");
AssertType(dy, "number");
        }
    }
});

// Check pattern similar to Object.defineProperty and Object.defineProperties

type PropDesc<T> = {
    value?: T;
    get?(): T;
    set?(value: T): void;
}

type PropDescMap<T> = {
    [K in keyof T]: PropDesc<T[K]>;
}

declare function defineProp<T, K extends string, U>(obj: T, name: K, desc: PropDesc<U> & ThisType<T>): T & Record<K, U>;

declare function defineProps<T, U>(obj: T, descs: PropDescMap<U> & ThisType<T>): T & U;

let p10 = defineProp(p1, "foo", { value: 42 });
AssertType(p10, "Point & Record<"foo", number>");
AssertType(defineProp(p1, "foo", { value: 42 }), "Point & Record<"foo", number>");
AssertType(defineProp, "<T, K extends string, U>(T, K, PropDesc<U> & ThisType<T>) => T & Record<K, U>");
AssertType(p1, "Point");
AssertType("foo", "string");
AssertType({ value: 42 }, "{ value: number; }");
AssertType(value, "number");
AssertType(42, "int");

p10.foo = p10.foo + 1;
AssertType(p10.foo = p10.foo + 1, "number");
AssertType(p10.foo, "number");
AssertType(p10.foo + 1, "number");
AssertType(p10.foo, "number");
AssertType(1, "int");

let p11 = defineProp(p1, "bar", {
AssertType(p11, "Point & Record<"bar", number>");
AssertType(defineProp, "<T, K extends string, U>(T, K, PropDesc<U> & ThisType<T>) => T & Record<K, U>");
AssertType(p1, "Point");
AssertType("bar", "string");
AssertType({    get() {        return this.x;    },    set(value: number) {        this.x = value;    }}, "{ get(): number; set(number): void; }");
AssertType(defineProp(p1, "bar", {    get() {        return this.x;    },    set(value: number) {        this.x = value;    }}), "Point & Record<"bar", number>");

    get() {
AssertType(get, "() => number");

AssertType(this.x, "number");
AssertType(this, "Point");
        return this.x;

    },
    set(value: number) {
AssertType(set, "(number) => void");
AssertType(value, "number");

        this.x = value;
AssertType(this.x = value, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(value, "number");
    }
});
p11.bar = p11.bar + 1;
AssertType(p11.bar = p11.bar + 1, "number");
AssertType(p11.bar, "number");
AssertType(p11.bar + 1, "number");
AssertType(p11.bar, "number");
AssertType(1, "int");

let p12 = defineProps(p1, {
AssertType(p12, "Point & { foo: number; bar: number; }");
AssertType(defineProps, "<T, U>(T, PropDescMap<U> & ThisType<T>) => T & U");
AssertType(p1, "Point");
AssertType({    foo: {        value: 42    },    bar: {        get(): number {            return this.x;        },        set(value: number) {            this.x = value;        }    }}, "{ foo: { value: number; }; bar: { get(): number; set(number): void; }; }");
AssertType(defineProps(p1, {    foo: {        value: 42    },    bar: {        get(): number {            return this.x;        },        set(value: number) {            this.x = value;        }    }}), "Point & { foo: number; bar: number; }");

    foo: {
AssertType(foo, "{ value: number; }");
AssertType({        value: 42    }, "{ value: number; }");

        value: 42
AssertType(value, "number");
AssertType(42, "int");

    },
    bar: {
AssertType(bar, "{ get(): number; set(number): void; }");
AssertType({        get(): number {            return this.x;        },        set(value: number) {            this.x = value;        }    }, "{ get(): number; set(number): void; }");

        get(): number {
AssertType(get, "() => number");

AssertType(this.x, "number");
AssertType(this, "Point");
            return this.x;

        },
        set(value: number) {
AssertType(set, "(number) => void");
AssertType(value, "number");

            this.x = value;
AssertType(this.x = value, "number");
AssertType(this.x, "number");
AssertType(this, "Point");
AssertType(value, "number");
        }
    }
});
p12.foo = p12.foo + 1;
AssertType(p12.foo = p12.foo + 1, "number");
AssertType(p12.foo, "number");
AssertType(p12.foo + 1, "number");
AssertType(p12.foo, "number");
AssertType(1, "int");

p12.bar = p12.bar + 1;
AssertType(p12.bar = p12.bar + 1, "number");
AssertType(p12.bar, "number");
AssertType(p12.bar + 1, "number");
AssertType(p12.bar, "number");
AssertType(1, "int");

// Proof of concept for typing of Vue.js

type Accessors<T> = { [K in keyof T]: (() => T[K]) | Computed<T[K]> };

type Dictionary<T> = { [x: string]: T }

type Computed<T> = {
    get?(): T;
    set?(value: T): void;
}

type VueOptions<D, M, P> = ThisType<D & M & P> & {
    data?: D | (() => D);
    methods?: M;
    computed?: Accessors<P>;
}

declare const Vue: new <D, M, P>(options: VueOptions<D, M, P>) => D & M & P;
AssertType(Vue, "new <D, M, P>(VueOptions<D, M, P>) => D & M & P");
AssertType(options, "VueOptions<D, M, P>");

let vue = new Vue({
AssertType(vue, "{ x: number; y: number; } & { f(string): number; } & { test: number; hello: string; }");
AssertType(Vue, "new <D, M, P>(VueOptions<D, M, P>) => D & M & P");
AssertType({    data: () => ({ x: 1, y: 2 }),    methods: {        f(x: string) {            return this.x;        }    },    computed: {        test(): number {            return this.x;        },        hello: {            get() {                return "hi";            },            set(value: string) {            }        }    }}, "{ data: () => { x: number; y: number; }; methods: { f(string): number; }; computed: { test(): number; hello: { get(): string; set(string): void; }; }; }");
AssertType(new Vue({    data: () => ({ x: 1, y: 2 }),    methods: {        f(x: string) {            return this.x;        }    },    computed: {        test(): number {            return this.x;        },        hello: {            get() {                return "hi";            },            set(value: string) {            }        }    }}), "{ x: number; y: number; } & { f(string): number; } & { test: number; hello: string; }");

    data: () => ({ x: 1, y: 2 }),
AssertType(data, "() => { x: number; y: number; }");
AssertType(() => ({ x: 1, y: 2 }), "() => { x: number; y: number; }");
AssertType(({ x: 1, y: 2 }), "{ x: number; y: number; }");
AssertType({ x: 1, y: 2 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");

    methods: {
AssertType(methods, "{ f(string): number; }");
AssertType({        f(x: string) {            return this.x;        }    }, "{ f(string): number; }");

        f(x: string) {
AssertType(f, "(string) => number");
AssertType(x, "string");

AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number; } & { f(string): number; } & { test: number; hello: string; }");
            return this.x;
        }
    },
    computed: {
AssertType(computed, "{ test(): number; hello: { get(): string; set(string): void; }; }");
AssertType({        test(): number {            return this.x;        },        hello: {            get() {                return "hi";            },            set(value: string) {            }        }    }, "{ test(): number; hello: { get(): string; set(string): void; }; }");

        test(): number {
AssertType(test, "() => number");

AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number; } & { f(string): number; } & { test: number; hello: string; }");
            return this.x;

        },
        hello: {
AssertType(hello, "{ get(): string; set(string): void; }");
AssertType({            get() {                return "hi";            },            set(value: string) {            }        }, "{ get(): string; set(string): void; }");

            get() {
AssertType(get, "() => string");

AssertType("hi", "string");
                return "hi";

            },
            set(value: string) {
AssertType(set, "(string) => void");
AssertType(value, "string");
            }
        }
    }
});

vue;
AssertType(vue, "{ x: number; y: number; } & { f(string): number; } & { test: number; hello: string; }");

vue.x;
AssertType(vue.x, "number");

vue.f("abc");
AssertType(vue.f("abc"), "number");
AssertType(vue.f, "(string) => number");
AssertType("abc", "string");

vue.test;
AssertType(vue.test, "number");

vue.hello;
AssertType(vue.hello, "string");


