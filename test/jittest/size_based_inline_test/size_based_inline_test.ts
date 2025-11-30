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

function EstimateBytecodeSize(funcName: string, description: string): void {
    print(`=== ${funcName}: ${description} ===`);
}

function TinyFunc1(x: number): number {
    return x + 1;
}

function TinyFunc2(x: number): number {
    return x * 2;
}

function TinyFunc3(x: number): number {
    return x - 3;
}

function TinyFunc4(x: number): number {
    return x / 4;
}

function TinyFunc5(x: number): number {
    return x % 5;
}

function SmallFunc1(x: number): number {
    let result = x;
    result += 1;
    result *= 2;
    return result;
}

function SmallFunc2(a: number, b: number): number {
    let sum = a + b;
    let diff = a - b;
    return sum * diff;
}

function SmallFunc3(s: string): number {
    return s.length * 2;
}

function SmallFunc4(arr: number[]): number {
    return arr.length;
}

function SmallFunc5(flag: boolean): string {
    return flag ? "true" : "false";
}

function MediumFunc1(x: number): number {
    let result = x;
    for (let i = 0; i < 25; i++) {
        result += i * x;
        if (i % 2 === 0) result *= 2;
        else result -= 1;
        result = Math.sqrt(Math.abs(result));
        result = Math.pow(result, 1.5);
        result = Math.floor(result);
        result = Math.abs(result);
        result += Math.sin(i);
        result = Math.cos(i);
    }
    for (let j = 0; j < 15; j++) {
        result += j * Math.tan(j);
        result = Math.ceil(result);
        result = Math.round(result);
        result = Math.floor(result / 2);
    }
    return result;
}

function MediumFunc2(arr: number[]): number {
    let sum = 0;
    let product = 1;
    for (let i = 0; i < arr.length; i++) {
        sum += arr[i];
        product *= arr[i] || 1;
        if (i % 3 === 0) sum += Math.sqrt(arr[i]);
        if (i % 5 === 0) product = Math.floor(product / 2);
        if (i % 7 === 0) sum += Math.log(arr[i] + 1);
        if (i % 11 === 0) product = Math.pow(product, 1.2);
        if (i % 13 === 0) sum += Math.sin(arr[i]);
        if (i % 17 === 0) product = Math.cos(arr[i]);
        if (i % 19 === 0) sum += Math.tan(arr[i]);
        if (i % 23 === 0) product = Math.atan(arr[i]);
    }
    for (let k = 0; k < 12; k++) {
        sum += k * Math.exp(k);
        product += k * Math.log(k + 1);
        sum = Math.floor(sum / 3);
        product = Math.ceil(product * 1.1);
    }
    return sum + product;
}

function MediumFunc3(s: string): string {
    let result = "";
    let count = 0;
    for (let i = 0; i < s.length; i++) {
        const char = s[i];
        if (char >= 'a' && char <= 'z') {
            result += char.toUpperCase();
            count++;
        } else if (char >= 'A' && char <= 'Z') {
            result += char.toLowerCase();
            count++;
        } else {
            result += '_';
        }
        if (count % 3 === 0) result += i;
        result += char.charCodeAt(0).toString();
        result += char.charCodeAt(0).toString(16);
        result += String.fromCharCode(char.charCodeAt(0) + 1);
    }
    for (let j = 0; j < 20; j++) {
        result += String.fromCharCode(s.charCodeAt(j % s.length) + j);
        result += s.charCodeAt(j % s.length).toString(2);
        result += s.charCodeAt(j % s.length).toString(8);
    }
    for (let k = 0; k < 15; k++) {
        result = result.replace(/[aeiou]/gi, String.fromCharCode(65 + k % 26));
        result = result.replace(/[0-9]/g, String.fromCharCode(48 + k % 10));
    }
    return result;
}

function MediumFunc4(n: number): number[] {
    const result: number[] = [];
    for (let i = 0; i < n; i++) {
        if (i % 2 === 0) {
            result.push(i * i);
        } else {
            result.push(i * 2 + 1);
        }
        if (i % 7 === 0) result[result.length - 1] += 100;
        if (i % 11 === 0) result.push(-i);
        result.push(Math.sqrt(i) + Math.sin(i));
        result.push(Math.floor(Math.pow(i, 1.3)));
        result.push(Math.ceil(Math.log(i + 1)));
        result.push(Math.round(Math.exp(i % 10)));
        result.push(Math.atan(i) * 180 / Math.PI);
        result.push(Math.acos(i % 2));
        result.push(Math.asin((i % 2) - 1));
    }
    return result;
}

function MediumFunc5(x: number, y: number, z: number): { sum: number, product: number, avg: number } {
    let sum = x + y + z;
    let product = x * y * z;
    let avg = sum / 3;
    for (let i = 0; i < 20; i++) {
        sum += Math.sqrt(x + y + z + i);
        sum += Math.pow(x + i, 1.5);
        sum += Math.log(y + i + 1);
        sum += Math.sin(z + i);
        sum += Math.cos(x + i);
        sum += Math.tan(y + i);
        sum += Math.atan2(z + i, x + i);
        sum += Math.exp(i / 10);
        sum += Math.floor(Math.pow(i, 1.2));

        product *= (1 + i * 0.1);
        product += Math.sqrt(product);
        product = Math.abs(product);
        product = Math.round(product);
        product += Math.sin(i) * Math.cos(i);

        avg = Math.sqrt(sum * product);
        avg = Math.log(avg + 1);
        avg = Math.exp(avg / 100);
        avg = Math.floor(avg * 1000) / 1000;
    }
    return { sum, product, avg };
}

function LargeFunc1(x: number): number {
    let result = x;
    for (let i = 0; i < 150; i++) {
        for (let j = 0; j < 80; j++) {
            if (i % 2 === 0) {
                if (j % 3 === 0) {
                    result += i * j + x;
                } else if (j % 3 === 1) {
                    result -= i + j * x;
                } else {
                    result *= i - j + x;
                }
            } else {
                switch (j % 4) {
                    case 0: result += i - j * x; break;
                    case 1: result -= i + j + x; break;
                    case 2: result *= i * j - x; break;
                    default: result /= i + j + x + 1;
                }
            }
            result = Math.sqrt(Math.abs(result));
            result = Math.pow(result, 1.1);
            result = Math.log(result + 1);
            result = Math.exp(result / 10);
            result = Math.sin(result);
            result = Math.cos(result);
            result = Math.tan(result);
            result = Math.atan(result);
            result = Math.floor(result * 100) / 100;
            result = Math.round(result);
            result = Math.ceil(result);
            result = Math.abs(result);
        }
    }
    return result;
}

function LargeFunc2(arr: number[]): number {
    let result = 0;
    for (let i = 0; i < arr.length; i++) {
        for (let j = 0; j < 100; j++) {
            result += arr[i] * j;
            if (j % 5 === 0) result += Math.sin(j) * Math.cos(i);
            if (j % 7 === 0) result *= Math.tan(i + j);
            if (j % 11 === 0) result += Math.log(j + i);
            if (j % 13 === 0) result *= Math.exp(j % 10);
            if (j % 17 === 0) result += Math.sqrt(j * i);
            if (j % 19 === 0) result *= Math.pow(i, j % 5);
            for (let k = 0; k < 30; k++) {
                result += k * (i + j);
                result = Math.log(Math.abs(result) + 1);
                result = Math.exp(Math.sin(result));
                result = Math.cos(result);
                result = Math.sin(result);
                result = Math.tan(result);
                result = Math.sqrt(Math.abs(result));
                result = Math.pow(result, 1.2);
                result = Math.floor(result);
                result = Math.ceil(result);
                result = Math.round(result);
                if (k % 7 === 0) result = Math.abs(result);
                if (k % 11 === 0) result *= 2;
                if (k % 13 === 0) result /= 3;
            }
        }
    }
    return result;
}

function LargeFunc3(str: string): string {
    let result = "";
    const chars = str.split('');
    for (let i = 0; i < chars.length; i++) {
        for (let j = 0; j < 10; j++) {
            const char = chars[i];
            let processed = char;

            if (j % 2 === 0) {
                processed = char.toUpperCase();
            } else {
                processed = char.toLowerCase();
            }

            if (j % 3 === 0) {
                processed += i.toString();
            }

            if (j % 5 === 0) {
                processed = String.fromCharCode(char.charCodeAt(0) + 1);
            }

            result += processed;

            if (j % 7 === 0) {
                result += '_';
            }
        }
    }
    return result;
}

function LargeFunc4(n: number): number {
    let result = 0;
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < 50; j++) {
            let temp = i * j + n;
            if (i % 2 === 0) {
                temp = Math.pow(temp, 2);
            } else {
                temp = Math.sqrt(temp + 1);
            }

            if (j % 3 === 0) {
                temp = Math.sin(temp);
            } else if (j % 3 === 1) {
                temp = Math.cos(temp);
            } else {
                temp = Math.tan(temp);
            }

            result += temp;

            if (i % 10 === 0 && j % 10 === 0) {
                result = Math.abs(result);
            }
        }
    }
    return result;
}

function LargeFunc5(obj: { [key: string]: number }): { [key: string]: string } {
    const result: { [key: string]: string } = {};
    const keys = Object.keys(obj);

    for (let i = 0; i < keys.length; i++) {
        const key = keys[i];
        const value = obj[key];

        let processed = "";

        for (let j = 0; j < 20; j++) {
            if (j % 2 === 0) {
                processed += key;
            } else {
                processed += value.toString();
            }

            if (j % 3 === 0) {
                processed += "_";
            }

            if (j % 5 === 0) {
                processed = processed.toUpperCase();
            }

            if (j % 7 === 0) {
                processed = processed.toLowerCase();
            }
        }

        result[key] = processed;

        if (i % 5 === 0) {
            result[key + "_extra"] = processed.split('').reverse().join('');
        }
    }

    return result;
}

function TinyFuncTest(): void {
    print(TinyFunc1(10));
    print(TinyFunc2(20));
    print(TinyFunc3(30));
    print(TinyFunc4(40));
    print(TinyFunc5(50));
}

function SmallFuncTest(): void {
    print(SmallFunc1(5));
    print(SmallFunc2(3, 4));
    print(SmallFunc3("hello"));
    print(SmallFunc4([1, 2, 3]));
    print(SmallFunc5(true));
}

function MediumFuncTest(): void {
    print(MediumFunc1(5));
    print(MediumFunc2([1, 2, 3, 4]));
    print(MediumFunc3("test"));
    print(MediumFunc4(5).length);
    print(MediumFunc5(1, 2, 3).sum);
}

function LargeFuncTest(): void {
    print(LargeFunc1(4));
    print(LargeFunc2([1, 2, 3]));
}

function MixedSizeTest(): void {
    print(SmallFunc1(10));
    print(LargeFunc1(3));
    print(TinyFunc1(5));
    print(LargeFunc2([1, 2]));
    print(SmallFunc2(1, 2));
}

function BoundaryTest(): void {
    let result = 5;
    result = TinyFunc1(result);
    result = SmallFunc1(result);
    result = MediumFunc1(result);
    result = LargeFunc1(result);
    print(result);
}

function BoundaryJustUnder45Func(x: number): number {
    let result = x;
    result += 1;
    result *= 2;
    result -= 3;
    result = result / 4;
    result = Math.floor(result);
    result = Math.abs(result);
    result += 5;
    result *= 3;
    return result;
}

function BoundaryJustOver45Func(x: number): number {
    let result = x;
    result += 1;
    result *= 2;
    result -= 3;
    result = result / 4;
    result = Math.floor(result);
    result = Math.abs(result);
    result += 5;
    result *= 3;

    for (let i = 0; i < 5; i++) {
        result += i;
    }
    result = Math.sqrt(result + 1);
    return result;
}

function TemplateBasedSmall(size: number, x: number): number {
    let result = x;
    for (let i = 0; i < size; i++) {
        result += i;
    }
    return result;
}

function TemplateBasedMedium(size: number, x: number): number {
    let result = x;
    for (let i = 0; i < size * 2; i++) {
        result += i * x;
        if (i % 3 === 0) result *= 2;
        if (i % 5 === 0) result = Math.abs(result);
    }
    for (let j = 0; j < size; j++) {
        result -= j;
    }
    return result;
}

function TemplateBasedLarge(size: number, x: number): number {
    let result = x;
    for (let i = 0; i < size * 3; i++) {
        for (let j = 0; j < size; j++) {
            result += i * j * x;
            if (i % 2 === 0) {
                result *= 2;
            } else {
                result /= 2;
            }
            if (j % 3 === 0) {
                result = Math.sqrt(Math.abs(result));
            }
        }
    }
    return result;
}

function SmallStringFunc(text: string): string {
    return text.toUpperCase().trim().substring(0, 10);
}

function MediumStringFunc(text: string): string {
    let result = text.trim();
    result = result.toLowerCase();
    result = result.replace(/\s+/g, '_');
    result = result.substring(0, 20);
    result = result.padEnd(30, 'x');
    return result;
}

function LargeStringFunc(text: string): string {
    let result = text;

    for (let i = 0; i < 10; i++) {
        result = result.toUpperCase();
        result = result.toLowerCase();
        result = result.split('').reverse().join('');
        result = result.replace(/a/g, '@');
        result = result.replace(/e/g, '3');
        result = result.replace(/i/g, '!');
        result = result.replace(/o/g, '0');
        result = result.substring(0, 100);
        result = result.padStart(150, '0');
    }

    const chars = result.split('');
    for (let i = 0; i < chars.length; i++) {
        if (i % 5 === 0) {
            chars[i] = chars[i].toUpperCase();
        } else if (i % 3 === 0) {
            chars[i] = String.fromCharCode(chars[i].charCodeAt(0) + 1);
        }
    }

    return chars.join('');
}

function SmallArrayFunc(arr: number[]): number {
    return arr.reduce((sum, x) => sum + x, 0);
}

function MediumArrayFunc(arr: number[]): number {
    let sum = 0;
    let product = 1;
    let max = arr[0] || 0;
    let min = arr[0] || 0;

    for (let i = 0; i < arr.length; i++) {
        sum += arr[i];
        product *= arr[i] || 1;
        max = Math.max(max, arr[i]);
        min = Math.min(min, arr[i]);

        if (i % 3 === 0) sum += i;
        if (i % 5 === 0) product = Math.abs(product);
    }

    return sum + product + max + min;
}

function LargeArrayFunc(arr: number[]): number {
    let sum = 0;
    let product = 1;
    let max = arr[0] || 0;
    let min = arr[0] || 0;
    let count = 0;
    let unique = new Set<number>();

    for (let i = 0; i < arr.length; i++) {
        sum += arr[i];
        product *= arr[i] || 1;
        max = Math.max(max, arr[i]);
        min = Math.min(min, arr[i]);
        count++;
        unique.add(arr[i]);

        if (i % 2 === 0) {
            sum += Math.sqrt(Math.abs(arr[i]));
            product = Math.floor(product / 2);
        }

        if (i % 3 === 0) {
            max += arr[i];
            min = Math.max(0, min - arr[i]);
        }

        if (i % 5 === 0) {
            sum += Math.pow(arr[i], 2);
            product = Math.abs(product * arr[i]);
        }

        for (let j = 0; j < 3; j++) {
            sum += arr[i] * j;
            if (j % 2 === 0) {
                product += arr[i] + j;
            }
        }
    }

    const avg = count > 0 ? sum / count : 0;
    const range = max - min;
    const uniqueCount = unique.size;

    return sum + product + max + min + count + uniqueCount + avg + range;
}

function SmallObjectFunc(obj: { [key: string]: any }): number {
    return Object.keys(obj).length;
}

function MediumObjectFunc(obj: { [key: string]: any }): number {
    let count = 0;
    let sum = 0;
    let hasNumbers = false;
    let hasStrings = false;

    for (const key in obj) {
        count++;
        const value = obj[key];

        if (typeof value === 'number') {
            sum += value;
            hasNumbers = true;
        } else if (typeof value === 'string') {
            sum += value.length;
            hasStrings = true;
        } else if (typeof value === 'object' && value !== null) {
            sum += Object.keys(value).length;
        }

        if (key.length > 5) {
            sum += key.length;
        }
    }

    return count + sum + (hasNumbers ? 100 : 0) + (hasStrings ? 200 : 0);
}

function LargeObjectFunc(obj: { [key: string]: any }): number {
    let totalResult = 0;
    let keyCount = 0;
    let valueCount = 0;
    let nestedObjectCount = 0;
    let arrayCount = 0;
    let numberSum = 0;
    let stringLengthSum = 0;

    for (const key in obj) {
        keyCount++;
        const value = obj[key];

        totalResult += key.length;

        if (typeof value === 'number') {
            numberSum += value;
            valueCount++;
            totalResult += value;
        } else if (typeof value === 'string') {
            stringLengthSum += value.length;
            valueCount++;
            totalResult += value.length;
        } else if (typeof value === 'boolean') {
            valueCount++;
            totalResult += value ? 1 : 0;
        } else if (typeof value === 'object' && value !== null) {
            if (Array.isArray(value)) {
                arrayCount++;
                for (let i = 0; i < value.length; i++) {
                    const element = value[i];
                    if (typeof element === 'number') {
                        totalResult += element;
                        numberSum += element;
                    } else if (typeof element === 'string') {
                        totalResult += element.length;
                        stringLengthSum += element.length;
                    } else if (typeof element === 'object' && element !== null) {
                        nestedObjectCount++;
                        totalResult += Object.keys(element).length;
                    }
                    valueCount++;
                }
            } else {
                nestedObjectCount++;
                for (const nestedKey in value) {
                    totalResult += nestedKey.length;
                    const nestedValue = value[nestedKey];
                    if (typeof nestedValue === 'number') {
                        totalResult += nestedValue;
                        numberSum += nestedValue;
                    } else if (typeof nestedValue === 'string') {
                        totalResult += nestedValue.length;
                        stringLengthSum += nestedValue.length;
                    }
                    valueCount++;
                }
            }
        }

        if (keyCount % 3 === 0) {
            totalResult = Math.sqrt(Math.abs(totalResult));
        }
        if (keyCount % 5 === 0) {
            totalResult = Math.pow(totalResult, 2) / 1000;
        }
        if (keyCount % 7 === 0) {
            totalResult = Math.floor(totalResult);
        }
    }

    totalResult += keyCount + valueCount + nestedObjectCount + arrayCount + numberSum + stringLengthSum;

    return totalResult;
}

function TestBoundarySizeFunctions(): void {
    print("=== Boundary Size Function Tests ===");

    for (let i = 0; i < 1; i++) {
        print(`BoundaryJustUnder45Func(${i}): ${BoundaryJustUnder45Func(i)}`);
        print(`BoundaryJustOver45Func(${i}): ${BoundaryJustOver45Func(i)}`);
    }
}

function TestTemplateBasedSizeFunctions(): void {
    print("=== Template-based Size Function Tests ===");

    for (let i = 0; i < 1; i++) {
        print(`TemplateBasedSmall(${i}, ${i * 10}): ${TemplateBasedSmall(i, i * 10)}`);
        print(`TemplateBasedMedium(${i}, ${i * 5}): ${TemplateBasedMedium(i, i * 5)}`);
    }

    for (let i = 0; i < 1; i++) {
        print(`TemplateBasedLarge(${i}, ${i * 2}): ${TemplateBasedLarge(i, i * 2)}`);
    }
}

function TestStringSizeFunctions(): void {
    print("=== String Size Function Tests ===");

    const testStrings = [
        "short",
        "medium_length_string",
        "this_is_a_very_long_string_that_should_test_the_processing_capabilities",
        "UPPERcase lowerCase MiXeD",
        "12345!@#$% special_chars",
        "multi\nline\nstring\nwith\nnewlines",
        "string with    multiple    spaces",
        "unicode_string_αβγδε_测试_🚀"
    ];

    for (let i = 0; i < testStrings.length; i++) {
        const str = testStrings[i];
        print(`SmallStringFunc("${str}"): ${SmallStringFunc(str)}`);
        print(`MediumStringFunc("${str}"): ${MediumStringFunc(str)}`);
        print(`LargeStringFunc("${str}"): ${LargeStringFunc(str).substring(0, 50)}...`);
    }
}

function TestArraySizeFunctions(): void {
    print("=== Array Size Function Tests ===");

    const testArrays = [
        [1, 2, 3, 4, 5],
        [10, 20, 30, 40, 50, 60, 70, 80, 90, 100],
        Array.from({length: 20}, (_, i) => i + 1),
        Array.from({length: 15}, (_, i) => (i + 1) * 10),
        [1, -1, 2, -2, 3, -3, 4, -4, 5, -5]
    ];

    for (let i = 0; i < testArrays.length; i++) {
        const arr = testArrays[i];
        print(`SmallArrayFunc([${arr.join(', ')}]): ${SmallArrayFunc(arr)}`);
        print(`MediumArrayFunc([${arr.join(', ')}]): ${MediumArrayFunc(arr)}`);
        print(`LargeArrayFunc([${arr.slice(0, 10).join(', ')}...]): ${LargeArrayFunc(arr)}`);
    }
}

function TestObjectSizeFunctions(): void {
    print("=== Object Size Function Tests ===");

    const testObjects = [
        { a: 1, b: 2, c: 3 },
        {
            name: "test",
            value: 42,
            active: true,
            data: [1, 2, 3],
            nested: { x: 1, y: 2 }
        },
        {
            num1: 100,
            num2: 200,
            str1: "hello world",
            str2: "this is a longer string",
            bool1: true,
            bool2: false,
            arr1: [1, 2, 3, 4, 5],
            nested1: {
                deep1: 1,
                deep2: "nested string",
                deep3: true
            }
        },
        (() => {
            const obj: { [key: string]: any } = {};
            for (let i = 0; i < 50; i++) {
                obj[`key${i}`] = i * 10;
                obj[`str${i}`] = `string_value_${i}`;
                obj[`bool${i}`] = i % 2 === 0;
            }
            return obj;
        })()
    ];

    for (let i = 0; i < testObjects.length; i++) {
        const obj = testObjects[i];
        print(`SmallObjectFunc(obj${i}): ${SmallObjectFunc(obj)}`);
        print(`MediumObjectFunc(obj${i}): ${MediumObjectFunc(obj)}`);
        print(`LargeObjectFunc(obj${i}): ${LargeObjectFunc(obj)}`);
    }
}

function TestSizePerformanceBoundaries(): void {
    print("=== Size Performance Boundary Tests ===");

    for (let i = 0; i < 1; i++) {
        print(`BoundaryJustUnder45Func(${i * 50}): ${BoundaryJustUnder45Func(i * 50)}`);
        print(`BoundaryJustOver45Func(${i * 20}): ${BoundaryJustOver45Func(i * 20)}`);
    }
}

function ExecuteExtendedSizeTests(): void {
    print("=== Extended Size-based Inline Testing ===");

    print("Extended Phase 1: Boundary size function tests");
    TestBoundarySizeFunctions();

    print("Extended Phase 2: Template-based size function tests");
    TestTemplateBasedSizeFunctions();

    print("Extended Phase 3: String size function tests");
    TestStringSizeFunctions();

    print("Extended Phase 4: Array size function tests");
    TestArraySizeFunctions();

    print("Extended Phase 5: Object size function tests");
    TestObjectSizeFunctions();

    print("Extended Phase 6: Size performance boundary tests");
    TestSizePerformanceBoundaries();

    print("Extended size-based inline testing completed");
}

print("=== Size-based Inline Testing ===");

print("Tiny Functions Test:");
TinyFuncTest();
ArkTools.jitCompileAsync(TinyFuncTest);
print(ArkTools.waitJitCompileFinish(TinyFuncTest));
TinyFuncTest();

print("Small Functions Test:");
SmallFuncTest();
ArkTools.jitCompileAsync(SmallFuncTest);
print(ArkTools.waitJitCompileFinish(SmallFuncTest));
SmallFuncTest();

print("Medium Functions Test:");
MediumFuncTest();
ArkTools.jitCompileAsync(MediumFuncTest);
print(ArkTools.waitJitCompileFinish(MediumFuncTest));
MediumFuncTest();

print("Large Functions Test:");
LargeFuncTest();
ArkTools.jitCompileAsync(LargeFuncTest);
print(ArkTools.waitJitCompileFinish(LargeFuncTest));
LargeFuncTest();

print("Mixed Size Test:");
MixedSizeTest();
ArkTools.jitCompileAsync(MixedSizeTest);
print(ArkTools.waitJitCompileFinish(MixedSizeTest));
MixedSizeTest();

print("Boundary Test:");
BoundaryTest();
ArkTools.jitCompileAsync(BoundaryTest);
print(ArkTools.waitJitCompileFinish(BoundaryTest));
BoundaryTest();

print("Extended Tests:");
ExecuteExtendedSizeTests();
ArkTools.jitCompileAsync(ExecuteExtendedSizeTests);
print(ArkTools.waitJitCompileFinish(ExecuteExtendedSizeTests));
ExecuteExtendedSizeTests();

print("Size-based inline testing completed");