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
 * @tc.name:sendabletypedarray
 * @tc.desc:test sendabletypedarray
 * @tc.type: FUNC
 * @tc.require: issue#I9EBOZ
 */

// @ts-nocheck
declare function print(str: any): string;

print("================Test Basic properties================");
[
  SendableInt8Array,
  SendableInt16Array,
  SendableInt32Array,
  SendableFloat32Array,
  SendableFloat64Array,
  SendableBigInt64Array,
  SendableUint8Array,
  SendableUint16Array,
  SendableUint8ClampedArray,
  SendableUint32Array,
  SendableBigUint64Array,
].forEach((ctor: Function) => {
  let array;
  if (ctor == SendableBigInt64Array || ctor == SendableBigUint64Array) {
    array = new ctor([1n, 2n, 3n, 4n]);
  } else {
    array = new ctor([1, 2, 3, 4]);
  }
  print('length: ' + array.length + ', byteLength: ' + array.byteLength);
  print('BYTES_PER_ELEMENT: ' + ctor.BYTES_PER_ELEMENT + ', ' + array.BYTES_PER_ELEMENT);
});

print("================Test Constructor================");
// Without New
try {
  let uint32 = SendableUint32Array([1, 2, 3, 4, 5]);
} catch (e) {
  print(e);
}

// From a length
let uint32 = new SendableUint32Array(2);
uint32[0] = 100;
print(uint32); // [100, 0]

// From an non-sendable array
uint32 = new SendableUint32Array([11, 22]);
print(uint32); // [11, 22]
uint32 = new SendableUint32Array(new Array(11, 22));
print(uint32); // [11, 22]

// From an sendable array
uint32 = new SendableUint32Array(new SendableArray(11, 22, 33));
print(uint32); // [11, 22, 33]

// From another non-sendable TypedArray
let uint8 = new SendableUint8Array(new Uint8Array([11, 22, 33]));
print(uint8);  // [11, 22, 33]
uint8 = new SendableUint8Array(new Uint32Array([1024, 11, 22]));
print(uint8); // [0, 22, 33]

// From another sendable TypedArray
uint8 = new SendableUint8Array(new SendableUint8Array([11, 22, 33]));
print(uint8); // [11, 22, 33]
uint8 = new SendableUint8Array(new SendableUint32Array([1024, 11, 22]));
print(uint8); // [0, 11, 22]

// From an non-sendable ArrayBuffer
try {
  const buffer = new ArrayBuffer(64);
  const float64 = new SendableFloat64Array(buffer, 8, 4);
} catch (e) {
  print('From an non-sendable ArrayBuffer: ' + e + ', errCode: ' + e.code);
}

// From an sendable ArrayBuffer
try {
  const buffer = new SendableArrayBuffer(64);
  const float64 = new SendableFloat64Array(buffer, 8, 4);
  float64.fill(0.1);
  print(float64);
} catch (e) {
  print("error: " + e);
}
try {
  let arrBuf = new SendableArrayBuffer(16);
  let sliced = new SendableInt16Array(arrBuf.slice(4, 10));
  print(sliced);
} catch (e) {
  print(e);
}
try {
  let arrBuf = new SendableArrayBuffer(16);
  let sliced = new SendableInt16Array(arrBuf.slice(4, 7));
  print(sliced);
} catch (e) {
  print(e);
}
try {
  let arrBuf = new SendableArrayBuffer(16);
  let sliced = new SendableInt32Array(arrBuf.slice(4, 11));
  print(sliced);
} catch (e) {
  print(e);
}
try {
  let arrBuf = new SendableArrayBuffer(16);
  let sliced = new SendableFloat64Array(arrBuf.slice(4, 11));
  print(sliced);
} catch (e) {
  print(e);
}
// From an arrayLike
class SendableObject {
  constructor() {
    'use sendable';
  }
};
let sObj = new SendableObject();
let obj = new Object();
uint32 = new SendableUint32Array({ [0]: sObj, [1]: obj, [2]: 300, [3]: 4000, length: 4 });
print("From an arrayLike: " + uint32.length + ", array: [" + uint32 + "]");

// From an iterable
const iterable = (function* () {
  yield* [1, 2, 3];
})();
let float64FromIterable = new SendableFloat64Array(iterable);
print("From an iterable: " + float64FromIterable.length + ", array: [" + float64FromIterable + "]");
print("================Test At================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  if (testTypedArrayAt1(ctor)) {
    print(ctor.name + ' test success');
  } else {
    print(ctor.name + ' test failed');
  }
});

[SendableBigInt64Array, SendableBigUint64Array].forEach((ctor: Function) => {
  if (testTypedArrayAt2(ctor)) {
    print(ctor.name + ' test success');
  } else {
    print(ctor.name + ' test failed');
  }
});

function testTypedArrayAt1(ctor) {
  let result = []
  let obj = new ctor(5);
  obj[0] = 10;
  obj[1] = 11;
  obj[2] = 12;
  obj[3] = 13;
  obj[4] = 14;
  result.push(obj.at(-1) == 14);
  result.push(obj.at(1.5) == 11);
  result.push(obj.at(-3) == 12);
  result.push(obj.at("3") == 13)
  result.push(obj.at(-6) == undefined);
  result.push(obj.at(5) == undefined);
  result.push(obj.at({}) == 10);
  for (let i = 0; i < result.length; i++) {
    if (!result[i]) {
      return false;
    }
  }
  return true;
}

function testTypedArrayAt2(ctor) {
  let result = []
  let obj = new ctor(5);
  obj[0] = 10n;
  obj[1] = 11n;
  obj[2] = 12n;
  obj[3] = 13n;
  obj[4] = 9017199254740995n;
  result.push(obj.at(-1) == 9017199254740995n);
  result.push(obj.at(1.5) == 11n);
  result.push(obj.at(-3) == 12n);
  result.push(obj.at("3") == 13n)
  result.push(obj.at(-6) == undefined);
  result.push(obj.at(5) == undefined);
  result.push(obj.at({}) == 10n);
  for (let i = 0; i < result.length; i++) {
    if (!result[i]) {
      return false;
    }
  }
  return true;
}

let uintc8 = new SendableUint8ClampedArray(2);
uintc8[0] = 42;
uintc8[1] = 1337;
print(uintc8[0]); // 42
print(uintc8[1]); // 255 (clamped)

print("================Test Join================");
let constructors = [
  [SendableUint8Array, [0, 1]],
  [SendableInt8Array, [0, 1]],
  [SendableUint16Array, [0, 1]],
  [SendableInt16Array, [0, 1]],
  [SendableUint32Array, [0, 1]],
  [SendableInt32Array, [0, 1]],
  [SendableFloat32Array, [0, 1]],
  [SendableFloat64Array, [0, 1]],
  [SendableUint8ClampedArray, [0, 1]],
  [SendableBigInt64Array, [0n, 1n]],
  [SendableBigUint64Array, [0n, 1n]],
];

let typedArray;
const separator = {
  toString() {
    ArkTools.arrayBufferDetach(typedArray.buffer);
    return '*';
  }
};

constructors.forEach(([constructor, arr]) => {
  typedArray = new constructor(arr);
  print(typedArray.join() == '0,1');
  print(typedArray.join('-') == '0-1');
  print(typedArray.join(separator) == '*'); // detach array buffer
});

try {
  let typedArray2 = new SendableInt8Array(65537);
  typedArray2.join(typedArray2);
} catch(e) {
  print(e);
}

print("================Test Subarray================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let obj = new ctor([10, 11, 12, 13, 14]);
  let result = obj.subarray();
  print(result);
  result = obj.subarray(1);
  print(result);
  result = obj.subarray(0, 5);
  print(result);
  result = obj.subarray(2, 10);
  print(result);
  result = obj.subarray(-1, 1);
  print(result.length);
  result = obj.subarray(2, -10);
  print(result.length);
});

print("================Test Includes================");
[
  [SendableUint8Array, [1, 2, 3]],
  [SendableInt8Array, [1, 2, 3]],
  [SendableUint16Array, [1, 2, 3]],
  [SendableInt16Array, [1, 2, 3]],
  [SendableUint32Array, [1, 2, 3]],
  [SendableInt32Array, [1, 2, 3]],
  [SendableFloat32Array, [1, 2, 3]],
  [SendableFloat64Array, [1, 2, 3]],
  [SendableUint8ClampedArray, [1, 2, 3]],
  [SendableBigInt64Array, [1n, 2n, 3n]],
  [SendableBigUint64Array, [1n, 2n, 3n]],
].forEach(([constructor, arr]) => {
  typedArray = new constructor(arr);
  if (typedArray instanceof SendableBigInt64Array || typedArray instanceof SendableBigUint64Array) {
    print(typedArray.includes(2n));
    print(typedArray.includes(4n));
    print(typedArray.includes(3n, 3));
  } else {
    print(typedArray.includes(2));
    print(typedArray.includes(4));
    print(typedArray.includes(3, 3));
  }
});

print("================Test Sort================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let array = new ctor([1, 21, 11, 31]);
  array.sort();
  print(array); // [1, 11, 21, 31]

  array = new ctor([1, 21, 11, 31]);
  array.sort((a: number, b: number) => {
    return b - a;
  });
  print(array); // [31, 21, 11, 1]
});

print("================Test Reverse================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let array = new ctor([1, 21, 11, 31]);
  array.reverse();
  print(array);
});

print("================Test Some================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const negatives = new ctor([-10, 20, -30, 40, -50]);
  const positives = new ctor([10, 20, 30, 40, 50]);
  print(
    negatives.some((element: number) => {
      return element < 0;
    }),
  );
  print(
    positives.some((element: number) => {
      return element < 0;
    }),
  );
});

print("================Test Every================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const negatives = new ctor([-10, -20, -30, -40, -50]);
  print(
    negatives.every((element: number) => {
      return element < 0;
    }),
  );
});

print("================Test Slice================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let array = new ctor([10, 20, 30, 40, 50]);
  let sliced = array.slice();
  print(sliced); // [10, 20, 30, 40, 50]
  sliced = array.slice(1);
  print(sliced); // [20, 30, 40, 50]
  sliced = array.slice(0, 5);
  print(sliced); // [10, 20, 30, 40, 50]
  sliced = array.slice(2, 10);
  print(sliced); // [30, 40, 50]
  sliced = array.slice(-1, 1);
  print(sliced.length); // 0
  sliced = array.slice(2, -10);
  print(sliced.length); // 0
});

print("================Test IndexOf================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const array = new ctor([10, 20, 30, 40, 50]);
  print(array.indexOf(50)); // 4
  print(array.indexOf(20, 3)); // -1
  print(array.indexOf(51)); // -1
});

print("================Test FindIndex================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const array = new ctor([10, -20, 30, -40, 50]);
  print(
    array.findIndex((element: number) => {
      return element < 0;
    }),
  );
});

print("================Test Find================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const array = new ctor([10, -20, 30, -40, 50]);
  print(
    array.find((element: number) => {
      return element < 0;
    }),
  );
});

print("================Test Fill================");
[SendableFloat64Array, SendableInt32Array, SendableInt16Array, SendableInt8Array, SendableUint8ClampedArray].forEach(
  (ctor: Function) => {
    print(new ctor([1, 2, 3]).fill(4)); // [4, 4, 4]
    print(new ctor([1, 2, 3]).fill(4, 1)); // [1, 4, 4]
    print(new ctor([1, 2, 3]).fill(4, 1, 2)); // [1, 4, 3]
    print(new ctor([1, 2, 3]).fill(4, 1, 1)); // [1, 2, 3]
    print(new ctor([1, 2, 3]).fill(4, -3, -2)); // [4, 2, 3]
  },
);

print("================Test Filter================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const array = new ctor([-10, 20, -30, 40, -50]);
  let filterd = array.filter((element: number) => {
    return element < 0;
  });
  print((filterd instanceof ctor) + ', filterd: [' + filterd + ']');
});

print("================Test CopyWith================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let array = new ctor([1, 2, 3, 4, 5, 6, 7, 8]);
  let copied = array.copyWithin(3, 1, 3);
  print((copied instanceof ctor) + ', copied: [' + copied + ']'); // [1, 2, 3, 2, 3, 6, 7, 8]
  array = new ctor([1, 2, 3, 4, 5, 6, 7, 8]);
  copied = array.copyWithin(3, 1);
  print(copied); // [1, 2, 3, 2, 3, 4, 5, 6]
});

print("================Test Map================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  let array = new ctor([25, 36, 49]);
  const mapped = array.map(Math.sqrt);
  print((mapped instanceof ctor) + ', mapped: [' + mapped + ']'); // [5, 6, 7]
});

print("================Test Keys/Values/Entries================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  const array = new ctor([10, 20, 30]);
  let keys = array.keys();
  let values = array.values();
  let entries = array.entries();
  for (let i = 0; i < array.length; i++) {
    print('key: ' + keys.next().value + ', value: ' + values.next().value + ', entries: ' + entries.next().value);
  }
});

print("================Test ForEach================");
new SendableUint32Array([0, 1, 2, 3]).forEach((element: number, index: number, array: SendableUint32Array) => {
  print(`array[${index}] = ${element}`);
});

try {
  new SendableUint32Array([0, 1, 2, 3]).forEach((element: number, index: number, array: SendableUint32Array) => {
    print(array.slice(1, 3));
  });
} catch (e) {
  print("ForEach: " + e + ", errCode: " + e.code);
}

try {
  new SendableUint32Array([0, 1, 2, 3]).forEach((element: number, index: number, array: SendableUint32Array) => {
    array.sort();
  });
} catch (e) {
  print("ForEach: " + e + ", errCode: " + e.code);
}

print("================Test From================");
[SendableFloat64Array, SendableInt32Array, SendableInt16Array, SendableInt8Array].forEach((ctor: Function) => {
  // From an iterable object (Set)
  let array = ctor.from(new Set([1, 2, 3]));
  print((array instanceof ctor) + ', array: [' + array + ']');
  // From a string
  array = ctor.from('123');
  print(array);
  // Use with arrow function and map
  array = ctor.from([1, 2, 3], (x) => x + x);
  print(array);
  // Generate a sequence of numbers
  Uint8Array.from({ length: 5 }, (v, k) => k);
  print(array);
});

print("================Test Set================");
let uint8Arr;
let uint16Arr;
let uint32Arr;
// Sendable array
uint8Arr = new SendableUint8Array(8);
uint8Arr.set(new SendableArray(1, 2, 3, 4, 5), 3);
print(uint8Arr);
// Non-sendable array
uint8Arr = new SendableUint8Array(8);
uint8Arr.set([1, 2, 3], 3);
print(uint8Arr);
// Sendable same type
uint8Arr = new SendableUint8Array(8);
uint8Arr.set(new SendableUint8Array([1, 2, 3, 4, 5, 6]), 2);
print(uint8Arr);
// Non-sendable same type
uint8Arr = new SendableUint8Array(8);
uint8Arr.set(new Uint8Array([1, 2, 3, 4, 5, 6]), 2);
print(uint8Arr);
// Sendable diff type
uint8Arr = new SendableUint8Array(8);
uint32Arr = new SendableUint32Array([1024, 1, 2])
uint8Arr.set(uint32Arr, 3);
print(uint8Arr);
// Non-sendable diff type
uint8Arr = new SendableUint8Array(8);
uint32Arr = new Uint32Array([1024, 1, 2])
uint8Arr.set(uint32Arr, 3);
print(uint8Arr);
// Set self
let uint8c = new Uint8ClampedArray(11);
uint8c.set(uint8c);
print(uint8c);
uint8Arr = new Uint8Array(11);
uint8Arr.fill(1);
uint8Arr.set(uint8Arr);
print(uint8Arr);
uint16Arr = new Uint16Array(11);
uint16Arr.fill(513);
uint16Arr.set(uint16Arr);
print(uint16Arr);
uint32Arr = new Uint32Array(11);
uint32Arr.fill(65536);
uint32Arr.set(uint32Arr);
print(uint32Arr);

print("================Test Freeze================");
try {
  let uint16 = new SendableUint16Array([1, 2, 3]);
  Object.freeze(uint16);
} catch (e) {
  print("Object.freeze error: " + e);
}
try {
  let uint16 = new SendableUint16Array();
  Object.freeze(uint16);
  print("Object.freeze freeze empty array success");
} catch (e) {
  print("Object.freeze error: " + e);
}
try {
  Object.defineProperty(SendableBigInt64Array.prototype, 'constructor', {
    get: function () {
      calls++;
    },
  });
} catch(e) {
  print("Object.defineProperty error: " + e);
}
try {
  let uint8 = new SendableUint8Array([1, 2, 3, 4]);
  uint8.nonExistProp = "nonExistProp";
} catch(e) {
  print("Add nonExistProp error: " + e);
}
try {
  let uint8 = new SendableUint8Array([1, 2, 3, 4]);
  uint8["nonExistProp"] = "nonExistProp";
} catch(e) {
  print("Add nonExistProp error: " + e);
}

print("================Test Inheritance================");
class SubUint32Array extends SendableUint32Array {
  constructor(params: any) {
    'use sendable';
    super(params);
  }
}

let subUint32Array = new SubUint32Array([1, 4, 3, 2, 5]);
print("array: [" + subUint32Array + "]" + ", sorted: [" + subUint32Array.sort() + "]");
print("length: " + subUint32Array.length + ", byteLength: " + subUint32Array.byteLength);
print("BYTES_PER_ELEMENT: " + subUint32Array.BYTES_PER_ELEMENT + ", " + SubUint32Array.BYTES_PER_ELEMENT);

class SubSubUint32Array extends SubUint32Array {
  constructor(params: any) {
    "use sendable";
    super(params)
  }
}
let subSubUint32Array = new SubSubUint32Array([1, 4, 3, 2, 5]);
print("array: [" + subSubUint32Array + "]" + ", sorted: [" + subSubUint32Array.sort() + "]");
print("length: " + subSubUint32Array.length + ", byteLength: " + subSubUint32Array.byteLength);
print("BYTES_PER_ELEMENT: " + subSubUint32Array.BYTES_PER_ELEMENT + ", " + SubSubUint32Array.BYTES_PER_ELEMENT);

print("================Test IC================");
[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor: Function) => {
  if (testTypeArrayIC(ctor)) {
    print(ctor.name + ' test IC success');
  } else {
    print(ctor.name + ' test IC failed');
  }
});
function testTypeArrayIC(ctor: Function) {
  let result = []
  let obj = new ctor(100);
  for (var i = 0; i < 100; i++) {
    obj[i] = i;
  }
  for (var i = 0; i < 100; i++) {
    result.push(obj[i] == i);
  }
  for (var i = 0; i < 100; i++) {
    result.push(obj.at(i) == i);
  }
  for (let i = 0; i < result.length; i++) {
    if (!result[i]) {
      return false;
    }
  }
  return true;
}

print("================Test ArrayBuffer================");
let sArrayBuffer : SendableArrayBuffer = new SendableArrayBuffer(16);
print("SendableArrayBuffer length: " + sArrayBuffer.byteLength);
let int32View: SendableInt32Array = new SendableInt32Array(sArrayBuffer);
int32View[1] = 42;
let sliced = new SendableInt32Array(sArrayBuffer.slice(4, 12));
print("SendableArrayBuffer sliced[0]: " + sliced[0]);

print(SendableArrayBuffer[Symbol.species] == SendableArrayBuffer);
print(SendableArrayBuffer.name == "SendableArrayBuffer");
print(sArrayBuffer[Symbol.toStringTag] == "SendableArrayBuffer");

let isview = SendableArrayBuffer.isView(new SendableInt32Array());
print('SendableArrayBuffer SendableInt32Array isView: ' + isview);
isview = SendableArrayBuffer.isView(new Int8Array(10));
print("SendableArrayBuffer Int8Array isView: " + isview);

class SubSendableArrayBuffer extends SendableArrayBuffer {
  constructor(params: any) {
    "use sendable";
    super(params);
  }
}
let subSArrayBuffer = new SubSendableArrayBuffer(20);
print("SubSendableArrayBuffer length: " + subSArrayBuffer.byteLength);
let int32View1: SendableInt32Array = new SendableInt32Array(subSArrayBuffer);
int32View1[2] = 5;
let slicedSub = new SendableInt32Array(subSArrayBuffer.slice(8, 16));
print("SubSendableArrayBuffer slicedSub[0]: " + slicedSub[0]);
let isviewSub = SubSendableArrayBuffer.isView(new SendableInt32Array());
print('SubSendableArrayBuffer SendableInt32Array isView: ' + isviewSub);
isviewSub = SendableArrayBuffer.isView(new Int8Array(10));
print("SendableArrayBuffer Int8Array isView: " + isviewSub);

function testReflectApply() {
  try {
    const v1 = new Uint8ClampedArray(Uint8ClampedArray, Uint8ClampedArray, Uint8ClampedArray);
    const v5 = new SendableUint16Array(1727);
    new SendableUint32Array(v5);
    const v8 = [v1];
    Reflect.apply(v1.slice, v5, v8);
    print("Reflect.apply slice success");
  } catch(e) {
    print("Reflect.apply slice error: " + e);
  }
}

testReflectApply()
