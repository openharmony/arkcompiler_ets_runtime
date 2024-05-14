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

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog reacted, was it really lazy?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.replace(/dog/g, 'monkey');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;

    print("string replace regexp flag is g: " + time3.toString());
}

{
    const re = /apples/gi;
    const str = 'Apples are round, and apples are juicy.';
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.replace(re, 'Christmas');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string replace regexp flag is gi :" + time3);
}

{
    function replacer(match, p1, p2, p3, offset, string) {
        return [p1, p2, p3].join(' - ');
    }
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        'abc12345#$*%'.replace(/([^\d]*)(\d*)([^\w]*)/, replacer);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string replace regexp pattern is /([^\d]*)(\d*)([^\w]*)/ : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.split(" ", 3);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string split number of input parameters is 2 : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog."
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.split(" ");
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string split number of input parameters is 1 : " + time3);
}

{
    const str = "Harry Trump ;Fred Barney; Helen Rigby ; Bill Abel ;Chris Hand "
    const re = /\s*(?:;|$)\s*/;
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.split(re);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string split into regexp.split : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog barked, was it really lazy?"
    const searchTerm = 'dog';
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.indexOf(searchTerm);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string indexOf input parameter is a substring : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog barked, was it really lazy?"
    const searchTerm = 'dDDog';
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.indexOf(searchTerm);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string indexOf Input parameter is not a substring : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog barked, was it really lazy?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.indexOf("",10);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string indexOf number of input parameters is 2 : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.slice(14);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string slice number of input parameters is 1 : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog."
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.slice(4, 19);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string slice number of input parameters is 2 : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.slice(-9, -5);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string slice input parameter is negative : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substring(1,10);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substring the first parameter is less than the second parameter : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog."
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substring(19,3);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substring the first parameter is greater than the second parameter : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substring(4,4);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substring The first parameter is equal to the second parameter : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substr(1,2);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substr the two parameters are adjacent numbers : " + time3);
}

{
    const str = 'abcdefghij'
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substr(-3,2);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substr negative number in parameter : " + time3);
}

{
    const str = "abcdefghij"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.substr(1,7);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string substr number of input parameters is 2 : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog reacted, was it really lazy?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.replace('dog', 'monkey');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string replace into string.replace searchtag in the middle of the input parameter : " + time3);
}

{
    const str =  'Twas the night before Xmas...'
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.replace('Twas', 'Christmas');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string replace into string.replace searchtag in the beginning of the input parameter : " + time3);
}

{
    const str = "The quick brown fox jumps over the lazy dog. If the dog reacted, was it really lazy?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.replace('fox', 'monkey');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string replace into string.repalce : " + time3);
}

{
    const str = "abc"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.repeat(3);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string repeat the parameter is integer : " + time3);
}

{
    const str = "abc"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.repeat(3.5);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string repeat the parameter is double : " + time3);
}

{
    const str = "abc"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.repeat(1);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string repeat the return value is himself : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.startsWith('Hel');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string startsWith return true : " + time3);
}

{
    const str =  'Saturday night plans';
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.startsWith('Sat');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string startsWith return true : " + time3);
}

{
    const str =  'Saturday night plans';
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.startsWith('Sat', 3);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string startsWith return false : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charCodeAt(4);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charCodeAt small input parameter value : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charCodeAt(10);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charCodeAt bigger input parameter value : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charCodeAt(99);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charCodeAt index out of range : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charAt(4);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charAt small input parameter value : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charAt(10);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charAt bigger input parameter value : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.charAt(99);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string charAt index out of range : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toLowerCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toLowerCase Hello World. How are you doing? : " + time3);
}

{
    const str = 'The quick brown fox jumps over the lazy dog.'
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toLowerCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toLowerCase The quick brown fox jumps over the lazy dog : " + time3);
}

{
    const str = "Hello World"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toLowerCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toLowerCase Hello World : " + time3);
}

{
    const str = "Hello World. How are you doing?"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toUpperCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toUpperCase Hello World. How are you doing? : " + time3);
}

{
    const str = "Hello World"
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toUpperCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toUpperCase Hello World : " + time3);
}

{
    const str = 'The quick brown fox jumps over the lazy dog.'
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.toUpperCase();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string toUpperCase The quick brown fox jumps over the lazy dog. : " + time3);
}

{
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        'a'.localeCompare('c');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string localeCompare letter a is before c yielding a negative value : " + time3);
}

{
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        'check'.localeCompare('against');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string localeCompare Alphabetically the word check after against yielding a positive value : " + time3);
}

{
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        'a'.localeCompare('a');
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("string localeCompare a and a are equivalent yielding a neutral value of zero : " + time3);
}

{
    const str = "   This is a test string to test the interface   "
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.trim();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("  This is a test string to test the interface    string trim : " + time3);
}

{
    const str =  '   Hello world!   '
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.trim();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("   Hello world!     string trim : " + time3);
}

{
    const str = "   This is a test string   "
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        str.trim();
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("   This is a test string    string trim : " + time3);
}