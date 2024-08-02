/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str:any):string;
var a : any[] = [ ["ok"], [1, 2, 3] ];
print(a[0][0]);
print(a[1][0]);
print(a[1][1]);
print(a[1][2]);

var b = {
    1 : ["yes", "no"],
    "100" : [4, 5],
}
print(b[1][0]);
print(b[1][1]);
print(b["100"][0]);
print(b["100"][1]);

class Attrs {
	constructor(a : number, b : number, c : number) {
		this.x = a;
		this.y = b;
		this.z = c;
	}
	x : number;
	y : number;
	z : number;
}
function test() : void {
	for (let j = 0; j < 5; ++j) {
		var _attrs : Attrs[] = [
			new Attrs(1, 1, 1),
			new Attrs(2, 2, 2),
			new Attrs(3, 3, 3),
			new Attrs(4, 4, 4),
			new Attrs(5, 5, 5),
		];
        print(_attrs[j].x);
	}
}
test();

const tests = [
	[
	  	99.99,
	  	"ark"
	],
	[
	  	-88.48,
	  	"brk"
	],
	[
		-1024,
		"crk"
  	],
	[
		6666,
		"drk"
  	],
];
  
for (const [number, strData] of tests) {
  	print(number);
  	print(strData);
}

const units = [
	"second",
	"minute",
];
  
const exceptions = {
	"minute": {
	  	"-1": "1 minute ago",
	  	'0': 'this minute',
	  	"1": "in 1 minute",
	},
	"second": {
	  	"-1": "1 second ago",
	  	"0": "now",
	  	"1": "in 1 second",
	},
};

{
	const v1 = new SendableInt8Array(SendableInt8Array, SendableInt8Array, SendableInt8Array);
	let a = v1[Symbol.toStringTag];
	let c = [a];
}
  
for (const unit of units) {
	const expected = unit in exceptions
		? [exceptions[unit]["1"], exceptions[unit]["0"], exceptions[unit]["0"], exceptions[unit]["-1"]]
	  	: [`in 1 ${unit}`, `in 0 ${unit}s`, `0 ${unit}s ago`, `1 ${unit} ago`];
  
	print(expected);
}

Object.defineProperty(Array.prototype, '0', {
	value: 42,
});
let arr = [, 2, 3];
arr.pop();
print(arr[1]);

// TestTypedArrayFindLast
{
	const LOOP_TIME = 20;

	function func(element, Last, array) {
		return element <= 0;
	}

	function TestTypedArrayFindLast(e) {
		let arr = new e([-5, 10, 20, 30, -40, 50.5, -60.6]);
		let result = arr.findLast(func);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Int8Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Uint8Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Uint8ClampedArray);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Int16Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Uint16Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Int32Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Uint32Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Float32Array);
	}

	for (let i = 0; i < LOOP_TIME; i++) {
		TestTypedArrayFindLast(Float64Array);
	}
}
