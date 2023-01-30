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

// === tests/cases/compiler/correlatedUnions.ts ===
declare function AssertType(value:any, type:string):void;
// Various repros from #30581

type RecordMap = { n: number, s: string, b: boolean };
type UnionRecord<K extends keyof RecordMap = keyof RecordMap> = { [P in K]: {
    kind: P,
    v: RecordMap[P],
    f: (v: RecordMap[P]) => void
}}[K];

function processRecord<K extends keyof RecordMap>(rec: UnionRecord<K>) {
    rec.f(rec.v);
AssertType(rec.f(rec.v), "void");
AssertType(rec.f, "(RecordMap[K]) => void");
AssertType(rec.v, "RecordMap[K]");
}

declare const r1: UnionRecord<'n'>;  // { kind: 'n', v: number, f: (v: number) => void 
AssertType(r1, "{ kind: "n"; v: number; f: (number) => void; }");
}

declare const r2: UnionRecord;  // { kind: 'n', ... } | { kind: 's', ... } | { kind: 'b', ... 
AssertType(r2, "UnionRecord<keyof RecordMap>");
}

processRecord(r1);
AssertType(processRecord(r1), "void");
AssertType(processRecord, "<K extends keyof RecordMap>(UnionRecord<K>) => void");
AssertType(r1, "{ kind: "n"; v: number; f: (number) => void; }");

processRecord(r2);
AssertType(processRecord(r2), "void");
AssertType(processRecord, "<K extends keyof RecordMap>(UnionRecord<K>) => void");
AssertType(r2, "UnionRecord<keyof RecordMap>");

processRecord({ kind: 'n', v: 42, f: v => v.toExponential() });
AssertType(processRecord({ kind: 'n', v: 42, f: v => v.toExponential() }), "void");
AssertType(processRecord, "<K extends keyof RecordMap>(UnionRecord<K>) => void");
AssertType({ kind: 'n', v: 42, f: v => v.toExponential() }, "{ kind: "n"; v: number; f: (number) => string; }");
AssertType(kind, "string");
AssertType('n', "string");
AssertType(v, "number");
AssertType(42, "int");
AssertType(f, "(number) => string");
AssertType(v => v.toExponential(), "(number) => string");
AssertType(v, "number");
AssertType(v.toExponential(), "string");
AssertType(v.toExponential, "(?union) => string");

// --------

type TextFieldData = { value: string }
type SelectFieldData = { options: string[], selectedValue: string }

type FieldMap = {
    text: TextFieldData;
    select: SelectFieldData;
}

type FormField<K extends keyof FieldMap> = { type: K, data: FieldMap[K] };

type RenderFunc<K extends keyof FieldMap> = (props: FieldMap[K]) => void;
type RenderFuncMap = { [K in keyof FieldMap]: RenderFunc<K> };

function renderTextField(props: TextFieldData) {}
function renderSelectField(props: SelectFieldData) {}

const renderFuncs: RenderFuncMap = {
AssertType(renderFuncs, "RenderFuncMap");
AssertType({    text: renderTextField,    select: renderSelectField,}, "{ text: (TextFieldData) => void; select: (SelectFieldData) => void; }");

    text: renderTextField,
AssertType(text, "(TextFieldData) => void");
AssertType(renderTextField, "(TextFieldData) => void");

    select: renderSelectField,
AssertType(select, "(SelectFieldData) => void");
AssertType(renderSelectField, "(SelectFieldData) => void");

};

function renderField<K extends keyof FieldMap>(field: FormField<K>) {
    const renderFn = renderFuncs[field.type];
AssertType(renderFn, "RenderFuncMap[K]");
AssertType(renderFuncs[field.type], "RenderFuncMap[K]");
AssertType(renderFuncs, "RenderFuncMap");
AssertType(field.type, "K");

    renderFn(field.data);
AssertType(renderFn(field.data), "void");
AssertType(renderFn, "RenderFuncMap[K]");
AssertType(field.data, "FieldMap[K]");
}

// --------

type TypeMap = {
    foo: string,
    bar: number
};

type Keys = keyof TypeMap;

type HandlerMap = { [P in Keys]: (x: TypeMap[P]) => void };

const handlers: HandlerMap = {
AssertType(handlers, "HandlerMap");
AssertType({    foo: s => s.length,    bar: n => n.toFixed(2)}, "{ foo: (string) => number; bar: (number) => string; }");

    foo: s => s.length,
AssertType(foo, "(string) => number");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

    bar: n => n.toFixed(2)
AssertType(bar, "(number) => string");
AssertType(n => n.toFixed(2), "(number) => string");
AssertType(n, "number");
AssertType(n.toFixed(2), "string");
AssertType(n.toFixed, "(?union) => string");
AssertType(2, "int");

};

type DataEntry<K extends Keys = Keys> = { [P in K]: {
    type: P,
    data: TypeMap[P]
}}[K];

const data: DataEntry[] = [
AssertType(data, "DataEntry<keyof TypeMap>[]");
AssertType([    { type: 'foo', data: 'abc' },    { type: 'foo', data: 'def' },    { type: 'bar', data: 42 },], "(union)[]");

    { type: 'foo', data: 'abc' },
AssertType({ type: 'foo', data: 'abc' }, "{ type: "foo"; data: string; }");
AssertType(type, "string");
AssertType('foo', "string");
AssertType(data, "string");
AssertType('abc', "string");

    { type: 'foo', data: 'def' },
AssertType({ type: 'foo', data: 'def' }, "{ type: "foo"; data: string; }");
AssertType(type, "string");
AssertType('foo', "string");
AssertType(data, "string");
AssertType('def', "string");

    { type: 'bar', data: 42 },
AssertType({ type: 'bar', data: 42 }, "{ type: "bar"; data: number; }");
AssertType(type, "string");
AssertType('bar', "string");
AssertType(data, "number");
AssertType(42, "int");

];

function process<K extends Keys>(data: DataEntry<K>[]) {
    data.forEach(block => {
AssertType(data.forEach(block => {        if (block.type in handlers) {            handlers[block.type](block.data)        }    }), "void");
AssertType(data.forEach, "((DataEntry<K>, number, DataEntry<K>[]) => void, ?any) => void");
AssertType(block => {        if (block.type in handlers) {            handlers[block.type](block.data)        }    }, "(DataEntry<K>) => void");
AssertType(block, "DataEntry<K>");

        if (block.type in handlers) {
AssertType(block.type in handlers, "boolean");
AssertType(block.type, "K");
AssertType(handlers, "HandlerMap");

            handlers[block.type](block.data)
AssertType(handlers[block.type](block.data), "void");
AssertType(handlers[block.type], "HandlerMap[K]");
AssertType(handlers, "HandlerMap");
AssertType(block.type, "K");
AssertType(block.data, "TypeMap[K]");
        }
    });
}

process(data);
AssertType(process(data), "void");
AssertType(process, "<K extends keyof TypeMap>(DataEntry<K>[]) => void");
AssertType(data, "DataEntry<keyof TypeMap>[]");

process([{ type: 'foo', data: 'abc' }]);
AssertType(process([{ type: 'foo', data: 'abc' }]), "void");
AssertType(process, "<K extends keyof TypeMap>(DataEntry<K>[]) => void");
AssertType([{ type: 'foo', data: 'abc' }], "{ type: "foo"; data: string; }[]");
AssertType({ type: 'foo', data: 'abc' }, "{ type: "foo"; data: string; }");
AssertType(type, "string");
AssertType('foo', "string");
AssertType(data, "string");
AssertType('abc', "string");

// --------

type LetterMap = { A: string, B: number }
type LetterCaller<K extends keyof LetterMap> = { [P in K]: { letter: Record<P, LetterMap[P]>, caller: (x: Record<P, LetterMap[P]>) => void } }[K];

function call<K extends keyof LetterMap>({ letter, caller }: LetterCaller<K>): void {
  caller(letter);
AssertType(caller(letter), "void");
AssertType(caller, "(Record<K, LetterMap[K]>) => void");
AssertType(letter, "Record<K, LetterMap[K]>");
}

type A = { A: string };
type B = { B: number };
type ACaller = (a: A) => void;
type BCaller = (b: B) => void;

declare const xx: { letter: A, caller: ACaller } | { letter: B, caller: BCaller };
AssertType(xx, "union");
AssertType(letter, "A");
AssertType(caller, "ACaller");
AssertType(letter, "B");
AssertType(caller, "BCaller");

call(xx);
AssertType(call(xx), "void");
AssertType(call, "<K extends keyof LetterMap>(LetterCaller<K>) => void");
AssertType(xx, "union");

// --------

type Ev<K extends keyof DocumentEventMap> = { [P in K]: {
    readonly name: P;
    readonly once?: boolean;
    readonly callback: (ev: DocumentEventMap[P]) => void;
}}[K];

function processEvents<K extends keyof DocumentEventMap>(events: Ev<K>[]) {
    for (const event of events) {
AssertType(event, "Ev<K>");
AssertType(events, "Ev<K>[]");

        document.addEventListener(event.name, (ev) => event.callback(ev), { once: event.once });
AssertType(document.addEventListener(event.name, (ev) => event.callback(ev), { once: event.once }), "void");
AssertType(document.addEventListener, "{ <K extends keyof DocumentEventMap>(K, (Document, DocumentEventMap[K]) => any, ?union): void; (string, EventListenerOrEventListenerObject, ?union): void; }");
AssertType(event.name, "K");
AssertType((ev) => event.callback(ev), "(Document, DocumentEventMap[K]) => void");
AssertType(ev, "DocumentEventMap[K]");
AssertType(event.callback(ev), "void");
AssertType(event.callback, "(DocumentEventMap[K]) => void");
AssertType(ev, "DocumentEventMap[K]");
AssertType({ once: event.once }, "{ once: union; }");
AssertType(once, "union");
AssertType(event.once, "union");
    }
}

function createEventListener<K extends keyof DocumentEventMap>({ name, once = false, callback }: Ev<K>): Ev<K> {
AssertType({ name, once, callback }, "{ name: K; once: boolean; callback: (DocumentEventMap[K]) => void; }");
AssertType(name, "K");
AssertType(once, "boolean");
AssertType(callback, "(DocumentEventMap[K]) => void");
    return { name, once, callback };
}

const clickEvent = createEventListener({
AssertType(clickEvent, "{ readonly name: "click"; readonly once?: union; readonly callback: (MouseEvent) => void; }");
AssertType(createEventListener({    name: "click",    callback: ev => console.log(ev),}), "{ readonly name: "click"; readonly once?: union; readonly callback: (MouseEvent) => void; }");
AssertType(createEventListener, "<K extends keyof DocumentEventMap>(Ev<K>) => Ev<K>");
AssertType({    name: "click",    callback: ev => console.log(ev),}, "{ name: "click"; callback: (MouseEvent) => void; }");

    name: "click",
AssertType(name, "string");
AssertType("click", "string");

    callback: ev => console.log(ev),
AssertType(callback, "(MouseEvent) => void");
AssertType(ev => console.log(ev), "(MouseEvent) => void");
AssertType(ev, "MouseEvent");
AssertType(console.log(ev), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ev, "MouseEvent");

});

const scrollEvent = createEventListener({
AssertType(scrollEvent, "{ readonly name: "scroll"; readonly once?: union; readonly callback: (Event) => void; }");
AssertType(createEventListener({    name: "scroll",    callback: ev => console.log(ev),}), "{ readonly name: "scroll"; readonly once?: union; readonly callback: (Event) => void; }");
AssertType(createEventListener, "<K extends keyof DocumentEventMap>(Ev<K>) => Ev<K>");
AssertType({    name: "scroll",    callback: ev => console.log(ev),}, "{ name: "scroll"; callback: (Event) => void; }");

    name: "scroll",
AssertType(name, "string");
AssertType("scroll", "string");

    callback: ev => console.log(ev),
AssertType(callback, "(Event) => void");
AssertType(ev => console.log(ev), "(Event) => void");
AssertType(ev, "Event");
AssertType(console.log(ev), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ev, "Event");

});

processEvents([clickEvent, scrollEvent]);
AssertType(processEvents([clickEvent, scrollEvent]), "void");
AssertType(processEvents, "<K extends keyof DocumentEventMap>(Ev<K>[]) => void");
AssertType([clickEvent, scrollEvent], "(union)[]");
AssertType(clickEvent, "{ readonly name: "click"; readonly once?: union; readonly callback: (MouseEvent) => void; }");
AssertType(scrollEvent, "{ readonly name: "scroll"; readonly once?: union; readonly callback: (Event) => void; }");

processEvents([
AssertType(processEvents([    { name: "click", callback: ev => console.log(ev) },    { name: "scroll", callback: ev => console.log(ev) },]), "void");
AssertType(processEvents, "<K extends keyof DocumentEventMap>(Ev<K>[]) => void");
AssertType([    { name: "click", callback: ev => console.log(ev) },    { name: "scroll", callback: ev => console.log(ev) },], "(union)[]");

    { name: "click", callback: ev => console.log(ev) },
AssertType({ name: "click", callback: ev => console.log(ev) }, "{ name: "click"; callback: (MouseEvent) => void; }");
AssertType(name, "string");
AssertType("click", "string");
AssertType(callback, "(MouseEvent) => void");
AssertType(ev => console.log(ev), "(MouseEvent) => void");
AssertType(ev, "MouseEvent");
AssertType(console.log(ev), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ev, "MouseEvent");

    { name: "scroll", callback: ev => console.log(ev) },
AssertType({ name: "scroll", callback: ev => console.log(ev) }, "{ name: "scroll"; callback: (Event) => void; }");
AssertType(name, "string");
AssertType("scroll", "string");
AssertType(callback, "(Event) => void");
AssertType(ev => console.log(ev), "(Event) => void");
AssertType(ev, "Event");
AssertType(console.log(ev), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ev, "Event");

]);

// --------

function ff1() {
    type ArgMap = {
AssertType(ArgMap, "{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }");

        sum: [a: number, b: number],
AssertType(sum, "[a: number, b: number]");

        concat: [a: string, b: string, c: string]
AssertType(concat, "[a: string, b: string, c: string]");
    }
    type Keys = keyof ArgMap;
AssertType(Keys, "keyof { sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }");

    const funs: { [P in Keys]: (...args: ArgMap[P]) => void } = {
AssertType(funs, "{ concat: (string, string, string) => void; sum: (number, number) => void; }");
AssertType(args, "{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[P]");
AssertType({        sum: (a, b) => a + b,        concat: (a, b, c) => a + b + c    }, "{ sum: (number, number) => number; concat: (string, string, string) => string; }");

        sum: (a, b) => a + b,
AssertType(sum, "(number, number) => number");
AssertType((a, b) => a + b, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");

        concat: (a, b, c) => a + b + c
AssertType(concat, "(string, string, string) => string");
AssertType((a, b, c) => a + b + c, "(string, string, string) => string");
AssertType(a, "string");
AssertType(b, "string");
AssertType(c, "string");
AssertType(a + b + c, "string");
AssertType(a + b, "string");
AssertType(a, "string");
AssertType(b, "string");
AssertType(c, "string");
    }
    function apply<K extends Keys>(funKey: K, ...args: ArgMap[K]) {
AssertType(apply, "<K extends keyof { sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }>(K, ...{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[K]) => void");
AssertType(funKey, "K");
AssertType(args, "{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[K]");

        const fn = funs[funKey];
AssertType(fn, "{ concat: (string, string, string) => void; sum: (number, number) => void; }[K]");
AssertType(funs[funKey], "{ concat: (string, string, string) => void; sum: (number, number) => void; }[K]");
AssertType(funs, "{ concat: (string, string, string) => void; sum: (number, number) => void; }");
AssertType(funKey, "K");

        fn(...args);
AssertType(fn(...args), "void");
AssertType(fn, "{ concat: (string, string, string) => void; sum: (number, number) => void; }[K]");
AssertType(...args, "union");
AssertType(args, "{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[K]");
    }
    const x1 = apply('sum', 1, 2)
AssertType(x1, "void");
AssertType(apply('sum', 1, 2), "void");
AssertType(apply, "<K extends keyof { sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }>(K, ...{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[K]) => void");
AssertType('sum', "string");
AssertType(1, "int");
AssertType(2, "int");

    const x2 = apply('concat', 'str1', 'str2', 'str3' )
AssertType(x2, "void");
AssertType(apply('concat', 'str1', 'str2', 'str3' ), "void");
AssertType(apply, "<K extends keyof { sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }>(K, ...{ sum: [a: number, b: number]; concat: [a: string, b: string, c: string]; }[K]) => void");
AssertType('concat', "string");
AssertType('str1', "string");
AssertType('str2', "string");
AssertType('str3', "string");
}

// Repro from #47368

type ArgMap = { a: number, b: string };
type Func<K extends keyof ArgMap> = (x: ArgMap[K]) => void;
type Funcs = { [K in keyof ArgMap]: Func<K> };

function f1<K extends keyof ArgMap>(funcs: Funcs, key: K, arg: ArgMap[K]) {
    funcs[key](arg);
AssertType(funcs[key](arg), "void");
AssertType(funcs[key], "Funcs[K]");
AssertType(funcs, "Funcs");
AssertType(key, "K");
AssertType(arg, "ArgMap[K]");
}

function f2<K extends keyof ArgMap>(funcs: Funcs, key: K, arg: ArgMap[K]) {
    const func = funcs[key];  // Type Funcs[K]
AssertType(func, "Funcs[K]");
AssertType(funcs[key], "Funcs[K]");
AssertType(funcs, "Funcs");
AssertType(key, "K");

    func(arg);
AssertType(func(arg), "void");
AssertType(func, "Funcs[K]");
AssertType(arg, "ArgMap[K]");
}

function f3<K extends keyof ArgMap>(funcs: Funcs, key: K, arg: ArgMap[K]) {
    const func: Func<K> = funcs[key];  // Error, Funcs[K] not assignable to Func<K>
AssertType(func, "Func<K>");
AssertType(funcs[key], "Funcs[K]");
AssertType(funcs, "Funcs");
AssertType(key, "K");

    func(arg);
AssertType(func(arg), "void");
AssertType(func, "Func<K>");
AssertType(arg, "ArgMap[K]");
}

function f4<K extends keyof ArgMap>(x: Funcs[keyof ArgMap], y: Funcs[K]) {
    x = y;
AssertType(x = y, "Funcs[K]");
AssertType(x, "union");
AssertType(y, "Funcs[K]");
}

// Repro from #47890

interface MyObj {
    someKey: {
      name: string;
    }
    someOtherKey: {
      name: number;
    }
}

const ref: MyObj = {
AssertType(ref, "MyObj");
AssertType({    someKey: { name: "" },    someOtherKey: { name: 42 }}, "{ someKey: { name: string; }; someOtherKey: { name: number; }; }");

    someKey: { name: "" },
AssertType(someKey, "{ name: string; }");
AssertType({ name: "" }, "{ name: string; }");
AssertType(name, "string");
AssertType("", "string");

    someOtherKey: { name: 42 
AssertType(someOtherKey, "{ name: number; }");

AssertType({ name: 42 }, "{ name: number; }");

AssertType(name, "number");

AssertType(42, "int");
}

};

function func<K extends keyof MyObj>(k: K): MyObj[K]['name'] | undefined {
    const myObj: Partial<MyObj>[K] = ref[k];
AssertType(myObj, "Partial<MyObj>[K]");
AssertType(ref[k], "MyObj[K]");
AssertType(ref, "MyObj");
AssertType(k, "K");

    if (myObj) {
AssertType(myObj, "Partial<MyObj>[K]");

AssertType(myObj.name, "union");
      return myObj.name;
    }
    const myObj2: Partial<MyObj>[keyof MyObj] = ref[k];
AssertType(myObj2, "union");
AssertType(ref[k], "union");
AssertType(ref, "MyObj");
AssertType(k, "K");

    if (myObj2) {
AssertType(myObj2, "union");

AssertType(myObj2.name, "union");
      return myObj2.name;
    }
AssertType(undefined, "undefined");
    return undefined;
}

// Repro from #48157

interface Foo {
    bar?: string
}

function foo<T extends keyof Foo>(prop: T, f: Required<Foo>) {
    bar(f[prop]);
AssertType(bar(f[prop]), "void");
AssertType(bar, "(string) => void");
AssertType(f[prop], "Required<Foo>[T]");
AssertType(f, "Required<Foo>");
AssertType(prop, "T");
}

declare function bar(t: string): void;

// Repro from #48246

declare function makeCompleteLookupMapping<T extends ReadonlyArray<any>, Attr extends keyof T[number]>(
    ops: T, attr: Attr): { [Item in T[number]as Item[Attr]]: Item };

const ALL_BARS = [{ name: 'a'}, {name: 'b'}] as const;
AssertType(ALL_BARS, "readonly [{ readonly name: "a"; }, { readonly name: "b"; }]");
AssertType([{ name: 'a'}, {name: 'b'}] as const, "readonly [{ readonly name: "a"; }, { readonly name: "b"; }]");
AssertType([{ name: 'a'}, {name: 'b'}], "readonly [{ readonly name: "a"; }, { readonly name: "b"; }]");
AssertType({ name: 'a'}, "{ readonly name: "a"; }");
AssertType(name, "string");
AssertType('a', "string");
AssertType({name: 'b'}, "{ readonly name: "b"; }");
AssertType(name, "string");
AssertType('b', "string");

const BAR_LOOKUP = makeCompleteLookupMapping(ALL_BARS, 'name');
AssertType(BAR_LOOKUP, "{ a: { readonly name: "a"; }; b: { readonly name: "b"; }; }");
AssertType(makeCompleteLookupMapping(ALL_BARS, 'name'), "{ a: { readonly name: "a"; }; b: { readonly name: "b"; }; }");
AssertType(makeCompleteLookupMapping, "<T extends readonly any[], Attr extends keyof T[number]>(T, Attr) => { [Item in T[number] as Item[Attr]]: Item; }");
AssertType(ALL_BARS, "readonly [{ readonly name: "a"; }, { readonly name: "b"; }]");
AssertType('name', "string");

type BarLookup = typeof BAR_LOOKUP;

type Baz = { [K in keyof BarLookup]: BarLookup[K]['name'] };


