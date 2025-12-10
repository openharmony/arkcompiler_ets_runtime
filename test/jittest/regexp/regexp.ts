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

// ==================== RegExp Basic ====================

function testRegExpTest(): string {
    const re = /hello/;
    return `${re.test('hello world')},${re.test('goodbye')}`;
}

function testRegExpExec(): string {
    const re = /(\w+)\s(\w+)/;
    const result = re.exec('hello world');
    if (!result) return 'null';
    return `${result[0]},${result[1]},${result[2]}`;
}

function testRegExpFlags(): string {
    const re = /abc/gi;
    return `${re.global},${re.ignoreCase},${re.multiline},${re.source}`;
}

function testRegExpCaseInsensitive(): string {
    const re = /hello/i;
    return `${re.test('HELLO')},${re.test('Hello')},${re.test('hello')}`;
}

function testRegExpGlobal(): string {
    const re = /a/g;
    const str = 'abracadabra';
    const matches: string[] = [];
    let match;
    while ((match = re.exec(str)) !== null) {
        matches.push(String(re.lastIndex));
    }
    return matches.join(',');
}

function testRegExpMultiline(): string {
    const re = /^start/gm;
    const str = 'start here\nstart there';
    const matches: number[] = [];
    let match;
    while ((match = re.exec(str)) !== null) {
        matches.push(match.index);
    }
    return matches.join(',');
}

// ==================== RegExp Character Classes ====================

function testRegExpDigits(): string {
    const re = /\d+/g;
    const str = 'abc123def456';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpNonDigits(): string {
    const re = /\D+/g;
    const str = 'abc123def456';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpWordChars(): string {
    const re = /\w+/g;
    const str = 'hello-world_123';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpWhitespace(): string {
    const re = /\s+/g;
    const str = 'hello   world\ttab';
    return str.replace(re, '_');
}

function testRegExpCharSet(): string {
    const re = /[aeiou]/g;
    const str = 'hello';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpNegatedCharSet(): string {
    const re = /[^aeiou]/g;
    const str = 'hello';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpRange(): string {
    const re = /[a-z]+/g;
    const str = 'Hello123World';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

// ==================== RegExp Quantifiers ====================

function testRegExpStar(): string {
    const re = /ab*c/;
    return `${re.test('ac')},${re.test('abc')},${re.test('abbbbc')}`;
}

function testRegExpPlus(): string {
    const re = /ab+c/;
    return `${re.test('ac')},${re.test('abc')},${re.test('abbbbc')}`;
}

function testRegExpQuestion(): string {
    const re = /colou?r/;
    return `${re.test('color')},${re.test('colour')},${re.test('colouur')}`;
}

function testRegExpExactCount(): string {
    const re = /a{3}/;
    return `${re.test('aa')},${re.test('aaa')},${re.test('aaaa')}`;
}

function testRegExpRangeCount(): string {
    const re = /a{2,4}/;
    return `${re.test('a')},${re.test('aa')},${re.test('aaaa')},${re.test('aaaaa')}`;
}

function testRegExpLazy(): string {
    const re = /a+?/g;
    const str = 'aaaa';
    const matches = str.match(re);
    return matches ? `${matches.length},${matches[0]}` : 'null';
}

// ==================== RegExp Groups ====================

function testRegExpCapturingGroup(): string {
    const re = /(\d{4})-(\d{2})-(\d{2})/;
    const result = re.exec('2024-03-15');
    if (!result) return 'null';
    return `${result[1]},${result[2]},${result[3]}`;
}

function testRegExpNonCapturingGroup(): string {
    const re = /(?:ab)+c/;
    return `${re.test('abc')},${re.test('ababc')}`;
}

function testRegExpNamedGroup(): string {
    const re = /(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})/;
    const result = re.exec('2024-03-15');
    if (!result || !result.groups) return 'null';
    return `${result.groups.year},${result.groups.month},${result.groups.day}`;
}

function testRegExpBackreference(): string {
    const re = /(\w+)\s+\1/;
    return `${re.test('hello hello')},${re.test('hello world')}`;
}

function testRegExpNamedBackreference(): string {
    const re = /(?<word>\w+)\s+\k<word>/;
    return `${re.test('hello hello')},${re.test('hello world')}`;
}

// ==================== RegExp Anchors ====================

function testRegExpStartAnchor(): string {
    const re = /^hello/;
    return `${re.test('hello world')},${re.test('world hello')}`;
}

function testRegExpEndAnchor(): string {
    const re = /world$/;
    return `${re.test('hello world')},${re.test('world hello')}`;
}

function testRegExpWordBoundary(): string {
    const re = /\bcat\b/;
    return `${re.test('the cat sat')},${re.test('category')},${re.test('bobcat')}`;
}

// ==================== RegExp Lookahead/Lookbehind ====================

function testRegExpPositiveLookahead(): string {
    const re = /\d+(?=%)/g;
    const str = '50% off, $100';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpNegativeLookahead(): string {
    const re = /\d+(?!%)/g;
    const str = '50% off, $100';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpPositiveLookbehind(): string {
    const re = /(?<=\$)\d+/g;
    const str = '50% off, $100';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpNegativeLookbehind(): string {
    const re = /(?<!\$)\d+/g;
    const str = '50% off, $100';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

// ==================== String Methods with RegExp ====================

function testStringMatch(): string {
    const str = 'hello123world456';
    const matches = str.match(/\d+/g);
    return matches ? matches.join(',') : 'null';
}

function testStringMatchAll(): string {
    const str = 'test1 test2 test3';
    const re = /test(\d)/g;
    const matches = [...str.matchAll(re)];
    return matches.map(m => m[1]).join(',');
}

function testStringSearch(): string {
    const str = 'hello world';
    return `${str.search(/world/)},${str.search(/xyz/)}`;
}

function testStringReplace(): string {
    const str = 'hello world';
    return str.replace(/world/, 'universe');
}

function testStringReplaceAll(): string {
    const str = 'foo bar foo baz foo';
    return str.replaceAll(/foo/g, 'qux');
}

function testStringReplaceWithGroups(): string {
    const str = '2024-03-15';
    return str.replace(/(\d{4})-(\d{2})-(\d{2})/, '$2/$3/$1');
}

function testStringReplaceWithFunction(): string {
    const str = 'hello world';
    return str.replace(/\w+/g, (match) => match.toUpperCase());
}

function testStringSplit(): string {
    const str = 'a1b2c3d';
    return str.split(/\d/).join(',');
}

// ==================== RegExp Advanced Patterns ====================

function testRegExpEmailValidation(): string {
    const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return `${re.test('test@example.com')},${re.test('invalid')},${re.test('a@b.c')}`;
}

function testRegExpPhoneValidation(): string {
    const re = /^\d{3}-\d{3}-\d{4}$/;
    return `${re.test('123-456-7890')},${re.test('1234567890')}`;
}

function testRegExpUrlParsing(): string {
    const re = /^(https?):\/\/([^/]+)(\/.*)?$/;
    const result = re.exec('https://example.com/path');
    if (!result) return 'null';
    return `${result[1]},${result[2]},${result[3] || '/'}`;
}

function testRegExpHexColor(): string {
    const re = /^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})$/;
    return `${re.test('#fff')},${re.test('#ffffff')},${re.test('#gggggg')}`;
}

function testRegExpIpAddress(): string {
    const re = /^(\d{1,3}\.){3}\d{1,3}$/;
    return `${re.test('192.168.1.1')},${re.test('256.1.1.1')},${re.test('1.2.3')}`;
}

// ==================== RegExp Unicode ====================

function testRegExpUnicodeFlag(): string {
    const re = /\p{L}+/gu;
    const str = 'Hello世界';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

function testRegExpUnicodeEscape(): string {
    const re = /[\u4e00-\u9fff]+/g;
    const str = 'Hello世界Test';
    const matches = str.match(re);
    return matches ? matches.join(',') : 'null';
}

// Warmup
for (let i = 0; i < 20; i++) {
    testRegExpTest();
    testRegExpExec();
    testRegExpFlags();
    testRegExpCaseInsensitive();
    testRegExpGlobal();
    testRegExpMultiline();
    testRegExpDigits();
    testRegExpNonDigits();
    testRegExpWordChars();
    testRegExpWhitespace();
    testRegExpCharSet();
    testRegExpNegatedCharSet();
    testRegExpRange();
    testRegExpStar();
    testRegExpPlus();
    testRegExpQuestion();
    testRegExpExactCount();
    testRegExpRangeCount();
    testRegExpLazy();
    testRegExpCapturingGroup();
    testRegExpNonCapturingGroup();
    testRegExpNamedGroup();
    testRegExpBackreference();
    testRegExpNamedBackreference();
    testRegExpStartAnchor();
    testRegExpEndAnchor();
    testRegExpWordBoundary();
    testRegExpPositiveLookahead();
    testRegExpNegativeLookahead();
    testRegExpPositiveLookbehind();
    testRegExpNegativeLookbehind();
    testStringMatch();
    testStringMatchAll();
    testStringSearch();
    testStringReplace();
    testStringReplaceAll();
    testStringReplaceWithGroups();
    testStringReplaceWithFunction();
    testStringSplit();
    testRegExpEmailValidation();
    testRegExpPhoneValidation();
    testRegExpUrlParsing();
    testRegExpHexColor();
    testRegExpIpAddress();
    testRegExpUnicodeFlag();
    testRegExpUnicodeEscape();
}

// JIT compile
ArkTools.jitCompileAsync(testRegExpTest);
ArkTools.jitCompileAsync(testRegExpExec);
ArkTools.jitCompileAsync(testRegExpFlags);
ArkTools.jitCompileAsync(testRegExpCaseInsensitive);
ArkTools.jitCompileAsync(testRegExpGlobal);
ArkTools.jitCompileAsync(testRegExpMultiline);
ArkTools.jitCompileAsync(testRegExpDigits);
ArkTools.jitCompileAsync(testRegExpNonDigits);
ArkTools.jitCompileAsync(testRegExpWordChars);
ArkTools.jitCompileAsync(testRegExpWhitespace);
ArkTools.jitCompileAsync(testRegExpCharSet);
ArkTools.jitCompileAsync(testRegExpNegatedCharSet);
ArkTools.jitCompileAsync(testRegExpRange);
ArkTools.jitCompileAsync(testRegExpStar);
ArkTools.jitCompileAsync(testRegExpPlus);
ArkTools.jitCompileAsync(testRegExpQuestion);
ArkTools.jitCompileAsync(testRegExpExactCount);
ArkTools.jitCompileAsync(testRegExpRangeCount);
ArkTools.jitCompileAsync(testRegExpLazy);
ArkTools.jitCompileAsync(testRegExpCapturingGroup);
ArkTools.jitCompileAsync(testRegExpNonCapturingGroup);
ArkTools.jitCompileAsync(testRegExpNamedGroup);
ArkTools.jitCompileAsync(testRegExpBackreference);
ArkTools.jitCompileAsync(testRegExpNamedBackreference);
ArkTools.jitCompileAsync(testRegExpStartAnchor);
ArkTools.jitCompileAsync(testRegExpEndAnchor);
ArkTools.jitCompileAsync(testRegExpWordBoundary);
ArkTools.jitCompileAsync(testRegExpPositiveLookahead);
ArkTools.jitCompileAsync(testRegExpNegativeLookahead);
ArkTools.jitCompileAsync(testRegExpPositiveLookbehind);
ArkTools.jitCompileAsync(testRegExpNegativeLookbehind);
ArkTools.jitCompileAsync(testStringMatch);
ArkTools.jitCompileAsync(testStringMatchAll);
ArkTools.jitCompileAsync(testStringSearch);
ArkTools.jitCompileAsync(testStringReplace);
ArkTools.jitCompileAsync(testStringReplaceAll);
ArkTools.jitCompileAsync(testStringReplaceWithGroups);
ArkTools.jitCompileAsync(testStringReplaceWithFunction);
ArkTools.jitCompileAsync(testStringSplit);
ArkTools.jitCompileAsync(testRegExpEmailValidation);
ArkTools.jitCompileAsync(testRegExpPhoneValidation);
ArkTools.jitCompileAsync(testRegExpUrlParsing);
ArkTools.jitCompileAsync(testRegExpHexColor);
ArkTools.jitCompileAsync(testRegExpIpAddress);
ArkTools.jitCompileAsync(testRegExpUnicodeFlag);
ArkTools.jitCompileAsync(testRegExpUnicodeEscape);

print("compile testRegExpTest: " + ArkTools.waitJitCompileFinish(testRegExpTest));
print("compile testRegExpExec: " + ArkTools.waitJitCompileFinish(testRegExpExec));
print("compile testRegExpFlags: " + ArkTools.waitJitCompileFinish(testRegExpFlags));
print("compile testRegExpCaseInsensitive: " + ArkTools.waitJitCompileFinish(testRegExpCaseInsensitive));
print("compile testRegExpGlobal: " + ArkTools.waitJitCompileFinish(testRegExpGlobal));
print("compile testRegExpMultiline: " + ArkTools.waitJitCompileFinish(testRegExpMultiline));
print("compile testRegExpDigits: " + ArkTools.waitJitCompileFinish(testRegExpDigits));
print("compile testRegExpNonDigits: " + ArkTools.waitJitCompileFinish(testRegExpNonDigits));
print("compile testRegExpWordChars: " + ArkTools.waitJitCompileFinish(testRegExpWordChars));
print("compile testRegExpWhitespace: " + ArkTools.waitJitCompileFinish(testRegExpWhitespace));
print("compile testRegExpCharSet: " + ArkTools.waitJitCompileFinish(testRegExpCharSet));
print("compile testRegExpNegatedCharSet: " + ArkTools.waitJitCompileFinish(testRegExpNegatedCharSet));
print("compile testRegExpRange: " + ArkTools.waitJitCompileFinish(testRegExpRange));
print("compile testRegExpStar: " + ArkTools.waitJitCompileFinish(testRegExpStar));
print("compile testRegExpPlus: " + ArkTools.waitJitCompileFinish(testRegExpPlus));
print("compile testRegExpQuestion: " + ArkTools.waitJitCompileFinish(testRegExpQuestion));
print("compile testRegExpExactCount: " + ArkTools.waitJitCompileFinish(testRegExpExactCount));
print("compile testRegExpRangeCount: " + ArkTools.waitJitCompileFinish(testRegExpRangeCount));
print("compile testRegExpLazy: " + ArkTools.waitJitCompileFinish(testRegExpLazy));
print("compile testRegExpCapturingGroup: " + ArkTools.waitJitCompileFinish(testRegExpCapturingGroup));
print("compile testRegExpNonCapturingGroup: " + ArkTools.waitJitCompileFinish(testRegExpNonCapturingGroup));
print("compile testRegExpNamedGroup: " + ArkTools.waitJitCompileFinish(testRegExpNamedGroup));
print("compile testRegExpBackreference: " + ArkTools.waitJitCompileFinish(testRegExpBackreference));
print("compile testRegExpNamedBackreference: " + ArkTools.waitJitCompileFinish(testRegExpNamedBackreference));
print("compile testRegExpStartAnchor: " + ArkTools.waitJitCompileFinish(testRegExpStartAnchor));
print("compile testRegExpEndAnchor: " + ArkTools.waitJitCompileFinish(testRegExpEndAnchor));
print("compile testRegExpWordBoundary: " + ArkTools.waitJitCompileFinish(testRegExpWordBoundary));
print("compile testRegExpPositiveLookahead: " + ArkTools.waitJitCompileFinish(testRegExpPositiveLookahead));
print("compile testRegExpNegativeLookahead: " + ArkTools.waitJitCompileFinish(testRegExpNegativeLookahead));
print("compile testRegExpPositiveLookbehind: " + ArkTools.waitJitCompileFinish(testRegExpPositiveLookbehind));
print("compile testRegExpNegativeLookbehind: " + ArkTools.waitJitCompileFinish(testRegExpNegativeLookbehind));
print("compile testStringMatch: " + ArkTools.waitJitCompileFinish(testStringMatch));
print("compile testStringMatchAll: " + ArkTools.waitJitCompileFinish(testStringMatchAll));
print("compile testStringSearch: " + ArkTools.waitJitCompileFinish(testStringSearch));
print("compile testStringReplace: " + ArkTools.waitJitCompileFinish(testStringReplace));
print("compile testStringReplaceAll: " + ArkTools.waitJitCompileFinish(testStringReplaceAll));
print("compile testStringReplaceWithGroups: " + ArkTools.waitJitCompileFinish(testStringReplaceWithGroups));
print("compile testStringReplaceWithFunction: " + ArkTools.waitJitCompileFinish(testStringReplaceWithFunction));
print("compile testStringSplit: " + ArkTools.waitJitCompileFinish(testStringSplit));
print("compile testRegExpEmailValidation: " + ArkTools.waitJitCompileFinish(testRegExpEmailValidation));
print("compile testRegExpPhoneValidation: " + ArkTools.waitJitCompileFinish(testRegExpPhoneValidation));
print("compile testRegExpUrlParsing: " + ArkTools.waitJitCompileFinish(testRegExpUrlParsing));
print("compile testRegExpHexColor: " + ArkTools.waitJitCompileFinish(testRegExpHexColor));
print("compile testRegExpIpAddress: " + ArkTools.waitJitCompileFinish(testRegExpIpAddress));
print("compile testRegExpUnicodeFlag: " + ArkTools.waitJitCompileFinish(testRegExpUnicodeFlag));
print("compile testRegExpUnicodeEscape: " + ArkTools.waitJitCompileFinish(testRegExpUnicodeEscape));

// Verify
print("testRegExpTest: " + testRegExpTest());
print("testRegExpExec: " + testRegExpExec());
print("testRegExpFlags: " + testRegExpFlags());
print("testRegExpCaseInsensitive: " + testRegExpCaseInsensitive());
print("testRegExpGlobal: " + testRegExpGlobal());
print("testRegExpMultiline: " + testRegExpMultiline());
print("testRegExpDigits: " + testRegExpDigits());
print("testRegExpNonDigits: " + testRegExpNonDigits());
print("testRegExpWordChars: " + testRegExpWordChars());
print("testRegExpWhitespace: " + testRegExpWhitespace());
print("testRegExpCharSet: " + testRegExpCharSet());
print("testRegExpNegatedCharSet: " + testRegExpNegatedCharSet());
print("testRegExpRange: " + testRegExpRange());
print("testRegExpStar: " + testRegExpStar());
print("testRegExpPlus: " + testRegExpPlus());
print("testRegExpQuestion: " + testRegExpQuestion());
print("testRegExpExactCount: " + testRegExpExactCount());
print("testRegExpRangeCount: " + testRegExpRangeCount());
print("testRegExpLazy: " + testRegExpLazy());
print("testRegExpCapturingGroup: " + testRegExpCapturingGroup());
print("testRegExpNonCapturingGroup: " + testRegExpNonCapturingGroup());
print("testRegExpNamedGroup: " + testRegExpNamedGroup());
print("testRegExpBackreference: " + testRegExpBackreference());
print("testRegExpNamedBackreference: " + testRegExpNamedBackreference());
print("testRegExpStartAnchor: " + testRegExpStartAnchor());
print("testRegExpEndAnchor: " + testRegExpEndAnchor());
print("testRegExpWordBoundary: " + testRegExpWordBoundary());
print("testRegExpPositiveLookahead: " + testRegExpPositiveLookahead());
print("testRegExpNegativeLookahead: " + testRegExpNegativeLookahead());
print("testRegExpPositiveLookbehind: " + testRegExpPositiveLookbehind());
print("testRegExpNegativeLookbehind: " + testRegExpNegativeLookbehind());
print("testStringMatch: " + testStringMatch());
print("testStringMatchAll: " + testStringMatchAll());
print("testStringSearch: " + testStringSearch());
print("testStringReplace: " + testStringReplace());
print("testStringReplaceAll: " + testStringReplaceAll());
print("testStringReplaceWithGroups: " + testStringReplaceWithGroups());
print("testStringReplaceWithFunction: " + testStringReplaceWithFunction());
print("testStringSplit: " + testStringSplit());
print("testRegExpEmailValidation: " + testRegExpEmailValidation());
print("testRegExpPhoneValidation: " + testRegExpPhoneValidation());
print("testRegExpUrlParsing: " + testRegExpUrlParsing());
print("testRegExpHexColor: " + testRegExpHexColor());
print("testRegExpIpAddress: " + testRegExpIpAddress());
print("testRegExpUnicodeFlag: " + testRegExpUnicodeFlag());
print("testRegExpUnicodeEscape: " + testRegExpUnicodeEscape());
