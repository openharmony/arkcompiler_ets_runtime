/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

var FastBuffer = undefined;

let encodeArr = ['utf8', 'utf-8', 'ucs2', 'ucs-2', 'ascii', 'latin1', 'binary',
    'utf16le', 'utf-16le', 'base64', 'base64url', 'hex'];

const UINT32_MAX = 2 ** 32 - 1; // 表示2^32-1,是uint32能表达的最大值

function normalizeEncoding(enc) {
    enc = enc.toLowerCase();
    if (enc === 'ucs2' || enc === 'ucs-2' || enc === 'utf-16le') {
        enc = 'utf16le';
    }
    if (enc === 'utf-8') {
        enc = 'utf8';
    }
    return enc;
}

if (globalThis["ArkPrivate"] != undefined) {
    let map = new Map();
    FastBuffer = ArkPrivate.Load(ArkPrivate.FastBuffer);
    {
        let buf2 = new FastBuffer(10);
        buf2.fill('F1刘FG', 0, 10, 'ascii');
        let str = buf2.toString("hex");
        map.set("buffer.fill() failed, expect: 46311846474631184647, output: " + str, str === "46311846474631184647");
      
        let buf3 = new FastBuffer(10);
        buf3.fill('F1FG', 0, 10, 'base64');
        str = buf3.toString("hex");
        map.set("buffer.fill() failed, expect: 17514617514617514617, output: " + str, str === "17514617514617514617");
      
        let buf4  = new FastBuffer(10);
        buf4.fill('F1刘FG', 0, 10, 'binary');
        str = buf4.toString("hex");
        map.set("buffer.fill() failed, expect: 46311846474631184647, output: " + str, str === "46311846474631184647");
      
        let buf5 = new FastBuffer(10);
        buf5.fill('F1刘FG', 0, 10, 'latin1');
        str = buf5.toString("hex");
        map.set("buffer.fill() failed, expect: 46311846474631184647, output: " + str, str === "46311846474631184647");
      
        let buf6 = new FastBuffer(10);
        buf6.fill('F1刘FG', 0, 10, normalizeEncoding('ucs2'));
        str = buf6.toString("hex");
        map.set("buffer.fill() failed, expect: 46003100185246004700, output: " + str, str === "46003100185246004700");
      
        let buf7 = new FastBuffer(10);
        buf7.fill('F1刘FG', 0, 10, 'utf8');
        str = buf7.toString("hex");
        map.set("buffer.fill() failed, expect: 4631e5889846474631e5, output: " + str, str === "4631e5889846474631e5");
    }
    {
        let uint8array = new Uint8Array(10).fill(0);
        let buf = new FastBuffer([1, 2, 3, 4, 5, 6, 7]);
        let num = buf.copy(uint8array, 0, 3);
        map.set("buffer.copy() failed, expect num: 4, output: " + num, num === 4);
        let str = uint8array.toString();
        map.set("buffer.copy() failed, expect str: 4,5,6,7,0,0,0,0,0,0, output: " + str, str === "4,5,6,7,0,0,0,0,0,0");
    }
    {
        let latin1str = '¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ';
        let strbuf = new FastBuffer(latin1str, "latin1");
        let toStr = strbuf.toString("latin1");
        map.set("buffer create and tostring latin1 failed, expect: " + latin1str + ", output: " + toStr, toStr === latin1str);
    }
    {
        let buf = new FastBuffer(10);
        let buf1 = new FastBuffer(10);
        buf1.fill('!');
        for (let i = 0; i < 10; i++) {
          buf[i] = i + 97;
        }
        let len = buf.copy(buf1, 0, 4, 10);
        let str = buf1.toString();
        map.set("buffer.copy() failed, expect len: 6, output: " + len, len === 6);
        map.set("buffer.copy() failed, expect str: efghij!!!!, output: " + str, str === "efghij!!!!");
    }
    {
        let buf = new FastBuffer(10);
        for (let i = 0; i < 10; i++) {
          buf[i] = i + 97;
        }
        let len = buf.copy(buf, 2);
        map.set("buffer.copy() failed, expect len: 8, output: " + len, len === 8);
        let str = buf.toString();
        map.set("buffer.copy() failed, expect str: ababcdefgh, output: " + str, str === "ababcdefgh");
    }
    {
        let buf1 = new FastBuffer("123656");
        let buf2 = new FastBuffer("1235");
        let num = buf1.copy(buf2);
        let str = buf2.toString();
        map.set("buffer.copy() failed, expect num: 4, output: " + num, num === 4);
        map.set("buffer.copy() failed, expect str: 1236, output: " + str, str === "1236");
    }
    {
        let buf1 = new FastBuffer("测试一");
        let buf2 = new FastBuffer("测试二");
        let res = buf1.compare(buf2);
        map.set("buffer.comapre() failed, expect res: -1, output: " + res, res === -1);
    }
    {
        let buf1 = new FastBuffer("1236测试");
        let buf2 = new FastBuffer("1236测试");
        let res = buf1.equals(buf2);
        map.set("buffer.equals() failed, expect res: true, output: " + res, res === true);
    }
    {
        let buf1 = new FastBuffer("测试特殊字符$#@!");
        let flag = buf1.includes("#@");
        map.set("buffer.includes() failed, expect flag: true, output: " + flag, flag === true);
        flag = buf1.includes("测试");
        map.set("buffer.includes() failed, expect flag: true, output: " + flag, flag === true);
    }
    {
        let buf1 = new FastBuffer("测试特殊字符$#@!");
        let index = buf1.indexOf("@");
        map.set("buffer.indexOf() failed, expect index: 20, output: " + index, index === 20);
    }
    {
        let buf = new FastBuffer('this buffer is a buffer');
        let index = buf.lastIndexOf('buffer', -4);
        map.set("buffer.indexOf() failed, expect index: 17, output: " + index, index === 17);
        index = buf.lastIndexOf('buffer', -5);
        map.set("buffer.indexOf() failed, expect index: 17, output: " + index, index === 17);
        index = buf.lastIndexOf('buffer', -7);
        map.set("buffer.indexOf() failed, expect index: 5, output: " + index, index === 5);
        index = buf.lastIndexOf('buffer', 1);
        map.set("buffer.indexOf() failed, expect index: -1, output: " + index, index === -1);
        index = buf.lastIndexOf('buffer', 9999);
        map.set("buffer.indexOf() failed, expect index: 17, output: " + index, index === 17);
        index = buf.lastIndexOf('buffer', -999);
        map.set("buffer.indexOf() failed, expect index: -1, output: " + index, index === -1);
        index = buf.lastIndexOf('buffer');
        map.set("buffer.indexOf() failed, expect index: 17, output: " + index, index === 17);
    }
    {
        let uarr = new Uint8Array(4);
        uarr[0] = 0x31;
        uarr[1] = 0x32;
        uarr[2] = 0x33;
        uarr[3] = 0x34;
        let buf1 = new FastBuffer(uarr);
        let keys = buf1.keys();
        let index = 0;
        for (const key of keys) {
          map.set("buffer.keys() failed, expect index: " + key +", output: " + index, index === key);
          index++;
        }
    }
    {
        let uarr = new Uint8Array(4);
        uarr[0] = 0x31;
        uarr[1] = 0x32;
        uarr[2] = 0x33;
        uarr[3] = 0x34;
        let buf1 = new FastBuffer(uarr);
        let values = buf1.values();
        let va = 0x31;
        for (const value of values) {
          map.set("buffer.values() failed, expect value: " + va +", output: " + value, value === va);
          va++;
        }
    }
    {
        let buf1 = new FastBuffer("1234");
        let entries = buf1.entries();
        let va = 49, index = 0;
        for (const [key, value] of entries) {
          map.set("buffer.entries() failed, expect key: " + index +", output: " + key, key === index);
          map.set("buffer.entries() failed, expect value: " + va +", output: " + value, value === va);
          va++;
          index++;
        }
    }
    {
        let buf = new FastBuffer('this buffer is a buffer');
        let res = buf.lastIndexOf('buffer', -4);
        map.set("buffer.lastIndexOf() failed, expect: 17, output: " + res, res === 17);
    }
    {
        let buf = new FastBuffer(8);
        buf.writeUInt8(123);
        let res = buf.readUInt8(0);
        map.set("buffer.writeUInt8() failed, expect: 123, output: " + res, res === 123);
      
        buf.writeUInt16BE(100, 0);
        res = buf.readUInt16BE(0);
        map.set("buffer.readUInt16BE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeUInt16LE(100, 0);
        res = buf.readUInt16LE(0);
        map.set("buffer.readUInt16LE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeUInt32BE(100, 0);
        res = buf.readUInt32BE(0);
        map.set("buffer.writeUInt32BE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeUInt32LE(100, 0);
        res = buf.readUInt32LE(0);
        map.set("buffer.writeUInt32LE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeUIntBE(100, 2, 6);
        res = buf.readUIntBE(2, 6);
        map.set("buffer.writeUIntBE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeUIntLE(100, 2, 6);
        res = buf.readUIntLE(2, 6);
        map.set("buffer.writeUIntLE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeInt8(123);
        res = buf.readInt8(0);
        map.set("buffer.writeInt8() failed, expect: 123, output: " + res, res === 123);
      
        buf.writeInt16BE(100, 0);
        res = buf.readInt16BE(0);
        map.set("buffer.readInt16BE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeInt16LE(100, 0);
        res = buf.readInt16LE(0);
        map.set("buffer.readInt16LE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeInt32BE(100, 0);
        res = buf.readInt32BE(0);
        map.set("buffer.writeInt32BE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeInt32LE(100, 0);
        res = buf.readInt32LE(0);
        map.set("buffer.writeInt32LE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeIntBE(100, 2, 6);
        res = buf.readIntBE(2, 6);
        map.set("buffer.writeIntBE() failed, expect: 100, output: " + res, res === 100);
      
        buf.writeIntLE(100, 2, 6);
        res = buf.readIntLE(2, 6);
        map.set("buffer.writeIntLE() failed, expect: 100, output: " + res, res === 100);
      
        const eps = 1e-3;
        const eeps = 1e-6;
        buf.writeFloatBE(114.514, 0);
        map.set("buffer.writeFloatBE() failed", Math.abs(buf.readFloatBE(0) - 114.514) < eps);
      
        buf.writeFloatLE(114.514, 0);
        map.set("buffer.writeFloatLE() failed", Math.abs(buf.readFloatLE(0) - 114.514) < eps);
      
        buf.writeDoubleBE(114.5141919, 0);
        map.set("buffer.writeDoubleBE() failed", Math.abs(buf.readDoubleBE(0) - 114.5141919) < eeps);
      
        buf.writeDoubleLE(114.5141919, 0);
        map.set("buffer.writeDoubleLE() failed", Math.abs(buf.readDoubleLE(0) - 114.5141919) < eeps);
    }

    {
        let buf = new FastBuffer(20);
        map.set("buffer.length:", buf.length == 20);
    }
    {
        let buf = new FastBuffer(20);
        buf.fill("abc");
        let flag = true;
        for (let i = 0; i + 3 < buf.length; i += 3) {
            let str = buf.toString("utf8", i, i + 3);
            if (str !== "abc") {
                flag = false;
            }
        }
        map.set("buffer.fill(string):", flag);
    }
    {
        let buf = new FastBuffer(20);
        buf.fill(123);
        let flag = true;
        for (let i = 0; i < buf.length; i ++) {
            if (buf.readInt8(i) !== 123) {
                flag = false;
            }
        }
        map.set("buffer.fill(number):", flag);
    }
    {
        let buf = new FastBuffer(20);
        let buff = new FastBuffer(20);
        buff.fill("abc");
        buf.fill(buff);
        let flag = true;
        for (let i = 0; i + 3 < buf.length; i += 3) {
            let str = buf.toString("utf8", i, i + 3);
            if (str !== "abc") {
                flag = false;
            }
        }
        map.set("buffer.fill(FastBuffer):", flag);
    }
    {
        let buf = new FastBuffer(20);
        let typeArray = new Uint8Array([1,2,3,4,5]);
        buf.fill(typeArray);
        let flag = true;
        for (let i = 0; i < buf.length; i++) {
            if (buf.readInt8(i) !== typeArray[i % 5]) {
                flag = false;
            }
        }
        map.set("buffer.fill(Uint8Array):", flag);
    }
    {
        let buf1 = new FastBuffer("hello world", 'utf8');
        buf1.copy(buf1);
        let buf2 = new FastBuffer(20);
        buf1.copy(buf2);
        map.set("buffer.copy() failed", buf1.compare(buf2, 0, 11) === 0);
    }
    {
        let buf1 = new FastBuffer("hello world", 'utf8');
        let array = new Uint8Array(11);
        buf1.copy(array)
        map.set("buffer.copy() failed", buf1.compare(array, 0, 11) === 0);
        map.set("buffer.equals() failed", buf1.equals(array));
    }
    {
        let buf1 = new FastBuffer("hello world", 'utf8');
        map.set("buffer.includes(string) failed", buf1.includes("hello"));
        let buf2 = new FastBuffer("   hello world", 'utf8');
        map.set("buffer.includes(string) failed", buf2.includes("hello"));
    }
    {
        let buf1 = new FastBuffer(10);
        buf1.writeUInt8(114, 0);
        buf1.writeUInt8(514 & 255, 1);
        map.set("buffer.includes(number) failed", buf1.includes(114));
        map.set("buffer.includes(number) failed", buf1.includes(514 & 255));
    }
    {
        let buf1 = new FastBuffer(10);
        buf1.writeUInt8(114, 0);
        buf1.writeUInt8(514 & 255, 1);
        let buf2 = new FastBuffer(10);
        buf2.writeUInt8(114, 0);
        map.set("buffer.includes(buffer) failed", !buf1.includes(buf2));
        let buf3 = new FastBuffer(1);
        buf3.writeUInt8(114, 0);
        map.set("buffer.includes(buffer) failed", buf1.includes(buf3));
    }
    {
        let temp = new FastBuffer(15);
        temp.write("aGVsbG8gd29ybGQh", 0, 12, "base64");
        let str = temp.toString('utf-8', 0, 12);
        map.set("buffer.write(str, 'base64') failed", str === "hello world!");
        for (const [key, value] of temp.entries()) {
            if (key >= str.length) {
                break;
            }
            if (str[key] != String.fromCharCode(value)) {
                map.set("buffer.entries() failed", false);
                break;
            }
        }
    }
    {
        let temp = new FastBuffer("hello world hello", "utf8");
        let index = temp.indexOf('hello');
        map.set("buffer.indexOf() failed", index === 0);
        index = temp.lastIndexOf('hello');
        map.set("buffer.lastIndexOf() failed", index === 12);
        index = temp.indexOf('www');
        map.set("buffer.indexOf() failed", index === -1);
        let flag = temp.includes("hello");
        map.set("buffer.indexOf() failed", flag);
    }
    {
        let array = [1,2,3,4,5];
        let temp = new FastBuffer(array);
        for (let i = 0; i < array.length; i++) {
            if(temp[i] != array[i]) {
                map.set("buffer.from(array) failed", false);
                break;
            }
        }
    }
    {
        let str = "hello world";
        let temp = new FastBuffer(str, "utf8");
        for (let i = 0; i < str.length; i++) {
            if(String.fromCharCode(temp[i]) != str[i]) {
                map.set("buffer.from(str) failed", false);
                break;
            }
        }
    }
    {
        let typeArray = new Uint8Array([1,2,3,4,5]);
        let temp = new FastBuffer(typeArray);
        map.set("buffer.from(UInt8Array) failed", temp.buffer === typeArray.buffer);
        for (let i = 0; i < temp.length; i++) {
            if(temp[i] != typeArray[i]) {
                map.set("buffer.from(UInt8Array) failed", false);
                break;
            }
        }
    }
    {
        let array = [114,514,1919,810];
        let temp = new FastBuffer(array);
        for (let i = 0; i < array.length; i++) {
            if(temp.readInt8(i) != (array[i] & 255)) {
                map.set("buffer.from(array) failed", false);
                break;
            }
        }
    }
    {
        let buf1 = new FastBuffer("abcd", "utf8");
        let buf2 = new FastBuffer("bcde", "utf8");
        map.set("buffer.compare(target) failed", (buf1.compare(buf2) == -1) == ("abcd" < "bcde"));
        map.set("buffer.compare(target) failed", (buf2.compare(buf1) == 1) == ("abcd" > "bcde"));
        let buf3 = new FastBuffer("abcde", "utf8");
        map.set("buffer.compare(target) failed", (buf1.compare(buf3) == 1) == ("abcd" < "abcde"));
        map.set("buffer.compare(target) failed", (buf1.compare(buf1) == 0) == ("abcd" == "abcd"));
    }
    {
        const buf1 = new FastBuffer('1234', 'utf8');
        const buf2 = new FastBuffer('0123', 'utf8');
        map.set("buffer.compare() failed", buf1.compare(buf2) === 1);
    }
    {
        let buf1 = new FastBuffer("abcd", 'utf8');
        map.set("buffer.toJson() failed", JSON.stringify(buf1) === "{\"0\":97,\"1\":98,\"2\":99,\"3\":100}");
    }

    {
        let buf = new FastBuffer(12);
        let src = "hello world!";
        let srcBase64 = "aGVsbG8gd29ybGQh";
        buf.write(src);
        map.set("buf.toString failed", buf.toString() === src);
        map.set("buf.toString failed", buf.toString("base64") === srcBase64);
    }

    {
        let buf = new FastBuffer(12);
        let src = "hello world!";
        let srcBase64 = "aGVsbG8gd29ybGQh";
        buf.write(srcBase64, 0, 12, 'base64');
        map.set("buf.toString failed", buf.toString() === src);
    }

    {
        let array = new Uint8Array([252,11,3,67,237,118,91,177,43]);
        let buf = new FastBuffer(array);
        let strUrl = buf.toString("base64url");
        let str = buf.toString("base64");
        map.set("create buffer baes64 01 failed", strUrl === "_AsDQ-12W7Er");
        map.set("create buffer baes64 01 failed", str === "/AsDQ+12W7Er");

        array = new Uint8Array([2,192,254,253,5,132,69]);
        buf = new FastBuffer(array);
        strUrl = buf.toString("base64url");
        str= buf.toString("base64");
        map.set("create buffer baes64 02 failed", strUrl === "AsD-_QWERQ");
        map.set("create buffer baes64 02 failed", str === "AsD+/QWERQ==");

        array = new Uint8Array([215,109,211,97,72,142,167,241]);
        buf = new FastBuffer(array);
        strUrl = buf.toString("base64url");
        str = buf.toString("base64");
        map.set("create buffer baes64 03 failed", strUrl === "123TYUiOp_E");
        map.set("create buffer baes64 03 failed", str === "123TYUiOp/E=");

        array = new Uint8Array([252]);
        buf = new FastBuffer(array);
        strUrl = buf.toString("base64url");
        str = buf.toString("base64");
        map.set("create buffer baes64 04 failed", strUrl === "_A");
        map.set("create buffer baes64 04 failed", str === "/A==");

        array = new Uint8Array([252,97]);
        buf = new FastBuffer(array);
        strUrl = buf.toString("base64url");
        str = buf.toString("base64");
        map.set("create buffer baes64 05 failed", strUrl === "_GE");
        map.set("create buffer baes64 05 failed", str === "/GE=");

        array = new Uint8Array([252,97,142]);
        buf  = new FastBuffer(array);
        strUrl = buf.toString("base64url");
        str = buf.toString("base64");
        map.set("create buffer baes64 06 failed", strUrl === "_GGO");
        map.set("create buffer baes64 06 failed", str === "/GGO");
    }

    {
        let ab = new ArrayBuffer(100);
        let buf = new FastBuffer(ab, 9.9, 9.9);
        let res = buf.toString('hex');
        map.set("create buffer not int parameter failed", res === "000000000000000000");
    }

    {
        let buf = new FastBuffer(2);
        buf.writeInt16BE(0x1234, 0);
        try {
          let ref = buf.readInt16LE(1);
        } catch (err) {
          map.set("buffer write out of bounds error failed", err.name === 'BusinessError');
          map.set("buffer write out of bounds error failed", err.message === 'The value of "offset" is out of range. It must be >= 0 and <= 0. Received value is: 1');
        }
    }

    {
        let buf = new FastBuffer("测试特殊字符$#@!");
        let len = buf.length;
        map.set("buffer test chinese$#@! failed", len === 22);
    }

    {
        let buf = new FastBuffer([0x12, 0x34, 0x56, 0x78, 0x90, 0xab]);
        let res = buf.readIntBE(0, 6).toString(16);
        map.set("buffer.readIntBE() failed, expect: 1234567890ab, output: " + res, res === '1234567890ab');
    }

    {
        let buf = new FastBuffer([0x12, 0x34, 0x56, 0x78, 0x90, 0xab]);
        let res = buf.readIntLE(0, 6).toString(16);
        map.set("buffer.readIntLE() failed, expect: -546f87a9cbee, output: " + res, res === '-546f87a9cbee');
    }

    {
        let buf = new FastBuffer([0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x70,
            0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78]);
        let res = buf.readBigUInt64BE(0).toString();
        map.set("buffer.readBigUInt64BE() failed, expect: 7161960797921896816, output: " + res, res === '7161960797921896816');

        let buf1 = new FastBuffer(8);
        let result = buf1.writeBigUInt64BE(BigInt(0xdecafafecacefade), 0);
        map.set("buffer.readBigUInt64BE() failed, expect: 8, output: " + result, result === 8);
    }
    {
        try{
            let buf = new FastBuffer([undefined]);
        } catch (e) {
            map.set("new buffer from array failed, expect: 401, output: " + e.code, e.code === 401);
        }
    }
    {
        let buf = new FastBuffer(3).fill("$*$");
        let str = buf.toString();
        map.set("testFill0092, expect: $*$, output: " + str, str === "$*$");
    }

    {
        try {
            let buf = new FastBuffer(3).fill("$*$", -1);
        } catch (err) {
            map.set("testFill0093, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
            let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 3. Received value is: -1';
            map.set("testFill0093, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
        }
    }

    {
        try {
            let buf = new FastBuffer(3).fill("$*$", 0, 5);
        } catch (err) {
            map.set("testFill0094, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
            let expectMsg = 'The value of "end" is out of range. It must be >= 0 and <= 3. Received value is: 5';
            map.set("testFill0094, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
        }
    }

    {
    let buf = new FastBuffer(8);
    try {
        let offset = buf.write("abcde", -1);
    } catch (err) {
        map.set("testWrite0105, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 7. Received value is: -1';
        map.set("testWrite0105, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(8);
    try {
        let offset = buf.write("abcde", 1, 9);
    } catch (err) {
        map.set("testWrite0106, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "length" is out of range. It must be >= 0 and <= 7. Received value is: 9';
        map.set("testWrite0106, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf1 = new FastBuffer("1236");
    let buf2 = new FastBuffer("1235");
    let res = buf1.compare(buf2);
    map.set("testCompare0110, expect: 1, output: " + res, res === 1);
    }

    {
    let buf1 = new FastBuffer("1236");
    let buf2 = new FastBuffer("1235");
    try {
        let res = buf1.compare(buf2, -1);
    } catch (err) {
        map.set("testCompare0112, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "targetStart" is out of range. It must be >= 0 and <= 4. Received value is: -1';
        map.set("testCompare0112, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf1 = new FastBuffer("1236");
    let buf2 = new FastBuffer("1235");
    let res = buf1.equals(buf2);
    map.set("testEquals0120, expect: false, output: " + res, res === false);
    }

    {
    let buf1 = new FastBuffer("1236测试");
    let buf2 = new FastBuffer("1236测试");
    let res = buf1.equals(buf2);
    map.set("testEquals0121, expect: true, output: " + res, res === true);
    }

    {
    let buf1 = new FastBuffer("O@O");
    let buf2 = new FastBuffer("O^O");
    let res = buf1.equals(buf2);
    map.set("testEquals0122, expect: false, output: " + res, res === false);
    }

    {
    let buf1 = new FastBuffer("1236");
    let buf2 = new FastBuffer("1235");
    let num = buf1.copy(buf2);
    map.set("testCopy0140, expect: 4, output: " + num, num === 4);
    let str = buf2.toString();
    map.set("testCopy0140, expect: 1236, output: " + str, str === "1236");
    }

    {
    let buf1 = new FastBuffer("abcdefg");
    let buf2 = new FastBuffer("1235789");
    let num = buf1.copy(buf2, 2, 1, 3);
    map.set("testCopy0141, expect: 2, output: " + num, num === 2);
    let str = buf2.toString();
    map.set("testCopy0141, expect: 12bc789, output: " + str, str === "12bc789");
    }

    {
    let buf1 = new FastBuffer("123656");
    let buf2 = new FastBuffer("1235");
    let num = buf1.copy(buf2);
    map.set("testCopy0143, expect: 4, output: " + num, num === 4);
    let str = buf2.toString();
    map.set("testCopy0143, expect: 1236, output: " + str, str === "1236");
    }

    {
    let buf1 = new FastBuffer("123656");
    let buf2 = new FastBuffer("1235");
    try {
        let num = buf1.copy(buf2, -1);
    } catch (err) {
        map.set("testCopy0144, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "targetStart" is out of range. It must be >= 0 and <= 4294967295. Received value is: -1';
        map.set("testCopy0144, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf1 = new FastBuffer("ab$#");
    let buf2 = new FastBuffer("123556");
    let num = buf1.copy(buf2);
    map.set("testCopy0146, expect: 4, output: " + num, num === 4);
    let str = buf2.toString();
    map.set("testCopy0146, expect: ab$#56, output: " + str, str === "ab$#56");
    }

    {
    let buf1 = new FastBuffer("1236");
    let str = buf1.toString();
    map.set("testToString0150, expect: 1236, output: " + str, str === "1236");
    }

    {
    let buf1 = new FastBuffer("张三");
    let str = buf1.toString();
    map.set("testToString0151, expect: 张三, output: " + str, str === "张三");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("binary");
    map.set("testToString0152, expect: abc, output: " + str, str === "abc");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("ascii");
    map.set("testToString0153, expect: abc, output: " + str, str === "abc");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("base64");
    map.set("testToString0154, expect: YWJj, output: " + str, str === "YWJj");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("hex");
    map.set("testToString0155, expect: 616263, output: " + str, str === "616263");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("latin1");
    map.set("testToString0156, expect: abc, output: " + str, str === "abc");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("ucs2");
    map.set("testToString0157, expect: 扡, output: " + str, str === "扡");
    }

    {
    let buf1 = new FastBuffer("abc");
    let str = buf1.toString("utf16le");
    map.set("testToString0158, expect: 扡, output: " + str, str === "扡");
    }

    {
    let buf1 = new FastBuffer("!@#$%^&*");
    let str = buf1.toString();
    map.set("testToString0159, expect: !@#$%^&*, output: " + str, str === "!@#$%^&*");
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.indexOf("3");
    map.set("testIndexOf0170, expect: 1, output: " + index, index === 1);
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.indexOf("3", 2);
    map.set("testIndexOf0171, expect: 3, output: " + index, index === 3);
    }

    {
    let buf1 = new FastBuffer("13236235");
    let index = buf1.indexOf("23");
    map.set("testIndexOf0173, expect: 2, output: " + index, index === 2);
    }

    {
    let buf1 = new FastBuffer("测试特殊字符$#@!");
    let index = buf1.indexOf("@");
    map.set("testIndexOf0174, expect: 20, output: " + index, index === 20);
    }

    {
    let buf1 = new FastBuffer("13236235");
    let index = buf1.indexOf("23", 3);
    map.set("testIndexOf0175, expect: 5, output: " + index, index === 5);
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.indexOf("a");
    map.set("testIndexOf0176, expect: -1, output: " + index, index === -1);
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.lastIndexOf("3");
    map.set("testLastIndexOf0180, expect: 3, output: " + index, index === 3);
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.lastIndexOf("3", 2);
    map.set("testLastIndexOf0181, expect: 1, output: " + index, index === 1);
    }

    {
    let buf1 = new FastBuffer("13236235");
    let index = buf1.lastIndexOf("23");
    map.set("testLastIndexOf0183, expect: 5, output: " + index, index === 5);
    }

    {
    let buf1 = new FastBuffer("13236235");
    let index = buf1.lastIndexOf("23", 3);
    map.set("testLastIndexOf0184, expect: 2, output: " + index, index === 2);
    }

    {
    let buf1 = new FastBuffer("13236");
    let index = buf1.lastIndexOf("a");
    map.set("testLastIndexOf0186, expect: -1, output: " + index, index === -1);
    }

    {
    let buf1 = new FastBuffer("13236");
    let flag = buf1.includes("3");
    map.set("testIncludes0190, expect: true, output: " + flag, flag === true);
    }

    {
    let buf1 = new FastBuffer("13236");
    let flag = buf1.includes("3", 2);
    map.set("testIncludes0191, expect: true, output: " + flag, flag === true);
    }

    {
    let buf1 = new FastBuffer("13236");
    let flag = buf1.includes("32");
    map.set("testIncludes0193, expect: true, output: " + flag, flag === true);
    }

    {
    let buf1 = new FastBuffer("13236");
    let flag = buf1.includes("32", 2);
    map.set("testIncludes0194, expect: false, output: " + flag, flag === false);
    }

    {
    let buf1 = new FastBuffer("测试特殊字符$#@!");
    let flag = buf1.includes("#@");
    map.set("testIncludes0195, expect: true, output: " + flag, flag === true);
    flag = buf1.includes("测试");
    map.set("testIncludes0195, expect: true, output: " + flag, flag === true);
    }

    {
    let buf1 = new FastBuffer("13236");
    let flag = buf1.includes("abc");
    map.set("testIncludes0196, expect: false, output: " + flag, flag === false);
    }
    {
    let buf1 = new FastBuffer("1234");
    let keys = buf1.keys();
    let index = 0;
    for (const key of keys) {
        map.set("testKeys0230, expect: " + index + ", output: " + key, key === index);
        index++;
    }
    }

    {
    let uarr = new Uint8Array(4);
    uarr[0] = 0x31;
    uarr[1] = 0x32;
    uarr[2] = 0x33;
    uarr[3] = 0x34;
    let buf1 = new FastBuffer(uarr);
    let keys = buf1.keys();
    let index = 0;
    for (const key of keys) {
        map.set("testKeys0235, expect: " + index + ", output: " + key, key === index);
        index++;
    }
    }

    {
    let buf1 = new FastBuffer("1234");
    let keys = buf1.values();
    let va = 49;
    for (const value of keys) {
        map.set("testValues0240, expect: " + va + ", output: " + value, value === va);
        va++;
    }
    }

    {
    let uarr = new Uint8Array(4);
    uarr[0] = 0x31;
    uarr[1] = 0x32;
    uarr[2] = 0x33;
    uarr[3] = 0x34;
    let buf1 = new FastBuffer(uarr);
    let keys = buf1.values();
    let va = 0x31;
    for (const value of keys) {
        map.set("testValues0245, expect: " + va + ", output: " + value, value === va);
        va++;
    }
    }

    {
    let uarr = new Uint8Array(3);
    uarr[0] = 0x12;
    uarr[1] = 0x34;
    uarr[2] = 0x56;
    let buf = new FastBuffer(uarr);
    let str = buf.toString("hex");
    map.set("testfrom0260, expect: 123456, output: " + str, str === "123456");
    }

    {
    const buf = new FastBuffer([0x62, 0x75, 0x66, 0x66, 0x65, 0x72]);
    let str = buf.toString("hex");
    map.set("testfrom0261, expect: 627566666572, output: " + str, str === "627566666572");
    }

    {
    const arr = new Uint8Array(2);
    arr[0] = 97;
    arr[1] = 98;
    const buf = new FastBuffer(arr.buffer);
    arr[0] = 99
    buf[1] = 100
    let str = buf.toString("hex");
    map.set("testfrom0262, expect: 6364, output: " + str, str === "6364");
    map.set("testfrom0262, expect: 100, output: " + arr[1], arr[1] === 100);
    }
    {
    const buf1 = new FastBuffer('buffer');
    const buf2 = new FastBuffer(buf1);
    buf1[0] = 0x61;
    let str = buf1.toString();
    map.set("testfrom0264, expect: auffer, output: " + str, str === "auffer");
    str = buf2.toString();
    map.set("testfrom0264, expect: buffer, output: " + str, str === "buffer");
    }

    {
    const buf1 = new FastBuffer('this is a test');
    let str = buf1.toString();
    map.set("testfrom0265, expect: this is a test, output: " + str, str === "this is a test");
    }

    {
    const str = `MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQC/OgIQLaulKklB\npZltSaWwM3Cnzcmvh+LoqYFYjCePUxnoJRDusFqy4sjwBx9fn/XSxxJ9A0KxMtXa\nr7YFjwQo3FsIjLZ3+8wS+Kydcg==`;
    const buf = new FastBuffer(str, 'base64');
    let ref = buf.readInt8(buf.length - 1);
    map.set("testfrom0269, expect: 114, output: " + ref, ref === 114);
    }

    {
    let buf = new FastBuffer(4);
    let ref = buf.writeInt32BE(0x12345678, 0);
    map.set("testWriteInt32BE0310, expect: 4, output: " + ref, ref === 4);
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32BE(0x12345678, -1);
    } catch (err) {
        map.set("testWriteInt32BE0311, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0. Received value is: -1';
        map.set("testWriteInt32BE0311, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32BE(0x123456789, 0);
    } catch (err) {
        map.set("testWriteInt32BE0312, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "value" is out of range. It must be >= -2147483648 and <= 2147483647. Received value is: 4886718345';
        map.set("testWriteInt32BE0312, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32BE(0x12345678, 1);
    } catch (err) {
        map.set("testWriteInt32BE0313, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 0. Received value is: 1';
        map.set("testWriteInt32BE0313, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(4);
    let ref = buf.writeInt32LE(0x12345678, 0);
    map.set("testWriteInt32LE0320, expect: 4, output: " + ref, ref === 4);
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32LE(0x12345678, -1);
    } catch (err) {
        map.set("testWriteInt32LE0321, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0. Received value is: -1';
        map.set("testWriteInt32LE0321, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32LE(0x123456789, 0);
    } catch (err) {
        map.set("testWriteInt32LE0322, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "value" is out of range. It must be >= -2147483648 and <= 2147483647. Received value is: 4886718345';
        map.set("testWriteInt32LE0322, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(4);
    try {
        let ref = buf.writeInt32LE(0x12345678, 1);
    } catch (err) {
        map.set("testWriteInt32LE0323, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 0. Received value is: 1';
        map.set("testWriteInt32LE0323, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(2);
    let ref = buf.writeInt16BE(0x7bca, 0);
    map.set("testWriteInt16BE0330, expect: 2, output: " + ref, ref === 2);
    }

    {
    let buf = new FastBuffer(2);
    try {
        let ref = buf.writeInt16BE(0x7bca, -1);
    } catch (err) {
        map.set("testWriteInt16BE0331, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0. Received value is: -1';
        map.set("testWriteInt16BE0331, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }

    {
    let buf = new FastBuffer(2);
    try {
        let ref = buf.writeInt16BE(0x123456, 0);
    } catch (err) {
        map.set("testWriteInt16BE0332, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "value" is out of range. It must be >= -32768 and <= 32767. Received value is: 1193046';
        map.set("testWriteInt16BE0332, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
    }
{
  // 1. Create empty buffer
  let buf = new FastBuffer(0);
  map.set("testConstructorEmpty, expect: 0, output: " + buf.length, buf.length === 0);

  // 2. Initialize with specified size
  buf = new FastBuffer(5);
  map.set("testConstructorSize, expect: 5, output: " + buf.length, buf.length === 5);

  // 3. Initialize with UTF-8 string
  buf = new FastBuffer("Hello");
  map.set("testConstructorString, expect: Hello, output: " + buf.toString(), buf.toString() === "Hello");

  // 4. Initialize from array
  buf = new FastBuffer([0x48, 0x65, 0x6c, 0x6c, 0x6f]); // "Hello"
  map.set("testConstructorArray, expect: Hello, output: " + buf.toString(), buf.toString() === "Hello");

  // 5. Initialize from ArrayBuffer
  let arrBuf = new ArrayBuffer(3);
  new Uint8Array(arrBuf).set([0x41, 0x42, 0x43]); // "ABC"
  buf = new FastBuffer(arrBuf);
  map.set("testConstructorArrayBuffer, expect: ABC, output: " + buf.toString(), buf.toString() === "ABC");

  // 6. Initialize from another FastBuffer (copy)
  let bufSrc = new FastBuffer("Copy");
  buf = new FastBuffer(bufSrc);
  map.set("testConstructorFastBuffer, expect: Copy, output: " + buf.toString(), buf.toString() === "Copy");
}
{
  // 13. Write UInt8 value
  let buf = new FastBuffer(1);
  buf.writeUInt8(0xAB, 0);
  map.set("testWriteUInt8, expect: AB, output: " + buf.toString('hex'), buf.toString('hex') === "ab");

  // 14. Read UInt8 value
  let val = buf.readUInt8(0);
  map.set("testReadUInt8, expect: 171, output: " + val, val === 0xAB);

  // 15. Write UInt16BE (big-endian)
  buf = new FastBuffer(2);
  buf.writeUInt16BE(0xABCD, 0);
  map.set("testWriteUInt16BE, expect: ABCD, output: " + buf.toString('hex'), buf.toString('hex') === "abcd");

  // 16. Read UInt16BE value
  val = buf.readUInt16BE(0);
  map.set("testReadUInt16BE, expect: 43981, output: " + val, val === 0xABCD);

  // 17. Write UInt16LE (little-endian)
  buf.writeUInt16LE(0xABCD, 0);
  map.set("testWriteUInt16LE, expect: CDAB, output: " + buf.toString('hex'), buf.toString('hex') === "cdab");

  // 18. Read UInt16LE value
  val = buf.readUInt16LE(0);
  map.set("testReadUInt16LE, expect: 43981, output: " + val, val === 0xABCD);

  // 19. Write UInt32BE
  buf = new FastBuffer(4);
  buf.writeUInt32BE(0x12345678, 0);
  map.set("testWriteUInt32BE, expect: 12345678, output: " + buf.toString('hex'), buf.toString('hex') === "12345678");

  // 20. Read UInt32BE value
  val = buf.readUInt32BE(0);
  map.set("testReadUInt32BE, expect: 305419896, output: " + val, val === 0x12345678);

  // 21. Write UInt32LE
  buf.writeUInt32LE(0x12345678, 0);
  map.set("testWriteUInt32LE, expect: 78563412, output: " + buf.toString('hex'), buf.toString('hex') === "78563412");

  // 22. Read UInt32LE value
  val = buf.readUInt32LE(0);
  map.set("testReadUInt32LE, expect: 305419896, output: " + val, val === 0x12345678);

  // 23. Write Int8 value
  buf = new FastBuffer(1);
  buf.writeInt8(-85, 0); // -85 = 0xAB (two's complement)
  map.set("testWriteInt8, expect: AB, output: " + buf.toString('hex'), buf.toString('hex') === "ab");

  // 24. Read Int8 value
  val = buf.readInt8(0);
  map.set("testReadInt8, expect: -85, output: " + val, val === -85);

  // 25. Write Int16BE
  buf = new FastBuffer(2);
  buf.writeInt16BE(-21555, 0); // 0xABCD = -21555 (two's complement)
  map.set("testWriteInt16BE, expect: ABCD, output: " + buf.toString('hex'), buf.toString('hex') === "abcd");

  // 26. Read Int16BE value
  val = buf.readInt16BE(0);
  map.set("testReadInt16BE, expect: -21555, output: " + val, val === -21555);

  // 27. Write Int16LE
  buf.writeInt16LE(-21555, 0);
  map.set("testWriteInt16LE, expect: CDAB, output: " + buf.toString('hex'), buf.toString('hex') === "cdab");

  // 28. Read Int16LE value
  val = buf.readInt16LE(0);
  map.set("testReadInt16LE, expect: -21555, output: " + val, val === -21555);

  // 29. Write Int32BE
  buf = new FastBuffer(4);
  buf.writeInt32BE(-19088744, 0); // 0xFEDCBA98 = -19088744 (two's complement)
  map.set("testWriteInt32BE, expect: fedcba98, output: " + buf.toString('hex'), buf.toString('hex') === "fedcba98");

  // 30. Read Int32BE value
  val = buf.readInt32BE(0);
  map.set("testReadInt32BE, expect: -19088744, output: " + val, val === -19088744);

  // 31. Write Int32LE
  buf.writeInt32LE(-19088744, 0);
  map.set("testWriteInt32LE, expect: 98badcfe, output: " + buf.toString('hex'), buf.toString('hex') === "98badcfe");

  // 32. Read Int32LE value
  val = buf.readInt32LE(0);
  map.set("testReadInt32LE, expect: -19088744, output: " + val, val === -19088744);

  // 33. Write FloatBE
  buf = new FastBuffer(4);
  buf.writeFloatBE(123.456, 0);
  let hex = buf.toString('hex');
  map.set("testWriteFloatBE, expect: 42f6e979, output: " + hex, hex === "42f6e979");

  // 34. Read FloatBE value
  let floatVal = buf.readFloatBE(0);
  map.set("testReadFloatBE, expect: 123.456, output: " + floatVal, floatVal.toFixed(3) === "123.456");

  // 35. Write FloatLE
  buf.writeFloatLE(123.456, 0);
  hex = buf.toString('hex');
  map.set("testWriteFloatLE, expect: 79e9f642, output: " + hex, hex === "79e9f642");

  // 36. Read FloatLE value
  floatVal = buf.readFloatLE(0);
  map.set("testReadFloatLE, expect: 123.456, output: " + floatVal, floatVal.toFixed(3) === "123.456");

  // 37. Write DoubleBE
  buf = new FastBuffer(8);
  buf.writeDoubleBE(123.456, 0);
  hex = buf.toString('hex');
  map.set("testWriteDoubleBE, expect: 405edd2f1a9fbe77, output: " + hex, hex === "405edd2f1a9fbe77");

  // 38. Read DoubleBE value
  let doubleVal = buf.readDoubleBE(0);
  map.set("testReadDoubleBE, expect: 123.456, output: " + doubleVal, doubleVal.toFixed(3) === "123.456");

  // 39. Write DoubleLE
  buf.writeDoubleLE(123.456, 0);
  hex = buf.toString('hex');
  map.set("testWriteDoubleLE, expect: 77be9f1a2fdd5e40, output: " + hex, hex === "77be9f1a2fdd5e40");

  // 40. Read DoubleLE value
  doubleVal = buf.readDoubleLE(0);
  map.set("testReadDoubleLE, expect: 123.456, output: " + doubleVal, doubleVal.toFixed(3) === "123.456");
}
{
  // 41. Fill entire buffer
  let buf = new FastBuffer(5);
  buf.fill("A");
  map.set("testFillChar, expect: AAAAA, output: " + buf.toString(), buf.toString() === "AAAAA");

  // 42. Fill partial buffer
  buf.fill("B", 1, 3);
  map.set("testFillPartial, expect: ABBAA, output: " + buf.toString(), buf.toString() === "ABBAA");

  // 43. Compare buffers (less than)
  let buf1 = new FastBuffer("ABC");
  let buf2 = new FastBuffer("ABD");
  let cmp = buf1.compare(buf2);
  map.set("testCompareLess, expect: -1, output: " + cmp, cmp === -1);

  // 44. Compare buffers (equal)
  buf2 = new FastBuffer("ABC");
  cmp = buf1.compare(buf2);
  map.set("testCompareEqual, expect: 0, output: " + cmp, cmp === 0);

  // 45. Compare buffers (greater than)
  buf1 = new FastBuffer("ABD");
  cmp = buf1.compare(buf2);
  map.set("testCompareGreater, expect: 1, output: " + cmp, cmp === 1);

  // 46. Copy between buffers
  buf1 = new FastBuffer("12345");
  buf2 = new FastBuffer("ABCDE");
  buf1.copy(buf2, 1, 1, 3);
  map.set("testCopy, expect: A23DE, output: " + buf2.toString(), buf2.toString() === "A23DE");

  // 48. Convert to string
  buf = new FastBuffer([0x48, 0x65, 0x6c, 0x6c, 0x6f]);
  map.set("testToString, expect: Hello, output: " + buf.toString(), buf.toString() === "Hello");

  // 49. Convert to Base64
  buf = new FastBuffer("A");
  map.set("testToStringBase64, expect: QQ==, output: " + buf.toString('base64'), buf.toString('base64') === "QQ==");

  // 50. Convert to Hex
  buf = new FastBuffer("A");
  map.set("testToStringHex, expect: 41, output: " + buf.toString('hex'), buf.toString('hex') === "41");

  // 51. IndexOf operation
  buf = new FastBuffer("Hello World");
  let idx = buf.indexOf("World");
  map.set("testIndexOf, expect: 6, output: " + idx, idx === 6);

  // 52. LastIndexOf operation
  buf = new FastBuffer("Hello World, World");
  idx = buf.lastIndexOf("World");
  map.set("testLastIndexOf, expect: 13, output: " + idx, idx === 13);

  // 53. Includes check
  buf = new FastBuffer("Hello");
  let has = buf.includes("ell");
  map.set("testIncludes, expect: true, output: " + has, has === true);

  // 54. Equality check
  buf1 = new FastBuffer("ABC");
  buf2 = new FastBuffer("ABC");
  let isEqual = buf1.equals(buf2);
  map.set("testEquals, expect: true, output: " + isEqual, isEqual === true);

  // 58. Iterate keys
  buf = new FastBuffer("AB");
  let keys = Array.from(buf.keys());
  map.set("testKeys, expect: 0,1, output: " + keys.join(','), keys.join(',') === "0,1");

  // 59. Iterate values
  let values = Array.from(buf.values());
  map.set("testValues, expect: 65,66, output: " + values.join(','), values.join(',') === "65,66");
}
{
  // 61. Read out-of-bound (UInt8)
  let buf = new FastBuffer(1);
  try {
    buf.readUInt8(1); // Out-of-bound access
  } catch (err) {
    map.set("testReadUInt8_OutOfRange, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
  }

  // 62. Write out-of-bound (UInt8)
  try {
    buf.writeUInt8(0, 2); // Out-of-bound access
  } catch (err) {
    map.set("testWriteUInt8_OutOfRange, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
  }

  // 63. Read insufficient data (UInt16BE)
  try {
    buf.readUInt16BE(0); // Buffer too small (needs 2 bytes)
  } catch (err) {
    map.set("testReadUInt16BE_Insufficient, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
  }

  // 64. Write out-of-bound (UInt16BE)
  try {
    buf.writeUInt16BE(0xABCD, 1); // Out-of-bound access
  } catch (err) {
    map.set("testWriteUInt16BE_OutOfRange, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
  }
}

{
  // 1. Create empty FastBuffer and verify length is 0
  let buf = new FastBuffer(0);
  map.set("testConstructorEmpty, expect: 0, output: " + buf.length, buf.length === 0);
}

{
  // 2. Initialize with size 7 and validate length
  let buf = new FastBuffer(7);
  map.set("testConstructorSize, expect: 7, output: " + buf.length, buf.length === 7);
}

{
  // 3. Initialize with UTF-8 string "Γειά σου"
  let buf = new FastBuffer("Γειά σου");
  map.set("testConstructorString, expect: Γειά σου, output: " + buf.toString(), buf.toString() === "Γειά σου");
}

{
  // 4. Initialize from array [0xC0, 0xFF, 0xEE]
  let buf = new FastBuffer([0xC0, 0xFF, 0xEE]);
  let hex = buf.toString('hex');
  map.set("testConstructorArray, expect: c0ffee, output: " + hex, hex === "c0ffee");
}

{
  // 5. Initialize from ArrayBuffer
  let arrBuf = new ArrayBuffer(4);
  new Uint8Array(arrBuf).set([0xDE, 0xAD, 0xBE, 0xEF]);
  let buf = new FastBuffer(arrBuf);
  map.set("testConstructorArrayBuffer, expect: deadbeef, output: " + buf.toString('hex'), buf.toString('hex') === "deadbeef");
}

// ===== READ/WRITE OPERATIONS =====
{
  // 9. Write UInt8 value 0xF0 at offset 0
  let buf = new FastBuffer(1);
  buf.writeUInt8(0xF0, 0);
  map.set("testWriteUInt8, expect: f0, output: " + buf.toString('hex'), buf.toString('hex') === "f0");
}

{
  // 11. Write Int16LE value -32767 at offset 0
  let buf = new FastBuffer(2);
  buf.writeInt16LE(-32767, 0);
  map.set("testWriteInt16LE, expect: 0180, output: " + buf.toString('hex'), buf.toString('hex') === "0180");
}

{
  // 12. Read Int32BE value 0x7FFFFFFF
  let buf = new FastBuffer(4);
  buf.writeInt32BE(0x7FFFFFFF, 0);
  let val = buf.readInt32BE(0);
  map.set("testReadInt32BE, expect: 2147483647, output: " + val, val === 2147483647);
}

{
  // 13. Write FloatLE value -123.456
  let buf = new FastBuffer(4);
  buf.writeFloatLE(-123.456, 0);
  let hex = buf.toString('hex');
  map.set("testWriteFloatLE, expect: 79e9f6c2, output: " + hex, hex === "79e9f6c2"); // IEEE754 representation
}

{
  // 14. Read DoubleBE value 1.7976931348623157e+308 (max double)
  let buf = new FastBuffer(8);
  buf.writeDoubleBE(1.7976931348623157e308, 0);
  let val = buf.readDoubleBE(0);
  map.set("testReadDoubleBE, expect: 1.7976931348623157e+308, output: " + val, val === 1.7976931348623157e308);
}

{
  // 16. Compare buffers "apple" and "zebra"
  let buf1 = new FastBuffer("apple");
  let buf2 = new FastBuffer("zebra");
  let cmp = buf1.compare(buf2);
  map.set("testCompareStrings, expect: -1, output: " + cmp, cmp === -1);
}

// ===== ENCODING CONVERSION =====
{
  // 19. Convert to Base64url encoding
  let buf = new FastBuffer("✓ test");
  let b64url = buf.toString('base64url');
  map.set("testToBase64URL, expect: 4pyTIHRlc3Q, output: " + b64url, b64url === "4pyTIHRlc3Q");
}

{
  // 20. Convert hex string "cafebabe" to buffer
  let buf = new FastBuffer("cafebabe", 'hex');
  map.set("testFromHex, expect: cafebabe, output: " + buf.toString('hex'), buf.toString('hex') === "cafebabe");
}

// ===== ITERATORS & SEARCH =====
{
  // 21. Iterate keys of buffer "XYZ"
  let buf = new FastBuffer("XYZ");
  let keys = Array.from(buf.keys());
  map.set("testKeysIter, expect: 0,1,2, output: " + keys.join(','), keys.join(',') === "0,1,2");
}

{
  // 22. Find last index of "λ" in buffer
  let buf = new FastBuffer("αβγδλθλ");
  let idx = buf.lastIndexOf("λ");
  map.set("testLastIndexOf, expect: 12, output: " + idx, idx === 12);
}

{
  // 23. Write beyond buffer length (should throw)
  let buf = new FastBuffer(3);
  try {
    buf.writeUInt32BE(0x12345678, 0); // Requires 4 bytes
  } catch (err) {
    map.set("testWriteOOB, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
  }
}

{
  // 49. Write negative integer to Int32LE
  let buf = new FastBuffer(4);
  buf.writeInt32LE(-2147483648, 0); // Min int32
  let val = buf.readInt32LE(0);
  map.set("testWriteMinInt32, expect: -2147483648, output: " + val, val === -2147483648);
}

{
  // 50. Check includes() with partial match
  let buf = new FastBuffer("search_term");
  let found = buf.includes("term");
  map.set("testIncludesPartial, expect: true, output: " + found, found === true);
}

// Test Case 1: Fill entire buffer with pattern "AB"
{
    let buf = new FastBuffer(2).fill("AB");
    let str = buf.toString();
    map.set("testFill0500, expect: AB, output: " + str, str === "AB");
}

// Test Case 2: Attempt fill with negative offset (should throw)
{
    try {
        let buf = new FastBuffer(4).fill("XY", -2);
    } catch (err) {
        map.set("testFill0501, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 4. Received value is: -2';
        map.set("testFill0501, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 3: Attempt fill with end position exceeding buffer length
{
    try {
        let buf = new FastBuffer(3).fill("123", 0, 6);
    } catch (err) {
        map.set("testFill0502, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "end" is out of range. It must be >= 0 and <= 3. Received value is: 6';
        map.set("testFill0502, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 4: Attempt write with negative offset
{
    let buf = new FastBuffer(5);
    try {
        let offset = buf.write("test", -3);
    } catch (err) {
        map.set("testWrite0503, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 4. Received value is: -3';
        map.set("testWrite0503, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 5: Attempt write with excessive length parameter
{
    let buf = new FastBuffer(6);
    try {
        let offset = buf.write("data", 1, 8);
    } catch (err) {
        map.set("testWrite0504, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "length" is out of range. It must be >= 0 and <= 5. Received value is: 8';
        map.set("testWrite0504, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 6: Compare buffers with different content
{
    let buf1 = new FastBuffer("apple");
    let buf2 = new FastBuffer("zebra");
    let res = buf1.compare(buf2);
    map.set("testCompare0505, expect: -1, output: " + res, res === -1);
}

// Test Case 7: Attempt compare with invalid targetStart
{
    let buf1 = new FastBuffer("test");
    let buf2 = new FastBuffer("test");
    try {
        let res = buf1.compare(buf2, -5);
    } catch (err) {
        map.set("testCompare0506, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "targetStart" is out of range. It must be >= 0 and <= 4. Received value is: -5';
        map.set("testCompare0506, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 8: Verify equals() with identical Greek text
{
    let buf1 = new FastBuffer("Γειά σου");
    let buf2 = new FastBuffer("Γειά σου");
    let res = buf1.equals(buf2);
    map.set("testEquals0507, expect: true, output: " + res, res === true);
}

// Test Case 9: Verify equals() with different symbols
{
    let buf1 = new FastBuffer("@#$");
    let buf2 = new FastBuffer("!@#");
    let res = buf1.equals(buf2);
    map.set("testEquals0508, expect: false, output: " + res, res === false);
}

// Test Case 10: Copy entire buffer content
{
    let buf1 = new FastBuffer("source");
    let buf2 = new FastBuffer(6);
    let num = buf1.copy(buf2);
    map.set("testCopy0509, expect: 6, output: " + num, num === 6);
    let str = buf2.toString();
    map.set("testCopy0509, expect: source, output: " + str, str === "source");
}

// Test Case 11: Copy partial content with offsets
{
    let buf1 = new FastBuffer("abcdef");
    let buf2 = new FastBuffer("123456");
    let num = buf1.copy(buf2, 1, 2, 4);
    map.set("testCopy0510, expect: 2, output: " + num, num === 2);
    let str = buf2.toString();
    map.set("testCopy0510, expect: 1cd456, output: " + str, str === "1cd456");
}

// Test Case 12: Attempt copy with invalid targetStart
{
    let buf1 = new FastBuffer("data");
    let buf2 = new FastBuffer("1234");
    try {
        let num = buf1.copy(buf2, -10);
    } catch (err) {
        map.set("testCopy0511, expect: BusinessError, output: " + err.name, err.name === 'BusinessError');
        let expectMsg = 'The value of "targetStart" is out of range. It must be >= 0 and <= 4294967295. Received value is: -10';
        map.set("testCopy0511, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
    }
}

// Test Case 13: Convert buffer to UTF-8 string
{
    let buf = new FastBuffer("Hello 世界");
    let str = buf.toString();
    map.set("testToString0512, expect: Hello 世界, output: " + str, str === "Hello 世界");
}

// Test Case 14: Convert buffer to Base64 encoding
{
    let buf = new FastBuffer("base64");
    let str = buf.toString("base64");
    map.set("testToString0513, expect: YmFzZTY0, output: " + str, str === "YmFzZTY0");
}

// Test Case 15: Convert buffer to Hex encoding
{
    let buf = new FastBuffer("hex");
    let str = buf.toString("hex");
    map.set("testToString0514, expect: 686578, output: " + str, str === "686578");
}

// Test Case 17: Find last index of character
{
    let buf = new FastBuffer("a,b,c,d,e");
    let index = buf.lastIndexOf(",");
    map.set("testLastIndexOf0516, expect: 7, output: " + index, index === 7);
}

// Test Case 18: Check if buffer includes substring
{
    let buf = new FastBuffer("search result");
    let flag = buf.includes("result");
    map.set("testIncludes0517, expect: true, output: " + flag, flag === true);
}

// Test Case 19: Iterate buffer keys
{
    let buf = new FastBuffer("ABC");
    let keys = Array.from(buf.keys());
    map.set("testKeys0518, expect: 0,1,2, output: " + keys.join(','), keys.join(',') === "0,1,2");
}

// Test Case 20: Iterate buffer values
{
    let buf = new FastBuffer("ABC");
    let values = Array.from(buf.values());
    map.set("testValues0519, expect: 65,66,67, output: " + values.join(','), values.join(',') === "65,66,67");
}

// Test Case 21: Initialize from array buffer
{
    let arr = new Uint8Array([0x48, 0x65, 0x6c, 0x6c, 0x6f]);
    let buf = new FastBuffer(arr.buffer);
    map.set("testFrom0520, expect: Hello, output: " + buf.toString(), buf.toString() === "Hello");
}

// Test Case 24: Write FloatLE value
{
    let buf = new FastBuffer(4);
    buf.writeFloatLE(-65.432, 0);
    let hex = buf.toString('hex');
    map.set("testWriteFloatLE0523, expect: 2fdd82c2, output: " + hex, hex === "2fdd82c2");
}

// Test Case 25: Read FloatBE value
{
    let buf = new FastBuffer([0x42, 0x8A, 0x3D, 0x71]);
    let val = buf.readFloatBE(0);
    map.set("testReadFloatBE0524, expect: 69.12, output: " + val, val.toFixed(2) === "69.12");
}

// Test Case 26: Create empty buffer
{
    let buf = new FastBuffer(0);
    map.set("testConstructor0525, expect: 0, output: " + buf.length, buf.length === 0);
}

// Test Case 27: Initialize with specific size
{
    let buf = new FastBuffer(10);
    map.set("testConstructor0526, expect: 10, output: " + buf.length, buf.length === 10);
}

// Test Case 28: Initialize with emoji string
{
    let buf = new FastBuffer("🚀🌕");
    map.set("testConstructor0527, expect: 🚀🌕, output: " + buf.toString(), buf.toString() === "🚀🌕");
}

// Test Case 29: Initialize from numeric array
{
    let buf = new FastBuffer([0x31, 0x32, 0x33]);
    map.set("testConstructor0528, expect: 123, output: " + buf.toString(), buf.toString() === "123");
}

// Test Case 30: Copy between buffers with overlap
{
    let buf = new FastBuffer("ABCDEF");
    buf.copy(buf, 2, 0, 3);
    map.set("testCopy0529, expect: ABABCF, output: " + buf.toString(), buf.toString() === "ABABCF");
}

// Test Case 31: Convert to Latin1 encoding
{
    let buf = new FastBuffer([0xC0, 0xFF, 0xEE]);
    let str = buf.toString("latin1");
    map.set("testToString0530, expect: Àÿî, output: " + str, str === "Àÿî");
}

// Test Case 32: Find index with start position
{
    let buf = new FastBuffer("banana");
    let index = buf.indexOf("na", 3);
    map.set("testIndexOf0531, expect: 4, output: " + index, index === 4);
}

// Test Case 33: Check includes with offset
{
    let buf = new FastBuffer("programming");
    let flag = buf.includes("gram", 3);
    map.set("testIncludes0532, expect: true, output: " + flag, flag === true);
}

// Test Case 34: Write UInt16LE value
{
    let buf = new FastBuffer(2);
    buf.writeUInt16LE(0xABCD, 0);
    map.set("testWriteUInt16LE0533, expect: cdab, output: " + buf.toString('hex'), buf.toString('hex') === "cdab");
}

// Test Case 35: Read Int16BE value
{
    let buf = new FastBuffer([0x12, 0x34]);
    let val = buf.readInt16BE(0);
    map.set("testReadInt16BE0534, expect: 4660, output: " + val, val === 4660);
}

// Test Case 39: Create from hex string
{
    let buf = new FastBuffer("cafebabe", "hex");
    map.set("testFromHex0538, expect: cafebabe, output: " + buf.toString('hex'), buf.toString('hex') === "cafebabe");
}

// Test Case 43: Fill buffer with numeric value
{
    let buf = new FastBuffer(3);
    buf.fill(65); // ASCII 'A'
    map.set("testFill0542, expect: AAA, output: " + buf.toString(), buf.toString() === "AAA");
}

// Test Case 46: Find index of non-existent substring
{
    let buf = new FastBuffer("content");
    let index = buf.indexOf("xyz");
    map.set("testIndexOf0545, expect: -1, output: " + index, index === -1);
}

// Test Case 47: Check includes with special characters
{
    let buf = new FastBuffer("100% ✓");
    let flag = buf.includes("% ✓");
    map.set("testIncludes0546, expect: true, output: " + flag, flag === true);
}

// Test Case 48: Iterate buffer entries
{
    let buf = new FastBuffer("AB");
    let entries = Array.from(buf.entries());
    let entriesStr = entries.map(e => e.join(':')).join(';');
    map.set("testEntries0547, expect: 0:65;1:66, output: " + entriesStr, entriesStr === "0:65;1:66");
}

// Test Case 49: Write DoubleLE value
{
    let buf = new FastBuffer(8);
    buf.writeDoubleLE(123.456, 0);
    let hex = buf.toString('hex');
    map.set("testWriteDoubleLE0548, expect: 77be9f1a2fdd5e40, output: " + hex, hex === "77be9f1a2fdd5e40");
}

// Test Case 50: Read UInt32LE value
{
    let buf = new FastBuffer([0x78, 0x56, 0x34, 0x12]);
    let val = buf.readUInt32LE(0);
    map.set("testReadUInt32LE0549, expect: 305419896, output: " + val, val === 305419896);
}

// ===== fill() =====
{
  // 1. Fill with Unicode characters
  let buf = new FastBuffer(12).fill("★");
  let str = buf.toString();
  map.set("testFill1001, expect: ★★★★, output: " + str, str === "★★★★");
}

{
  // 2. Fill partial buffer with multi-byte character
  let buf = new FastBuffer(5).fill("€", 1, 4);
  let str = buf.toString();
  map.set("testFill1002, expect: \0€\0, output: " + str, str === "\0€\0");
}

{
  // 3. Fill empty buffer (no-op)
  let buf = new FastBuffer(0).fill("X");
  map.set("testFill1003, expect: 0, output: " + buf.length, buf.length === 0);
}

// ===== write() =====
{
  // 1. Write with offset exceeding buffer length
  let buf = new FastBuffer(5);
  try {
    buf.write("data", 6);
  } catch (err) {
    let expectMsg = 'The value of "offset" is out of range. It must be >= 0 and <= 4. Received value is: 6';
    map.set("testWrite1004, expect: " + expectMsg + ", output: " + err.message, err.message === expectMsg);
  }
}

{
  // 2. Write partial UTF-8 string
  let buf = new FastBuffer(10);
  buf.write("汉字测试", 2, 3);
  map.set("testWrite1005, expect: 汉, output: " + buf.toString(), buf.toString().includes("汉"));
}

{
  // 3. Write exact buffer capacity
  let buf = new FastBuffer(5);
  let written = buf.write("12345");
  map.set("testWrite1006, expect: 5, output: " + written, written === 5);
}

// ===== compare() =====
{
  // 1. Compare buffers with Unicode equality
  let buf1 = new FastBuffer("αβγ");
  let buf2 = new FastBuffer("αβγ");
  let res = buf1.compare(buf2);
  map.set("testCompare1007, expect: 0, output: " + res, res === 0);
}

{
  // 2. Compare buffers with different lengths
  let buf1 = new FastBuffer("apple");
  let buf2 = new FastBuffer("app");
  let res = buf1.compare(buf2);
  map.set("testCompare1008, expect: 1, output: " + res, res === 1);
}

{
  // 3. Compare buffers with target offset
  let buf1 = new FastBuffer("xyz");
  let buf2 = new FastBuffer("abcxyz");
  let res = buf1.compare(buf2, 3);
  map.set("testCompare1009, expect: 0, output: " + res, res === 0);
}

// ===== equals() =====
{
  // 2. Compare different encoding representations
  let buf1 = new FastBuffer("café", "utf8");
  let buf2 = new FastBuffer("café", "latin1");
  let res = buf1.equals(buf2);
  map.set("testEquals1011, expect: false, output: " + res, res === false);
}

{
  // 3. Compare buffers with same content but different memory
  let buf1 = new FastBuffer("clone");
  let buf2 = new FastBuffer(buf1);
  let res = buf1.equals(buf2);
  map.set("testEquals1012, expect: true, output: " + res, res === true);
}

// ===== copy() =====
{
  // 1. Copy to offset position
  let buf1 = new FastBuffer("XYZ");
  let buf2 = new FastBuffer("ABCDEF");
  buf1.copy(buf2, 2);
  map.set("testCopy1013, expect: ABXYZF, output: " + buf2.toString(), buf2.toString() === "ABXYZF");
}

{
  // 2. Copy with source range
  let buf1 = new FastBuffer("123456");
  let buf2 = new FastBuffer("ABCDEF");
  buf1.copy(buf2, 1, 2, 5);
  map.set("testCopy1014, expect: A345EF, output: " + buf2.toString(), buf2.toString() === "A345EF");
}

{
  // 3. Copy empty buffer
  let buf1 = new FastBuffer("");
  let buf2 = new FastBuffer("test");
  let num = buf1.copy(buf2);
  map.set("testCopy1015, expect: 0, output: " + num, num === 0);
}

// ===== toString() =====
{
  let buf = new FastBuffer("中文");
  let str = buf.toString();
  map.set("testToString1016, expect: 中文, output: " + str, str === "中文");
}

{
  // 2. Convert binary data to hex
  let buf = new FastBuffer([0x48, 0x65, 0x6c, 0x6c, 0x6f]);
  let str = buf.toString("hex");
  map.set("testToString1017, expect: 48656c6c6f, output: " + str, str === "48656c6c6f");
}

{
  // 3. Convert to Base64URL
  let buf = new FastBuffer("safe<>");
  let str = buf.toString("base64url");
  map.set("testToString1018, expect: c2FmZTw-, output: " + str, str === "c2FmZTw-");
}

// ===== indexOf() =====
{
  let buf = new FastBuffer("寻找汉字");
  let idx = buf.indexOf("汉字");
  map.set("testIndexOf1019, expect: 6, output: " + idx, idx === 6);
}

{
  // 2. Search non-existent binary pattern
  let buf = new FastBuffer([0x01, 0x02, 0x03]);
  let idx = buf.indexOf("\x04");
  map.set("testIndexOf1020, expect: -1, output: " + idx, idx === -1);
}

{
  // 3. Search with start position beyond buffer
  let buf = new FastBuffer("test");
  let idx = buf.indexOf("t", 5);
  map.set("testIndexOf1021, expect: -1, output: " + idx, idx === -1);
}

// ===== lastIndexOf() =====
{
  // 1. Find last occurrence in mixed content
  let buf = new FastBuffer("a1b2c3d4e5");
  let idx = buf.lastIndexOf("3");
  map.set("testLastIndexOf1022, expect: 5, output: " + idx, idx === 5);
}

{
  // 2. Multi-byte character search
  let buf = new FastBuffer("€10€20€30");
  let idx = buf.lastIndexOf("€");
  map.set("testLastIndexOf1023, expect: 10, output: " + idx, idx === 10);
}

{
  // 3. Search with negative start position
  let buf = new FastBuffer("test");
  let idx = buf.lastIndexOf("t", -5);
  map.set("testLastIndexOf1024, expect: -1, output: " + idx, idx === -1);
}

// ===== includes() =====
{
  // 2. Check empty buffer
  let buf = new FastBuffer("");
  let flag = buf.includes("data");
  map.set("testIncludes1026, expect: false, output: " + flag, flag === false);
}

{
  // 3. Check partial match at boundary
  let buf = new FastBuffer("boundary");
  let flag = buf.includes("ary", 5);
  map.set("testIncludes1027, expect: true, output: " + flag, flag === true);
}

// ===== keys() =====
{
  // 1. Iterate empty buffer keys
  let buf = new FastBuffer(0);
  let keys = Array.from(buf.keys());
  map.set("testKeys1028, expect: , output: " + keys.join(','), keys.join(',') === "");
}

{
  // 2. Iterate large buffer keys
  let buf = new FastBuffer(1000);
  let keyCount = 0;
  for (const key of buf.keys()) keyCount++;
  map.set("testKeys1029, expect: 1000, output: " + keyCount, keyCount === 1000);
}

{
  // 3. Verify key order
  let buf = new FastBuffer("ABC");
  let keys = Array.from(buf.keys());
  map.set("testKeys1030, expect: 0,1,2, output: " + keys.join(','), keys.join(',') === "0,1,2");
}

// ===== values() =====
{
  // 1. Iterate binary data values
  let buf = new FastBuffer([0x00, 0xFF, 0x7F]);
  let values = Array.from(buf.values());
  map.set("testValues1031, expect: 0,255,127, output: " + values.join(','), values.join(',') === "0,255,127");
}

{
  // 2. Verify Unicode code points
  let buf = new FastBuffer("Ω");
  let values = Array.from(buf.values());
  map.set("testValues1032, expect: 206,169, output: " + values.join(','), values.join(',') === "206,169");
}

{
  // 3. Empty buffer values
  let buf = new FastBuffer(0);
  let values = Array.from(buf.values());
  map.set("testValues1033, expect: , output: " + values.join(','), values.join(',') === "");
}


    {
        let size = 10;
        let buf = new FastBuffer(size);
        map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0001 failed, buffer length:" + buf.length, buf.length === 10);
    }

    {
        try {
            let size = UINT32_MAX + 1;
            let buf = new FastBuffer(size);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0002 failed", false);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0002 failed, error code:" + e.code, e.code === 10200001);
        }
    }

    {
        try {
            let buf = new FastBuffer(null);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0003 failed", false);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0003 failed, error code:" + e.code, e.code === 401);
        }
    }

    {
        try {
            let buf = new FastBuffer(10);
            buf.fill(' ');
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0004 failed, buffer length:" + buf.length, buf.length === 10);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0004 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf = new FastBuffer(10);
            buf.fill(null);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0005 failed, buffer length:" + buf.length, buf.length === 10);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0005 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf1 = new FastBuffer(10);
            let buf = new FastBuffer(10);
            buf.fill(buf1);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0006 failed, buffer length:" + buf.length, buf.length === 10);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0006 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf = new FastBuffer(10);
            buf.fill("ab$#", 0, 10, null);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0007 failed, buffer length:" + buf.length, buf.length === 10);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0007 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf = new FastBuffer(0);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0008 failed, buffer length:" + buf.length, buf.length === 0);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0008 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf1 = new FastBuffer(10);
            let buf = new FastBuffer(0);
            buf.fill(buf1);
            let flag = new FastBuffer(0);
            flag.fill(buf);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0009 failed, flag length:" + flag.length, flag.length === 0);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0009 failed, error code:" + e.code, false);
        }
    }

    {
        try {
            let buf = new FastBuffer(-5);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0010 failed", false);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0010 failed, error code:" + e.code, e.code === 10200001);
        }
    }

    {
        try {
            let buf = new FastBuffer(5.5);
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0011 failed, buffer length:", buf.length === 5);
        } catch (e) {
            map.set("SUB_COMMONLIBRARY_FASTBUFFER_BASETEST_ALLOC_0011 failed, error code:" + e.code, false);
        }
    }

    let flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    map.forEach(elements);
    if (!flag) {
        print("Test Buffer success!!!");
    } else {
        print("Test Buffer fail: " + flag);
    }
}
export let bufferRes = "Test Buffer done";
