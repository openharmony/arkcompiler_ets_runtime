/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
{
    // 基础功能测试（ASCII 和 BMP 字符）
    assert_equal("A".toLowerCase() === "a", true);
    assert_equal("a".toLowerCase() === "a", true);
    assert_equal("ABC".toLowerCase() === "abc", true);
    assert_equal("abc".toLowerCase() === "abc", true);
    assert_equal("AbCdEf".toLowerCase() === "abcdef", true);
    assert_equal("Hello World".toLowerCase() === "hello world", true);
    assert_equal("123!@#".toLowerCase() === "123!@#", true);
    assert_equal(" ".toLowerCase() === " ", true);
    assert_equal("".toLowerCase() === "", true);

    // Unicode BMP 字符测试
    assert_equal("É".toLowerCase() === "é", true);
    assert_equal("Ü".toLowerCase() === "ü", true);
    assert_equal("Ñ".toLowerCase() === "ñ", true);
    assert_equal("Α".toLowerCase() === "α", true);
    assert_equal("Β".toLowerCase() === "β", true);
    assert_equal("ABC ΑΒΓ".toLowerCase() === "abc αβγ", true);
    assert_equal("Я".toLowerCase() === "я", true);
    assert_equal("ПРИВЕТ".toLowerCase() === "привет", true);
    assert_equal("中".toLowerCase() === "中", true);
    assert_equal("中文测试".toLowerCase() === "中文测试", true);
    assert_equal("あ".toLowerCase() === "あ", true);
    assert_equal("ア".toLowerCase() === "ア", true);
    assert_equal("日本語".toLowerCase() === "日本語", true);
    assert_equal("한".toLowerCase() === "한", true);
    assert_equal("한국어".toLowerCase() === "한국어", true);

    // 特殊大小写映射测试
    assert_equal("SS".toLowerCase() === "ss", true);
    assert_equal("STRASSE".toLowerCase() === "strasse", true);
    assert_equal("I".toLowerCase() === "i", true);
    assert_equal("Σ".toLowerCase() === "σ", true);
    assert_equal("ΌΔΥΣΣΕΎΣ".toLowerCase() === "όδυσσεύς", true);

    // 孤立代理测试
    const loneHigh = "\uD800";
    const resultHigh = loneHigh.toLowerCase();
    assert_equal(resultHigh.length === 1, true);
    assert_equal(resultHigh.charCodeAt(0) === 0xD800, true);
    assert_equal(resultHigh === "\uD800", true);
    assert_equal(resultHigh.isWellFormed() === false, true);

    // 边界情况测试
    assert_equal("".toLowerCase() === "", true);
    assert_equal("".toLowerCase().length === 0, true);
    assert_equal("X".toLowerCase() === "x", true);
    assert_equal("x".toLowerCase() === "x", true);
    const longStr = "A".repeat(10000, true);
    const longLower = longStr.toLowerCase();
    assert_equal(longLower === "a".repeat(10000), true);
    assert_equal("\t".toLowerCase() === "\t", true);
    assert_equal("\n".toLowerCase() === "\n", true);
    assert_equal("\u00A0".toLowerCase() === "\u00A0", true);
    assert_equal("\u200B".toLowerCase() === "\u200B", true);
    assert_equal("\uFEFF".toLowerCase() === "\uFEFF", true);

    // 混合复杂场景
    const complex1 = "HELLO 世界 😀 \uD800 End";
    const result1 = complex1.toLowerCase();
    assert_equal(result1 === "hello 世界 😀 \uD800 end", true);

    const complex2 = "😀\uD800\uD83D\uDE00";
    const result2 = complex2.toLowerCase();
    assert_equal(result2 === "😀\uD800\uD83D\uDE00", true);

    const complex3 = "Aa\uD800Bb";
    assert_equal(complex3.toLowerCase() === "aa\uD800bb", true);

    const complex4 = "\uD800\uD801\uD802";
    assert_equal(complex4.toLowerCase() === "\uD800\uD801\uD802", true);
}

{
    assert_equal("HELLO\x00WORLD".toLocaleLowerCase() === "hello\x00world", true);

    // 基础功能测试（ASCII 和 BMP 字符）
    assert_equal("A".toLocaleLowerCase() === "a", true);
    assert_equal("a".toLocaleLowerCase() === "a", true);
    assert_equal("ABC".toLocaleLowerCase() === "abc", true);
    assert_equal("abc".toLocaleLowerCase() === "abc", true);
    assert_equal("AbCdEf".toLocaleLowerCase() === "abcdef", true);
    assert_equal("Hello World".toLocaleLowerCase() === "hello world", true);
    assert_equal("123!@#".toLocaleLowerCase() === "123!@#", true);
    assert_equal(" ".toLocaleLowerCase() === " ", true);
    assert_equal("".toLocaleLowerCase() === "", true);

    // Unicode BMP 字符测试
    assert_equal("É".toLocaleLowerCase() === "é", true);
    assert_equal("Ü".toLocaleLowerCase() === "ü", true);
    assert_equal("Ñ".toLocaleLowerCase() === "ñ", true);
    assert_equal("Α".toLocaleLowerCase() === "α", true);
    assert_equal("Β".toLocaleLowerCase() === "β", true);
    assert_equal("ABC ΑΒΓ".toLocaleLowerCase() === "abc αβγ", true);
    assert_equal("Я".toLocaleLowerCase() === "я", true);
    assert_equal("ПРИВЕТ".toLocaleLowerCase() === "привет", true);
    assert_equal("中".toLocaleLowerCase() === "中", true);
    assert_equal("中文测试".toLocaleLowerCase() === "中文测试", true);
    assert_equal("あ".toLocaleLowerCase() === "あ", true);
    assert_equal("ア".toLocaleLowerCase() === "ア", true);
    assert_equal("日本語".toLocaleLowerCase() === "日本語", true);
    assert_equal("한".toLocaleLowerCase() === "한", true);
    assert_equal("한국어".toLocaleLowerCase() === "한국어", true);

    // 特殊大小写映射测试
    assert_equal("SS".toLocaleLowerCase() === "ss", true);
    assert_equal("STRASSE".toLocaleLowerCase() === "strasse", true);
    assert_equal("I".toLocaleLowerCase() === "i", true);
    assert_equal("Σ".toLocaleLowerCase() === "σ", true);
    assert_equal("ΌΔΥΣΣΕΎΣ".toLocaleLowerCase() === "όδυσσεύς", true);

    // 孤立代理测试
    const loneHigh = "\uD800";
    const resultHigh = loneHigh.toLocaleLowerCase();
    assert_equal(resultHigh.length === 1, true);
    assert_equal(resultHigh.charCodeAt(0) === 0xD800, true);
    assert_equal(resultHigh === "\uD800", true);
    assert_equal(resultHigh.isWellFormed() === false, true);

    // 边界情况测试
    assert_equal("".toLocaleLowerCase() === "", true);
    assert_equal("".toLocaleLowerCase().length === 0, true);
    assert_equal("X".toLocaleLowerCase() === "x", true);
    assert_equal("x".toLocaleLowerCase() === "x", true);
    const longStr = "A".repeat(10000);
    const longLower = longStr.toLocaleLowerCase();
    assert_equal(longLower === "a".repeat(10000), true);
    assert_equal("\t".toLocaleLowerCase() === "\t", true);
    assert_equal("\n".toLocaleLowerCase() === "\n", true);
    assert_equal("\u00A0".toLocaleLowerCase() === "\u00A0", true);
    assert_equal("\u200B".toLocaleLowerCase() === "\u200B", true);
    assert_equal("\uFEFF".toLocaleLowerCase() === "\uFEFF", true);

    // 混合复杂场景
    const complex1 = "HELLO 世界 😀 \uD800 End";
    const result1 = complex1.toLocaleLowerCase();
    assert_equal(result1 === "hello 世界 😀 \uD800 end", true);

    const complex2 = "😀\uD800\uD83D\uDE00";
    const result2 = complex2.toLocaleLowerCase();
    assert_equal(result2 === "😀\uD800\uD83D\uDE00", true);

    const complex3 = "Aa\uD800Bb";
    assert_equal(complex3.toLocaleLowerCase() === "aa\uD800bb", true);

    const complex4 = "\uD800\uD801\uD802";
    assert_equal(complex4.toLocaleLowerCase() === "\uD800\uD801\uD802", true);
}
test_end();