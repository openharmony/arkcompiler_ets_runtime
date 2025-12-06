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

// ==================== Transform Methods ====================

function testArrayMap(): string {
    const arr = [1, 2, 3, 4, 5];
    const doubled = arr.map((x) => x * 2);
    return doubled.join(",");
}

function testArrayMapWithIndex(): string {
    const arr = ["a", "b", "c"];
    const result = arr.map((val, idx) => `${idx}:${val}`);
    return result.join(",");
}

function testArrayMapChained(): string {
    const arr = [1, 2, 3];
    const result = arr
        .map((x) => x * 2)
        .map((x) => x + 1)
        .map((x) => x.toString());
    return result.join(",");
}

function testArrayMapWithObject(): string {
    const arr = [{ x: 1 }, { x: 2 }, { x: 3 }];
    const result = arr.map((obj) => obj.x * 10);
    return result.join(",");
}

function testArrayFilter(): string {
    const arr = [1, 2, 3, 4, 5, 6, 7, 8];
    const evens = arr.filter((x) => x % 2 === 0);
    return evens.join(",");
}

function testArrayFilterWithIndex(): string {
    const arr = [10, 20, 30, 40, 50];
    const result = arr.filter((val, idx) => idx % 2 === 0);
    return result.join(",");
}

function testArrayFilterMultiCondition(): string {
    const arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const result = arr.filter((x) => x > 3 && x < 8 && x % 2 === 0);
    return result.join(",");
}

function testArrayFilterEmpty(): string {
    const arr = [1, 2, 3];
    const result = arr.filter((x) => x > 10);
    return `length:${result.length}`;
}

function testArrayReduce(): string {
    const arr = [1, 2, 3, 4, 5];
    const sum = arr.reduce((acc, val) => acc + val, 0);
    const product = arr.reduce((acc, val) => acc * val, 1);
    return `${sum},${product}`;
}

function testArrayReduceWithoutInit(): string {
    const arr = [1, 2, 3, 4, 5];
    const sum = arr.reduce((acc, val) => acc + val);
    return `${sum}`;
}

function testArrayReduceToObject(): string {
    const arr = [
        ["a", 1],
        ["b", 2],
        ["c", 3],
    ] as [string, number][];
    const obj = arr.reduce((acc, [key, val]) => {
        acc[key] = val;
        return acc;
    }, {} as { [key: string]: number });
    return `${obj.a},${obj.b},${obj.c}`;
}

function testArrayReduceMax(): string {
    const arr = [3, 7, 2, 9, 1, 5];
    const max = arr.reduce((a, b) => (a > b ? a : b));
    const min = arr.reduce((a, b) => (a < b ? a : b));
    return `${max},${min}`;
}

function testArrayReduceRight(): string {
    const arr = ["a", "b", "c", "d"];
    const result = arr.reduceRight((acc, val) => acc + val, "");
    return result;
}

function testArrayReduceRightNested(): string {
    const arr = [
        [1, 2],
        [3, 4],
        [5, 6],
    ];
    const result = arr.reduceRight(
        (acc, val) => acc.concat(val),
        [] as number[]
    );
    return result.join(",");
}

function testArrayFlatMap(): string {
    const arr = [1, 2, 3];
    const result = arr.flatMap((x) => [x, x * 2]);
    return result.join(",");
}

function testArrayFlatMapFilter(): string {
    const arr = [1, 2, 3, 4, 5];
    const result = arr.flatMap((x) => (x % 2 === 0 ? [x, x * 10] : []));
    return result.join(",");
}

function testArrayFlatMapString(): string {
    const arr = ["hello", "world"];
    const result = arr.flatMap((s) => s.split(""));
    return result.slice(0, 5).join(",");
}

// ==================== Search Methods ====================

function testArrayFind(): string {
    const arr = [1, 5, 10, 15, 20];
    const found = arr.find((x) => x > 8);
    const notFound = arr.find((x) => x > 100);
    return `${found},${notFound}`;
}

function testArrayFindWithObject(): string {
    const arr = [
        { id: 1, name: "a" },
        { id: 2, name: "b" },
        { id: 3, name: "c" },
    ];
    const found = arr.find((obj) => obj.id === 2);
    return found ? found.name : "not found";
}

function testArrayFindIndex(): string {
    const arr = [1, 5, 10, 15, 20];
    const idx = arr.findIndex((x) => x > 8);
    const notIdx = arr.findIndex((x) => x > 100);
    return `${idx},${notIdx}`;
}

function testArrayFindIndexWithObject(): string {
    const arr = [{ val: 10 }, { val: 20 }, { val: 30 }];
    const idx = arr.findIndex((obj) => obj.val === 20);
    return `${idx}`;
}

function testArrayFindLast(): string {
    const arr = [1, 5, 10, 15, 20];
    const found = arr.findLast((x) => x < 18);
    return `${found}`;
}

function testArrayFindLastWithCondition(): string {
    const arr = [2, 4, 6, 8, 10, 12];
    const found = arr.findLast((x) => x % 4 === 0);
    return `${found}`;
}

function testArrayFindLastIndex(): string {
    const arr = [1, 5, 10, 15, 20];
    const idx = arr.findLastIndex((x) => x < 18);
    return `${idx}`;
}

function testArrayFindLastIndexMultiple(): string {
    const arr = [1, 2, 3, 2, 1, 2, 3];
    const idx = arr.findLastIndex((x) => x === 2);
    return `${idx}`;
}

function testArrayIncludes(): string {
    const arr = [1, 2, 3, NaN, 4];
    return `${arr.includes(2)},${arr.includes(5)},${arr.includes(NaN)}`;
}

function testArrayIncludesFromIndex(): string {
    const arr = [1, 2, 3, 4, 5, 3, 6];
    return `${arr.includes(3, 4)},${arr.includes(3, 6)},${arr.includes(3, -3)}`;
}

function testArrayIndexOf(): string {
    const arr = [1, 2, 3, 2, 1];
    return `${arr.indexOf(2)},${arr.indexOf(2, 2)},${arr.indexOf(5)}`;
}

function testArrayIndexOfNegative(): string {
    const arr = [1, 2, 3, 4, 5];
    return `${arr.indexOf(4, -2)},${arr.indexOf(1, -2)},${arr.indexOf(5, -1)}`;
}

function testArrayLastIndexOf(): string {
    const arr = [1, 2, 3, 2, 1];
    return `${arr.lastIndexOf(2)},${arr.lastIndexOf(1)}`;
}

function testArrayLastIndexOfFromIndex(): string {
    const arr = [1, 2, 3, 2, 1, 2];
    return `${arr.lastIndexOf(2, 3)},${arr.lastIndexOf(2, 2)},${arr.lastIndexOf(
        2,
        -2
    )}`;
}

// ==================== Check Methods ====================

function testArraySome(): string {
    const arr = [1, 2, 3, 4, 5];
    const hasEven = arr.some((x) => x % 2 === 0);
    const hasNegative = arr.some((x) => x < 0);
    return `${hasEven},${hasNegative}`;
}

function testArraySomeEmpty(): string {
    const arr: number[] = [];
    const result = arr.some((x) => x > 0);
    return `${result}`;
}

function testArraySomeWithObject(): string {
    const arr = [{ active: false }, { active: true }, { active: false }];
    const hasActive = arr.some((obj) => obj.active);
    return `${hasActive}`;
}

function testArrayEvery(): string {
    const arr = [2, 4, 6, 8, 10];
    const allEven = arr.every((x) => x % 2 === 0);
    const allPositive = arr.every((x) => x > 0);
    const allBig = arr.every((x) => x > 5);
    return `${allEven},${allPositive},${allBig}`;
}

function testArrayEveryEmpty(): string {
    const arr: number[] = [];
    const result = arr.every((x) => x > 0);
    return `${result}`;
}

function testArrayEveryWithObject(): string {
    const arr = [{ val: 10 }, { val: 20 }, { val: 30 }];
    const allPositive = arr.every((obj) => obj.val > 0);
    const allBig = arr.every((obj) => obj.val > 15);
    return `${allPositive},${allBig}`;
}

function testArrayIsArray(): string {
    return `${Array.isArray([])},${Array.isArray({})},${Array.isArray(
        "array"
    )}`;
}

function testArrayIsArrayAdvanced(): string {
    return `${Array.isArray(new Array())},${Array.isArray(
        Array.prototype
    )},${Array.isArray(null)},${Array.isArray(undefined)}`;
}

// ==================== Modify Methods ====================

function testArraySort(): string {
    const arr1 = [3, 1, 4, 1, 5, 9, 2, 6];
    const sorted = [...arr1].sort((a, b) => a - b);
    const desc = [...arr1].sort((a, b) => b - a);
    return `${sorted.slice(0, 4).join(",")},${desc.slice(0, 4).join(",")}`;
}

function testArraySortStrings(): string {
    const arr = ["banana", "apple", "cherry", "date"];
    const sorted = [...arr].sort();
    return sorted.join(",");
}

function testArraySortByProperty(): string {
    const arr = [
        { name: "c", age: 30 },
        { name: "a", age: 20 },
        { name: "b", age: 25 },
    ];
    const sortedByAge = [...arr].sort((a, b) => a.age - b.age);
    return sortedByAge.map((x) => x.name).join(",");
}

function testArraySortStable(): string {
    const arr = [
        { val: 1, id: "a" },
        { val: 2, id: "b" },
        { val: 1, id: "c" },
        { val: 2, id: "d" },
    ];
    const sorted = [...arr].sort((a, b) => a.val - b.val);
    return sorted.map((x) => x.id).join(",");
}

function testArrayReverse(): string {
    const arr = [1, 2, 3, 4, 5];
    const reversed = [...arr].reverse();
    return reversed.join(",");
}

function testArrayReverseInPlace(): string {
    const arr = ["a", "b", "c"];
    const original = arr.join(",");
    arr.reverse();
    return `${original},${arr.join(",")}`;
}

function testArraySplice(): string {
    const arr = [1, 2, 3, 4, 5];
    const removed = arr.splice(2, 2, "a", "b", "c");
    return `${removed.join(",")},${arr.join(",")}`;
}

function testArraySpliceDelete(): string {
    const arr = [1, 2, 3, 4, 5];
    const removed = arr.splice(1, 3);
    return `${removed.join(",")},${arr.join(",")}`;
}

function testArraySpliceInsert(): string {
    const arr = [1, 2, 5];
    arr.splice(2, 0, 3, 4);
    return arr.join(",");
}

function testArraySpliceNegative(): string {
    const arr = [1, 2, 3, 4, 5];
    const removed = arr.splice(-2, 1);
    return `${removed.join(",")},${arr.join(",")}`;
}

function testArraySlice(): string {
    const arr = [1, 2, 3, 4, 5];
    return `${arr.slice(1, 4).join(",")},${arr.slice(-2).join(",")}`;
}

function testArraySliceNegative(): string {
    const arr = [1, 2, 3, 4, 5];
    return `${arr.slice(-3, -1).join(",")},${arr.slice(-3).join(",")}`;
}

function testArraySliceCopy(): string {
    const arr = [1, 2, 3];
    const copy = arr.slice();
    arr[0] = 100;
    return `${arr[0]},${copy[0]}`;
}

function testArrayFill(): string {
    const arr = new Array(5).fill(0);
    arr.fill(1, 1, 4);
    return arr.join(",");
}

function testArrayFillObject(): string {
    const arr = new Array(3).fill({ x: 0 });
    arr[0].x = 1;
    return `${arr[0].x},${arr[1].x},${arr[2].x}`;
}

function testArrayFillNegativeIndex(): string {
    const arr = [1, 2, 3, 4, 5];
    arr.fill(0, -3, -1);
    return arr.join(",");
}

function testArrayCopyWithin(): string {
    const arr = [1, 2, 3, 4, 5];
    arr.copyWithin(0, 3);
    return arr.join(",");
}

function testArrayCopyWithinNegative(): string {
    const arr = [1, 2, 3, 4, 5];
    arr.copyWithin(-2, 0, 2);
    return arr.join(",");
}

function testArrayCopyWithinOverlap(): string {
    const arr = [1, 2, 3, 4, 5];
    arr.copyWithin(1, 2);
    return arr.join(",");
}

function testArrayAt(): string {
    const arr = [1, 2, 3, 4, 5];
    return `${arr.at(0)},${arr.at(-1)},${arr.at(-2)}`;
}

function testArrayAtOutOfBounds(): string {
    const arr = [1, 2, 3];
    return `${arr.at(10)},${arr.at(-10)}`;
}

// ==================== Flat Methods ====================

function testArrayFlat(): string {
    const arr = [1, [2, 3], [4, [5, 6]]];
    const flat1 = arr.flat();
    const flat2 = arr.flat(2);
    return `${flat1.join(",")},${flat2.join(",")}`;
}

function testArrayFlatDeep(): string {
    const arr = [1, [2, [3, [4, [5]]]]];
    const flatInfinity = arr.flat(Infinity);
    return flatInfinity.join(",");
}

function testArrayFlatEmpty(): string {
    const arr = [1, [2, , 4], 5];
    const result = arr.flat();
    return result.join(",");
}

function testArrayFlatWithHoles(): string {
    const arr = [1, , 3, [4, , 6]];
    const result = arr.flat();
    return `length:${result.length},${result.join(",")}`;
}

// ==================== Creation Methods ====================

function testArrayFrom(): string {
    const fromStr = Array.from("abc");
    const fromSet = Array.from(new Set([1, 2, 3]));
    const fromMap = Array.from({ length: 3 }, (_, i) => i * 2);
    return `${fromStr.join(",")},${fromSet.join(",")},${fromMap.join(",")}`;
}

function testArrayFromMapFn(): string {
    const result = Array.from([1, 2, 3], (x) => x * x);
    return result.join(",");
}

function testArrayFromIterable(): string {
    const map = new Map([
        ["a", 1],
        ["b", 2],
    ]);
    const result = Array.from(map, ([key, val]) => `${key}:${val}`);
    return result.join(",");
}

function testArrayOf(): string {
    const arr = Array.of(1, 2, 3, 4, 5);
    return arr.join(",");
}

function testArrayOfSingle(): string {
    const arr = Array.of(7);
    const arrConstructor = new Array(7);
    return `${arr.length},${arrConstructor.length}`;
}

function testArrayConstructor(): string {
    const arr1 = new Array(3);
    const arr2 = new Array(1, 2, 3);
    return `${arr1.length},${arr2.join(",")}`;
}

// ==================== Join/Concat Methods ====================

function testArrayJoin(): string {
    const arr = [1, 2, 3];
    return `${arr.join()},${arr.join("-")},${arr.join("")}`;
}

function testArrayJoinWithNull(): string {
    const arr = [1, null, undefined, 4];
    return arr.join(",");
}

function testArrayConcat(): string {
    const arr1 = [1, 2];
    const arr2 = [3, 4];
    const arr3 = [5, 6];
    return arr1.concat(arr2, arr3).join(",");
}

function testArrayConcatNested(): string {
    const arr1 = [1, [2, 3]];
    const arr2 = [[4, 5], 6];
    const result = arr1.concat(arr2);
    return `${result.length},${result[1]}`;
}

function testArrayConcatSpread(): string {
    const arr1 = [1, 2];
    const arr2 = [3, 4];
    const result = [...arr1, ...arr2, 5];
    return result.join(",");
}

// ==================== Stack/Queue Operations ====================

function testArrayPush(): string {
    const arr = [1, 2];
    const len = arr.push(3, 4, 5);
    return `${len},${arr.join(",")}`;
}

function testArrayPop(): string {
    const arr = [1, 2, 3];
    const popped = arr.pop();
    return `${popped},${arr.join(",")}`;
}

function testArrayShift(): string {
    const arr = [1, 2, 3];
    const shifted = arr.shift();
    return `${shifted},${arr.join(",")}`;
}

function testArrayUnshift(): string {
    const arr = [3, 4, 5];
    const len = arr.unshift(1, 2);
    return `${len},${arr.join(",")}`;
}

function testArrayStackOperations(): string {
    const stack: number[] = [];
    stack.push(1);
    stack.push(2);
    stack.push(3);
    const p1 = stack.pop();
    const p2 = stack.pop();
    return `${p1},${p2},${stack.join(",")}`;
}

function testArrayQueueOperations(): string {
    const queue: number[] = [];
    queue.push(1);
    queue.push(2);
    queue.push(3);
    const s1 = queue.shift();
    const s2 = queue.shift();
    return `${s1},${s2},${queue.join(",")}`;
}

// ==================== Iteration Methods ====================

function testArrayForEach(): string {
    const arr = [1, 2, 3];
    let sum = 0;
    arr.forEach((x) => (sum += x));
    return `${sum}`;
}

function testArrayForEachIndex(): string {
    const arr = ["a", "b", "c"];
    const result: string[] = [];
    arr.forEach((val, idx) => result.push(`${idx}:${val}`));
    return result.join(",");
}

function testArrayKeys(): string {
    const arr = ["a", "b", "c"];
    const keys = [...arr.keys()];
    return keys.join(",");
}

function testArrayKeysWithHoles(): string {
    const arr = [1, , 3];
    const keys = [...arr.keys()];
    return keys.join(",");
}

function testArrayValues(): string {
    const arr = ["a", "b", "c"];
    const values = [...arr.values()];
    return values.join(",");
}

function testArrayEntries(): string {
    const arr = ["a", "b"];
    const entries = [...arr.entries()];
    return entries.map(([i, v]) => `${i}:${v}`).join(",");
}

function testArrayIterator(): string {
    const arr = [1, 2, 3];
    const result: number[] = [];
    for (const val of arr) {
        result.push(val * 2);
    }
    return result.join(",");
}

// ==================== toString/toLocaleString ====================

function testArrayToString(): string {
    const arr = [1, 2, 3];
    return arr.toString();
}

function testArrayToStringNested(): string {
    const arr = [1, [2, 3], 4];
    return arr.toString();
}

// ==================== Advanced Patterns ====================

function testArrayChaining(): string {
    const arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const result = arr
        .filter((x) => x % 2 === 0)
        .map((x) => x * 2)
        .reduce((acc, val) => acc + val, 0);
    return `${result}`;
}

function testArrayUnique(): string {
    const arr = [1, 2, 2, 3, 3, 3, 4];
    const unique = [...new Set(arr)];
    return unique.join(",");
}

function testArrayIntersection(): string {
    const arr1 = [1, 2, 3, 4, 5];
    const arr2 = [3, 4, 5, 6, 7];
    const intersection = arr1.filter((x) => arr2.includes(x));
    return intersection.join(",");
}

function testArrayDifference(): string {
    const arr1 = [1, 2, 3, 4, 5];
    const arr2 = [3, 4, 5, 6, 7];
    const difference = arr1.filter((x) => !arr2.includes(x));
    return difference.join(",");
}

function testArrayGroupBy(): string {
    const arr = [1, 2, 3, 4, 5, 6];
    const groups: { [key: string]: number[] } = {};
    arr.forEach((x) => {
        const key = x % 2 === 0 ? "even" : "odd";
        if (!groups[key]) groups[key] = [];
        groups[key].push(x);
    });
    return `odd:${groups["odd"].join(",")},even:${groups["even"].join(",")}`;
}

function testArrayZip(): string {
    const arr1 = [1, 2, 3];
    const arr2 = ["a", "b", "c"];
    const zipped = arr1.map((val, idx) => [val, arr2[idx]]);
    return zipped.map(([a, b]) => `${a}${b}`).join(",");
}

function testArrayPartition(): string {
    const arr = [1, 2, 3, 4, 5, 6];
    const evens: number[] = [];
    const odds: number[] = [];
    arr.forEach((x) => (x % 2 === 0 ? evens : odds).push(x));
    return `evens:${evens.join(",")},odds:${odds.join(",")}`;
}

function testArrayChunk(): string {
    const arr = [1, 2, 3, 4, 5, 6, 7];
    const size = 3;
    const chunks: number[][] = [];
    for (let i = 0; i < arr.length; i += size) {
        chunks.push(arr.slice(i, i + size));
    }
    return chunks.map((c) => c.join(",")).join("|");
}

function testArrayShuffle(): string {
    const arr = [1, 2, 3, 4, 5];
    const shuffled = [...arr].sort(() => 0.5 - Math.random());
    return `length:${shuffled.length},includes5:${shuffled.includes(5)}`;
}

function testArrayDeepCopy(): string {
    const arr = [
        [1, 2],
        [3, 4],
    ];
    const copy = arr.map((inner) => [...inner]);
    copy[0][0] = 100;
    return `original:${arr[0][0]},copy:${copy[0][0]}`;
}

// ==================== TypedArray-like Operations ====================

function testArrayFillRange(): string {
    const arr = Array.from({ length: 5 }, (_, i) => i + 1);
    return arr.join(",");
}

function testArrayRange(): string {
    const range = (start: number, end: number) =>
        Array.from({ length: end - start }, (_, i) => i + start);
    return range(5, 10).join(",");
}

function testArrayRepeat(): string {
    const arr = Array(3).fill([1, 2]).flat();
    return arr.join(",");
}

// Warmup
for (let i = 0; i < 20; i++) {
    testArrayMap();
    testArrayMapWithIndex();
    testArrayMapChained();
    testArrayMapWithObject();
    testArrayFilter();
    testArrayFilterWithIndex();
    testArrayFilterMultiCondition();
    testArrayFilterEmpty();
    testArrayReduce();
    testArrayReduceWithoutInit();
    testArrayReduceToObject();
    testArrayReduceMax();
    testArrayReduceRight();
    testArrayReduceRightNested();
    testArrayFlatMap();
    testArrayFlatMapFilter();
    testArrayFlatMapString();
    testArrayFind();
    testArrayFindWithObject();
    testArrayFindIndex();
    testArrayFindIndexWithObject();
    testArrayFindLast();
    testArrayFindLastWithCondition();
    testArrayFindLastIndex();
    testArrayFindLastIndexMultiple();
    testArrayIncludes();
    testArrayIncludesFromIndex();
    testArrayIndexOf();
    testArrayIndexOfNegative();
    testArrayLastIndexOf();
    testArrayLastIndexOfFromIndex();
    testArraySome();
    testArraySomeEmpty();
    testArraySomeWithObject();
    testArrayEvery();
    testArrayEveryEmpty();
    testArrayEveryWithObject();
    testArrayIsArray();
    testArrayIsArrayAdvanced();
    testArraySort();
    testArraySortStrings();
    testArraySortByProperty();
    testArraySortStable();
    testArrayReverse();
    testArrayReverseInPlace();
    testArraySplice();
    testArraySpliceDelete();
    testArraySpliceInsert();
    testArraySpliceNegative();
    testArraySlice();
    testArraySliceNegative();
    testArraySliceCopy();
    testArrayFill();
    testArrayFillObject();
    testArrayFillNegativeIndex();
    testArrayCopyWithin();
    testArrayCopyWithinNegative();
    testArrayCopyWithinOverlap();
    testArrayAt();
    testArrayAtOutOfBounds();
    testArrayFlat();
    testArrayFlatDeep();
    testArrayFlatEmpty();
    testArrayFlatWithHoles();
    testArrayFrom();
    testArrayFromMapFn();
    testArrayFromIterable();
    testArrayOf();
    testArrayOfSingle();
    testArrayConstructor();
    testArrayJoin();
    testArrayJoinWithNull();
    testArrayConcat();
    testArrayConcatNested();
    testArrayConcatSpread();
    testArrayPush();
    testArrayPop();
    testArrayShift();
    testArrayUnshift();
    testArrayStackOperations();
    testArrayQueueOperations();
    testArrayForEach();
    testArrayForEachIndex();
    testArrayKeys();
    testArrayKeysWithHoles();
    testArrayValues();
    testArrayEntries();
    testArrayIterator();
    testArrayToString();
    testArrayToStringNested();
    testArrayChaining();
    testArrayUnique();
    testArrayIntersection();
    testArrayDifference();
    testArrayGroupBy();
    testArrayZip();
    testArrayPartition();
    testArrayChunk();
    testArrayShuffle();
    testArrayDeepCopy();
    testArrayFillRange();
    testArrayRange();
    testArrayRepeat();
}

// JIT compile
ArkTools.jitCompileAsync(testArrayMap);
ArkTools.jitCompileAsync(testArrayMapWithIndex);
ArkTools.jitCompileAsync(testArrayMapChained);
ArkTools.jitCompileAsync(testArrayMapWithObject);
ArkTools.jitCompileAsync(testArrayFilter);
ArkTools.jitCompileAsync(testArrayFilterWithIndex);
ArkTools.jitCompileAsync(testArrayFilterMultiCondition);
ArkTools.jitCompileAsync(testArrayFilterEmpty);
ArkTools.jitCompileAsync(testArrayReduce);
ArkTools.jitCompileAsync(testArrayReduceWithoutInit);
ArkTools.jitCompileAsync(testArrayReduceToObject);
ArkTools.jitCompileAsync(testArrayReduceMax);
ArkTools.jitCompileAsync(testArrayReduceRight);
ArkTools.jitCompileAsync(testArrayReduceRightNested);
ArkTools.jitCompileAsync(testArrayFlatMap);
ArkTools.jitCompileAsync(testArrayFlatMapFilter);
ArkTools.jitCompileAsync(testArrayFlatMapString);
ArkTools.jitCompileAsync(testArrayFind);
ArkTools.jitCompileAsync(testArrayFindWithObject);
ArkTools.jitCompileAsync(testArrayFindIndex);
ArkTools.jitCompileAsync(testArrayFindIndexWithObject);
ArkTools.jitCompileAsync(testArrayFindLast);
ArkTools.jitCompileAsync(testArrayFindLastWithCondition);
ArkTools.jitCompileAsync(testArrayFindLastIndex);
ArkTools.jitCompileAsync(testArrayFindLastIndexMultiple);
ArkTools.jitCompileAsync(testArrayIncludes);
ArkTools.jitCompileAsync(testArrayIncludesFromIndex);
ArkTools.jitCompileAsync(testArrayIndexOf);
ArkTools.jitCompileAsync(testArrayIndexOfNegative);
ArkTools.jitCompileAsync(testArrayLastIndexOf);
ArkTools.jitCompileAsync(testArrayLastIndexOfFromIndex);
ArkTools.jitCompileAsync(testArraySome);
ArkTools.jitCompileAsync(testArraySomeEmpty);
ArkTools.jitCompileAsync(testArraySomeWithObject);
ArkTools.jitCompileAsync(testArrayEvery);
ArkTools.jitCompileAsync(testArrayEveryEmpty);
ArkTools.jitCompileAsync(testArrayEveryWithObject);
ArkTools.jitCompileAsync(testArrayIsArray);
ArkTools.jitCompileAsync(testArrayIsArrayAdvanced);
ArkTools.jitCompileAsync(testArraySort);
ArkTools.jitCompileAsync(testArraySortStrings);
ArkTools.jitCompileAsync(testArraySortByProperty);
ArkTools.jitCompileAsync(testArraySortStable);
ArkTools.jitCompileAsync(testArrayReverse);
ArkTools.jitCompileAsync(testArrayReverseInPlace);
ArkTools.jitCompileAsync(testArraySplice);
ArkTools.jitCompileAsync(testArraySpliceDelete);
ArkTools.jitCompileAsync(testArraySpliceInsert);
ArkTools.jitCompileAsync(testArraySpliceNegative);
ArkTools.jitCompileAsync(testArraySlice);
ArkTools.jitCompileAsync(testArraySliceNegative);
ArkTools.jitCompileAsync(testArraySliceCopy);
ArkTools.jitCompileAsync(testArrayFill);
ArkTools.jitCompileAsync(testArrayFillObject);
ArkTools.jitCompileAsync(testArrayFillNegativeIndex);
ArkTools.jitCompileAsync(testArrayCopyWithin);
ArkTools.jitCompileAsync(testArrayCopyWithinNegative);
ArkTools.jitCompileAsync(testArrayCopyWithinOverlap);
ArkTools.jitCompileAsync(testArrayAt);
ArkTools.jitCompileAsync(testArrayAtOutOfBounds);
ArkTools.jitCompileAsync(testArrayFlat);
ArkTools.jitCompileAsync(testArrayFlatDeep);
ArkTools.jitCompileAsync(testArrayFlatEmpty);
ArkTools.jitCompileAsync(testArrayFlatWithHoles);
ArkTools.jitCompileAsync(testArrayFrom);
ArkTools.jitCompileAsync(testArrayFromMapFn);
ArkTools.jitCompileAsync(testArrayFromIterable);
ArkTools.jitCompileAsync(testArrayOf);
ArkTools.jitCompileAsync(testArrayOfSingle);
ArkTools.jitCompileAsync(testArrayConstructor);
ArkTools.jitCompileAsync(testArrayJoin);
ArkTools.jitCompileAsync(testArrayJoinWithNull);
ArkTools.jitCompileAsync(testArrayConcat);
ArkTools.jitCompileAsync(testArrayConcatNested);
ArkTools.jitCompileAsync(testArrayConcatSpread);
ArkTools.jitCompileAsync(testArrayPush);
ArkTools.jitCompileAsync(testArrayPop);
ArkTools.jitCompileAsync(testArrayShift);
ArkTools.jitCompileAsync(testArrayUnshift);
ArkTools.jitCompileAsync(testArrayStackOperations);
ArkTools.jitCompileAsync(testArrayQueueOperations);
ArkTools.jitCompileAsync(testArrayForEach);
ArkTools.jitCompileAsync(testArrayForEachIndex);
ArkTools.jitCompileAsync(testArrayKeys);
ArkTools.jitCompileAsync(testArrayKeysWithHoles);
ArkTools.jitCompileAsync(testArrayValues);
ArkTools.jitCompileAsync(testArrayEntries);
ArkTools.jitCompileAsync(testArrayIterator);
ArkTools.jitCompileAsync(testArrayToString);
ArkTools.jitCompileAsync(testArrayToStringNested);
ArkTools.jitCompileAsync(testArrayChaining);
ArkTools.jitCompileAsync(testArrayUnique);
ArkTools.jitCompileAsync(testArrayIntersection);
ArkTools.jitCompileAsync(testArrayDifference);
ArkTools.jitCompileAsync(testArrayGroupBy);
ArkTools.jitCompileAsync(testArrayZip);
ArkTools.jitCompileAsync(testArrayPartition);
ArkTools.jitCompileAsync(testArrayChunk);
ArkTools.jitCompileAsync(testArrayShuffle);
ArkTools.jitCompileAsync(testArrayDeepCopy);
ArkTools.jitCompileAsync(testArrayFillRange);
ArkTools.jitCompileAsync(testArrayRange);
ArkTools.jitCompileAsync(testArrayRepeat);

print("compile testArrayMap: " + ArkTools.waitJitCompileFinish(testArrayMap));
print("compile testArrayMapWithIndex: " + ArkTools.waitJitCompileFinish(testArrayMapWithIndex));
print("compile testArrayMapChained: " + ArkTools.waitJitCompileFinish(testArrayMapChained));
print("compile testArrayMapWithObject: " + ArkTools.waitJitCompileFinish(testArrayMapWithObject));
print("compile testArrayFilter: " + ArkTools.waitJitCompileFinish(testArrayFilter));
print("compile testArrayFilterWithIndex: " + ArkTools.waitJitCompileFinish(testArrayFilterWithIndex));
print("compile testArrayFilterMultiCondition: " + ArkTools.waitJitCompileFinish(testArrayFilterMultiCondition));
print("compile testArrayFilterEmpty: " + ArkTools.waitJitCompileFinish(testArrayFilterEmpty));
print("compile testArrayReduce: " + ArkTools.waitJitCompileFinish(testArrayReduce));
print("compile testArrayReduceWithoutInit: " + ArkTools.waitJitCompileFinish(testArrayReduceWithoutInit));
print("compile testArrayReduceToObject: " + ArkTools.waitJitCompileFinish(testArrayReduceToObject));
print("compile testArrayReduceMax: " + ArkTools.waitJitCompileFinish(testArrayReduceMax));
print("compile testArrayReduceRight: " + ArkTools.waitJitCompileFinish(testArrayReduceRight));
print("compile testArrayReduceRightNested: " + ArkTools.waitJitCompileFinish(testArrayReduceRightNested));
print("compile testArrayFlatMap: " + ArkTools.waitJitCompileFinish(testArrayFlatMap));
print("compile testArrayFlatMapFilter: " + ArkTools.waitJitCompileFinish(testArrayFlatMapFilter));
print("compile testArrayFlatMapString: " + ArkTools.waitJitCompileFinish(testArrayFlatMapString));
print("compile testArrayFind: " + ArkTools.waitJitCompileFinish(testArrayFind));
print("compile testArrayFindWithObject: " + ArkTools.waitJitCompileFinish(testArrayFindWithObject));
print("compile testArrayFindIndex: " + ArkTools.waitJitCompileFinish(testArrayFindIndex));
print("compile testArrayFindIndexWithObject: " + ArkTools.waitJitCompileFinish(testArrayFindIndexWithObject));
print("compile testArrayFindLast: " + ArkTools.waitJitCompileFinish(testArrayFindLast));
print("compile testArrayFindLastWithCondition: " + ArkTools.waitJitCompileFinish(testArrayFindLastWithCondition));
print("compile testArrayFindLastIndex: " + ArkTools.waitJitCompileFinish(testArrayFindLastIndex));
print("compile testArrayFindLastIndexMultiple: " + ArkTools.waitJitCompileFinish(testArrayFindLastIndexMultiple));
print("compile testArrayIncludes: " + ArkTools.waitJitCompileFinish(testArrayIncludes));
print("compile testArrayIncludesFromIndex: " + ArkTools.waitJitCompileFinish(testArrayIncludesFromIndex));
print("compile testArrayIndexOf: " + ArkTools.waitJitCompileFinish(testArrayIndexOf));
print("compile testArrayIndexOfNegative: " + ArkTools.waitJitCompileFinish(testArrayIndexOfNegative));
print("compile testArrayLastIndexOf: " + ArkTools.waitJitCompileFinish(testArrayLastIndexOf));
print("compile testArrayLastIndexOfFromIndex: " + ArkTools.waitJitCompileFinish(testArrayLastIndexOfFromIndex));
print("compile testArraySome: " + ArkTools.waitJitCompileFinish(testArraySome));
print("compile testArraySomeEmpty: " + ArkTools.waitJitCompileFinish(testArraySomeEmpty));
print("compile testArraySomeWithObject: " + ArkTools.waitJitCompileFinish(testArraySomeWithObject));
print("compile testArrayEvery: " + ArkTools.waitJitCompileFinish(testArrayEvery));
print("compile testArrayEveryEmpty: " + ArkTools.waitJitCompileFinish(testArrayEveryEmpty));
print("compile testArrayEveryWithObject: " + ArkTools.waitJitCompileFinish(testArrayEveryWithObject));
print("compile testArrayIsArray: " + ArkTools.waitJitCompileFinish(testArrayIsArray));
print("compile testArrayIsArrayAdvanced: " + ArkTools.waitJitCompileFinish(testArrayIsArrayAdvanced));
print("compile testArraySort: " + ArkTools.waitJitCompileFinish(testArraySort));
print("compile testArraySortStrings: " + ArkTools.waitJitCompileFinish(testArraySortStrings));
print("compile testArraySortByProperty: " + ArkTools.waitJitCompileFinish(testArraySortByProperty));
print("compile testArraySortStable: " + ArkTools.waitJitCompileFinish(testArraySortStable));
print("compile testArrayReverse: " + ArkTools.waitJitCompileFinish(testArrayReverse));
print("compile testArrayReverseInPlace: " + ArkTools.waitJitCompileFinish(testArrayReverseInPlace));
print("compile testArraySplice: " + ArkTools.waitJitCompileFinish(testArraySplice));
print("compile testArraySpliceDelete: " + ArkTools.waitJitCompileFinish(testArraySpliceDelete));
print("compile testArraySpliceInsert: " + ArkTools.waitJitCompileFinish(testArraySpliceInsert));
print("compile testArraySpliceNegative: " + ArkTools.waitJitCompileFinish(testArraySpliceNegative));
print("compile testArraySlice: " + ArkTools.waitJitCompileFinish(testArraySlice));
print("compile testArraySliceNegative: " + ArkTools.waitJitCompileFinish(testArraySliceNegative));
print("compile testArraySliceCopy: " + ArkTools.waitJitCompileFinish(testArraySliceCopy));
print("compile testArrayFill: " + ArkTools.waitJitCompileFinish(testArrayFill));
print("compile testArrayFillObject: " + ArkTools.waitJitCompileFinish(testArrayFillObject));
print("compile testArrayFillNegativeIndex: " + ArkTools.waitJitCompileFinish(testArrayFillNegativeIndex));
print("compile testArrayCopyWithin: " + ArkTools.waitJitCompileFinish(testArrayCopyWithin));
print("compile testArrayCopyWithinNegative: " + ArkTools.waitJitCompileFinish(testArrayCopyWithinNegative));
print("compile testArrayCopyWithinOverlap: " + ArkTools.waitJitCompileFinish(testArrayCopyWithinOverlap));
print("compile testArrayAt: " + ArkTools.waitJitCompileFinish(testArrayAt));
print("compile testArrayAtOutOfBounds: " + ArkTools.waitJitCompileFinish(testArrayAtOutOfBounds));
print("compile testArrayFlat: " + ArkTools.waitJitCompileFinish(testArrayFlat));
print("compile testArrayFlatDeep: " + ArkTools.waitJitCompileFinish(testArrayFlatDeep));
print("compile testArrayFlatEmpty: " + ArkTools.waitJitCompileFinish(testArrayFlatEmpty));
print("compile testArrayFlatWithHoles: " + ArkTools.waitJitCompileFinish(testArrayFlatWithHoles));
print("compile testArrayFrom: " + ArkTools.waitJitCompileFinish(testArrayFrom));
print("compile testArrayFromMapFn: " + ArkTools.waitJitCompileFinish(testArrayFromMapFn));
print("compile testArrayFromIterable: " + ArkTools.waitJitCompileFinish(testArrayFromIterable));
print("compile testArrayOf: " + ArkTools.waitJitCompileFinish(testArrayOf));
print("compile testArrayOfSingle: " + ArkTools.waitJitCompileFinish(testArrayOfSingle));
print("compile testArrayConstructor: " + ArkTools.waitJitCompileFinish(testArrayConstructor));
print("compile testArrayJoin: " + ArkTools.waitJitCompileFinish(testArrayJoin));
print("compile testArrayJoinWithNull: " + ArkTools.waitJitCompileFinish(testArrayJoinWithNull));
print("compile testArrayConcat: " + ArkTools.waitJitCompileFinish(testArrayConcat));
print("compile testArrayConcatNested: " + ArkTools.waitJitCompileFinish(testArrayConcatNested));
print("compile testArrayConcatSpread: " + ArkTools.waitJitCompileFinish(testArrayConcatSpread));
print("compile testArrayPush: " + ArkTools.waitJitCompileFinish(testArrayPush));
print("compile testArrayPop: " + ArkTools.waitJitCompileFinish(testArrayPop));
print("compile testArrayShift: " + ArkTools.waitJitCompileFinish(testArrayShift));
print("compile testArrayUnshift: " + ArkTools.waitJitCompileFinish(testArrayUnshift));
print("compile testArrayStackOperations: " + ArkTools.waitJitCompileFinish(testArrayStackOperations));
print("compile testArrayQueueOperations: " + ArkTools.waitJitCompileFinish(testArrayQueueOperations));
print("compile testArrayForEach: " + ArkTools.waitJitCompileFinish(testArrayForEach));
print("compile testArrayForEachIndex: " + ArkTools.waitJitCompileFinish(testArrayForEachIndex));
print("compile testArrayKeys: " + ArkTools.waitJitCompileFinish(testArrayKeys));
print("compile testArrayKeysWithHoles: " + ArkTools.waitJitCompileFinish(testArrayKeysWithHoles));
print("compile testArrayValues: " + ArkTools.waitJitCompileFinish(testArrayValues));
print("compile testArrayEntries: " + ArkTools.waitJitCompileFinish(testArrayEntries));
print("compile testArrayIterator: " + ArkTools.waitJitCompileFinish(testArrayIterator));
print("compile testArrayToString: " + ArkTools.waitJitCompileFinish(testArrayToString));
print("compile testArrayToStringNested: " + ArkTools.waitJitCompileFinish(testArrayToStringNested));
print("compile testArrayChaining: " + ArkTools.waitJitCompileFinish(testArrayChaining));
print("compile testArrayUnique: " + ArkTools.waitJitCompileFinish(testArrayUnique));
print("compile testArrayIntersection: " + ArkTools.waitJitCompileFinish(testArrayIntersection));
print("compile testArrayDifference: " + ArkTools.waitJitCompileFinish(testArrayDifference));
print("compile testArrayGroupBy: " + ArkTools.waitJitCompileFinish(testArrayGroupBy));
print("compile testArrayZip: " + ArkTools.waitJitCompileFinish(testArrayZip));
print("compile testArrayPartition: " + ArkTools.waitJitCompileFinish(testArrayPartition));
print("compile testArrayChunk: " + ArkTools.waitJitCompileFinish(testArrayChunk));
print("compile testArrayShuffle: " + ArkTools.waitJitCompileFinish(testArrayShuffle));
print("compile testArrayDeepCopy: " + ArkTools.waitJitCompileFinish(testArrayDeepCopy));
print("compile testArrayFillRange: " + ArkTools.waitJitCompileFinish(testArrayFillRange));
print("compile testArrayRange: " + ArkTools.waitJitCompileFinish(testArrayRange));
print("compile testArrayRepeat: " + ArkTools.waitJitCompileFinish(testArrayRepeat));

// Verify
print("testArrayMap: " + testArrayMap());
print("testArrayMapWithIndex: " + testArrayMapWithIndex());
print("testArrayMapChained: " + testArrayMapChained());
print("testArrayMapWithObject: " + testArrayMapWithObject());
print("testArrayFilter: " + testArrayFilter());
print("testArrayFilterWithIndex: " + testArrayFilterWithIndex());
print("testArrayFilterMultiCondition: " + testArrayFilterMultiCondition());
print("testArrayFilterEmpty: " + testArrayFilterEmpty());
print("testArrayReduce: " + testArrayReduce());
print("testArrayReduceWithoutInit: " + testArrayReduceWithoutInit());
print("testArrayReduceToObject: " + testArrayReduceToObject());
print("testArrayReduceMax: " + testArrayReduceMax());
print("testArrayReduceRight: " + testArrayReduceRight());
print("testArrayReduceRightNested: " + testArrayReduceRightNested());
print("testArrayFlatMap: " + testArrayFlatMap());
print("testArrayFlatMapFilter: " + testArrayFlatMapFilter());
print("testArrayFlatMapString: " + testArrayFlatMapString());
print("testArrayFind: " + testArrayFind());
print("testArrayFindWithObject: " + testArrayFindWithObject());
print("testArrayFindIndex: " + testArrayFindIndex());
print("testArrayFindIndexWithObject: " + testArrayFindIndexWithObject());
print("testArrayFindLast: " + testArrayFindLast());
print("testArrayFindLastWithCondition: " + testArrayFindLastWithCondition());
print("testArrayFindLastIndex: " + testArrayFindLastIndex());
print("testArrayFindLastIndexMultiple: " + testArrayFindLastIndexMultiple());
print("testArrayIncludes: " + testArrayIncludes());
print("testArrayIncludesFromIndex: " + testArrayIncludesFromIndex());
print("testArrayIndexOf: " + testArrayIndexOf());
print("testArrayIndexOfNegative: " + testArrayIndexOfNegative());
print("testArrayLastIndexOf: " + testArrayLastIndexOf());
print("testArrayLastIndexOfFromIndex: " + testArrayLastIndexOfFromIndex());
print("testArraySome: " + testArraySome());
print("testArraySomeEmpty: " + testArraySomeEmpty());
print("testArraySomeWithObject: " + testArraySomeWithObject());
print("testArrayEvery: " + testArrayEvery());
print("testArrayEveryEmpty: " + testArrayEveryEmpty());
print("testArrayEveryWithObject: " + testArrayEveryWithObject());
print("testArrayIsArray: " + testArrayIsArray());
print("testArrayIsArrayAdvanced: " + testArrayIsArrayAdvanced());
print("testArraySort: " + testArraySort());
print("testArraySortStrings: " + testArraySortStrings());
print("testArraySortByProperty: " + testArraySortByProperty());
print("testArraySortStable: " + testArraySortStable());
print("testArrayReverse: " + testArrayReverse());
print("testArrayReverseInPlace: " + testArrayReverseInPlace());
print("testArraySplice: " + testArraySplice());
print("testArraySpliceDelete: " + testArraySpliceDelete());
print("testArraySpliceInsert: " + testArraySpliceInsert());
print("testArraySpliceNegative: " + testArraySpliceNegative());
print("testArraySlice: " + testArraySlice());
print("testArraySliceNegative: " + testArraySliceNegative());
print("testArraySliceCopy: " + testArraySliceCopy());
print("testArrayFill: " + testArrayFill());
print("testArrayFillObject: " + testArrayFillObject());
print("testArrayFillNegativeIndex: " + testArrayFillNegativeIndex());
print("testArrayCopyWithin: " + testArrayCopyWithin());
print("testArrayCopyWithinNegative: " + testArrayCopyWithinNegative());
print("testArrayCopyWithinOverlap: " + testArrayCopyWithinOverlap());
print("testArrayAt: " + testArrayAt());
print("testArrayAtOutOfBounds: " + testArrayAtOutOfBounds());
print("testArrayFlat: " + testArrayFlat());
print("testArrayFlatDeep: " + testArrayFlatDeep());
print("testArrayFlatEmpty: " + testArrayFlatEmpty());
print("testArrayFlatWithHoles: " + testArrayFlatWithHoles());
print("testArrayFrom: " + testArrayFrom());
print("testArrayFromMapFn: " + testArrayFromMapFn());
print("testArrayFromIterable: " + testArrayFromIterable());
print("testArrayOf: " + testArrayOf());
print("testArrayOfSingle: " + testArrayOfSingle());
print("testArrayConstructor: " + testArrayConstructor());
print("testArrayJoin: " + testArrayJoin());
print("testArrayJoinWithNull: " + testArrayJoinWithNull());
print("testArrayConcat: " + testArrayConcat());
print("testArrayConcatNested: " + testArrayConcatNested());
print("testArrayConcatSpread: " + testArrayConcatSpread());
print("testArrayPush: " + testArrayPush());
print("testArrayPop: " + testArrayPop());
print("testArrayShift: " + testArrayShift());
print("testArrayUnshift: " + testArrayUnshift());
print("testArrayStackOperations: " + testArrayStackOperations());
print("testArrayQueueOperations: " + testArrayQueueOperations());
print("testArrayForEach: " + testArrayForEach());
print("testArrayForEachIndex: " + testArrayForEachIndex());
print("testArrayKeys: " + testArrayKeys());
print("testArrayKeysWithHoles: " + testArrayKeysWithHoles());
print("testArrayValues: " + testArrayValues());
print("testArrayEntries: " + testArrayEntries());
print("testArrayIterator: " + testArrayIterator());
print("testArrayToString: " + testArrayToString());
print("testArrayToStringNested: " + testArrayToStringNested());
print("testArrayChaining: " + testArrayChaining());
print("testArrayUnique: " + testArrayUnique());
print("testArrayIntersection: " + testArrayIntersection());
print("testArrayDifference: " + testArrayDifference());
print("testArrayGroupBy: " + testArrayGroupBy());
print("testArrayZip: " + testArrayZip());
print("testArrayPartition: " + testArrayPartition());
print("testArrayChunk: " + testArrayChunk());
print("testArrayShuffle: " + testArrayShuffle());
print("testArrayDeepCopy: " + testArrayDeepCopy());
print("testArrayFillRange: " + testArrayFillRange());
print("testArrayRange: " + testArrayRange());
print("testArrayRepeat: " + testArrayRepeat());
