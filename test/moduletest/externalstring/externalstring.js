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

/*
 * @tc.name:externalstring
 * @tc.desc:test external string
 * @tc.type: FUNC
 * @tc.require: issue11746
 */

// Testing cases of creating external strings with different encodings.
function testCreateCachedExternalString(strArray, isCompressed) {
    for (let str of strArray) {
        let extStr = ArkTools.createCachedExternalString(str, isCompressed);
        assert_equal(extStr, str);
    }
}
testCreateCachedExternalString(["Hello world!", "你好，世界！", " 表情字符：😈 "], true);
testCreateCachedExternalString(["Hello, world!", "#$%&'()  *+-AB[^~"], true);
testCreateCachedExternalString(["Hello, world!", "#$%&'()  *+-AB[^~"], false);

// Testing cases of concatenating external strings with different encodings.
function testConcatExternalString(strArray, expectedStr, isCompressed) {
    let extStr1 = "";
    for (let str of strArray) {
        extStr1 += ArkTools.createCachedExternalString(str, isCompressed);
    }
    assert_equal(extStr1, expectedStr);
    let extStr2 = "";
    for (let str of strArray) {
        extStr2 = extStr2.concat(ArkTools.createCachedExternalString(str, isCompressed));
    }
    assert_equal(extStr2, expectedStr);
}
testConcatExternalString(['"你好，世界！"', "  翻译成英文是：", '" Hello, World!"'], '"你好，世界！"  翻译成英文是：" Hello, World!"', true);
testConcatExternalString(['"你好，世界！"', "  翻译成英文是：", '" Hello, World!"'], '"你好，世界！"  翻译成英文是：" Hello, World!"', false);

// Testing cases of string builin interfaces for external strings.
function testExternalStringBuiltinInterfaces(isCompressed) {
    if (!isCompressed) {
        let extStr1 = ArkTools.createCachedExternalString('"你好，世界！"  翻译成英文是： "Hello, World!"');
        assert_equal(extStr1.at(2), "好");
        assert_equal(extStr1.charAt(1), "你");
        assert_true(extStr1.endsWith('World!"'));
        assert_true(extStr1.includes("世界"));
        assert_equal(extStr1.indexOf("World"), 26);
        assert_equal(extStr1.lastIndexOf("!"), 31);
        let regex = /[A-Z]/g;
        assert_equal(extStr1.match(regex), ["H", "W"]);
        assert_equal(extStr1.search(regex), 19);
        assert_equal(extStr1.slice(19, 24), "Hello");
        let extStr1Arr = extStr1.split(" ");
        assert_equal(extStr1Arr[0], '"你好，世界！"');
        assert_equal(extStr1Arr[2], "翻译成英文是：");
        assert_equal(extStr1Arr[3], '"Hello,');
        assert_equal(extStr1Arr[4], 'World!"');
        assert_true(extStr1.startsWith('"你好，世界！"'));
        assert_equal(extStr1.substring(19, 25), "Hello,");
        assert_equal(extStr1.toUpperCase(), '"你好，世界！"  翻译成英文是： "HELLO, WORLD!"');
        assert_equal(extStr1.toLowerCase(), '"你好，世界！"  翻译成英文是： "hello, world!"');
        assert_equal(extStr1.valueOf(), '"你好，世界！"  翻译成英文是： "Hello, World!"');

        let extStr2 = ArkTools.createCachedExternalString("★♲☃");
        assert_equal(extStr2.charCodeAt(0), 9733);
        assert_equal(extStr2.codePointAt(1), 9842);
        let extStr2Repeat = extStr2.repeat(3);
        assert_equal(extStr2Repeat, "★♲☃★♲☃★♲☃");
        assert_equal(extStr2Repeat.replace("♲☃", "⭐😊😔"), "★⭐😊😔★♲☃★♲☃");
        assert_equal(extStr2Repeat.replaceAll("♲☃", "⭐😊😔"), "★⭐😊😔★⭐😊😔★⭐😊😔");
        assert_equal(extStr2.padEnd(8, "⭐😊😔"), "★♲☃⭐😊😔");
        assert_equal(extStr2.padStart(8, "⭐😊😔"), "⭐😊😔★♲☃");

        let extStr3 = ArkTools.createCachedExternalString("ab\uD83D\uDE04c");
        assert_true(extStr3.isWellFormed());
        assert_equal(extStr3.toWellFormed(), "ab😄c");

        let extStr5 = ArkTools.createCachedExternalString("\u0041\u006d\u00e9\u006c\u0069\u0065");
        assert_equal(extStr5.normalize(), "Amélie");
    }

    let regexp = /t(e)(st(\d?))/g;
    let extStr4 = ArkTools.createCachedExternalString("test1test2", isCompressed);
    assert_equal([...extStr4.matchAll(regexp)], [["test1", "e", "st1", "1"], ["test2", "e", "st2", "2"]]);

    let extStr6 = ArkTools.createCachedExternalString("  Hello world!   ", isCompressed);
    assert_equal(extStr6.trim(), "Hello world!");
    assert_equal(extStr6.trimStart(), "Hello world!   ");
    assert_equal(extStr6.trimEnd(), "  Hello world!");

    let extStr7 = ArkTools.createCachedExternalString("Hello, World! #$%&'()  *+-AB[^~", isCompressed);
    assert_equal(extStr7.at(2), "l");
    assert_equal(extStr7.charAt(1), "e");
    assert_true(extStr7.endsWith("*+-AB[^~"));
    assert_true(extStr7.includes("#$%&'()"));
    assert_equal(extStr7.indexOf("World"), 7);
    assert_equal(extStr7.lastIndexOf("!"), 12);
    let regex2 = /[A-Z]/g;
    assert_equal(extStr7.match(regex2), ["H", "W", "A", "B"]);
    assert_equal(extStr7.search(regex2), 0);
    assert_equal(extStr7.slice(7, 12), "World");
    let extStr7Arr = extStr7.split(" ");
    assert_equal(extStr7Arr[0], "Hello,");
    assert_equal(extStr7Arr[1], "World!");
    assert_equal(extStr7Arr[2], "#$%&'()");
    assert_equal(extStr7Arr[4], "*+-AB[^~");
    assert_true(extStr7.startsWith("Hello,"));
    assert_equal(extStr7.substring(7, 13), "World!");
    assert_equal(extStr7.toUpperCase(), "HELLO, WORLD! #$%&'()  *+-AB[^~");
    assert_equal(extStr7.toLowerCase(), "hello, world! #$%&'()  *+-ab[^~");
    assert_equal(extStr7.valueOf(), "Hello, World! #$%&'()  *+-AB[^~");
}
testExternalStringBuiltinInterfaces(true);
testExternalStringBuiltinInterfaces(false);

// Testing case of JSON.stringify with external strings.
function testJSONStringifyExternalString(isCompressed) {
    if (!isCompressed) {
        let extStr1 = ArkTools.createCachedExternalString("你好，fantastic⭐世界！😊，everyone must live happily!★♲☃😊");
        assert_equal(JSON.stringify(extStr1), '"你好，fantastic⭐世界！😊，everyone must live happily!★♲☃😊"');
    }
    let extStr2 = ArkTools.createCachedExternalString("Hello, fantastic world!, everyone must live happily!", isCompressed);
    assert_equal(JSON.stringify(extStr2), '"Hello, fantastic world!, everyone must live happily!"');
}
testJSONStringifyExternalString(true);
testJSONStringifyExternalString(false);

// Testing case of JSON.parse with external strings.
function testJSONParseExternalString(isCompressed) {
    let extJsonString = ArkTools.createCachedExternalString('"external json string"', isCompressed);
    assert_equal(JSON.parse(extJsonString), "external json string");

    let extJsonObject = ArkTools.createCachedExternalString('{"kind":"external","value":1,"items":["a","b"]}', isCompressed);
    let obj = JSON.parse(extJsonObject);
    assert_equal(obj.kind, "external");
    assert_equal(obj.value, 1);
    assert_equal(obj.items.length, 2);
    assert_equal(obj.items[0], "a");
    assert_equal(obj.items[1], "b");
}
testJSONParseExternalString(true);
testJSONParseExternalString(false);

// Testing cases of external strings in objects and arrays.
function testExternalStringInObjectsAndArrays(isCompressed) {
    let extStr1 = ArkTools.createCachedExternalString("ObjKey1", isCompressed);
    let str1 = "ObjKey2";
    let extStr2 = ArkTools.createCachedExternalString("ObjKey3", isCompressed);
    let extStr3 = ArkTools.createCachedExternalString("ObjKey4", isCompressed);
    let extStr4 = ArkTools.createCachedExternalString("ObjKey5", isCompressed);
    let extStr5 = ArkTools.createCachedExternalString("ObjVal1", isCompressed);
    let num1 = 123;
    let obj1 = {
        extStr1: "ObjVal2",
        str1: extStr5,
        extStr2: 123456,
        num1: "ObjVal3",
    };
    let obj2 = {
        extStr3: obj1,
        extStr4: [obj1, extStr5, 1234567],
    }
    assert_equal(JSON.stringify(obj2), '{"extStr3":{"extStr1":"ObjVal2","str1":"ObjVal1","extStr2":123456,"num1":"ObjVal3"},"extStr4":[{"extStr1":"ObjVal2","str1":"ObjVal1","extStr2":123456,"num1":"ObjVal3"},"ObjVal1",1234567]}');
    assert_equal(obj2.extStr3.str1, "ObjVal1");
    assert_equal(obj2.extStr4[0].str1, "ObjVal1");
    assert_equal(obj2.extStr3.extStr1, "ObjVal2");
    assert_equal(obj2.extStr3.extStr2, 123456);
    assert_equal(obj2.extStr4[0].extStr1, "ObjVal2");
    assert_equal(obj2.extStr4[0].extStr2, 123456);
}
testExternalStringInObjectsAndArrays(true);
testExternalStringInObjectsAndArrays(false);

// Testing cases of construct an external String containing special characters
// 1) Character traversal: !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~
// 2) Escape characters: \a \b \f \n \r \t \v \\ \' \" \? \0 \d
// 3) Emojis: 🀄 🏴
// 4) Illegal encoding: \udc04
function testExternalStringForSpecialCharacters(isCompressed) {
    // 1) Character traversal
    const specialChars = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    let extSpecialChars = ArkTools.createCachedExternalString(specialChars, isCompressed);
    for (let i = 0; i < specialChars.length; i++) {
        assert_equal(extSpecialChars.charAt(i), specialChars.charAt(i));
    }
    if (!isCompressed) {
        // 2) Escape characters
        const escapeChars = "\a\b\f\n\r\t\v\\\'\"\\?\0\d";
        let extEscapeChars = ArkTools.createCachedExternalString(escapeChars);
        assert_equal(extEscapeChars, escapeChars);
        // 3) Emojis
        const emojis = "🀄 🏴";
        let extEmojis = ArkTools.createCachedExternalString(emojis);
        assert_equal(extEmojis, "🀄 🏴");
        // 4) Illegal encoding
        const illegalEncoding = "\udc04";
        let extIllegalEncoding = ArkTools.createCachedExternalString(illegalEncoding);
        assert_equal(extIllegalEncoding.isWellFormed(), false);
    }
}
testExternalStringForSpecialCharacters(true);
testExternalStringForSpecialCharacters(false);

test_end();
