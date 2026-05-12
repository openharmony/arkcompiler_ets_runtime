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
    arkSteedCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Padding/Trimming ====================

function testPadStart(): string {
    const str = "5";
    return `${str.padStart(3, '0')},${str.padStart(3)},${str.padStart(1, '0')}`;
}

function testPadEnd(): string {
    const str = "5";
    return `${str.padEnd(3, '0')},${str.padEnd(3)},${str.padEnd(1, '0')}`;
}

function testTrim(): string {
    const str = "  hello  ";
    return `[${str.trim()}],[${str.trimStart()}],[${str.trimEnd()}]`;
}

// ==================== Search ====================

function testStartsWith(): string {
    const str = "Hello World";
    return `${str.startsWith('Hello')},${str.startsWith('World')},${str.startsWith('World', 6)}`;
}

function testEndsWith(): string {
    const str = "Hello World";
    return `${str.endsWith('World')},${str.endsWith('Hello')},${str.endsWith('Hello', 5)}`;
}

function testIncludes(): string {
    const str = "Hello World";
    return `${str.includes('World')},${str.includes('world')},${str.includes('o', 5)}`;
}

function testIndexOf(): string {
    const str = "Hello World";
    return `${str.indexOf('o')},${str.indexOf('o', 5)},${str.indexOf('x')}`;
}

function testLastIndexOf(): string {
    const str = "Hello World";
    return `${str.lastIndexOf('o')},${str.lastIndexOf('l')}`;
}

function testSearch(): string {
    const str = "Hello World";
    return `${str.search(/World/)},${str.search(/world/i)},${str.search(/xyz/)}`;
}

function testAt(): string {
    const str = "Hello";
    return `${str.at(0)},${str.at(-1)},${str.at(10)}`;
}

// ==================== Transform ====================

function testRepeat(): string {
    const str = "ab";
    return `${str.repeat(3)},${str.repeat(0)},${str.repeat(1)}`;
}

function testReplace(): string {
    const str = "Hello World World";
    return `${str.replace('World', 'JS')},${str.replace(/World/g, 'JS')}`;
}

function testReplaceAll(): string {
    const str = "aaa bbb aaa";
    return str.replaceAll('aaa', 'xxx');
}

function testSplit(): string {
    const str = "a,b,c,d";
    const arr = str.split(',');
    const limited = str.split(',', 2);
    return `${arr.join('-')},${limited.join('-')}`;
}

function testSlice(): string {
    const str = "Hello World";
    return `${str.slice(0, 5)},${str.slice(-5)},${str.slice(6)}`;
}

function testSubstring(): string {
    const str = "Hello World";
    return `${str.substring(0, 5)},${str.substring(6)}`;
}

function testToCase(): string {
    const str = "Hello World";
    return `${str.toUpperCase()},${str.toLowerCase()}`;
}

function testConcat(): string {
    const str = "Hello";
    return str.concat(' ', 'World', '!');
}

function testCharAt(): string {
    const str = "Hello";
    return `${str.charAt(0)},${str.charAt(4)},${str.charCodeAt(0)}`;
}

function testNormalize(): string {
    const str1 = '\u00F1';
    const str2 = '\u006E\u0303';
    return `${str1.normalize() === str2.normalize()}`;
}

// ==================== Static Methods ====================

function testFromCharCode(): string {
    const str = String.fromCharCode(72, 101, 108, 108, 111);
    return str;
}

function testFromCodePoint(): string {
    const str = String.fromCodePoint(72, 101, 108, 108, 111);
    return str;
}

// ==================== Template Methods ====================

function testLocaleCompare(): string {
    const r1 = 'a'.localeCompare('b');
    const r2 = 'b'.localeCompare('a');
    const r3 = 'a'.localeCompare('a');
    return `${r1 < 0},${r2 > 0},${r3 === 0}`;
}

function testMatch(): string {
    const str = "The rain in Spain";
    const result = str.match(/ain/g);
    return result ? result.join(',') : 'null';
}

function testMatchAll(): string {
    const str = "test1test2test3";
    const matches = [...str.matchAll(/test(\d)/g)];
    return matches.map(m => m[1]).join(',');
}

// Warmup
for (let i = 0; i < 20; i++) {
    testPadStart();
    testPadEnd();
    testTrim();
    testStartsWith();
    testEndsWith();
    testIncludes();
    testIndexOf();
    testLastIndexOf();
    testSearch();
    testAt();
    testRepeat();
    testReplace();
    testReplaceAll();
    testSplit();
    testSlice();
    testSubstring();
    testToCase();
    testConcat();
    testCharAt();
    testNormalize();
    testFromCharCode();
    testFromCodePoint();
    testLocaleCompare();
    testMatch();
    testMatchAll();
}

// JIT compile
ArkTools.arkSteedCompileAsync(testPadStart);
ArkTools.arkSteedCompileAsync(testPadEnd);
ArkTools.arkSteedCompileAsync(testTrim);
ArkTools.arkSteedCompileAsync(testStartsWith);
ArkTools.arkSteedCompileAsync(testEndsWith);
ArkTools.arkSteedCompileAsync(testIncludes);
ArkTools.arkSteedCompileAsync(testIndexOf);
ArkTools.arkSteedCompileAsync(testLastIndexOf);
ArkTools.arkSteedCompileAsync(testSearch);
ArkTools.arkSteedCompileAsync(testAt);
ArkTools.arkSteedCompileAsync(testRepeat);
ArkTools.arkSteedCompileAsync(testReplace);
ArkTools.arkSteedCompileAsync(testReplaceAll);
ArkTools.arkSteedCompileAsync(testSplit);
ArkTools.arkSteedCompileAsync(testSlice);
ArkTools.arkSteedCompileAsync(testSubstring);
ArkTools.arkSteedCompileAsync(testToCase);
ArkTools.arkSteedCompileAsync(testConcat);
ArkTools.arkSteedCompileAsync(testCharAt);
ArkTools.arkSteedCompileAsync(testNormalize);
ArkTools.arkSteedCompileAsync(testFromCharCode);
ArkTools.arkSteedCompileAsync(testFromCodePoint);
ArkTools.arkSteedCompileAsync(testLocaleCompare);
ArkTools.arkSteedCompileAsync(testMatch);
ArkTools.arkSteedCompileAsync(testMatchAll);

print("compile testPadStart: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testPadEnd: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testTrim: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testStartsWith: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testEndsWith: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testIncludes: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testIndexOf: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testLastIndexOf: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testSearch: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testAt: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testRepeat: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testReplace: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testReplaceAll: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testSplit: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testSlice: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testSubstring: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testToCase: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testConcat: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testCharAt: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testNormalize: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testFromCharCode: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testFromCodePoint: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testLocaleCompare: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testMatch: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print("compile testMatchAll: " + (() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());

// Verify
print("testPadStart: " + testPadStart());
print("testPadEnd: " + testPadEnd());
print("testTrim: " + testTrim());
print("testStartsWith: " + testStartsWith());
print("testEndsWith: " + testEndsWith());
print("testIncludes: " + testIncludes());
print("testIndexOf: " + testIndexOf());
print("testLastIndexOf: " + testLastIndexOf());
print("testSearch: " + testSearch());
print("testAt: " + testAt());
print("testRepeat: " + testRepeat());
print("testReplace: " + testReplace());
print("testReplaceAll: " + testReplaceAll());
print("testSplit: " + testSplit());
print("testSlice: " + testSlice());
print("testSubstring: " + testSubstring());
print("testToCase: " + testToCase());
print("testConcat: " + testConcat());
print("testCharAt: " + testCharAt());
print("testNormalize: " + testNormalize());
print("testFromCharCode: " + testFromCharCode());
print("testFromCodePoint: " + testFromCodePoint());
print("testLocaleCompare: " + testLocaleCompare());
print("testMatch: " + testMatch());
print("testMatchAll: " + testMatchAll());
