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

// ==================== Map Basic ====================

function testMapBasicOps(): string {
    const map = new Map<string, number>();
    map.set('a', 1);
    map.set('b', 2);
    map.set('c', 3);
    return `${map.get('a')},${map.get('b')},${map.has('c')},${map.has('d')},${map.size}`;
}

function testMapDelete(): string {
    const map = new Map([['a', 1], ['b', 2], ['c', 3]]);
    const deleted = map.delete('b');
    const notDeleted = map.delete('x');
    return `${deleted},${notDeleted},${map.size},${map.has('b')}`;
}

function testMapClear(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    map.clear();
    return `${map.size}`;
}

function testMapGetUndefined(): string {
    const map = new Map<string, number>();
    map.set('a', 1);
    return `${map.get('a')},${map.get('nonexistent')}`;
}

function testMapOverwrite(): string {
    const map = new Map<string, number>();
    map.set('key', 1);
    map.set('key', 2);
    map.set('key', 3);
    return `${map.get('key')},${map.size}`;
}

function testMapNullUndefinedKey(): string {
    const map = new Map<any, string>();
    map.set(null, 'null value');
    map.set(undefined, 'undefined value');
    return `${map.get(null)},${map.get(undefined)},${map.size}`;
}

function testMapNumberKey(): string {
    const map = new Map<number, string>();
    map.set(1, 'one');
    map.set(2, 'two');
    map.set(NaN, 'nan');
    return `${map.get(1)},${map.get(NaN)},${map.has(NaN)}`;
}

// ==================== Map Iteration ====================

function testMapIterate(): string {
    const map = new Map([['a', 1], ['b', 2], ['c', 3]]);
    const keys = [...map.keys()].join(',');
    const values = [...map.values()].join(',');
    return `${keys},${values}`;
}

function testMapEntries(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    const entries = [...map.entries()].map(([k, v]) => `${k}:${v}`).join(',');
    return entries;
}

function testMapForEach(): string {
    const map = new Map([['a', 1], ['b', 2], ['c', 3]]);
    let sum = 0;
    map.forEach((value) => { sum += value; });
    return `${sum}`;
}

function testMapForEachWithKey(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    const result: string[] = [];
    map.forEach((value, key) => result.push(`${key}=${value}`));
    return result.join(',');
}

function testMapForOf(): string {
    const map = new Map([['x', 10], ['y', 20]]);
    const result: string[] = [];
    for (const [key, value] of map) {
        result.push(`${key}:${value}`);
    }
    return result.join(',');
}

function testMapIteratorOrder(): string {
    const map = new Map<string, number>();
    map.set('c', 3);
    map.set('a', 1);
    map.set('b', 2);
    return [...map.keys()].join(',');
}

// ==================== Map Chaining ====================

function testMapChaining(): string {
    const map = new Map<string, number>()
        .set('a', 1)
        .set('b', 2)
        .set('c', 3);
    return `${map.size}`;
}

function testMapChainingWithDelete(): string {
    const map = new Map<string, number>()
        .set('a', 1)
        .set('b', 2)
        .set('c', 3);
    map.delete('b');
    return `${map.size},${map.has('b')}`;
}

// ==================== Map Object Key ====================

function testMapObjectKey(): string {
    const key1 = { id: 1 };
    const key2 = { id: 2 };
    const map = new Map<object, string>();
    map.set(key1, 'first');
    map.set(key2, 'second');
    return `${map.get(key1)},${map.get(key2)},${map.size}`;
}

function testMapObjectKeyIdentity(): string {
    const key1 = { id: 1 };
    const key2 = { id: 1 };
    const map = new Map<object, string>();
    map.set(key1, 'first');
    map.set(key2, 'second');
    return `${map.size},${map.get(key1)},${map.get(key2)}`;
}

function testMapArrayKey(): string {
    const key1 = [1, 2, 3];
    const key2 = [1, 2, 3];
    const map = new Map<number[], string>();
    map.set(key1, 'array1');
    map.set(key2, 'array2');
    return `${map.size},${map.get(key1)}`;
}

function testMapFunctionKey(): string {
    const fn1 = () => 1;
    const fn2 = () => 2;
    const map = new Map<Function, string>();
    map.set(fn1, 'func1');
    map.set(fn2, 'func2');
    return `${map.size},${map.has(fn1)}`;
}

// ==================== Map Conversion ====================

function testMapFromEntries(): string {
    const entries: [string, number][] = [['a', 1], ['b', 2], ['c', 3]];
    const map = new Map(entries);
    return `${map.size},${map.get('b')}`;
}

function testMapToArray(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    const arr = Array.from(map);
    return `${arr.length},${arr[0][0]},${arr[0][1]}`;
}

function testMapToObject(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    const obj = Object.fromEntries(map);
    return `${obj.a},${obj.b}`;
}

function testMapSpread(): string {
    const map1 = new Map([['a', 1]]);
    const map2 = new Map([['b', 2]]);
    const combined = new Map([...map1, ...map2]);
    return `${combined.size},${combined.get('a')},${combined.get('b')}`;
}

// ==================== Set Basic ====================

function testSetBasicOps(): string {
    const set = new Set<number>();
    set.add(1);
    set.add(2);
    set.add(3);
    set.add(2);
    return `${set.has(1)},${set.has(4)},${set.size}`;
}

function testSetDelete(): string {
    const set = new Set([1, 2, 3, 4, 5]);
    const deleted = set.delete(3);
    const notDeleted = set.delete(10);
    return `${deleted},${notDeleted},${set.size}`;
}

function testSetClear(): string {
    const set = new Set([1, 2, 3]);
    set.clear();
    return `${set.size}`;
}

function testSetAddChaining(): string {
    const set = new Set<number>()
        .add(1)
        .add(2)
        .add(3);
    return `${set.size}`;
}

function testSetNaNHandling(): string {
    const set = new Set<number>();
    set.add(NaN);
    set.add(NaN);
    return `${set.size},${set.has(NaN)}`;
}

function testSetNullUndefined(): string {
    const set = new Set<any>();
    set.add(null);
    set.add(undefined);
    set.add(null);
    return `${set.size},${set.has(null)},${set.has(undefined)}`;
}

function testSetZeroHandling(): string {
    const set = new Set<number>();
    set.add(0);
    set.add(-0);
    return `${set.size},${set.has(0)},${set.has(-0)}`;
}

// ==================== Set Iteration ====================

function testSetIterate(): string {
    const set = new Set([3, 1, 2]);
    return [...set].join(',');
}

function testSetForEach(): string {
    const set = new Set([1, 2, 3, 4, 5]);
    let sum = 0;
    set.forEach(v => { sum += v; });
    return `${sum}`;
}

function testSetForEachWithIndex(): string {
    const set = new Set(['a', 'b', 'c']);
    const result: string[] = [];
    set.forEach((value, value2, s) => {
        result.push(`${value}=${value2}`);
    });
    return result.join(',');
}

function testSetForOf(): string {
    const set = new Set([10, 20, 30]);
    const result: number[] = [];
    for (const value of set) {
        result.push(value);
    }
    return result.join(',');
}

function testSetKeys(): string {
    const set = new Set(['x', 'y', 'z']);
    return [...set.keys()].join(',');
}

function testSetValues(): string {
    const set = new Set(['x', 'y', 'z']);
    return [...set.values()].join(',');
}

function testSetEntries(): string {
    const set = new Set(['a', 'b']);
    const entries = [...set.entries()];
    return entries.map(([k, v]) => `${k}:${v}`).join(',');
}

function testSetIteratorOrder(): string {
    const set = new Set<string>();
    set.add('c');
    set.add('a');
    set.add('b');
    return [...set].join(',');
}

// ==================== Set From Array ====================

function testSetFromArray(): string {
    const arr = [1, 2, 2, 3, 3, 3, 4];
    const set = new Set(arr);
    return `${set.size},${[...set].join(',')}`;
}

function testSetFromString(): string {
    const str = 'hello';
    const set = new Set(str);
    return `${set.size},${[...set].join(',')}`;
}

function testSetFromMap(): string {
    const map = new Map([['a', 1], ['b', 2]]);
    const set = new Set(map.keys());
    return `${set.size},${[...set].join(',')}`;
}

// ==================== Set Operations ====================

function testSetOperations(): string {
    const a = new Set([1, 2, 3, 4]);
    const b = new Set([3, 4, 5, 6]);
    const union = new Set([...a, ...b]);
    const intersection = new Set([...a].filter(x => b.has(x)));
    const difference = new Set([...a].filter(x => !b.has(x)));
    return `${union.size},${intersection.size},${difference.size}`;
}

function testSetUnion(): string {
    const a = new Set([1, 2, 3]);
    const b = new Set([2, 3, 4]);
    const union = new Set([...a, ...b]);
    return [...union].sort((x, y) => x - y).join(',');
}

function testSetIntersection(): string {
    const a = new Set([1, 2, 3, 4]);
    const b = new Set([2, 4, 6, 8]);
    const intersection = new Set([...a].filter(x => b.has(x)));
    return [...intersection].join(',');
}

function testSetDifference(): string {
    const a = new Set([1, 2, 3, 4, 5]);
    const b = new Set([2, 4]);
    const difference = new Set([...a].filter(x => !b.has(x)));
    return [...difference].join(',');
}

function testSetSymmetricDifference(): string {
    const a = new Set([1, 2, 3]);
    const b = new Set([2, 3, 4]);
    const symmetricDiff = new Set(
        [...a].filter(x => !b.has(x)).concat([...b].filter(x => !a.has(x)))
    );
    return [...symmetricDiff].sort((x, y) => x - y).join(',');
}

function testSetIsSubset(): string {
    const a = new Set([1, 2]);
    const b = new Set([1, 2, 3, 4]);
    const isSubset = [...a].every(x => b.has(x));
    return `${isSubset}`;
}

function testSetIsSuperset(): string {
    const a = new Set([1, 2, 3, 4]);
    const b = new Set([1, 2]);
    const isSuperset = [...b].every(x => a.has(x));
    return `${isSuperset}`;
}

function testSetIsDisjoint(): string {
    const a = new Set([1, 2, 3]);
    const b = new Set([4, 5, 6]);
    const c = new Set([3, 4, 5]);
    const isDisjointAB = ![...a].some(x => b.has(x));
    const isDisjointAC = ![...a].some(x => c.has(x));
    return `${isDisjointAB},${isDisjointAC}`;
}

// ==================== Set Object Elements ====================

function testSetObjectElements(): string {
    const obj1 = { id: 1 };
    const obj2 = { id: 2 };
    const obj3 = { id: 1 };
    const set = new Set<object>();
    set.add(obj1);
    set.add(obj2);
    set.add(obj3);
    return `${set.size},${set.has(obj1)},${set.has(obj3)}`;
}

function testSetArrayElements(): string {
    const arr1 = [1, 2];
    const arr2 = [1, 2];
    const set = new Set<number[]>();
    set.add(arr1);
    set.add(arr2);
    return `${set.size}`;
}

// ==================== WeakMap ====================

function testWeakMapBasic(): string {
    const wm = new WeakMap<object, string>();
    const key = { id: 1 };
    wm.set(key, 'value');
    return `${wm.has(key)},${wm.get(key)}`;
}

function testWeakMapDelete(): string {
    const wm = new WeakMap<object, number>();
    const key = { id: 1 };
    wm.set(key, 42);
    const hasBefore = wm.has(key);
    wm.delete(key);
    const hasAfter = wm.has(key);
    return `${hasBefore},${hasAfter}`;
}

function testWeakMapMultipleKeys(): string {
    const wm = new WeakMap<object, string>();
    const key1 = { id: 1 };
    const key2 = { id: 2 };
    const key3 = { id: 3 };
    wm.set(key1, 'one');
    wm.set(key2, 'two');
    wm.set(key3, 'three');
    return `${wm.get(key1)},${wm.get(key2)},${wm.get(key3)}`;
}

function testWeakMapChaining(): string {
    const wm = new WeakMap<object, number>();
    const key1 = { id: 1 };
    const key2 = { id: 2 };
    wm.set(key1, 10).set(key2, 20);
    return `${wm.get(key1)},${wm.get(key2)}`;
}

function testWeakMapOverwrite(): string {
    const wm = new WeakMap<object, number>();
    const key = { id: 1 };
    wm.set(key, 1);
    wm.set(key, 2);
    wm.set(key, 3);
    return `${wm.get(key)}`;
}

function testWeakMapArrayKey(): string {
    const wm = new WeakMap<object, string>();
    const key = [1, 2, 3];
    wm.set(key, 'array');
    return `${wm.has(key)},${wm.get(key)}`;
}

// ==================== WeakSet ====================

function testWeakSetBasic(): string {
    const ws = new WeakSet<object>();
    const obj1 = { id: 1 };
    const obj2 = { id: 2 };
    ws.add(obj1);
    ws.add(obj2);
    return `${ws.has(obj1)},${ws.has(obj2)},${ws.has({ id: 3 })}`;
}

function testWeakSetDelete(): string {
    const ws = new WeakSet<object>();
    const obj = { id: 1 };
    ws.add(obj);
    const hasBefore = ws.has(obj);
    ws.delete(obj);
    const hasAfter = ws.has(obj);
    return `${hasBefore},${hasAfter}`;
}

function testWeakSetChaining(): string {
    const ws = new WeakSet<object>();
    const obj1 = { id: 1 };
    const obj2 = { id: 2 };
    ws.add(obj1).add(obj2);
    return `${ws.has(obj1)},${ws.has(obj2)}`;
}

function testWeakSetDuplicates(): string {
    const ws = new WeakSet<object>();
    const obj = { id: 1 };
    ws.add(obj);
    ws.add(obj);
    ws.add(obj);
    return `${ws.has(obj)}`;
}

function testWeakSetArrayElement(): string {
    const ws = new WeakSet<object>();
    const arr = [1, 2, 3];
    ws.add(arr);
    return `${ws.has(arr)}`;
}

function testWeakSetFunctionElement(): string {
    const ws = new WeakSet<object>();
    const fn = () => 42;
    ws.add(fn);
    return `${ws.has(fn)}`;
}

// ==================== Map/Set Size and Empty ====================

function testMapSizeEmpty(): string {
    const map = new Map<string, number>();
    const sizeEmpty = map.size;
    map.set('a', 1);
    const sizeOne = map.size;
    map.set('b', 2);
    const sizeTwo = map.size;
    return `${sizeEmpty},${sizeOne},${sizeTwo}`;
}

function testSetSizeEmpty(): string {
    const set = new Set<number>();
    const sizeEmpty = set.size;
    set.add(1);
    const sizeOne = set.size;
    set.add(2);
    const sizeTwo = set.size;
    return `${sizeEmpty},${sizeOne},${sizeTwo}`;
}

// ==================== Utility Patterns ====================

function testMapAsCache(): string {
    const cache = new Map<string, number>();
    const getValue = (key: string): number => {
        if (cache.has(key)) {
            return cache.get(key)!;
        }
        const value = key.length * 10;
        cache.set(key, value);
        return value;
    };
    const v1 = getValue('hello');
    const v2 = getValue('hello');
    const v3 = getValue('world');
    return `${v1},${v2},${v3},${cache.size}`;
}

function testSetAsUniqueFilter(): string {
    const arr = [1, 2, 3, 2, 1, 4, 3, 5];
    const unique = [...new Set(arr)];
    return unique.join(',');
}

function testMapGroupBy(): string {
    const items = [
        { type: 'fruit', name: 'apple' },
        { type: 'fruit', name: 'banana' },
        { type: 'vegetable', name: 'carrot' }
    ];
    const groups = new Map<string, string[]>();
    items.forEach(item => {
        const list = groups.get(item.type) || [];
        list.push(item.name);
        groups.set(item.type, list);
    });
    return `${groups.get('fruit')!.length},${groups.get('vegetable')!.length}`;
}

function testSetToggle(): string {
    const set = new Set<number>();
    const toggle = (value: number) => {
        if (set.has(value)) {
            set.delete(value);
        } else {
            set.add(value);
        }
    };
    toggle(1);
    toggle(2);
    toggle(1);
    toggle(3);
    return [...set].sort((a, b) => a - b).join(',');
}

// Warmup
for (let i = 0; i < 20; i++) {
    testMapBasicOps();
    testMapDelete();
    testMapClear();
    testMapGetUndefined();
    testMapOverwrite();
    testMapNullUndefinedKey();
    testMapNumberKey();
    testMapIterate();
    testMapEntries();
    testMapForEach();
    testMapForEachWithKey();
    testMapForOf();
    testMapIteratorOrder();
    testMapChaining();
    testMapChainingWithDelete();
    testMapObjectKey();
    testMapObjectKeyIdentity();
    testMapArrayKey();
    testMapFunctionKey();
    testMapFromEntries();
    testMapToArray();
    testMapToObject();
    testMapSpread();
    testSetBasicOps();
    testSetDelete();
    testSetClear();
    testSetAddChaining();
    testSetNaNHandling();
    testSetNullUndefined();
    testSetZeroHandling();
    testSetIterate();
    testSetForEach();
    testSetForEachWithIndex();
    testSetForOf();
    testSetKeys();
    testSetValues();
    testSetEntries();
    testSetIteratorOrder();
    testSetFromArray();
    testSetFromString();
    testSetFromMap();
    testSetOperations();
    testSetUnion();
    testSetIntersection();
    testSetDifference();
    testSetSymmetricDifference();
    testSetIsSubset();
    testSetIsSuperset();
    testSetIsDisjoint();
    testSetObjectElements();
    testSetArrayElements();
    testWeakMapBasic();
    testWeakMapDelete();
    testWeakMapMultipleKeys();
    testWeakMapChaining();
    testWeakMapOverwrite();
    testWeakMapArrayKey();
    testWeakSetBasic();
    testWeakSetDelete();
    testWeakSetChaining();
    testWeakSetDuplicates();
    testWeakSetArrayElement();
    testWeakSetFunctionElement();
    testMapSizeEmpty();
    testSetSizeEmpty();
    testMapAsCache();
    testSetAsUniqueFilter();
    testMapGroupBy();
    testSetToggle();
}

// JIT compile
ArkTools.jitCompileAsync(testMapBasicOps);
ArkTools.jitCompileAsync(testMapDelete);
ArkTools.jitCompileAsync(testMapClear);
ArkTools.jitCompileAsync(testMapGetUndefined);
ArkTools.jitCompileAsync(testMapOverwrite);
ArkTools.jitCompileAsync(testMapNullUndefinedKey);
ArkTools.jitCompileAsync(testMapNumberKey);
ArkTools.jitCompileAsync(testMapIterate);
ArkTools.jitCompileAsync(testMapEntries);
ArkTools.jitCompileAsync(testMapForEach);
ArkTools.jitCompileAsync(testMapForEachWithKey);
ArkTools.jitCompileAsync(testMapForOf);
ArkTools.jitCompileAsync(testMapIteratorOrder);
ArkTools.jitCompileAsync(testMapChaining);
ArkTools.jitCompileAsync(testMapChainingWithDelete);
ArkTools.jitCompileAsync(testMapObjectKey);
ArkTools.jitCompileAsync(testMapObjectKeyIdentity);
ArkTools.jitCompileAsync(testMapArrayKey);
ArkTools.jitCompileAsync(testMapFunctionKey);
ArkTools.jitCompileAsync(testMapFromEntries);
ArkTools.jitCompileAsync(testMapToArray);
ArkTools.jitCompileAsync(testMapToObject);
ArkTools.jitCompileAsync(testMapSpread);
ArkTools.jitCompileAsync(testSetBasicOps);
ArkTools.jitCompileAsync(testSetDelete);
ArkTools.jitCompileAsync(testSetClear);
ArkTools.jitCompileAsync(testSetAddChaining);
ArkTools.jitCompileAsync(testSetNaNHandling);
ArkTools.jitCompileAsync(testSetNullUndefined);
ArkTools.jitCompileAsync(testSetZeroHandling);
ArkTools.jitCompileAsync(testSetIterate);
ArkTools.jitCompileAsync(testSetForEach);
ArkTools.jitCompileAsync(testSetForEachWithIndex);
ArkTools.jitCompileAsync(testSetForOf);
ArkTools.jitCompileAsync(testSetKeys);
ArkTools.jitCompileAsync(testSetValues);
ArkTools.jitCompileAsync(testSetEntries);
ArkTools.jitCompileAsync(testSetIteratorOrder);
ArkTools.jitCompileAsync(testSetFromArray);
ArkTools.jitCompileAsync(testSetFromString);
ArkTools.jitCompileAsync(testSetFromMap);
ArkTools.jitCompileAsync(testSetOperations);
ArkTools.jitCompileAsync(testSetUnion);
ArkTools.jitCompileAsync(testSetIntersection);
ArkTools.jitCompileAsync(testSetDifference);
ArkTools.jitCompileAsync(testSetSymmetricDifference);
ArkTools.jitCompileAsync(testSetIsSubset);
ArkTools.jitCompileAsync(testSetIsSuperset);
ArkTools.jitCompileAsync(testSetIsDisjoint);
ArkTools.jitCompileAsync(testSetObjectElements);
ArkTools.jitCompileAsync(testSetArrayElements);
ArkTools.jitCompileAsync(testWeakMapBasic);
ArkTools.jitCompileAsync(testWeakMapDelete);
ArkTools.jitCompileAsync(testWeakMapMultipleKeys);
ArkTools.jitCompileAsync(testWeakMapChaining);
ArkTools.jitCompileAsync(testWeakMapOverwrite);
ArkTools.jitCompileAsync(testWeakMapArrayKey);
ArkTools.jitCompileAsync(testWeakSetBasic);
ArkTools.jitCompileAsync(testWeakSetDelete);
ArkTools.jitCompileAsync(testWeakSetChaining);
ArkTools.jitCompileAsync(testWeakSetDuplicates);
ArkTools.jitCompileAsync(testWeakSetArrayElement);
ArkTools.jitCompileAsync(testWeakSetFunctionElement);
ArkTools.jitCompileAsync(testMapSizeEmpty);
ArkTools.jitCompileAsync(testSetSizeEmpty);
ArkTools.jitCompileAsync(testMapAsCache);
ArkTools.jitCompileAsync(testSetAsUniqueFilter);
ArkTools.jitCompileAsync(testMapGroupBy);
ArkTools.jitCompileAsync(testSetToggle);

print("compile testMapBasicOps: " + ArkTools.waitJitCompileFinish(testMapBasicOps));
print("compile testMapDelete: " + ArkTools.waitJitCompileFinish(testMapDelete));
print("compile testMapClear: " + ArkTools.waitJitCompileFinish(testMapClear));
print("compile testMapGetUndefined: " + ArkTools.waitJitCompileFinish(testMapGetUndefined));
print("compile testMapOverwrite: " + ArkTools.waitJitCompileFinish(testMapOverwrite));
print("compile testMapNullUndefinedKey: " + ArkTools.waitJitCompileFinish(testMapNullUndefinedKey));
print("compile testMapNumberKey: " + ArkTools.waitJitCompileFinish(testMapNumberKey));
print("compile testMapIterate: " + ArkTools.waitJitCompileFinish(testMapIterate));
print("compile testMapEntries: " + ArkTools.waitJitCompileFinish(testMapEntries));
print("compile testMapForEach: " + ArkTools.waitJitCompileFinish(testMapForEach));
print("compile testMapForEachWithKey: " + ArkTools.waitJitCompileFinish(testMapForEachWithKey));
print("compile testMapForOf: " + ArkTools.waitJitCompileFinish(testMapForOf));
print("compile testMapIteratorOrder: " + ArkTools.waitJitCompileFinish(testMapIteratorOrder));
print("compile testMapChaining: " + ArkTools.waitJitCompileFinish(testMapChaining));
print("compile testMapChainingWithDelete: " + ArkTools.waitJitCompileFinish(testMapChainingWithDelete));
print("compile testMapObjectKey: " + ArkTools.waitJitCompileFinish(testMapObjectKey));
print("compile testMapObjectKeyIdentity: " + ArkTools.waitJitCompileFinish(testMapObjectKeyIdentity));
print("compile testMapArrayKey: " + ArkTools.waitJitCompileFinish(testMapArrayKey));
print("compile testMapFunctionKey: " + ArkTools.waitJitCompileFinish(testMapFunctionKey));
print("compile testMapFromEntries: " + ArkTools.waitJitCompileFinish(testMapFromEntries));
print("compile testMapToArray: " + ArkTools.waitJitCompileFinish(testMapToArray));
print("compile testMapToObject: " + ArkTools.waitJitCompileFinish(testMapToObject));
print("compile testMapSpread: " + ArkTools.waitJitCompileFinish(testMapSpread));
print("compile testSetBasicOps: " + ArkTools.waitJitCompileFinish(testSetBasicOps));
print("compile testSetDelete: " + ArkTools.waitJitCompileFinish(testSetDelete));
print("compile testSetClear: " + ArkTools.waitJitCompileFinish(testSetClear));
print("compile testSetAddChaining: " + ArkTools.waitJitCompileFinish(testSetAddChaining));
print("compile testSetNaNHandling: " + ArkTools.waitJitCompileFinish(testSetNaNHandling));
print("compile testSetNullUndefined: " + ArkTools.waitJitCompileFinish(testSetNullUndefined));
print("compile testSetZeroHandling: " + ArkTools.waitJitCompileFinish(testSetZeroHandling));
print("compile testSetIterate: " + ArkTools.waitJitCompileFinish(testSetIterate));
print("compile testSetForEach: " + ArkTools.waitJitCompileFinish(testSetForEach));
print("compile testSetForEachWithIndex: " + ArkTools.waitJitCompileFinish(testSetForEachWithIndex));
print("compile testSetForOf: " + ArkTools.waitJitCompileFinish(testSetForOf));
print("compile testSetKeys: " + ArkTools.waitJitCompileFinish(testSetKeys));
print("compile testSetValues: " + ArkTools.waitJitCompileFinish(testSetValues));
print("compile testSetEntries: " + ArkTools.waitJitCompileFinish(testSetEntries));
print("compile testSetIteratorOrder: " + ArkTools.waitJitCompileFinish(testSetIteratorOrder));
print("compile testSetFromArray: " + ArkTools.waitJitCompileFinish(testSetFromArray));
print("compile testSetFromString: " + ArkTools.waitJitCompileFinish(testSetFromString));
print("compile testSetFromMap: " + ArkTools.waitJitCompileFinish(testSetFromMap));
print("compile testSetOperations: " + ArkTools.waitJitCompileFinish(testSetOperations));
print("compile testSetUnion: " + ArkTools.waitJitCompileFinish(testSetUnion));
print("compile testSetIntersection: " + ArkTools.waitJitCompileFinish(testSetIntersection));
print("compile testSetDifference: " + ArkTools.waitJitCompileFinish(testSetDifference));
print("compile testSetSymmetricDifference: " + ArkTools.waitJitCompileFinish(testSetSymmetricDifference));
print("compile testSetIsSubset: " + ArkTools.waitJitCompileFinish(testSetIsSubset));
print("compile testSetIsSuperset: " + ArkTools.waitJitCompileFinish(testSetIsSuperset));
print("compile testSetIsDisjoint: " + ArkTools.waitJitCompileFinish(testSetIsDisjoint));
print("compile testSetObjectElements: " + ArkTools.waitJitCompileFinish(testSetObjectElements));
print("compile testSetArrayElements: " + ArkTools.waitJitCompileFinish(testSetArrayElements));
print("compile testWeakMapBasic: " + ArkTools.waitJitCompileFinish(testWeakMapBasic));
print("compile testWeakMapDelete: " + ArkTools.waitJitCompileFinish(testWeakMapDelete));
print("compile testWeakMapMultipleKeys: " + ArkTools.waitJitCompileFinish(testWeakMapMultipleKeys));
print("compile testWeakMapChaining: " + ArkTools.waitJitCompileFinish(testWeakMapChaining));
print("compile testWeakMapOverwrite: " + ArkTools.waitJitCompileFinish(testWeakMapOverwrite));
print("compile testWeakMapArrayKey: " + ArkTools.waitJitCompileFinish(testWeakMapArrayKey));
print("compile testWeakSetBasic: " + ArkTools.waitJitCompileFinish(testWeakSetBasic));
print("compile testWeakSetDelete: " + ArkTools.waitJitCompileFinish(testWeakSetDelete));
print("compile testWeakSetChaining: " + ArkTools.waitJitCompileFinish(testWeakSetChaining));
print("compile testWeakSetDuplicates: " + ArkTools.waitJitCompileFinish(testWeakSetDuplicates));
print("compile testWeakSetArrayElement: " + ArkTools.waitJitCompileFinish(testWeakSetArrayElement));
print("compile testWeakSetFunctionElement: " + ArkTools.waitJitCompileFinish(testWeakSetFunctionElement));
print("compile testMapSizeEmpty: " + ArkTools.waitJitCompileFinish(testMapSizeEmpty));
print("compile testSetSizeEmpty: " + ArkTools.waitJitCompileFinish(testSetSizeEmpty));
print("compile testMapAsCache: " + ArkTools.waitJitCompileFinish(testMapAsCache));
print("compile testSetAsUniqueFilter: " + ArkTools.waitJitCompileFinish(testSetAsUniqueFilter));
print("compile testMapGroupBy: " + ArkTools.waitJitCompileFinish(testMapGroupBy));
print("compile testSetToggle: " + ArkTools.waitJitCompileFinish(testSetToggle));

// Verify
print("testMapBasicOps: " + testMapBasicOps());
print("testMapDelete: " + testMapDelete());
print("testMapClear: " + testMapClear());
print("testMapGetUndefined: " + testMapGetUndefined());
print("testMapOverwrite: " + testMapOverwrite());
print("testMapNullUndefinedKey: " + testMapNullUndefinedKey());
print("testMapNumberKey: " + testMapNumberKey());
print("testMapIterate: " + testMapIterate());
print("testMapEntries: " + testMapEntries());
print("testMapForEach: " + testMapForEach());
print("testMapForEachWithKey: " + testMapForEachWithKey());
print("testMapForOf: " + testMapForOf());
print("testMapIteratorOrder: " + testMapIteratorOrder());
print("testMapChaining: " + testMapChaining());
print("testMapChainingWithDelete: " + testMapChainingWithDelete());
print("testMapObjectKey: " + testMapObjectKey());
print("testMapObjectKeyIdentity: " + testMapObjectKeyIdentity());
print("testMapArrayKey: " + testMapArrayKey());
print("testMapFunctionKey: " + testMapFunctionKey());
print("testMapFromEntries: " + testMapFromEntries());
print("testMapToArray: " + testMapToArray());
print("testMapToObject: " + testMapToObject());
print("testMapSpread: " + testMapSpread());
print("testSetBasicOps: " + testSetBasicOps());
print("testSetDelete: " + testSetDelete());
print("testSetClear: " + testSetClear());
print("testSetAddChaining: " + testSetAddChaining());
print("testSetNaNHandling: " + testSetNaNHandling());
print("testSetNullUndefined: " + testSetNullUndefined());
print("testSetZeroHandling: " + testSetZeroHandling());
print("testSetIterate: " + testSetIterate());
print("testSetForEach: " + testSetForEach());
print("testSetForEachWithIndex: " + testSetForEachWithIndex());
print("testSetForOf: " + testSetForOf());
print("testSetKeys: " + testSetKeys());
print("testSetValues: " + testSetValues());
print("testSetEntries: " + testSetEntries());
print("testSetIteratorOrder: " + testSetIteratorOrder());
print("testSetFromArray: " + testSetFromArray());
print("testSetFromString: " + testSetFromString());
print("testSetFromMap: " + testSetFromMap());
print("testSetOperations: " + testSetOperations());
print("testSetUnion: " + testSetUnion());
print("testSetIntersection: " + testSetIntersection());
print("testSetDifference: " + testSetDifference());
print("testSetSymmetricDifference: " + testSetSymmetricDifference());
print("testSetIsSubset: " + testSetIsSubset());
print("testSetIsSuperset: " + testSetIsSuperset());
print("testSetIsDisjoint: " + testSetIsDisjoint());
print("testSetObjectElements: " + testSetObjectElements());
print("testSetArrayElements: " + testSetArrayElements());
print("testWeakMapBasic: " + testWeakMapBasic());
print("testWeakMapDelete: " + testWeakMapDelete());
print("testWeakMapMultipleKeys: " + testWeakMapMultipleKeys());
print("testWeakMapChaining: " + testWeakMapChaining());
print("testWeakMapOverwrite: " + testWeakMapOverwrite());
print("testWeakMapArrayKey: " + testWeakMapArrayKey());
print("testWeakSetBasic: " + testWeakSetBasic());
print("testWeakSetDelete: " + testWeakSetDelete());
print("testWeakSetChaining: " + testWeakSetChaining());
print("testWeakSetDuplicates: " + testWeakSetDuplicates());
print("testWeakSetArrayElement: " + testWeakSetArrayElement());
print("testWeakSetFunctionElement: " + testWeakSetFunctionElement());
print("testMapSizeEmpty: " + testMapSizeEmpty());
print("testSetSizeEmpty: " + testSetSizeEmpty());
print("testMapAsCache: " + testMapAsCache());
print("testSetAsUniqueFilter: " + testSetAsUniqueFilter());
print("testMapGroupBy: " + testMapGroupBy());
print("testSetToggle: " + testSetToggle());
