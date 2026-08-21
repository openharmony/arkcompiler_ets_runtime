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
ArkTools.arkSteedCompileSync(testPadStart);
ArkTools.arkSteedCompileSync(testPadEnd);
ArkTools.arkSteedCompileSync(testTrim);
ArkTools.arkSteedCompileSync(testStartsWith);
ArkTools.arkSteedCompileSync(testEndsWith);
ArkTools.arkSteedCompileSync(testIncludes);
ArkTools.arkSteedCompileSync(testIndexOf);
ArkTools.arkSteedCompileSync(testLastIndexOf);
ArkTools.arkSteedCompileSync(testSearch);
ArkTools.arkSteedCompileSync(testAt);
ArkTools.arkSteedCompileSync(testRepeat);
ArkTools.arkSteedCompileSync(testReplace);
ArkTools.arkSteedCompileSync(testReplaceAll);
ArkTools.arkSteedCompileSync(testSplit);
ArkTools.arkSteedCompileSync(testSlice);
ArkTools.arkSteedCompileSync(testSubstring);
ArkTools.arkSteedCompileSync(testToCase);
ArkTools.arkSteedCompileSync(testConcat);
ArkTools.arkSteedCompileSync(testCharAt);
ArkTools.arkSteedCompileSync(testNormalize);
ArkTools.arkSteedCompileSync(testFromCharCode);
ArkTools.arkSteedCompileSync(testFromCodePoint);
ArkTools.arkSteedCompileSync(testLocaleCompare);
ArkTools.arkSteedCompileSync(testMatch);
ArkTools.arkSteedCompileSync(testMatchAll);

print("compile testPadStart: " + true);
print("compile testPadEnd: " + true);
print("compile testTrim: " + true);
print("compile testStartsWith: " + true);
print("compile testEndsWith: " + true);
print("compile testIncludes: " + true);
print("compile testIndexOf: " + true);
print("compile testLastIndexOf: " + true);
print("compile testSearch: " + true);
print("compile testAt: " + true);
print("compile testRepeat: " + true);
print("compile testReplace: " + true);
print("compile testReplaceAll: " + true);
print("compile testSplit: " + true);
print("compile testSlice: " + true);
print("compile testSubstring: " + true);
print("compile testToCase: " + true);
print("compile testConcat: " + true);
print("compile testCharAt: " + true);
print("compile testNormalize: " + true);
print("compile testFromCharCode: " + true);
print("compile testFromCodePoint: " + true);
print("compile testLocaleCompare: " + true);
print("compile testMatch: " + true);
print("compile testMatchAll: " + true);

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
