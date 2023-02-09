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

// === tests/cases/conformance/types/spread/objectSpreadRepeatedNullCheckPerf.ts ===
declare function AssertType(value:any, type:string):void;
interface Props {
    readonly a?: string
    readonly b?: string
    readonly c?: string
    readonly d?: string
    readonly e?: string
    readonly f?: string
    readonly g?: string
    readonly h?: string
    readonly i?: string
    readonly j?: string
    readonly k?: string
    readonly l?: string
    readonly m?: string
    readonly n?: string
    readonly o?: string
    readonly p?: string
    readonly q?: string
    readonly r?: string
    readonly s?: string
    readonly t?: string
    readonly u?: string
    readonly v?: string
    readonly w?: string
    readonly x?: string
    readonly y?: string
    readonly z?: string
}

function parseWithSpread(config: Record<string, number>): Props {
AssertType({        ...config.a !== undefined && { a: config.a.toString() },        ...config.b !== undefined && { b: config.b.toString() },        ...config.c !== undefined && { c: config.c.toString() },        ...config.d !== undefined && { d: config.d.toString() },        ...config.e !== undefined && { e: config.e.toString() },        ...config.f !== undefined && { f: config.f.toString() },        ...config.g !== undefined && { g: config.g.toString() },        ...config.h !== undefined && { h: config.h.toString() },        ...config.i !== undefined && { i: config.i.toString() },        ...config.j !== undefined && { j: config.j.toString() },        ...config.k !== undefined && { k: config.k.toString() },        ...config.l !== undefined && { l: config.l.toString() },        ...config.m !== undefined && { m: config.m.toString() },        ...config.n !== undefined && { n: config.n.toString() },        ...config.o !== undefined && { o: config.o.toString() },        ...config.p !== undefined && { p: config.p.toString() },        ...config.q !== undefined && { q: config.q.toString() },        ...config.r !== undefined && { r: config.r.toString() },        ...config.s !== undefined && { s: config.s.toString() },        ...config.t !== undefined && { t: config.t.toString() },        ...config.u !== undefined && { u: config.u.toString() },        ...config.v !== undefined && { v: config.v.toString() },        ...config.w !== undefined && { w: config.w.toString() },        ...config.x !== undefined && { x: config.x.toString() },        ...config.y !== undefined && { y: config.y.toString() },        ...config.z !== undefined && { z: config.z.toString() }    }, "{ z?: union; y?: union; x?: union; w?: union; v?: union; u?: union; t?: union; s?: union; r?: union; q?: union; p?: union; o?: union; n?: union; m?: union; l?: union; k?: union; j?: union; i?: union; h?: union; g?: union; f?: union; e?: union; d?: union; c?: union; b?: union; a?: union; }");
    return {

        ...config.a !== undefined && { a: config.a.toString() },
AssertType(config.a !== undefined && { a: config.a.toString() }, "union");
AssertType(config.a !== undefined, "boolean");
AssertType(config.a, "number");
AssertType(undefined, "undefined");
AssertType({ a: config.a.toString() }, "{ a: string; }");
AssertType(a, "string");
AssertType(config.a.toString(), "string");
AssertType(config.a.toString, "(?union) => string");
AssertType(config.a, "number");

        ...config.b !== undefined && { b: config.b.toString() },
AssertType(config.b !== undefined && { b: config.b.toString() }, "union");
AssertType(config.b !== undefined, "boolean");
AssertType(config.b, "number");
AssertType(undefined, "undefined");
AssertType({ b: config.b.toString() }, "{ b: string; }");
AssertType(b, "string");
AssertType(config.b.toString(), "string");
AssertType(config.b.toString, "(?union) => string");
AssertType(config.b, "number");

        ...config.c !== undefined && { c: config.c.toString() },
AssertType(config.c !== undefined && { c: config.c.toString() }, "union");
AssertType(config.c !== undefined, "boolean");
AssertType(config.c, "number");
AssertType(undefined, "undefined");
AssertType({ c: config.c.toString() }, "{ c: string; }");
AssertType(c, "string");
AssertType(config.c.toString(), "string");
AssertType(config.c.toString, "(?union) => string");
AssertType(config.c, "number");

        ...config.d !== undefined && { d: config.d.toString() },
AssertType(config.d !== undefined && { d: config.d.toString() }, "union");
AssertType(config.d !== undefined, "boolean");
AssertType(config.d, "number");
AssertType(undefined, "undefined");
AssertType({ d: config.d.toString() }, "{ d: string; }");
AssertType(d, "string");
AssertType(config.d.toString(), "string");
AssertType(config.d.toString, "(?union) => string");
AssertType(config.d, "number");

        ...config.e !== undefined && { e: config.e.toString() },
AssertType(config.e !== undefined && { e: config.e.toString() }, "union");
AssertType(config.e !== undefined, "boolean");
AssertType(config.e, "number");
AssertType(undefined, "undefined");
AssertType({ e: config.e.toString() }, "{ e: string; }");
AssertType(e, "string");
AssertType(config.e.toString(), "string");
AssertType(config.e.toString, "(?union) => string");
AssertType(config.e, "number");

        ...config.f !== undefined && { f: config.f.toString() },
AssertType(config.f !== undefined && { f: config.f.toString() }, "union");
AssertType(config.f !== undefined, "boolean");
AssertType(config.f, "number");
AssertType(undefined, "undefined");
AssertType({ f: config.f.toString() }, "{ f: string; }");
AssertType(f, "string");
AssertType(config.f.toString(), "string");
AssertType(config.f.toString, "(?union) => string");
AssertType(config.f, "number");

        ...config.g !== undefined && { g: config.g.toString() },
AssertType(config.g !== undefined && { g: config.g.toString() }, "union");
AssertType(config.g !== undefined, "boolean");
AssertType(config.g, "number");
AssertType(undefined, "undefined");
AssertType({ g: config.g.toString() }, "{ g: string; }");
AssertType(g, "string");
AssertType(config.g.toString(), "string");
AssertType(config.g.toString, "(?union) => string");
AssertType(config.g, "number");

        ...config.h !== undefined && { h: config.h.toString() },
AssertType(config.h !== undefined && { h: config.h.toString() }, "union");
AssertType(config.h !== undefined, "boolean");
AssertType(config.h, "number");
AssertType(undefined, "undefined");
AssertType({ h: config.h.toString() }, "{ h: string; }");
AssertType(h, "string");
AssertType(config.h.toString(), "string");
AssertType(config.h.toString, "(?union) => string");
AssertType(config.h, "number");

        ...config.i !== undefined && { i: config.i.toString() },
AssertType(config.i !== undefined && { i: config.i.toString() }, "union");
AssertType(config.i !== undefined, "boolean");
AssertType(config.i, "number");
AssertType(undefined, "undefined");
AssertType({ i: config.i.toString() }, "{ i: string; }");
AssertType(i, "string");
AssertType(config.i.toString(), "string");
AssertType(config.i.toString, "(?union) => string");
AssertType(config.i, "number");

        ...config.j !== undefined && { j: config.j.toString() },
AssertType(config.j !== undefined && { j: config.j.toString() }, "union");
AssertType(config.j !== undefined, "boolean");
AssertType(config.j, "number");
AssertType(undefined, "undefined");
AssertType({ j: config.j.toString() }, "{ j: string; }");
AssertType(j, "string");
AssertType(config.j.toString(), "string");
AssertType(config.j.toString, "(?union) => string");
AssertType(config.j, "number");

        ...config.k !== undefined && { k: config.k.toString() },
AssertType(config.k !== undefined && { k: config.k.toString() }, "union");
AssertType(config.k !== undefined, "boolean");
AssertType(config.k, "number");
AssertType(undefined, "undefined");
AssertType({ k: config.k.toString() }, "{ k: string; }");
AssertType(k, "string");
AssertType(config.k.toString(), "string");
AssertType(config.k.toString, "(?union) => string");
AssertType(config.k, "number");

        ...config.l !== undefined && { l: config.l.toString() },
AssertType(config.l !== undefined && { l: config.l.toString() }, "union");
AssertType(config.l !== undefined, "boolean");
AssertType(config.l, "number");
AssertType(undefined, "undefined");
AssertType({ l: config.l.toString() }, "{ l: string; }");
AssertType(l, "string");
AssertType(config.l.toString(), "string");
AssertType(config.l.toString, "(?union) => string");
AssertType(config.l, "number");

        ...config.m !== undefined && { m: config.m.toString() },
AssertType(config.m !== undefined && { m: config.m.toString() }, "union");
AssertType(config.m !== undefined, "boolean");
AssertType(config.m, "number");
AssertType(undefined, "undefined");
AssertType({ m: config.m.toString() }, "{ m: string; }");
AssertType(m, "string");
AssertType(config.m.toString(), "string");
AssertType(config.m.toString, "(?union) => string");
AssertType(config.m, "number");

        ...config.n !== undefined && { n: config.n.toString() },
AssertType(config.n !== undefined && { n: config.n.toString() }, "union");
AssertType(config.n !== undefined, "boolean");
AssertType(config.n, "number");
AssertType(undefined, "undefined");
AssertType({ n: config.n.toString() }, "{ n: string; }");
AssertType(n, "string");
AssertType(config.n.toString(), "string");
AssertType(config.n.toString, "(?union) => string");
AssertType(config.n, "number");

        ...config.o !== undefined && { o: config.o.toString() },
AssertType(config.o !== undefined && { o: config.o.toString() }, "union");
AssertType(config.o !== undefined, "boolean");
AssertType(config.o, "number");
AssertType(undefined, "undefined");
AssertType({ o: config.o.toString() }, "{ o: string; }");
AssertType(o, "string");
AssertType(config.o.toString(), "string");
AssertType(config.o.toString, "(?union) => string");
AssertType(config.o, "number");

        ...config.p !== undefined && { p: config.p.toString() },
AssertType(config.p !== undefined && { p: config.p.toString() }, "union");
AssertType(config.p !== undefined, "boolean");
AssertType(config.p, "number");
AssertType(undefined, "undefined");
AssertType({ p: config.p.toString() }, "{ p: string; }");
AssertType(p, "string");
AssertType(config.p.toString(), "string");
AssertType(config.p.toString, "(?union) => string");
AssertType(config.p, "number");

        ...config.q !== undefined && { q: config.q.toString() },
AssertType(config.q !== undefined && { q: config.q.toString() }, "union");
AssertType(config.q !== undefined, "boolean");
AssertType(config.q, "number");
AssertType(undefined, "undefined");
AssertType({ q: config.q.toString() }, "{ q: string; }");
AssertType(q, "string");
AssertType(config.q.toString(), "string");
AssertType(config.q.toString, "(?union) => string");
AssertType(config.q, "number");

        ...config.r !== undefined && { r: config.r.toString() },
AssertType(config.r !== undefined && { r: config.r.toString() }, "union");
AssertType(config.r !== undefined, "boolean");
AssertType(config.r, "number");
AssertType(undefined, "undefined");
AssertType({ r: config.r.toString() }, "{ r: string; }");
AssertType(r, "string");
AssertType(config.r.toString(), "string");
AssertType(config.r.toString, "(?union) => string");
AssertType(config.r, "number");

        ...config.s !== undefined && { s: config.s.toString() },
AssertType(config.s !== undefined && { s: config.s.toString() }, "union");
AssertType(config.s !== undefined, "boolean");
AssertType(config.s, "number");
AssertType(undefined, "undefined");
AssertType({ s: config.s.toString() }, "{ s: string; }");
AssertType(s, "string");
AssertType(config.s.toString(), "string");
AssertType(config.s.toString, "(?union) => string");
AssertType(config.s, "number");

        ...config.t !== undefined && { t: config.t.toString() },
AssertType(config.t !== undefined && { t: config.t.toString() }, "union");
AssertType(config.t !== undefined, "boolean");
AssertType(config.t, "number");
AssertType(undefined, "undefined");
AssertType({ t: config.t.toString() }, "{ t: string; }");
AssertType(t, "string");
AssertType(config.t.toString(), "string");
AssertType(config.t.toString, "(?union) => string");
AssertType(config.t, "number");

        ...config.u !== undefined && { u: config.u.toString() },
AssertType(config.u !== undefined && { u: config.u.toString() }, "union");
AssertType(config.u !== undefined, "boolean");
AssertType(config.u, "number");
AssertType(undefined, "undefined");
AssertType({ u: config.u.toString() }, "{ u: string; }");
AssertType(u, "string");
AssertType(config.u.toString(), "string");
AssertType(config.u.toString, "(?union) => string");
AssertType(config.u, "number");

        ...config.v !== undefined && { v: config.v.toString() },
AssertType(config.v !== undefined && { v: config.v.toString() }, "union");
AssertType(config.v !== undefined, "boolean");
AssertType(config.v, "number");
AssertType(undefined, "undefined");
AssertType({ v: config.v.toString() }, "{ v: string; }");
AssertType(v, "string");
AssertType(config.v.toString(), "string");
AssertType(config.v.toString, "(?union) => string");
AssertType(config.v, "number");

        ...config.w !== undefined && { w: config.w.toString() },
AssertType(config.w !== undefined && { w: config.w.toString() }, "union");
AssertType(config.w !== undefined, "boolean");
AssertType(config.w, "number");
AssertType(undefined, "undefined");
AssertType({ w: config.w.toString() }, "{ w: string; }");
AssertType(w, "string");
AssertType(config.w.toString(), "string");
AssertType(config.w.toString, "(?union) => string");
AssertType(config.w, "number");

        ...config.x !== undefined && { x: config.x.toString() },
AssertType(config.x !== undefined && { x: config.x.toString() }, "union");
AssertType(config.x !== undefined, "boolean");
AssertType(config.x, "number");
AssertType(undefined, "undefined");
AssertType({ x: config.x.toString() }, "{ x: string; }");
AssertType(x, "string");
AssertType(config.x.toString(), "string");
AssertType(config.x.toString, "(?union) => string");
AssertType(config.x, "number");

        ...config.y !== undefined && { y: config.y.toString() },
AssertType(config.y !== undefined && { y: config.y.toString() }, "union");
AssertType(config.y !== undefined, "boolean");
AssertType(config.y, "number");
AssertType(undefined, "undefined");
AssertType({ y: config.y.toString() }, "{ y: string; }");
AssertType(y, "string");
AssertType(config.y.toString(), "string");
AssertType(config.y.toString, "(?union) => string");
AssertType(config.y, "number");

        ...config.z !== undefined && { z: config.z.toString() 
AssertType(config.z !== undefined && { z: config.z.toString() }, "union");

AssertType(config.z !== undefined, "boolean");

AssertType(config.z, "number");

AssertType(undefined, "undefined");

AssertType({ z: config.z.toString() }, "{ z: string; }");

AssertType(z, "string");

AssertType(config.z.toString(), "string");

AssertType(config.z.toString, "(?union) => string");

AssertType(config.z, "number");
}
    }
}

parseWithSpread({ a: 1, b: 2, z: 26 })
AssertType(parseWithSpread({ a: 1, b: 2, z: 26 }), "Props");
AssertType(parseWithSpread, "(Record<string, number>) => Props");
AssertType({ a: 1, b: 2, z: 26 }, "{ a: number; b: number; z: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");
AssertType(z, "number");
AssertType(26, "int");


