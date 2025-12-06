/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Object.keys/values/entries ====================

function testObjectKeys(): string {
    const obj = { a: 1, b: 2, c: 3 };
    return Object.keys(obj).join(',');
}

function testObjectValues(): string {
    const obj = { a: 1, b: 2, c: 3 };
    return Object.values(obj).join(',');
}

function testObjectEntries(): string {
    const obj = { a: 1, b: 2 };
    return Object.entries(obj).map(([k, v]) => `${k}:${v}`).join(',');
}

function testObjectFromEntries(): string {
    const entries: [string, number][] = [['a', 1], ['b', 2], ['c', 3]];
    const obj = Object.fromEntries(entries);
    return `${obj.a},${obj.b},${obj.c}`;
}

// ==================== Object.assign ====================

function testObjectAssign(): string {
    const target = { a: 1 };
    const source1 = { b: 2 };
    const source2 = { c: 3 };
    const result = Object.assign(target, source1, source2);
    return `${result.a},${result.b},${result.c},${target === result}`;
}

function testObjectAssignOverwrite(): string {
    const target = { a: 1, b: 2 };
    const source = { b: 10, c: 3 };
    Object.assign(target, source);
    return `${target.a},${target.b},${(target as any).c}`;
}

function testObjectAssignShallow(): string {
    const target = { nested: { x: 1 } };
    const source = { nested: { y: 2 } };
    Object.assign(target, source);
    return `${(target.nested as any).x},${target.nested.y}`;
}

// ==================== Object.freeze/seal ====================

function testObjectFreeze(): string {
    const obj = { a: 1, b: 2 };
    Object.freeze(obj);
    try {
        (obj as any).c = 3;
    } catch (e) {}
    try {
        obj.a = 10;
    } catch (e) {}
    return `${Object.isFrozen(obj)},${obj.a},${(obj as any).c}`;
}

function testObjectSeal(): string {
    const obj: any = { a: 1, b: 2 };
    Object.seal(obj);
    obj.a = 10;
    try {
        obj.c = 3;
    } catch (e) {}
    try {
        delete obj.b;
    } catch (e) {}
    return `${Object.isSealed(obj)},${obj.a},${obj.b},${obj.c}`;
}

function testObjectIsFrozenSealed(): string {
    const obj1 = {};
    const obj2 = Object.freeze({});
    const obj3 = Object.seal({});
    return `${Object.isFrozen(obj1)},${Object.isFrozen(obj2)},${Object.isSealed(obj3)}`;
}

// ==================== Object.defineProperty ====================

function testDefineProperty(): string {
    const obj: any = {};
    Object.defineProperty(obj, 'x', {
        value: 42,
        writable: false,
        enumerable: true,
        configurable: false
    });
    try {
        obj.x = 100;
    } catch (e) {}
    return `${obj.x}`;
}

function testDefinePropertyGetSet(): string {
    const obj: any = { _value: 0 };
    Object.defineProperty(obj, 'value', {
        get() { return this._value; },
        set(v) { this._value = v * 2; },
        enumerable: true
    });
    obj.value = 10;
    return `${obj.value},${obj._value}`;
}

function testDefineProperties(): string {
    const obj: any = {};
    Object.defineProperties(obj, {
        a: { value: 1, enumerable: true },
        b: { value: 2, enumerable: true },
        c: { value: 3, enumerable: false }
    });
    return `${obj.a},${obj.b},${obj.c},${Object.keys(obj).length}`;
}

// ==================== Object.getOwnPropertyDescriptor ====================

function testGetOwnPropertyDescriptor(): string {
    const obj = { a: 1 };
    const desc = Object.getOwnPropertyDescriptor(obj, 'a');
    return `${desc?.value},${desc?.writable},${desc?.enumerable}`;
}

function testGetOwnPropertyDescriptors(): string {
    const obj = { a: 1, b: 2 };
    const descs = Object.getOwnPropertyDescriptors(obj);
    return `${descs.a.value},${descs.b.value}`;
}

function testGetOwnPropertyNames(): string {
    const obj = { a: 1, b: 2 };
    Object.defineProperty(obj, 'hidden', { value: 3, enumerable: false });
    const names = Object.getOwnPropertyNames(obj);
    const keys = Object.keys(obj);
    return `${names.length},${keys.length}`;
}

// ==================== Object.getPrototypeOf/setPrototypeOf ====================

function testGetPrototypeOf(): string {
    const proto = { x: 10 };
    const obj = Object.create(proto);
    return `${Object.getPrototypeOf(obj) === proto}`;
}

function testSetPrototypeOf(): string {
    const proto = { getValue: () => 42 };
    const obj: any = { a: 1 };
    Object.setPrototypeOf(obj, proto);
    return `${obj.getValue()}`;
}

function testObjectCreate(): string {
    const proto = { greet() { return "Hello"; } };
    const obj = Object.create(proto, {
        name: { value: "Test", enumerable: true }
    });
    return `${obj.greet()},${obj.name}`;
}

// ==================== Object.is ====================

function testObjectIs(): string {
    const r1 = Object.is(1, 1);
    const r2 = Object.is(NaN, NaN);
    const r3 = Object.is(0, -0);
    const r4 = Object.is({}, {});
    return `${r1},${r2},${r3},${r4}`;
}

// ==================== Object.hasOwn ====================

function testHasOwnProperty(): string {
    const obj = { a: 1 };
    const proto = { b: 2 };
    Object.setPrototypeOf(obj, proto);
    return `${obj.hasOwnProperty('a')},${obj.hasOwnProperty('b')},${(obj as any).b}`;
}

// ==================== Object iteration ====================

function testObjectIteration(): string {
    const obj = { a: 1, b: 2, c: 3 };
    const result: string[] = [];
    for (const key in obj) {
        if (obj.hasOwnProperty(key)) {
            result.push(`${key}=${(obj as any)[key]}`);
        }
    }
    return result.join(',');
}

// Warmup
for (let i = 0; i < 20; i++) {
    testObjectKeys();
    testObjectValues();
    testObjectEntries();
    testObjectFromEntries();
    testObjectAssign();
    testObjectAssignOverwrite();
    testObjectAssignShallow();
    testObjectFreeze();
    testObjectSeal();
    testObjectIsFrozenSealed();
    testDefineProperty();
    testDefinePropertyGetSet();
    testDefineProperties();
    testGetOwnPropertyDescriptor();
    testGetOwnPropertyDescriptors();
    testGetOwnPropertyNames();
    testGetPrototypeOf();
    testSetPrototypeOf();
    testObjectCreate();
    testObjectIs();
    testHasOwnProperty();
    testObjectIteration();
}

// JIT compile
ArkTools.jitCompileAsync(testObjectKeys);
ArkTools.jitCompileAsync(testObjectValues);
ArkTools.jitCompileAsync(testObjectEntries);
ArkTools.jitCompileAsync(testObjectFromEntries);
ArkTools.jitCompileAsync(testObjectAssign);
ArkTools.jitCompileAsync(testObjectAssignOverwrite);
ArkTools.jitCompileAsync(testObjectAssignShallow);
ArkTools.jitCompileAsync(testObjectFreeze);
ArkTools.jitCompileAsync(testObjectSeal);
ArkTools.jitCompileAsync(testObjectIsFrozenSealed);
ArkTools.jitCompileAsync(testDefineProperty);
ArkTools.jitCompileAsync(testDefinePropertyGetSet);
ArkTools.jitCompileAsync(testDefineProperties);
ArkTools.jitCompileAsync(testGetOwnPropertyDescriptor);
ArkTools.jitCompileAsync(testGetOwnPropertyDescriptors);
ArkTools.jitCompileAsync(testGetOwnPropertyNames);
ArkTools.jitCompileAsync(testGetPrototypeOf);
ArkTools.jitCompileAsync(testSetPrototypeOf);
ArkTools.jitCompileAsync(testObjectCreate);
ArkTools.jitCompileAsync(testObjectIs);
ArkTools.jitCompileAsync(testHasOwnProperty);
ArkTools.jitCompileAsync(testObjectIteration);

print("compile testObjectKeys: " + ArkTools.waitJitCompileFinish(testObjectKeys));
print("compile testObjectValues: " + ArkTools.waitJitCompileFinish(testObjectValues));
print("compile testObjectEntries: " + ArkTools.waitJitCompileFinish(testObjectEntries));
print("compile testObjectFromEntries: " + ArkTools.waitJitCompileFinish(testObjectFromEntries));
print("compile testObjectAssign: " + ArkTools.waitJitCompileFinish(testObjectAssign));
print("compile testObjectAssignOverwrite: " + ArkTools.waitJitCompileFinish(testObjectAssignOverwrite));
print("compile testObjectAssignShallow: " + ArkTools.waitJitCompileFinish(testObjectAssignShallow));
print("compile testObjectFreeze: " + ArkTools.waitJitCompileFinish(testObjectFreeze));
print("compile testObjectSeal: " + ArkTools.waitJitCompileFinish(testObjectSeal));
print("compile testObjectIsFrozenSealed: " + ArkTools.waitJitCompileFinish(testObjectIsFrozenSealed));
print("compile testDefineProperty: " + ArkTools.waitJitCompileFinish(testDefineProperty));
print("compile testDefinePropertyGetSet: " + ArkTools.waitJitCompileFinish(testDefinePropertyGetSet));
print("compile testDefineProperties: " + ArkTools.waitJitCompileFinish(testDefineProperties));
print("compile testGetOwnPropertyDescriptor: " + ArkTools.waitJitCompileFinish(testGetOwnPropertyDescriptor));
print("compile testGetOwnPropertyDescriptors: " + ArkTools.waitJitCompileFinish(testGetOwnPropertyDescriptors));
print("compile testGetOwnPropertyNames: " + ArkTools.waitJitCompileFinish(testGetOwnPropertyNames));
print("compile testGetPrototypeOf: " + ArkTools.waitJitCompileFinish(testGetPrototypeOf));
print("compile testSetPrototypeOf: " + ArkTools.waitJitCompileFinish(testSetPrototypeOf));
print("compile testObjectCreate: " + ArkTools.waitJitCompileFinish(testObjectCreate));
print("compile testObjectIs: " + ArkTools.waitJitCompileFinish(testObjectIs));
print("compile testHasOwnProperty: " + ArkTools.waitJitCompileFinish(testHasOwnProperty));
print("compile testObjectIteration: " + ArkTools.waitJitCompileFinish(testObjectIteration));

// Verify
print("testObjectKeys: " + testObjectKeys());
print("testObjectValues: " + testObjectValues());
print("testObjectEntries: " + testObjectEntries());
print("testObjectFromEntries: " + testObjectFromEntries());
print("testObjectAssign: " + testObjectAssign());
print("testObjectAssignOverwrite: " + testObjectAssignOverwrite());
print("testObjectAssignShallow: " + testObjectAssignShallow());
print("testObjectFreeze: " + testObjectFreeze());
print("testObjectSeal: " + testObjectSeal());
print("testObjectIsFrozenSealed: " + testObjectIsFrozenSealed());
print("testDefineProperty: " + testDefineProperty());
print("testDefinePropertyGetSet: " + testDefinePropertyGetSet());
print("testDefineProperties: " + testDefineProperties());
print("testGetOwnPropertyDescriptor: " + testGetOwnPropertyDescriptor());
print("testGetOwnPropertyDescriptors: " + testGetOwnPropertyDescriptors());
print("testGetOwnPropertyNames: " + testGetOwnPropertyNames());
print("testGetPrototypeOf: " + testGetPrototypeOf());
print("testSetPrototypeOf: " + testSetPrototypeOf());
print("testObjectCreate: " + testObjectCreate());
print("testObjectIs: " + testObjectIs());
print("testHasOwnProperty: " + testHasOwnProperty());
print("testObjectIteration: " + testObjectIteration());
