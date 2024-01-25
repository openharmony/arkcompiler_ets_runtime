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

declare function print(a:any):string;

var LOOP_TIME = 100;

class C {
    x:number;
    constructor() {
        this.x = 123;
    }
}

let c1 = new C();
print(c1.x);

let c2 = new C();
print(c2.x);


function foo(p) {
    return p[1]
}

{
	let a = [1, 2]
	let b = [1, 2.1, 3]
	for (let i = 0; i < 10000; i++) {
		var a1 = foo(a)
		var a2 = foo(b);
	}
	print(a1)
	print(a2)
}

// test stownbyindex ic: (too many loop will fail at debug unittest timeout)
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

for (let i = 0; i < LOOP_TIME; i++){
	for (let j = 0; j < 5; ++j) {
		var _attrs : Attrs[] = [
			new Attrs(1, 1, 1),
			new Attrs(2, 2, 2),
			new Attrs(3, 3, 3),
			new Attrs(4, 4, 4),
			new Attrs(5, 5, 5),
		];
		if (i == LOOP_TIME - 1) {
			print(_attrs[j].x);
		}
	}
}

const tests = [
	[99.99, "ark"],
	[-88.48, "brk"],
	[-1024, "crk"],
	[6666, "drk"],
];

for (let i = 0; i < LOOP_TIME; i++){
    for (const [number, strData] of tests) {
		if (i == LOOP_TIME - 1) {
			print(number);
			print(strData);
		}
    }
}

const exceptions = {
	"minute" : {
	  	"-1" : "1 minute ago",
	  	'0' : 'this minute',
	  	"1" : "in 1 minute",
	},
	"second" : {
	  	"-1" : "1 second ago",
	  	"0" : "now",
	  	"1" : "in 1 second",
	},
};

const units = [
	"second",
	"minute",
];

for (let i = 0; i < LOOP_TIME; i++){
    for (const unit of units) {
		const expected = unit in exceptions ?
			[exceptions[unit]["1"], exceptions[unit]["0"], exceptions[unit]["0"], exceptions[unit]["-1"]] :
			[`in 1 ${unit}`, `in 0 ${unit}s`, `0 ${unit}s ago`, `1 ${unit} ago`];
		if (i == LOOP_TIME - 1) {
			print(expected);
		}
    }
}

for (let i = 0; i < LOOP_TIME; i++){
    Object.defineProperty(Array.prototype, '0', {
        value: 42,
    });
    let arr = [, 2, 3];
    arr.pop();
	if (i == LOOP_TIME - 1) {
		print(arr[1]);
	}
}


for (let i = 0; i < LOOP_TIME; i++){
    var a : any[] = [ ["ok"], [1, 2, 3] ];
    var b = {
        1 : ["yes", "no"],
        "100" : [4, 5],
    }
	if (i == LOOP_TIME - 1) {
		print(a[0][0]);
		print(a[1][0]);
		print(a[1][1]);
		print(a[1][2]);
		
		print(b[1][0]);
		print(b[1][1]);
		print(b["100"][0]);
		print(b["100"][1]);
	}
}


