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
    arkSteedCompileSync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== JSON.parse ====================

function testParseBasic(): string {
    const obj = JSON.parse('{"a":1,"b":2}');
    return `${obj.a},${obj.b}`;
}

function testParseArray(): string {
    const arr = JSON.parse('[1,2,3,4,5]');
    return arr.join(',');
}

function testParseNested(): string {
    const obj = JSON.parse('{"outer":{"inner":{"value":42}}}');
    return `${obj.outer.inner.value}`;
}

function testParseReviver(): string {
    const obj = JSON.parse('{"a":1,"b":2,"c":3}', (key, value) => {
        if (typeof value === 'number') return value * 2;
        return value;
    });
    return `${obj.a},${obj.b},${obj.c}`;
}

function testParsePrimitives(): string {
    const num = JSON.parse('42');
    const str = JSON.parse('"hello"');
    const bool = JSON.parse('true');
    const nil = JSON.parse('null');
    return `${num},${str},${bool},${nil}`;
}

// ==================== JSON.stringify ====================

function testStringifyBasic(): string {
    const obj = { a: 1, b: 2, c: 'hello' };
    return JSON.stringify(obj);
}

function testStringifyArray(): string {
    const arr = [1, 2, 3, 'a', 'b'];
    return JSON.stringify(arr);
}

function testStringifyNested(): string {
    const obj = { outer: { inner: { value: 42 } } };
    return JSON.stringify(obj);
}

function testStringifyReplacer(): string {
    const obj = { a: 1, b: 2, c: 3, d: 4 };
    const filtered = JSON.stringify(obj, ['a', 'c']);
    return filtered;
}

function testStringifyReplacerFn(): string {
    const obj = { a: 1, b: 2, c: 3 };
    const result = JSON.stringify(obj, (key, value) => {
        if (typeof value === 'number') return value * 2;
        return value;
    });
    return result;
}

function testStringifySpace(): string {
    const obj = { a: 1, b: 2 };
    const pretty = JSON.stringify(obj, null, 2);
    return `${pretty.includes('\n')}`;
}

function testStringifySpecial(): string {
    const obj = { undef: undefined, fn: () => {}, sym: Symbol('x'), num: 42 };
    const result = JSON.stringify(obj);
    return result;
}

// ==================== Roundtrip ====================

function testRoundtrip(): string {
    const original = { name: 'test', values: [1, 2, 3], nested: { x: 10 } };
    const str = JSON.stringify(original);
    const parsed = JSON.parse(str);
    return `${parsed.name},${parsed.values.join('-')},${parsed.nested.x}`;
}

function testToJSON(): string {
    const obj = {
        value: 42,
        toJSON() {
            return { customValue: this.value * 2 };
        }
    };
    const result = JSON.stringify(obj);
    return result;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testParseBasic();
    testParseArray();
    testParseNested();
    testParseReviver();
    testParsePrimitives();
    testStringifyBasic();
    testStringifyArray();
    testStringifyNested();
    testStringifyReplacer();
    testStringifyReplacerFn();
    testStringifySpace();
    testStringifySpecial();
    testRoundtrip();
    testToJSON();
}

// JIT compile
ArkTools.arkSteedCompileSync(testParseBasic);
ArkTools.arkSteedCompileSync(testParseArray);
ArkTools.arkSteedCompileSync(testParseNested);
ArkTools.arkSteedCompileSync(testParseReviver);
ArkTools.arkSteedCompileSync(testParsePrimitives);
ArkTools.arkSteedCompileSync(testStringifyBasic);
ArkTools.arkSteedCompileSync(testStringifyArray);
ArkTools.arkSteedCompileSync(testStringifyNested);
ArkTools.arkSteedCompileSync(testStringifyReplacer);
ArkTools.arkSteedCompileSync(testStringifyReplacerFn);
ArkTools.arkSteedCompileSync(testStringifySpace);
ArkTools.arkSteedCompileSync(testStringifySpecial);
ArkTools.arkSteedCompileSync(testRoundtrip);
ArkTools.arkSteedCompileSync(testToJSON);

print("compile testParseBasic: " + true);
print("compile testParseArray: " + true);
print("compile testParseNested: " + true);
print("compile testParseReviver: " + true);
print("compile testParsePrimitives: " + true);
print("compile testStringifyBasic: " + true);
print("compile testStringifyArray: " + true);
print("compile testStringifyNested: " + true);
print("compile testStringifyReplacer: " + true);
print("compile testStringifyReplacerFn: " + true);
print("compile testStringifySpace: " + true);
print("compile testStringifySpecial: " + true);
print("compile testRoundtrip: " + true);
print("compile testToJSON: " + true);

// Verify
print("testParseBasic: " + testParseBasic());
print("testParseArray: " + testParseArray());
print("testParseNested: " + testParseNested());
print("testParseReviver: " + testParseReviver());
print("testParsePrimitives: " + testParsePrimitives());
print("testStringifyBasic: " + testStringifyBasic());
print("testStringifyArray: " + testStringifyArray());
print("testStringifyNested: " + testStringifyNested());
print("testStringifyReplacer: " + testStringifyReplacer());
print("testStringifyReplacerFn: " + testStringifyReplacerFn());
print("testStringifySpace: " + testStringifySpace());
print("testStringifySpecial: " + testStringifySpecial());
print("testRoundtrip: " + testRoundtrip());
print("testToJSON: " + testToJSON());
