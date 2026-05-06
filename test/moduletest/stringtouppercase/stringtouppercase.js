/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License", true);
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
    assert_equal("a".toUpperCase() === "A", true);
    assert_equal("A".toUpperCase() === "A", true);
    assert_equal("abc".toUpperCase() === "ABC", true);
    assert_equal("ABC".toUpperCase() === "ABC", true);
    assert_equal("AbCdEf".toUpperCase() === "ABCDEF", true);
    assert_equal("Hello World".toUpperCase() === "HELLO WORLD", true);
    assert_equal("123!@#".toUpperCase() === "123!@#", true);
    assert_equal(" ".toUpperCase() === " ", true);
    assert_equal("".toUpperCase() === "", true);

    // Unicode BMP 字符测试
    assert_equal("é".toUpperCase() === "É", true);
    assert_equal("ü".toUpperCase() === "Ü", true);
    assert_equal("ñ".toUpperCase() === "Ñ", true);
    assert_equal("α".toUpperCase() === "Α", true);
    assert_equal("β".toUpperCase() === "Β", true);
    assert_equal("abc αβγ".toUpperCase() === "ABC ΑΒΓ", true);
    assert_equal("я".toUpperCase() === "Я", true);
    assert_equal("привет".toUpperCase() === "ПРИВЕТ", true);
    assert_equal("中".toUpperCase() === "中", true);
    assert_equal("中文测试".toUpperCase() === "中文测试", true);
    assert_equal("あ".toUpperCase() === "あ", true);
    assert_equal("ア".toUpperCase() === "ア", true);
    assert_equal("日本語".toUpperCase() === "日本語", true);
    assert_equal("한".toUpperCase() === "한", true);
    assert_equal("한국어".toUpperCase() === "한국어", true);

    // 特殊大小写映射测试
    assert_equal("ß".toUpperCase() === "SS", true);
    assert_equal("straße".toUpperCase() === "STRASSE", true);
    assert_equal("i".toUpperCase() === "I", true);
    assert_equal("ς".toUpperCase() === "Σ", true);

    // 孤立代理测试
    const loneHigh = "\uD800";
    const resultHigh = loneHigh.toUpperCase();
    assert_equal(resultHigh.length === 1, true);
    assert_equal(resultHigh.charCodeAt(0) === 0xD800, true);
    assert_equal(resultHigh === "\uD800", true);
    assert_equal(resultHigh.isWellFormed() === false, true);

    // 边界情况测试
    assert_equal("".toUpperCase() === "", true);
    assert_equal("".toUpperCase().length === 0, true);
    assert_equal("x".toUpperCase() === "X", true);
    assert_equal("X".toUpperCase() === "X", true);
    const longStr = "a".repeat(10000);
    const longUpper = longStr.toUpperCase();
    assert_equal(longUpper === "A".repeat(10000), true);
    assert_equal("\t".toUpperCase() === "\t", true);
    assert_equal("\n".toUpperCase() === "\n", true);
    assert_equal("\u00A0".toUpperCase() === "\u00A0", true);
    assert_equal("\u200B".toUpperCase() === "\u200B", true);
    assert_equal("\uFEFF".toUpperCase() === "\uFEFF", true);

    // 混合复杂场景
    const complex1 = "Hello 世界 😀 \uD800 End";
    const result1 = complex1.toUpperCase();
    assert_equal(result1 === "HELLO 世界 😀 \uD800 END", true);

    const complex2 = "😀\uD800\uD83D\uDE00";
    const result2 = complex2.toUpperCase();
    assert_equal(result2 === "😀\uD800\uD83D\uDE00", true);

    const complex3 = "Aa\uD800Bb";
    assert_equal(complex3.toUpperCase() === "AA\uD800BB", true);

    const complex4 = "\uD800\uD801\uD802";
    assert_equal(complex4.toUpperCase() === "\uD800\uD801\uD802", true);
}

test_end();