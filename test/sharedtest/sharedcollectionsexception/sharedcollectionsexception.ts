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

/*
 * @tc.name:sharedcollectionsexception
 * @tc.desc:test sharedcollectionsexception
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class NormalClass {
    public num: number = 0;
    constructor(num: number) {
        this.num = num;
    }
}

function bindErrorTest() {
    print("Start bindErrorTest");
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);
    const normal = new NormalClass(10);
    const unbouondAt = array1.at;
    const boundAt = unbouondAt.bind(normal);
    try {
        boundAt(2);
        print("call boundAt success.");
    } catch (err) {
        print("call boundAt fail. err: " + err + ", errCode: " + err.code);
    }

    const unboundConcat = array1.concat;
    const boundConcat = unboundConcat.bind(normal);
    try {
        boundConcat();
        print("Call boundConcat succeed.");
    } catch (err) {
        print("Call boundConcat fail. err: " + err + ", errCode: " + err.code);
    }

    const unboundEntries = array1.entries;
    const boundEntries = unboundEntries.bind(normal);
    try {
        boundEntries();
        print("Call boundEntries succeed.");
    } catch (err) {
        print("Call boundEntries failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundFill = array1.fill;
    const boundFill = unboundFill.bind(normal);
    try {
        boundFill();
        print("Call boundFill succeed.");
    } catch (err) {
        print("Call boundFill failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundFilter = array1.filter;
    const boundFilter = unboundFilter.bind(normal);
    try {
        boundFilter();
        print("Call boundFilter succeed.");
    } catch (err) {
        print("Call boundFilter failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundFind = array1.find;
    const boundFind = unboundFind.bind(normal);
    try {
        boundFind();
        print("Call boundFind succeed.");
    } catch (err) {
        print("Call boundFind failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundFindIndex = array1.findIndex;
    const boundFindIndex = unboundFindIndex.bind(normal);
    try {
        boundFindIndex();
        print("Call boundFindIndex succeed.");
    } catch (err) {
        print("Call boundFindIndex failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundForEach = array1.forEach;
    const boundForEach = unboundForEach.bind(normal);
    try {
        boundForEach();
        print("Call boundForEach succeed.");
    } catch (err) {
        print("Call boundForEach failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundIndexOf = array1.indexOf;
    const boundIndexOf = unboundIndexOf.bind(normal);
    try {
        boundIndexOf();
        print("Call boundIndexOf succeed.");
    } catch (err) {
        print("Call boundIndexOf failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundJoin = array1.join;
    const boundJoin = unboundJoin.bind(normal);
    try {
        boundJoin();
        print("Call boundJoin succeed.");
    } catch (err) {
        print("Call boundJoin failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundKeys = array1.keys;
    const boundKeys = unboundKeys.bind(normal);
    try {
        boundKeys();
        print("Call boundKeys succeed.");
    } catch (err) {
        print("Call boundKeys failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundMap = array1.map;
    const boundMap = unboundMap.bind(normal);
    try {
        boundMap();
        print("Call boundMap succeed.");
    } catch (err) {
        print("Call boundMap failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundPop = array1.pop;
    const boundPop = unboundPop.bind(normal);
    try {
        boundPop();
        print("Call boundPop succeed.");
    } catch (err) {
        print("Call boundPop failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundPush = array1.push;
    const boundPush = unboundPush.bind(normal);
    try {
        boundPush();
        print("Call boundPush succeed.");
    } catch (err) {
        print("Call boundPush failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundReduce = array1.reduce;
    const boundReduce = unboundReduce.bind(normal);
    try {
        boundReduce();
        print("Call boundReduce succeed.");
    } catch (err) {
        print("Call boundReduce failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundShift = array1.shift;
    const boundShift = unboundShift.bind(normal);
    try {
        boundShift();
        print("Call boundShift succeed.");
    } catch (err) {
        print("Call boundShift failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundSlice = array1.slice;
    const boundSlice = unboundSlice.bind(normal);
    try {
        boundSlice();
        print("Call boundSlice succeed.");
    } catch (err) {
        print("Call boundSlice failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundSort = array1.sort;
    const boundSort = unboundSort.bind(normal);
    try {
        boundSort();
        print("Call boundSort succeed.");
    } catch (err) {
        print("Call boundSort failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundToString = array1.toString;
    const boundToString = unboundToString.bind(normal);
    try {
        boundToString();
        print("Call boundToString succeed.");
    } catch (err) {
        print("Call boundToString failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundUnshift = array1.unshift;
    const boundUnshift = unboundUnshift.bind(normal);
    try {
        boundUnshift();
        print("Call boundUnshift succeed.");
    } catch (err) {
        print("Call boundUnshift failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundValues = array1.values;
    const boundValues = unboundValues.bind(normal);
    try {
        boundValues();
        print("Call boundValues succeed.");
    } catch (err) {
        print("Call boundValues failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundIncludes = array1.includes;
    const boundIncludes = unboundIncludes.bind(normal);
    try {
        boundIncludes();
        print("Call boundIncludes succeed.");
    } catch (err) {
        print("Call boundIncludes failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundShrinkTo = array1.shrinkTo;
    const boundShrinkTo = unboundShrinkTo.bind(normal);
    try {
        boundShrinkTo();
        print("Call boundShrinkTo succeed.");
    } catch (err) {
        print("Call boundShrinkTo failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundExtendTo = array1.extendTo;
    const boundExtendTo = unboundExtendTo.bind(normal);
    try {
        boundExtendTo();
        print("Call boundExtendTo succeed.");
    } catch (err) {
        print("Call boundExtendTo failed. err: " + err + ", errCode: " + err.code);
    }
}

// SendableMap ut
function bindErrorTestMap() {
    print("Start map bindErrorTest");
    const map0 = new SendableMap<string>([['key', 'value']]);
    const normal = new NormalClass(10);

    const unboundClear = map0.clear;
    const boundClear = unboundClear.bind(normal);
    try {
        boundClear();
        print("Call boundClear succeed.");
    } catch (err) {
        print("Call boundClear failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundDelete = map0.delete;
    const boundDelete = unboundDelete.bind(normal);
    try {
        boundDelete();
        print("Call boundDelete succeed.");
    } catch (err) {
        print("Call boundDelete failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundEntries = map0.entries;
    const boundEntries = unboundEntries.bind(normal);
    try {
        boundEntries();
        print("Call boundEntries succeed.");
    } catch (err) {
        print("Call boundEntries failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundForEach = map0.forEach;
    const boundForEach = unboundForEach.bind(normal);
    try {
        boundForEach();
        print("Call boundForEach succeed.");
    } catch (err) {
        print("Call boundForEach failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundGet = map0.get;
    const boundGet = unboundGet.bind(normal);
    try {
        boundGet();
        print("Call boundGet succeed.");
    } catch (err) {
        print("Call boundGet failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundHas = map0.has;
    const boundHas = unboundHas.bind(normal);
    try {
        boundHas();
        print("Call boundHas succeed.");
    } catch (err) {
        print("Call boundHas failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundKeys = map0.keys;
    const boundKeys = unboundKeys.bind(normal);
    try {
        boundKeys();
        print("Call boundKeys succeed.");
    } catch (err) {
        print("Call boundKeys failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundSet = map0.set;
    const boundSet = unboundSet.bind(normal);
    try {
        boundSet();
        print("Call boundSet succeed.");
    } catch (err) {
        print("Call boundSet failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundValues = map0.values;
    const boundValues = unboundValues.bind(normal);
    try {
        boundValues();
        print("Call boundValues succeed.");
    } catch (err) {
        print("Call boundValues failed. err: " + err + ", errCode: " + err.code);
    }
}

// SendableSet ut
function bindErrorTestSet() {
    print("Start set bindErrorTest");
    const set0 = new SendableSet<number>([0, 1, 2]);
    const normal = new NormalClass(10);

    const unboundAdd = set0.add;
    const boundAdd = unboundAdd.bind(normal);
    try {
        boundAdd();
        print("Call boundAdd succeed.");
    } catch (err) {
        print("Call boundAdd failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundClear = set0.clear;
    const boundClear = unboundClear.bind(normal);
    try {
        boundClear();
        print("Call boundClear succeed.");
    } catch (err) {
        print("Call boundClear failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundDelete = set0.delete;
    const boundDelete = unboundDelete.bind(normal);
    try {
        boundDelete();
        print("Call boundDelete succeed.");
    } catch (err) {
        print("Call boundDelete failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundEntries = set0.entries;
    const boundEntries = unboundEntries.bind(normal);
    try {
        boundEntries();
        print("Call boundEntries succeed.");
    } catch (err) {
        print("Call boundEntries failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundForEach = set0.forEach;
    const boundForEach = unboundForEach.bind(normal);
    try {
        boundForEach();
        print("Call boundForEach succeed.");
    } catch (err) {
        print("Call boundForEach failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundHas = set0.has;
    const boundHas = unboundHas.bind(normal);
    try {
        boundHas();
        print("Call boundHas succeed.");
    } catch (err) {
        print("Call boundHas failed. err: " + err + ", errCode: " + err.code);
    }

    const unboundValues = set0.values;
    const boundValues = unboundValues.bind(normal);
    try {
        boundValues();
        print("Call boundValues succeed.");
    } catch (err) {
        print("Call boundValues failed. err: " + err + ", errCode: " + err.code);
    }
}

// for SendableArray
function createErrorTest(): void {
    print("Start createErrorTest");
    try {
        const arr = new SendableArray<string>(-1);
        print("Init with small first element: -1 success.");
    } catch (err) {
        print("Init with small first element: -1, err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SendableArray<string>(0xffff);
        print("Init with big first element: 0xffff success.");
    } catch (err) {
        print("Init with big first element: 0xffff, err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SendableArray<string>(0xffffffffff);
        print("Init exceed max length success.");
    } catch (err) {
        print("Init exceed max length: err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SendableArray<NormalClass>(new NormalClass(1), new NormalClass(2));
        print("Create with non-sendable element success.");
    } catch (err) {
        print("Create with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function fromErrorTest(): void {
    print("Start fromErrorTest");
    try {
        SendableArray.from<NormalClass>(Array.from([new NormalClass(1), new NormalClass(2)]));
        print("Create from non-sendable iterator success.");
    } catch (err) {
        print("Create from non-sendable iterator fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SendableArray.from<NormalClass>([new NormalClass(1), new NormalClass(2)]);
        print("Create from non-sendable element success.");
    } catch (err) {
        print("Create from non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        SendableArray.from<number, NormalClass>([1, 2, 3], (x: number) => new NormalClass(x));
        print("Create from mapper: non-sendable element success.");
    } catch (err) {
        print("Create from mapper: non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function staticCreateErrorTest(): void {
    print("Start staticCreateErrorTest");
    try {
        SendableArray.create<NormalClass>(4, new NormalClass(1), new NormalClass(2));
        print("Static create with non-sendable initialValue success.");
    } catch (err) {
        print("Static create from non-sendable initialValue fail. err: " + err + ", errCode: " + err.code);
    }
}

function atErrorTest() {
    print("Start atErrorTest");
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);
    try {
        print("at invalid index success: " + array1.at("hi"));
    } catch (err) {
        print("at invalid index: err: " + err + ", errCode: " + err.code);
    }
}

function concatErrorTest(): void {
    print("Start concatErrorTest");
    let array: SendableArray<number> = new SendableArray<number>(1, 3, 5);
    let normalArray = new Array<NormalClass>(new NormalClass(2), new NormalClass(4), new NormalClass(6));

    try {
        array.concat(normalArray);
        print("concat with non-sendable array success.");
    } catch (err) {
        print("concat with non-sendable array fail.err: " + err + ", errCode: " + err.code);
    }

    try {
        array.concat(new NormalClass(2));
        print("concat with non-sendable element success.");
    } catch (err) {
        print("concat with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function directCallConstructor() {
    print("Start directCallConstructor");

    try {
        SendableArray<number>();
        print('direct call SendableArray ctor with empty success.');
    } catch (err) {
        print('direct call SendableArray ctor with empty fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableArray<number>(5);
        print('direct call SendableArray ctor with length.');
    } catch (err) {
        print('direct call SendableArray ctor with length fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableArray<number>(5, 10);
        print('direct call SendableArray ctor with elements.');
    } catch (err) {
        print('direct call SendableArray ctor with elements fail. err: ' + err + ', errCode: ' + err.code);
    }
}

function directCallConstructorMap() {
    print("Start map directCallConstructor");

    try {
        SendableMap<number, number>();
        print('direct call SendableMap ctor with empty success.');
    } catch (err) {
        print('direct call SendableMap ctor with empty fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableMap<number, number>(5);
        print('direct call SendableMap ctor with length.');
    } catch (err) {
        print('direct call SendableMap ctor with length fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableMap<number, number>([[5, 10]]);
        print('direct call SendableMap ctor with elements.');
    } catch (err) {
        print('direct call SendableMap ctor with elements fail. err: ' + err + ', errCode: ' + err.code);
    }
}

function directCallConstructorSet() {
    print("Start set directCallConstructor");

    try {
        SendableSet<number>();
        print('direct call SendableSet ctor with empty success.');
    } catch (err) {
        print('direct call SendableSet ctor with empty fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableSet<number>(5);
        print('direct call SendableSet ctor with length.');
    } catch (err) {
        print('direct call SendableSet ctor with length fail. err: ' + err + ', errCode: ' + err.code);
    }

    try {
        SendableSet<number>(5, 10);
        print('direct call SendableSet ctor with elements.');
    } catch (err) {
        print('direct call SendableSet ctor with elements fail. err: ' + err + ', errCode: ' + err.code);
    }
}

function fillErrorTest() {
    print("Start Test fillErrorTest")
    const array1 = new SendableArray<number>(1, 2, 3, 4);
    try {
        array1.fill(new NormalClass(2), 2, 4);
        print("fill array with non-sendable object and position success.");
    } catch (err) {
        print("fill array with non-sendable object and position fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        array1.fill(new NormalClass(3), 1);
        print("fill array with non-sendable object and start position success.");
    } catch (err) {
        print("fill array with non-sendable object and start position fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        array1.fill(new NormalClass(4));
        print("fill array with non-sendable object success.");
    } catch (err) {
        print("fill array with non-sendable object fail. err: " + err + ", errCode: " + err.code);
    }
}

function mapErrorTest() {
    print("Start mapErrorTest")
    const array = new SendableArray<number>(1, 4, 9, 16);
    try {
        array.map<NormalClass>((x: number) => new NormalClass(x));
        print("map array with non-sendable success.");
    } catch (err) {
        print("map array with non-sendable fail. err: " + err + ", errCode: " + err.code);
    }
}

function pushErrorTest() {
    print("Start pushErrorTest")
    let array: SendableArray<number> = new SendableArray<number>(1, 3, 5);
    try {
        array.push(new NormalClass(5));
        print("push array with non-sendable element success.");
    } catch (err) {
        print("push array with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function unshiftErrorTest() {
    print("Start Test unshiftErrorTest")
    const array = new SendableArray<number>(1, 2, 3);
    try {
        array.unshift(new NormalClass(4), new NormalClass(5));
        print("unshift array with non-sendable element success.");
    } catch (err) {
        print("unshift array with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function extendToErrorTest() {
    print("Start Test extendToErrorTest")
    const array = new SendableArray<number>(1, 2, 3);
    try {
        array.extendTo(5, new NormalClass(4));
        print("extendTo array with non-sendable element success.");
    } catch (err) {
        print("extendTo array with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function concurrencyErrorTest() {
    const array1 = new SendableArray<number>(1, 2, 3);
    try {
        array1.forEach((key: number, _: number, array: SendableArray) => {
          array.push(5);
        });
      } catch (err) {
        print("add element while iterate array fail. err: " + err + ", errCode: " + err.code);
      }
      try {
        array1.forEach((key: number, _: number, array: SendableArray) => {
          if (key % 2 == 0) {
            array.pop();
          }
        });
      } catch (err) {
        print("pop element while iterate array fail. err: " + err + ", errCode: " + err.code);
      }
      try {
        array1.forEach((key: number, _: number, array: SendableArray) => {
          array.shrinkTo(0);
        });
      } catch (err) {
        print("shrink while iterate array fail. err: " + err + ", errCode: " + err.code);
      }
      try {
        array1.forEach((key: number, _: number, array: SendableArray) => {
          array.extendTo(array1.length + 1, 100);
        });
      } catch (err) {
        print("extend while iterate array fail. err: " + err + ", errCode: " + err.code);
      }
}

// common bindTest
bindErrorTest();
print('Start Test SendableMap');
bindErrorTestMap();

print('Start Test SendableSet');
bindErrorTestSet();

print('Start Test SendableArray');
createErrorTest();
fromErrorTest();
atErrorTest();
concatErrorTest();
fillErrorTest()
mapErrorTest()
pushErrorTest()
unshiftErrorTest()
staticCreateErrorTest();
extendToErrorTest();
directCallConstructor();
directCallConstructorMap();
directCallConstructorSet();
concurrencyErrorTest();
