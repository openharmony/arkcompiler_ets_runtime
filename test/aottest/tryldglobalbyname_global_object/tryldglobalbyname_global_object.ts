/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
// @ts-nocheck
declare function print(arg: any, arg1?: any): string;

print(Function);
globalThis.Function = "Function";
print(Function);
delete globalThis.Function;
try {
  print(Function);
} catch (error) {
  print(error);
}

print(RangeError);
globalThis.RangeError = "RangeError";
print(RangeError);
delete globalThis.RangeError;
try {
  print(RangeError);
} catch (error) {
  print(error);
}

print(Error);
globalThis.Error = "Error";
print(Error);
delete globalThis.Error;
try {
  print(Error);
} catch (error) {
  print(error);
}

print(Object);
globalThis.Object = "Object";
print(Object);
delete globalThis.Object;
try {
  print(Object);
} catch (error) {
  print(error);
}

print(SyntaxError);
globalThis.SyntaxError = "SyntaxError";
print(SyntaxError);
delete globalThis.SyntaxError;
try {
  print(SyntaxError);
} catch (error) {
  print(error);
}

print(TypeError);
globalThis.TypeError = "TypeError";
print(TypeError);
delete globalThis.TypeError;
try {
  print(TypeError);
} catch (error) {
  print(error);
}

print(ReferenceError);
globalThis.ReferenceError = "ReferenceError";
print(ReferenceError);
delete globalThis.ReferenceError;
try {
  print(ReferenceError);
} catch (error) {
  print(error);
}

print(URIError);
globalThis.URIError = "URIError";
print(URIError);
delete globalThis.URIError;
try {
  print(URIError);
} catch (error) {
  print(error);
}

print(Symbol);
globalThis.Symbol = "Symbol";
print(Symbol);
delete globalThis.Symbol;
try {
  print(Symbol);
} catch (error) {
  print(error);
}

print(EvalError);
globalThis.EvalError = "EvalError";
print(EvalError);
delete globalThis.EvalError;
try {
  print(EvalError);
} catch (error) {
  print(error);
}

print(Number);
globalThis.Number = "Number";
print(Number);
delete globalThis.Number;
try {
  print(Number);
} catch (error) {
  print(error);
}

print(parseFloat);
globalThis.parseFloat = "parseFloat";
print(parseFloat);
delete globalThis.parseFloat;
try {
  print(parseFloat);
} catch (error) {
  print(error);
}

print(Date);
globalThis.Date = "Date";
print(Date);
delete globalThis.Date;
try {
  print(Date);
} catch (error) {
  print(error);
}

print(Boolean);
globalThis.Boolean = "Boolean";
print(Boolean);
delete globalThis.Boolean;
try {
  print(Boolean);
} catch (error) {
  print(error);
}

print(BigInt);
globalThis.BigInt = "BigInt";
print(BigInt);
delete globalThis.BigInt;
try {
  print(BigInt);
} catch (error) {
  print(error);
}

print(parseInt);
globalThis.parseInt = "parseInt";
print(parseInt);
delete globalThis.parseInt;
try {
  print(parseInt);
} catch (error) {
  print(error);
}

print(WeakMap);
globalThis.WeakMap = "WeakMap";
print(WeakMap);
delete globalThis.WeakMap;
try {
  print(WeakMap);
} catch (error) {
  print(error);
}

print(RegExp);
globalThis.RegExp = "RegExp";
print(RegExp);
delete globalThis.RegExp;
try {
  print(RegExp);
} catch (error) {
  print(error);
}

print(Set);
globalThis.Set = "Set";
print(Set);
delete globalThis.Set;
try {
  print(Set);
} catch (error) {
  print(error);
}

print(Map);
globalThis.Map = "Map";
print(Map);
delete globalThis.Map;
try {
  print(Map);
} catch (error) {
  print(error);
}

print(WeakRef);
globalThis.WeakRef = "WeakRef";
print(WeakRef);
delete globalThis.WeakRef;
try {
  print(WeakRef);
} catch (error) {
  print(error);
}

print(WeakSet);
globalThis.WeakSet = "WeakSet";
print(WeakSet);
delete globalThis.WeakSet;
try {
  print(WeakSet);
} catch (error) {
  print(error);
}

print(FinalizationRegistry);
globalThis.FinalizationRegistry = "FinalizationRegistry";
print(FinalizationRegistry);
delete globalThis.FinalizationRegistry;
try {
  print(FinalizationRegistry);
} catch (error) {
  print(error);
}

print(Array);
globalThis.Array = "Array";
print(Array);
delete globalThis.Array;
try {
  print(Array);
} catch (error) {
  print(error);
}

print(Uint8ClampedArray);
globalThis.Uint8ClampedArray = "Uint8ClampedArray";
print(Uint8ClampedArray);
delete globalThis.Uint8ClampedArray;
try {
  print(Uint8ClampedArray);
} catch (error) {
  print(error);
}

print(Uint8Array);
globalThis.Uint8Array = "Uint8Array";
print(Uint8Array);
delete globalThis.Uint8Array;
try {
  print(Uint8Array);
} catch (error) {
  print(error);
}

print(TypedArray);
globalThis.TypedArray = "TypedArray";
print(TypedArray);
delete globalThis.TypedArray;
try {
  print(TypedArray);
} catch (error) {
  print(error);
}

print(Int8Array);
globalThis.Int8Array = "Int8Array";
print(Int8Array);
delete globalThis.Int8Array;
try {
  print(Int8Array);
} catch (error) {
  print(error);
}

print(Uint16Array);
globalThis.Uint16Array = "Uint16Array";
print(Uint16Array);
delete globalThis.Uint16Array;
try {
  print(Uint16Array);
} catch (error) {
  print(error);
}

print(Uint32Array);
globalThis.Uint32Array = "Uint32Array";
print(Uint32Array);
delete globalThis.Uint32Array;
try {
  print(Uint32Array);
} catch (error) {
  print(error);
}

print(Int16Array);
globalThis.Int16Array = "Int16Array";
print(Int16Array);
delete globalThis.Int16Array;
try {
  print(Int16Array);
} catch (error) {
  print(error);
}

print(Int32Array);
globalThis.Int32Array = "Int32Array";
print(Int32Array);
delete globalThis.Int32Array;
try {
  print(Int32Array);
} catch (error) {
  print(error);
}

print(Float32Array);
globalThis.Float32Array = "Float32Array";
print(Float32Array);
delete globalThis.Float32Array;
try {
  print(Float32Array);
} catch (error) {
  print(error);
}

print(Float64Array);
globalThis.Float64Array = "Float64Array";
print(Float64Array);
delete globalThis.Float64Array;
try {
  print(Float64Array);
} catch (error) {
  print(error);
}

print(BigInt64Array);
globalThis.BigInt64Array = "BigInt64Array";
print(BigInt64Array);
delete globalThis.BigInt64Array;
try {
  print(BigInt64Array);
} catch (error) {
  print(error);
}

print(BigUint64Array);
globalThis.BigUint64Array = "BigUint64Array";
print(BigUint64Array);
delete globalThis.BigUint64Array;
try {
  print(BigUint64Array);
} catch (error) {
  print(error);
}

print(SharedArrayBuffer);
globalThis.SharedArrayBuffer = "SharedArrayBuffer";
print(SharedArrayBuffer);
delete globalThis.SharedArrayBuffer;
try {
  print(SharedArrayBuffer);
} catch (error) {
  print(error);
}

print(DataView);
globalThis.DataView = "DataView";
print(DataView);
delete globalThis.DataView;
try {
  print(DataView);
} catch (error) {
  print(error);
}

print(String);
globalThis.String = "String";
print(String);
delete globalThis.String;
try {
  print(String);
} catch (error) {
  print(error);
}

print(ArrayBuffer);
globalThis.ArrayBuffer = "ArrayBuffer";
print(ArrayBuffer);
delete globalThis.ArrayBuffer;
try {
  print(ArrayBuffer);
} catch (error) {
  print(error);
}

print(eval);
globalThis.eval = "eval";
print(eval);
delete globalThis.eval;
try {
  print(eval);
} catch (error) {
  print(error);
}

print(isFinite);
globalThis.isFinite = "isFinite";
print(isFinite);
delete globalThis.isFinite;
try {
  print(isFinite);
} catch (error) {
  print(error);
}

print(decodeURI);
globalThis.decodeURI = "decodeURI";
print(decodeURI);
delete globalThis.decodeURI;
try {
  print(decodeURI);
} catch (error) {
  print(error);
}

print(decodeURIComponent);
globalThis.decodeURIComponent = "decodeURIComponent";
print(decodeURIComponent);
delete globalThis.decodeURIComponent;
try {
  print(decodeURIComponent);
} catch (error) {
  print(error);
}

print(isNaN);
globalThis.isNaN = "isNaN";
print(isNaN);
delete globalThis.isNaN;
try {
  print(isNaN);
} catch (error) {
  print(error);
}

print(encodeURI);
globalThis.encodeURI = "encodeURI";
print(encodeURI);
delete globalThis.encodeURI;
try {
  print(encodeURI);
} catch (error) {
  print(error);
}

print(encodeURIComponent);
globalThis.encodeURIComponent = "encodeURIComponent";
print(encodeURIComponent);
delete globalThis.encodeURIComponent;
try {
  print(encodeURIComponent);
} catch (error) {
  print(error);
}

print(Math);
globalThis.Math = "Math";
print(Math);
delete globalThis.Math;
try {
  print(Math);
} catch (error) {
  print(error);
}

print(JSON);
globalThis.JSON = "JSON";
print(JSON);
delete globalThis.JSON;
try {
  print(JSON);
} catch (error) {
  print(error);
}

print(Atomics);
globalThis.Atomics = "Atomics";
print(Atomics);
delete globalThis.Atomics;
try {
  print(Atomics);
} catch (error) {
  print(error);
}

print(Reflect);
globalThis.Reflect = "Reflect";
print(Reflect);
delete globalThis.Reflect;
try {
  print(Reflect);
} catch (error) {
  print(error);
}

print(Promise);
globalThis.Promise = "Promise";
print(Promise);
delete globalThis.Promise;
try {
  print(Promise);
} catch (error) {
  print(error);
}

print(Proxy);
globalThis.Proxy = "Proxy";
print(Proxy);
delete globalThis.Proxy;
try {
  print(Proxy);
} catch (error) {
  print(error);
}

print(GeneratorFunction);
globalThis.GeneratorFunction = "GeneratorFunction";
print(GeneratorFunction);
delete globalThis.GeneratorFunction;
try {
  print(GeneratorFunction);
} catch (error) {
  print(error);
}

print(Intl);
globalThis.Intl = "Intl";
print(Intl);
delete globalThis.Intl;
try {
  print(Intl);
} catch (error) {
  print(error);
}
