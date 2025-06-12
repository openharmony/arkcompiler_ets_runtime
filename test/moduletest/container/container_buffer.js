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
