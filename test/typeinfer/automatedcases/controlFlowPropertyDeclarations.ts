/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/controlFlowPropertyDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from ##8913

declare let require:any;
AssertType(require, "any");

let HTMLDOMPropertyConfig = require('react/lib/HTMLDOMPropertyConfig');
AssertType(HTMLDOMPropertyConfig, "any");
AssertType(require('react/lib/HTMLDOMPropertyConfig'), "any");
AssertType(require, "any");
AssertType('react/lib/HTMLDOMPropertyConfig', "string");

// Populate property map with ReactJS's attribute and property mappings
// TODO handle/use .Properties value eg: MUST_USE_PROPERTY is not HTML attr
for (let propname in HTMLDOMPropertyConfig.Properties) {
  if (!HTMLDOMPropertyConfig.Properties.hasOwnProperty(propname)) {
AssertType(!HTMLDOMPropertyConfig.Properties.hasOwnProperty(propname), "boolean");
AssertType(HTMLDOMPropertyConfig.Properties.hasOwnProperty(propname), "any");
AssertType(HTMLDOMPropertyConfig.Properties.hasOwnProperty, "any");
AssertType(HTMLDOMPropertyConfig.Properties, "any");
AssertType(propname, "string");

    continue;
  }

  let mapFrom = HTMLDOMPropertyConfig.DOMAttributeNames[propname] || propname.toLowerCase();
AssertType(mapFrom, "any");
AssertType(HTMLDOMPropertyConfig.DOMAttributeNames[propname] || propname.toLowerCase(), "any");
AssertType(HTMLDOMPropertyConfig.DOMAttributeNames[propname], "any");
AssertType(HTMLDOMPropertyConfig.DOMAttributeNames, "any");
AssertType(propname, "string");
AssertType(propname.toLowerCase(), "string");
AssertType(propname.toLowerCase, "() => string");
}

/**
 * Repeats a string a certain number of times.
 * Also: the future is bright and consists of native string repetition:
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/repeat
 *
 * @param {string} string  String to repeat
 * @param {number} times   Number of times to repeat string. Integer.
 * @see http://jsperf.com/string-repeater/2
 */
function repeatString(string, times) {
  if (times === 1) {
AssertType(times === 1, "boolean");
AssertType(times, "any");
AssertType(1, "int");

AssertType(string, "any");
    return string;
  }
  if (times < 0) { throw new Error(); 
AssertType(times < 0, "boolean");

AssertType(times, "any");

AssertType(0, "int");

AssertType(new Error(), "Error");

AssertType(Error, "ErrorConstructor");
}

  let repeated = '';
AssertType(repeated, "string");
AssertType('', "string");

  while (times) {
AssertType(times, "any");

    if (times & 1) {
AssertType(times & 1, "number");
AssertType(times, "any");
AssertType(1, "int");

      repeated += string;
AssertType(repeated += string, "string");
AssertType(repeated, "string");
AssertType(string, "any");
    }
    if (times >>= 1) {
AssertType(times >>= 1, "number");
AssertType(times, "any");
AssertType(1, "int");

      string += string;
AssertType(string += string, "any");
AssertType(string, "any");
AssertType(string, "any");
    }
  }
AssertType(repeated, "string");
  return repeated;
}

/**
 * Determine if the string ends with the specified substring.
 *
 * @param {string} haystack String to search in
 * @param {string} needle   String to search for
 * @
return {boolean}
 */
function endsWith(haystack, needle) {
AssertType(haystack.slice(-needle.length) === needle, "boolean");
AssertType(haystack.slice(-needle.length), "any");
AssertType(haystack.slice, "any");
AssertType(-needle.length, "number");
AssertType(needle.length, "any");
AssertType(needle, "any");
  return haystack.slice(-needle.length) === needle;
}

/**
 * Trim the specified substring off the string. If the string does not end
 * with the specified substring, this is a no-op.
 *
 * @param {string} haystack String to search in
 * @param {string} needle   String to search for
 * @
return {string}
 */
function trimEnd(haystack, needle) {
AssertType(endsWith(haystack, needle)    ? haystack.slice(0, -needle.length)    : haystack, "any");
AssertType(endsWith(haystack, needle), "boolean");
AssertType(endsWith, "(any, any) => boolean");
AssertType(haystack, "any");
AssertType(needle, "any");
  return endsWith(haystack, needle)

    ? haystack.slice(0, -needle.length)
AssertType(haystack.slice(0, -needle.length), "any");
AssertType(haystack.slice, "any");
AssertType(0, "int");
AssertType(-needle.length, "number");
AssertType(needle.length, "any");

    : haystack;
AssertType(haystack, "any");
}

/**
 * Convert a hyphenated string to camelCase.
 */
function hyphenToCamelCase(string) {
AssertType(string.replace(/-(.)/g, function(match, chr) {    return chr.toUpperCase();  }), "any");
AssertType(string.replace, "any");
AssertType(/-(.)/g, "RegExp");
AssertType(function(match, chr) {    return chr.toUpperCase();  }, "(any, any) => any");
AssertType(match, "any");
AssertType(chr, "any");
  return string.replace(/-(.)/g, function(match, chr) {

AssertType(chr.toUpperCase(), "any");
AssertType(chr.toUpperCase, "any");
    return chr.toUpperCase();

  });
}

/**
 * Determines if the specified string consists entirely of whitespace.
 */
function isEmpty(string) {
AssertType(!/[^\s]/.test(string), "boolean");
AssertType(/[^\s]/.test(string), "boolean");
AssertType(/[^\s]/.test, "(string) => boolean");
AssertType(/[^\s]/, "RegExp");
AssertType(string, "any");
   return !/[^\s]/.test(string);
}

/**
 * Determines if the CSS value can be converted from a
 * 'px' suffixed string to a numeric value
 *
 * @param {string} value CSS property value
 * @
return {boolean}
 */
function isConvertiblePixelValue(value) {
AssertType(/^\d+px$/.test(value), "boolean");
AssertType(/^\d+px$/.test, "(string) => boolean");
AssertType(/^\d+px$/, "RegExp");
AssertType(value, "any");
  return /^\d+px$/.test(value);
}

export class HTMLtoJSX {
    private output: string;
    private level: number;
    private _inPreTag: boolean;


  /**
   * Handles processing of the specified text node
   *
   * @param {TextNode} node
   */
  _visitText = (node) => {
    let parentTag = node.parentNode && node.parentNode.tagName.toLowerCase();
AssertType(parentTag, "any");
AssertType(node.parentNode && node.parentNode.tagName.toLowerCase(), "any");
AssertType(node.parentNode, "any");
AssertType(node.parentNode.tagName.toLowerCase(), "any");
AssertType(node.parentNode.tagName.toLowerCase, "any");
AssertType(node.parentNode.tagName, "any");
AssertType(node.parentNode, "any");

    if (parentTag === 'textarea' || parentTag === 'style') {
AssertType(parentTag === 'textarea' || parentTag === 'style', "boolean");
AssertType(parentTag === 'textarea', "boolean");
AssertType(parentTag, "any");
AssertType('textarea', "string");
AssertType(parentTag === 'style', "boolean");
AssertType(parentTag, "any");
AssertType('style', "string");

      // Ignore text content of textareas and styles, as it will have already been moved
      // to a "defaultValue" attribute and "dangerouslySetInnerHTML" attribute respectively.
      return;
    }

    let text = ''
AssertType(text, "string");
AssertType('', "string");

    if (this._inPreTag) {
AssertType(this._inPreTag, "boolean");
AssertType(this, "this");

      // If this text is contained within a <pre>, we need to ensure the JSX
      // whitespace coalescing rules don't eat the whitespace. This means
      // wrapping newlines and sequences of two or more spaces in letiables.
      text = text
AssertType(text, "string");
AssertType(text        .replace(/\r/g, '')        .replace(/( {2,}|\n|\t|\{|\})/g, function(whitespace) {          return '{' + JSON.stringify(whitespace) + '}';        }), "string");
AssertType(text        .replace(/\r/g, '')        .replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(text        .replace(/\r/g, ''), "string");
AssertType(text        .replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(text = text        .replace(/\r/g, '')        .replace(/( {2,}|\n|\t|\{|\})/g, function(whitespace) {          return '{' + JSON.stringify(whitespace) + '}';        }), "string");

        .replace(/\r/g, '')
AssertType(/\r/g, "RegExp");
AssertType('', "string");

        .replace(/( {2,}|\n|\t|\{|\})/g, function(whitespace) {
AssertType(/( {2,}|\n|\t|\{|\})/g, "RegExp");
AssertType(whitespace, "string");
AssertType(function(whitespace) {          return '{' + JSON.stringify(whitespace) + '}';        }, "(string) => string");

AssertType('{' + JSON.stringify(whitespace) + '}', "string");
AssertType('{' + JSON.stringify(whitespace), "string");
AssertType('{', "string");
AssertType(JSON.stringify(whitespace), "string");
AssertType(JSON.stringify, "{ (any, ?(any, string, any) => any, ?union): string; (any, ?(union)[], ?union): string; }");
AssertType(whitespace, "string");
AssertType('}', "string");
          return '{' + JSON.stringify(whitespace) + '}';

        });
    } else {
      // If there's a newline in the text, adjust the indent level
      if (text.indexOf('\n') > -1) {
AssertType(text.indexOf('\n') > -1, "boolean");
AssertType(text.indexOf('\n'), "number");
AssertType(text.indexOf, "(string, ?number) => number");
AssertType('\n', "string");
AssertType(-1, "int");
AssertType(1, "int");
      }
    }
    this.output += text;
AssertType(this.output += text, "string");
AssertType(this.output, "string");
AssertType(this, "this");
AssertType(text, "string");
  }



};

/**
 * Handles parsing of inline styles
 */
export class StyleParser {
  styles = {};
  toJSXString = () => {
    for (let key in this.styles) {
AssertType(key, "string");
AssertType(this.styles, "{}");
AssertType(this, "this");

      if (!this.styles.hasOwnProperty(key)) {
AssertType(!this.styles.hasOwnProperty(key), "boolean");
AssertType(this.styles.hasOwnProperty(key), "boolean");
AssertType(this.styles.hasOwnProperty, "(PropertyKey) => boolean");
AssertType(this.styles, "{}");
AssertType(this, "this");
AssertType(key, "string");
      }
    }
  }
}

