/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
(function (S, T) {
  typeof exports == "object" && typeof module == "object" ? module.exports = T() : typeof define == "function" && define.amd ? define([], T) : typeof exports == "object" ? exports.modoc = T() : S.modoc = T();
})(this, () => (() => {
  var L = {
    963: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var d;

      (function (y) {
        y[y.PUSHED = 0] = "PUSHED", y[y.MERGED = 1] = "MERGED", y[y.REORDERED = 2] = "REORDERED";
      })(d = p.PushResult || (p.PushResult = {}));

      var g = function () {
        function y(h) {
          h === void 0 && (h = []), this.stringified = !1, this.ops = h;
        }

        return y.prototype.push = function (h) {
          if (this.stringified) throw new Error("Cannot push new operators to a stringified operatorList");
          var c = this.ops.length;
          if (c === 0) return this.ops.push(h), d.PUSHED;
          var f = d.PUSHED,
              i = this.ops[c - 1];
          if (i.action === "remove" && h.action === "insert" && (c -= 1, i = this.ops[c - 1], f = d.REORDERED, i === void 0)) return this.ops.unshift(h), f;
          var a = i.merge(h);
          return a ? (this.ops[c - 1] = a, f = d.MERGED) : c === this.ops.length ? this.ops.push(h) : this.ops.splice(c, 0, h), f;
        }, y.prototype.stringify = function (h) {
          h === void 0 && (h = !1), this.stringified = !0;

          for (var c = this.ops.length, f = "", i = 0; i < c; i++) {
            var a = this.ops[i];
            (!h || i < c - 1 || !a.isEmpty()) && (f += a.stringify());
          }

          return f;
        }, y;
      }();

      p.default = g;
    },
    827: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      }), p.keyToGul = {
        color: "0",
        background: "1",
        "font-size": "2",
        width: "3",
        height: "4",
        rowspan: "5",
        colspan: "6",
        align: "7",
        bold: "8",
        italic: "9",
        underline: "a",
        vertical: "b",
        strike: "c",
        wrap: "d",
        fixedRowsTop: "e",
        fixedColumnsLeft: "f",
        link: "g",
        format: "h",
        formula: "i",
        filter: "j",
        "border-top": "k",
        "border-right": "l",
        "border-bottom": "m",
        "border-left": "n",
        line: "o",
        guid: "p",
        author: "q",
        size: "r",
        list: "s",
        layout: "t",
        margin: "u",
        font: "v",
        header: "w",
        indent: "x",
        comment: "y",
        "comment-block": "z",
        "code-block": "A",
        name: "B",
        order: "C",
        autoFit: "D",
        userID: "E",
        nextIndex: "F",
        id: "G",
        colVisible: "H",
        rowVisible: "I",
        autoFormatter: "J",
        copyIndex: "K",
        manualAlign: "L",
        lock: "M",
        left: "N",
        top: "O"
      };
      var d = ["color", "background", "font-size", "width", "height", "rowspan", "colspan", "align", "bold", "italic", "underline", "vertical", "strike", "wrap", "fixedRowsTop", "fixedColumnsLeft", "link", "format", "formula", "filter", "border-top", "border-right", "border-bottom", "border-left", "line", "guid", "author", "size", "list", "layout", "margin", "font", "header", "indent", "comment", "comment-block", "code-block", "name", "order", "autoFit", "userID", "nextIndex", "id", "colVisible", "rowVisible", "autoFormatter", "copyIndex", "manualAlign", "lock", "left", "top", "cellValue"];

      function g(e) {
        var o = e.charCodeAt(0),
            r;
        if (o >= 48 && o <= 57) r = d[o - 48];else if (o >= 65 && o <= 90) r = d[o - 29];else if (o >= 97 && o <= 122) r = d[o - 87];else throw new Error('Unknown gul "' + e + '"');
        return r;
      }

      p.getAttributeKeyByGul = g;

      function y(e) {
        var o = p.keyToGul[e];
        if (o) return o;
        var r = e.length.toString(36),
            t = r.length;
        if (t > 2) throw new Error("Key length of attribute should <= 2: " + r);
        var n = t === 1 ? "$" : "%";
        return "" + n + r + e;
      }

      p.getGulByAttributeKey = y;

      function h(e, o) {
        var r = y(e);
        return typeof o == "string" ? r + ":" + o : typeof o == "number" ? r + "*" + o.toString() : typeof o == "boolean" ? r + (o ? "1" : "0") : r;
      }

      p.combinedAttributeWithValue = h, p.keyValueToGul = {
        81: "!",
        "2*12": '"',
        "2*11": "#",
        "2*10": "$",
        "2*9": "%",
        "2*8": "&",
        "6*2": "'",
        "6*3": "(",
        "6*4": ")",
        "5*2": "*",
        "5*3": "+",
        "5*4": ",",
        "0:#ff0000": "-",
        "0:#ffffff": ".",
        "b:bottom": "/",
        "h:normal": "0",
        1: "1",
        "1:#ffffff": "2",
        "1:#ff0000": "3",
        "1:#ffff00": "4",
        "1:#00ffff": "5",
        "7:left": "6",
        "7:center": "7",
        "7:right": "8",
        "1*4": "9",
        "d:text-wrap": ":",
        "d:text-no-wrap": ";",
        "h:YYYY/MM/DD": "<",
        'l:["",1]': "=",
        'n:["",1]': ">",
        'k:["",1]': "?",
        'm:["",1]': "@",
        'l:["#2b2b2b",1]': "A",
        'n:["#2b2b2b",1]': "B",
        'k:["#2b2b2b",1]': "C",
        'm:["#2b2b2b",1]': "D"
      };
      var c = [["bold", !0], ["font-size", 12], ["font-size", 11], ["font-size", 10], ["font-size", 9], ["font-size", 8], ["colspan", 2], ["colspan", 3], ["colspan", 4], ["rowspan", 2], ["rowspan", 3], ["rowspan", 4], ["color", "#ff0000"], ["color", "#ffffff"], ["vertical", "bottom"], ["format", "normal"], ["background", null], ["background", "#ffffff"], ["background", "#ff0000"], ["background", "#ffff00"], ["background", "#00ffff"], ["align", "left"], ["align", "center"], ["align", "right"], ["background", 4], ["wrap", "text-wrap"], ["wrap", "text-no-wrap"], ["format", "YYYY/MM/DD"], ["border-right", '["",1]'], ["border-left", '["",1]'], ["border-top", '["",1]'], ["border-bottom", '["",1]'], ["border-right", '["#2b2b2b",1]'], ["border-left", '["#2b2b2b",1]'], ["border-top", '["#2b2b2b",1]'], ["border-bottom", '["#2b2b2b",1]']];

      function f(e) {
        return p.keyValueToGul.hasOwnProperty(e) ? p.keyValueToGul[e] : null;
      }

      p.getGulByCombinedKeyValue = f;

      function i(e) {
        var o = e.charCodeAt(0) - 33;
        return c[o];
      }

      p.getKeyValueByGul = i;

      function a(e) {
        var o;

        if (e[0] === "$") {
          var r = parseInt(e[1], 36);
          o = e.substring(2, 2 + r), e = e.substring(1 + r);
        } else if (e[0] === "%") {
          var r = parseInt(e[1] + e[2], 36);
          o = e.substring(3, 3 + r), e = e.substring(2 + r);
        } else o = g(e[0]);

        if (e.length === 1) return [o, null];
        var t = e[1];
        if (t === "1") return [o, !0];
        if (t === "0") return [o, !1];
        var n = e.substring(2);
        return [o, t === ":" ? n : Number(n)];
      }

      p.parseCombinedAttributeValue = a;
    },
    433: (m, p, d) => {
      "use strict";

      var g = d(125);
      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(511),
          h = d(827),
          c = d(472),
          f = !1,
          i = !1,
          a = !1,
          e = {},
          o = function () {
        function r(t) {
          t === void 0 && (t = {
            array: [],
            packedData: ""
          }), this[y.TYPE_PROPERTY] = "Pool", this.cache = {}, typeof t == "string" ? (this.array = JSON.parse(t), this.packedData = t) : (this.array = t.array, this.packedData = t.packedData);
        }

        return r.isPool = function (t) {
          return g(t) === "object" && t !== null && t[y.TYPE_PROPERTY] === "Pool";
        }, r.setIPoolConfig = function (t) {
          t && (f = !!t.enableCache, i = !!t.enableNullIgnore, a = !!t.rawStringIgnoreAttrs, e = t.rawStringIgnoreAttrs || {});
        }, r.prototype.isSubsetOf = function (t) {
          if (this === t) return !0;
          var n = this.array.length;
          if (n === 0) return !0;
          if (n > t.array.length) return !1;

          for (var s = 0; s < n; s++) if (this.array[s] !== t.array[s]) return !1;

          return !0;
        }, r.prototype.getMap = function () {
          if (!this.map) {
            this.map = {};

            for (var t = 0; t < this.array.length; t++) this.map[this.array[t]] = String(t);
          }

          return this.map;
        }, r.prototype.packAttributes = function (t) {
          if (!t) return "";
          var n = Object.keys(t),
              s = n.length;
          if (s === 0) return "";

          for (var u = [], l = [], v = 0; v < s; v++) {
            var w = n[v],
                b = t[w];
            if (g(b) === "object" && b !== null) for (var k = c.objectToKeyPaths(b, "." + w), A = 0, D = k; A < D.length; A++) {
              var P = D[A],
                  O = this.getAddress(P[0], P[1]);
              O[0] === " " ? u.push(O) : l.push(O);
            } else {
              var O = this.getAddress(w, b);
              O[0] === " " ? u.push(O) : l.push(O);
            }
          }

          return l.sort().join("") + u.sort().join("");
        }, r.prototype.getAddress = function (t, n) {
          var s = h.combinedAttributeWithValue(t, n),
              u = h.getGulByCombinedKeyValue(s);
          if (typeof u == "string") return u;
          var l = this.getMap();

          if (!l.hasOwnProperty(s)) {
            this.packedData = void 0;
            var v = this.array.length;
            l[s] = String(v), this.array.push(s);
          }

          return " " + l[s];
        }, r.prototype.unpackAttributes = function (t, n) {
          n === void 0 && (n = !0);

          for (var s = {}, u = 0, l = 0; u < t.length && t[u] !== " ";) {
            var v = h.getKeyValueByGul(t[u]),
                w = v[0],
                b = v[1];
            n && w[0] === "." ? c.keyPathToObject(s, w, b) : s[w] = b, u += 1;
          }

          if (u < t.length - 1) {
            var k = t.substring(u + 1).split(" ");
            u++;

            for (var A = 0, D = k; A < D.length; A++) {
              var P = D[A],
                  O = f ? this.cache[P] || (this.cache[P] = h.parseCombinedAttributeValue(this.array[P])) : h.parseCombinedAttributeValue(this.array[P]),
                  w = O[0],
                  b = O[1];
              if (n && w[0] === ".") c.keyPathToObject(s, w, b);else if ((!i || b !== null) && (s[w] = b, a && e[w])) {
                var E = s.rawString || t,
                    j = P.length + 1,
                    _ = u - l;

                s.rawString = E.substring(0, _) + E.substring(_ + j), l += j;
              }
              u += P.length + 1;
            }
          }

          return a && !s.rawString && (s.rawString = t), s;
        }, r.prototype.stringify = function () {
          return typeof this.packedData != "string" && (this.packedData = this.array.length ? JSON.stringify(this.array) : ""), this.packedData;
        }, r.prototype.clone = function () {
          return new r({
            array: this.array.slice(),
            packedData: this.packedData
          });
        }, r;
      }();

      p.default = o;
    },
    472: (m, p, d) => {
      "use strict";

      var g = d(125);
      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      function y(c, f, i) {
        i === void 0 && (i = []);

        for (var a = Object.keys(c), e = 0, o = a; e < o.length; e++) {
          var r = o[e],
              t = c[r],
              n = f + "." + r;
          g(t) === "object" && t !== null ? y(t, n, i) : i.push([n, t]);
        }

        return i;
      }

      p.objectToKeyPaths = y;

      function h(c, f, i) {
        for (var a = f.split("."), e = a.length, o = 1; o < e - 1; o++) {
          var r = a[o],
              t = c[r];
          c = g(t) === "object" && t !== null ? t : c[r] = {};
        }

        var n = a[e - 1];
        i === null && c[n] !== void 0 && c[n] !== null || (c[n] = i);
      }

      p.keyPathToObject = h;
    },
    297: (m, p, d) => {
      "use strict";

      var g = d(125);
      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var y = d(511);

      function h(c) {
        return g(c) === "object" && c !== null && c[y.TYPE_PROPERTY] === "Delta";
      }

      p.isDelta = h;
    },
    228: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var d = function () {
        function g(y, h) {
          this.deltaConstructor = y, this.operatorConstructor = h;
        }

        return g.prototype.compose = function (y, h, c) {
          throw new Error("#compose() is not implemented");
        }, g.prototype.transform = function (y, h, c) {
          throw new Error("#transform() is not implemented");
        }, g.prototype.invert = function (y, h) {
          throw new Error("#invert() is not implemented");
        }, g.prototype.parse = function (y) {
          throw new Error("#parse() is not implemented");
        }, g.prototype.pack = function (y, h) {
          throw new Error("#pack() is not implemented");
        }, g.prototype.toJSON = function (y) {
          throw new Error("#toJSON() is not implemented");
        }, g.prototype.explain = function (y, h) {
          return "" + y + h;
        }, g;
      }();

      p.default = d;
    },
    113: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var f = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (i, a) {
          i.__proto__ = a;
        } || function (i, a) {
          for (var e in a) a.hasOwnProperty(e) && (i[e] = a[e]);
        };

        return function (i, a) {
          f(i, a);

          function e() {
            this.constructor = i;
          }

          i.prototype = a === null ? Object.create(a) : (e.prototype = a.prototype, new e());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(228),
          h = d(297),
          c = function (f) {
        g(i, f);

        function i() {
          return f !== null && f.apply(this, arguments) || this;
        }

        return i.prototype.compose = function (a, e, o) {
          return this.parse(a).compose(this.parse(e), o).stringify();
        }, i.prototype.transform = function (a, e, o) {
          return this.parse(a).transform(this.parse(e), o).stringify();
        }, i.prototype.invert = function (a, e) {
          return this.parse(a).invert(this.parse(e)).stringify();
        }, i.prototype.parse = function (a) {
          return new this.deltaConstructor(a);
        }, i.prototype.pack = function (a) {
          if (Array.isArray(a)) return new this.deltaConstructor(a).stringify();
          if (h.isDelta(a)) return a.stringify();
          throw new Error("Unsupported delta value for packing " + a);
        }, i.prototype.toJSON = function (a) {
          return this.parse(a).toJSON();
        }, i.prototype.explain = function (a, e) {
          return this.parse(e).explain(a);
        }, i;
      }(y.default);

      p.default = c;
    },
    134: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var f = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (i, a) {
          i.__proto__ = a;
        } || function (i, a) {
          for (var e in a) a.hasOwnProperty(e) && (i[e] = a[e]);
        };

        return function (i, a) {
          f(i, a);

          function e() {
            this.constructor = i;
          }

          i.prototype = a === null ? Object.create(a) : (e.prototype = a.prototype, new e());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(116),
          h = d(228),
          c = function (f) {
        g(i, f);

        function i() {
          return f !== null && f.apply(this, arguments) || this;
        }

        return i.prototype.compose = function (a, e, o) {
          for (var r = this.parse(a).slice(0), t = this.parse(e).slice(0), n = Math.max(r.length, t.length), s = new Array(n), u = 0; u < n; u++) {
            var l = r[u],
                v = t[u];
            s[u] = l ? v ? l.compose(v, o) : l : v;
          }

          return this.pack(s);
        }, i.prototype.transform = function (a, e, o) {
          for (var r = this.parse(a).slice(0), t = this.parse(e).slice(0), n = Math.max(r.length, t.length), s = new Array(n), u = 0; u < n; u++) {
            var l = r[u],
                v = t[u];
            s[u] = l ? v ? l.transform(v, o) : void 0 : v;
          }

          return this.pack(s);
        }, i.prototype.invert = function (a, e) {
          for (var o = this.parse(a).slice(0), r = this.parse(e).slice(0), t = Math.max(o.length, r.length), n = new Array(t), s = 0; s < t; s++) {
            var u = o[s],
                l = r[s];
            n[s] = u ? l ? u.invert(l) : void 0 : l;
          }

          return this.pack(n);
        }, i.prototype.parse = function (a) {
          for (var e = 0, o = []; e < a.length;) {
            var r = a.indexOf("$", e);
            if (r === -1) break;
            var t = parseInt(a.substring(e, r), 36),
                n = r + 1 + t,
                s = t === 0 ? void 0 : new this.deltaConstructor(a.substring(r + 1, n));
            e = n, o.push(s);
          }

          return o;
        }, i.prototype.pack = function (a) {
          for (var e = "", o = 0, r = a; o < r.length; o++) {
            var t = r[o],
                n = y.possibleDeltaInputToString(this.deltaConstructor, t);
            e += n.length.toString(36) + "$" + n;
          }

          return e;
        }, i.prototype.toJSON = function (a) {
          var e = this.parse(a);
          return e.map(function (o) {
            return o ? o.toJSON() : void 0;
          });
        }, i.prototype.explain = function (a, e) {
          for (var o = a + `List:
`, r = this.parse(e), t = 0; t < r.length; t++) {
            var n = r[t];
            o += a + ("Item #" + t + `:
`), o += n ? n.explain(a) : "[EMPTY]";
          }

          return o;
        }, i;
      }(h.default);

      p.default = c;
    },
    696: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var c = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (f, i) {
          f.__proto__ = i;
        } || function (f, i) {
          for (var a in i) i.hasOwnProperty(a) && (f[a] = i[a]);
        };

        return function (f, i) {
          c(f, i);

          function a() {
            this.constructor = f;
          }

          f.prototype = i === null ? Object.create(i) : (a.prototype = i.prototype, new a());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(228),
          h = function (c) {
        g(f, c);

        function f() {
          return c !== null && c.apply(this, arguments) || this;
        }

        return f.prototype.compose = function (i, a, e) {
          for (var o = this.parse(i), r = this.parse(a), t = Object.keys(r), n = 0, s = t; n < s.length; n++) {
            var u = s[n],
                l = o[u],
                v = r[u];
            o[u] = l === void 0 ? v : l.compose(v, e);
          }

          return this.pack(o);
        }, f.prototype.transform = function (i, a, e) {
          for (var o = this.parse(i), r = this.parse(a), t = Object.keys(r), n = 0, s = t; n < s.length; n++) {
            var u = s[n],
                l = o[u];

            if (l !== void 0) {
              var v = r[u];
              r[u] = l.transform(v, e);
            }
          }

          return this.pack(r);
        }, f.prototype.invert = function (i, a) {
          for (var e = this.parse(i), o = this.parse(a), r = Object.keys(o), t = 0, n = r; t < n.length; t++) {
            var s = n[t],
                u = e[s] || new this.deltaConstructor([]);
            o[s] = u.invert(o[s]);
          }

          return this.pack(o);
        }, f.prototype.parse = function (i) {
          for (var a = {}, e = 0; e < i.length;) {
            var o = i.substring(e, e + 8),
                r = i.indexOf("#", e + 8),
                t = parseInt(i.substring(e + 8, r), 36),
                n = new this.deltaConstructor(i.substring(r + 1, r + 1 + t));
            a[o] = n, e = r + 1 + t;
          }

          return a;
        }, f.prototype.pack = function (i) {
          for (var a = "", e = Object.keys(i).sort(), o = 0, r = e; o < r.length; o++) {
            var t = r[o];
            if (t.length !== 8) throw new Error("element id length must equal to 8, got " + t);
            var n = i[t],
                s = Array.isArray(n) ? new this.deltaConstructor(n) : n,
                u = s.getIter();

            if (!!u.hasNext()) {
              var l = s.stringify();
              a += "" + t + l.length.toString(36) + "#" + l;
            }
          }

          return a;
        }, f.prototype.toJSON = function (i) {
          for (var a = {}, e = this.parse(i), o = Object.keys(e), r = 0, t = o; r < t.length; r++) {
            var n = t[r],
                s = e[n];
            a[n] = s.toJSON();
          }

          return a;
        }, f.prototype.explain = function (i, a) {
          for (var e = i + `Elements:
`, o = this.parse(a), r = Object.keys(o), t = 0, n = r; t < n.length; t++) {
            var s = n[t],
                u = o[s];
            e += u.explain(i);
          }

          return e;
        }, f;
      }(y.default);

      p.default = h;
    },
    996: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var f = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (i, a) {
          i.__proto__ = a;
        } || function (i, a) {
          for (var e in a) a.hasOwnProperty(e) && (i[e] = a[e]);
        };

        return function (i, a) {
          f(i, a);

          function e() {
            this.constructor = i;
          }

          i.prototype = a === null ? Object.create(a) : (e.prototype = a.prototype, new e());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(228),
          h = d(292),
          c = function (f) {
        g(i, f);

        function i() {
          return f !== null && f.apply(this, arguments) || this;
        }

        return i.prototype.compose = function (a, e, o) {
          return this.pack(h.merge(this.parse(a), this.parse(e), !o.isDocument), !!o.isDocument);
        }, i.prototype.transform = function (a, e, o) {
          return this.pack(h.transform(this.parse(a), this.parse(e), o), !1);
        }, i.prototype.invert = function (a, e) {
          return this.pack(h.invert(this.parse(a), this.parse(e)), !1);
        }, i.prototype.parse = function (a) {
          return JSON.parse(a);
        }, i.prototype.pack = function (a, e) {
          return e ? JSON.stringify(a, function (o, r) {
            return r === null ? void 0 : r;
          }) : JSON.stringify(a);
        }, i.prototype.toJSON = function (a) {
          return this.parse(a);
        }, i.prototype.explain = function (a, e) {
          return JSON.stringify(this.parse(e), null, a);
        }, i;
      }(y.default);

      p.default = c;
    },
    252: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var c = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (f, i) {
          f.__proto__ = i;
        } || function (f, i) {
          for (var a in i) i.hasOwnProperty(a) && (f[a] = i[a]);
        };

        return function (f, i) {
          c(f, i);

          function a() {
            this.constructor = f;
          }

          f.prototype = i === null ? Object.create(i) : (a.prototype = i.prototype, new a());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(134),
          h = function (c) {
        g(f, c);

        function f() {
          return c !== null && c.apply(this, arguments) || this;
        }

        return f.prototype.pack = function (i) {
          return c.prototype.pack.call(this, Array.isArray(i) ? i : [i.slides, i.layouts, i.notes]);
        }, f.prototype.toJSON = function (i) {
          var a = c.prototype.toJSON.call(this, i);
          return {
            slides: a[0],
            layouts: a[1],
            notes: a[2]
          };
        }, f.prototype.explain = function (i, a) {
          var e = i + `Slides:
`,
              o = this.parse(a),
              r = o[0],
              t = o[1],
              n = o[2];
          return e += r ? r.explain(i) : "[EMPTY]", e += i + `Layouts:
`, e += t ? t.explain(i) : "[EMPTY]", e += i + `Notes:
`, e += n ? n.explain(i) : "[EMPTY]", e;
        }, f;
      }(y.default);

      p.default = h;
    },
    800: function (m, p, d) {
      "use strict";

      var g = this && this.__extends || function () {
        var o = Object.setPrototypeOf || {
          __proto__: []
        } instanceof Array && function (r, t) {
          r.__proto__ = t;
        } || function (r, t) {
          for (var n in t) t.hasOwnProperty(n) && (r[n] = t[n]);
        };

        return function (r, t) {
          o(r, t);

          function n() {
            this.constructor = r;
          }

          r.prototype = t === null ? Object.create(t) : (n.prototype = t.prototype, new n());
        };
      }();

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(116),
          h = d(776),
          c = d(228),
          f = d(106),
          i = d(963),
          a = d(877),
          e = function (o) {
        g(r, o);

        function r() {
          return o !== null && o.apply(this, arguments) || this;
        }

        return r.prototype.pack = function (t) {
          var n = t.rows,
              s = t.cols,
              u = y.possibleDeltaInputToString(this.deltaConstructor, n),
              l = y.possibleDeltaInputToString(this.deltaConstructor, s);
          return u.length.toString(36) + "$" + u + l;
        }, r.prototype.toJSON = function (t) {
          var n = this.parse(t),
              s = n[0],
              u = n[1];
          return {
            rows: s === null ? null : s.toJSON(),
            cols: u === null ? null : u.toJSON()
          };
        }, r.prototype.explain = function (t, n) {
          var s = t + `Rows:
`,
              u = this.parse(n),
              l = u[0],
              v = u[1];
          return s += l === null ? "[EMPTY]" : l.explain(t), s += t + `Cols:
`, s += v === null ? "[EMPTY]" : v.explain(t), s;
        }, r.prototype.parse = function (t) {
          var n = t.indexOf("$"),
              s = parseInt(t.substring(0, n), 36),
              u = n + 1 + s,
              l = s === 0 ? null : new this.deltaConstructor(t.substring(n + 1, u)),
              v = t.length === u ? null : new this.deltaConstructor(t.substring(u));
          return [l, v];
        }, r.prototype.compose = function (t, n, s) {
          var u = this.parse(t),
              l = u[0],
              v = u[1],
              w = this.parse(n),
              b = w[0],
              k = w[1],
              A = b === null ? l : l === null ? b : l.compose(b, s),
              D = h.reindexInnerDeltas(v, b),
              P = k === null ? D : D === null ? k : D.compose(k, s);
          return this.pack({
            rows: A,
            cols: P
          });
        }, r.prototype.transform = function (t, n, s) {
          var u = this.parse(t),
              l = u[0],
              v = u[1],
              w = this.parse(n),
              b = w[0],
              k = w[1],
              A = l === null || b === null ? b : l.transform(b, s),
              D = l === null || b === null ? l : b.transform(l, !s),
              P = h.reindexInnerDeltas(v, A),
              O = h.reindexInnerDeltas(k, D),
              E = P === null || O === null ? O : P.transform(O, s);
          return this.pack({
            rows: A,
            cols: E
          });
        }, r.prototype.invert = function (t, n) {
          var s = this,
              u = this.parse(t),
              l = u[0],
              v = u[1],
              w = this.parse(n),
              b = w[0],
              k = w[1],
              A = b && (l === null ? new this.deltaConstructor([]) : l).invert(b),
              D = h.invertInnerDeltas(v, b);

          if (k !== null) {
            var P = v === null ? new a.default([]) : v;

            if (D) {
              var O = h.DeltaToIndexModifiers(k);
              D = h.reindexDelta(D, O);
            }

            var E = h.reindexInnerDeltas(k, A);
            if (E === null) throw new Error("ASSERT: targetCols === null");
            var j = P.invert(E, function (_, x) {
              if (x.dataType !== "delta") return _.invert(x);

              for (var N = new f.Pool(), U = new i.default([]), I = _.dataType === "number" ? null : new s.deltaConstructor(_.getCustomData()).getIter({
                targetPool: N
              }), C = new s.deltaConstructor(x.getCustomData()).getIter({
                targetPool: N
              }); C.hasNext();) {
                var H = void 0,
                    R = void 0;

                if (I !== null && I.hasNext()) {
                  var J = Math.min(I.peekLength(), C.peekLength());
                  H = I.next(J, "insert"), R = C.next(J);
                } else {
                  var Y = C.peekLength();
                  H = new s.operatorConstructor({
                    action: "insert",
                    data: Y
                  }), R = C.next();
                }

                U.push(H.invert(R));
              }

              return _.invert(x, {
                delta: new s.deltaConstructor(U.stringify(!0), N).stringify()
              });
            });
            D = D ? D.compose(j) : j;
          }

          return this.pack({
            rows: A,
            cols: D
          });
        }, r;
      }(c.default);

      p.default = e;
    },
    776: (m, p, d) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var g = d(963),
          y = d(397),
          h = d(793);

      function c(o) {
        for (var r = [], t = -1, n = o.getIter(); n.hasNext();) {
          var s = n.next(),
              u = s.action,
              l = s.length;
          t !== -1 && r[t][0] === u ? r[t][1] += l : r[++t] = [u, l];
        }

        return r;
      }

      p.DeltaToIndexModifiers = c;

      function f(o, r) {
        for (var t = o.getPoolCopy(), n = new g.default([]), s = o.getIter({
          sourcePool: t,
          targetPool: t
        }), u = new Array(r.length), l = 0; l < r.length && s.hasNext(); l++) {
          var v = u[l] || r[l],
              w = v[0],
              b = v[1];

          switch (w) {
            case "insert":
              n.push(new h.default({
                action: "retain",
                data: b,
                packedAttributes: "",
                pool: t
              }));
              break;

            case "retain":
              {
                var k = s.peekLength();
                k >= b ? n.push(s.next(b)) : (n.push(s.next()), u[l] = ["retain", b - k], l -= 1);
                break;
              }

            case "remove":
              {
                var k = s.peekLength();
                k >= b ? s.skip(b) : (s.skip(), u[l] = ["remove", b - k], l -= 1);
                break;
              }
          }
        }

        for (; s.hasNext();) n.push(s.next());

        return new y.Delta(n.stringify(!0), t);
      }

      p.reindexDelta = f;

      function i(o, r) {
        if (o === null || r === null) return o;
        var t = c(r);
        return t.length === 0 ? o : o.map(function (n) {
          var s = {
            action: n.action,
            data: n.data
          };

          if (s.action !== "remove" && n.hasAttributes() && (s.attributes = n.getAttributes()), n.dataType === "delta") {
            var u = f(new y.Delta(n.getCustomData()), t);
            s.data = u.isEmpty() ? 1 : {
              delta: u.stringify()
            };
          } else s.data = n.data;

          return s;
        });
      }

      p.reindexInnerDeltas = i;

      function a(o, r) {
        for (var t = o.getPoolCopy(), n = new g.default([]), s = o.getIter({
          sourcePool: t,
          targetPool: t
        }), u = new Array(r.length), l = 0; l < r.length && s.hasNext(); l++) {
          var v = u[l] || r[l],
              w = v[0],
              b = v[1];

          switch (w) {
            case "retain":
              {
                var k = s.peekLength(),
                    A = Math.min(b, k);
                n.push(new h.default({
                  action: "retain",
                  data: A,
                  packedAttributes: "",
                  pool: t
                })), s.skip(A), k < b && (u[l] = ["retain", b - k], l -= 1);
                break;
              }

            case "remove":
              {
                var k = s.peekLength();
                k >= b ? n.push(s.next(b)) : (n.push(s.next()), u[l] = ["remove", b - k], l -= 1);
                break;
              }
          }
        }

        for (var D = 0; s.hasNext();) D += s.peekLength(), s.skip();

        return D > 0 && n.push(new h.default({
          action: "retain",
          data: D,
          packedAttributes: "",
          pool: t
        })), new y.Delta(n.stringify(!0), t);
      }

      function e(o, r) {
        if (o === null || r === null) return null;

        for (var t = c(r), n = !1, s = 0, u = t; s < u.length; s++) {
          var l = u[s];

          if (l[0] === "remove") {
            n = !0;
            break;
          }
        }

        return n ? o.map(function (v) {
          var w = {
            action: "retain",
            data: v.data
          };

          if (v.hasAttributes() && (w.attributes = v.getAttributes()), v.dataType === "delta") {
            var b = a(new y.Delta(v.getCustomData()), t);
            w.data = b.isEmpty() ? 1 : {
              delta: b.stringify()
            };
          } else w.data = v.data;

          return w;
        }) : null;
      }

      p.invertInnerDeltas = e;
    },
    116: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      function d(g, y) {
        if (y == null) return "";

        if (Array.isArray(y)) {
          if (y.length === 0) return "";
          var h = new g(y);
          return h.isEmpty() ? "" : h.stringify();
        }

        return y.isEmpty() ? "" : y.stringify();
      }

      p.possibleDeltaInputToString = d;
    },
    202: (m, p, d) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var g = d(800),
          y = d(113),
          h = d(696),
          c = d(252),
          f = d(996),
          i = d(134);

      function a(o, r) {
        return function (n) {
          return new n(o, r);
        };
      }

      function e(o, r, t) {
        var n = a(r, t);
        o.registerDataHandler("delta", n(y.default)), o.registerDataHandler("table", n(g.default)), o.registerDataHandler("presentation", n(c.default)), o.registerDataHandler("slide", n(h.default)), o.registerDataHandler("mindmap", n(h.default)), o.registerDataHandler("object", n(f.default)), o.registerDataHandler("form", n(i.default));
      }

      p.registerDataHandlers = e;
    },
    3: (m, p, d) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var g = d(870),
          y = d(963),
          h = function () {
        function c(f, i, a, e) {
          this.ops = f, this.pool = i, this.targetPool = a, this.operatorConstructor = e, this.cursor = 0, this.offset = 0;
        }

        return c.prototype.isPoolCompatible = function () {
          return this.isCompatible === void 0 && (this.isCompatible = this.pool.isSubsetOf(this.targetPool)), this.isCompatible;
        }, c.prototype.hasNext = function () {
          return this.cursor < this.ops.length;
        }, c.prototype.remainingStrs = function (f) {
          for (; this.hasNext();) {
            var i = this.next(),
                a = f.push(i);

            if (a !== y.PushResult.REORDERED) {
              var e = this.cursor;
              return this.cursor = this.ops.length, this.nextData = void 0, f.stringify() + this.ops.substring(e);
            }
          }

          return f.stringify(!0);
        }, c.prototype.skip = function (f) {
          f === void 0 && (f = 1 / 0);
          var i = this.peekNext();
          if (i === null) throw new Error("No more operators");
          var a = this.offset,
              e;
          return f + a >= i.dataLength ? (e = i.dataLength - a, this.nextData = void 0, this.cursor = i.opIndex + i.opLength, this.offset = 0) : (e = f, this.offset += e), [e, a, i];
        }, c.prototype.next = function (f, i) {
          f === void 0 && (f = 1 / 0);
          var a = this.skip(f),
              e = a[0],
              o = a[1],
              r = a[2],
              t;

          switch (r.dataType) {
            case "number":
              t = e;
              break;

            case "string":
              t = this.ops.substring(r.dataIndex + 1 + o, r.dataIndex + 1 + o + e);
              break;

            default:
              var n = this.ops[r.dataIndex + 1 + o],
                  s = this.ops.substring(r.dataIndex + 1 + 1, r.dataIndex + 1 + r.actualLength);
              t = (b = {}, b[g.initialToObjectKey[n]] = s, b);
              break;
          }

          var u = this.ops.substring(r.dataIndex + 1 + r.actualLength, r.opIndex + r.opLength),
              l = this.targetPool,
              v = u !== "" && !this.isPoolCompatible(),
              w;
          return o === 0 && e === r.dataLength && !v && (w = this.ops.substring(r.opIndex, r.dataIndex + 1 + r.actualLength)), v && (u = l.packAttributes(this.pool.unpackAttributes(u))), new this.operatorConstructor({
            action: i || r.action,
            data: t,
            packedAttributes: u,
            pool: l
          }, w);
          var b;
        }, c.prototype.peekAction = function () {
          var f = this.peekNext();
          return f === null ? "retain" : f.action;
        }, c.prototype.peekLength = function () {
          var f = this.peekNext();
          return f === null ? 1 / 0 : f.dataLength - this.offset;
        }, c.prototype.peekNext = function () {
          if (!this.hasNext()) return null;

          if (this.nextData === void 0) {
            var f = void 0,
                i = void 0;
            i = this.cursor + 1;

            for (var a = this.ops.length; i < a;) {
              switch (this.ops[i]) {
                case "!":
                  f = "insert";
                  break;

                case "@":
                  f = "retain";
                  break;

                case "#":
                  f = "remove";
                  break;
              }

              if (f !== void 0) break;
              i += 1;
            }

            var e = i,
                o = parseInt(this.ops.substring(this.cursor, e), 36);
            i = e + 1;

            for (var r = void 0; i < a;) {
              switch (this.ops[i]) {
                case "!":
                  r = "string";
                  break;

                case "#":
                  r = "number";
                  break;

                case "@":
                  r = "object";
                  break;
              }

              if (r !== void 0) break;
              i += 1;
            }

            var t = i,
                n = parseInt(this.ops.substring(e + 1, t), 36),
                s = r === "number" ? 0 : n,
                u = r === "object" ? 1 : n;
            this.nextData = {
              action: f,
              dataIndex: t,
              dataLength: u,
              opIndex: e,
              opLength: o,
              dataType: r,
              actualLength: s
            };
          }

          return this.nextData;
        }, c;
      }();

      p.default = h;
    },
    877: function (m, p, d) {
      "use strict";

      var g = d(125),
          y = this && this.__assign || Object.assign || function (r) {
        for (var t, n = 1, s = arguments.length; n < s; n++) {
          t = arguments[n];

          for (var u in t) Object.prototype.hasOwnProperty.call(t, u) && (r[u] = t[u]);
        }

        return r;
      };

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var h = d(963),
          c = d(433),
          f = d(511),
          i = d(793),
          a = d(3),
          e = d(297),
          o = function () {
        function r(t, n) {
          if (this[f.TYPE_PROPERTY] = "Delta", Array.isArray(t)) {
            var s = new h.default();
            (!c.default.isPool(n) || t.length === 0) && (n = new c.default());

            for (var u = 0, l = t; u < l.length; u++) {
              var v = l[u],
                  w = n.packAttributes(v.attributes);
              s.push(new i.default({
                action: v.action,
                data: v.data,
                pool: n,
                packedAttributes: w
              }));
            }

            this.ops = s.stringify(!0), this.pool = n;
          } else if (n === void 0) {
            var b = t.indexOf(":");
            if (b <= 0) throw new Error("Invalid delta: " + t);
            var k = parseInt(t.substring(0, b), 36);
            if (isNaN(k)) throw new Error("Invalid delta: " + t);
            var A = b + 1 + k;
            this.ops = t.substring(b + 1, A), this.pool = k === 0 || t.length === A ? new c.default() : new c.default(t.substring(b + k + 1));
          } else this.ops = t, this.pool = t === "" ? new c.default() : n;
        }

        return r.prototype.isEmpty = function () {
          return this.ops.length === 0;
        }, r.prototype.getIter = function (t) {
          var n = t === void 0 ? {} : t,
              s = n.targetPool,
              u = n.sourcePool;
          return new a.default(this.ops, u || this.pool, s || this.pool, i.default);
        }, Object.defineProperty(r.prototype, "length", {
          get: function () {
            for (var n = this.getIter(), s = 0; n.hasNext();) s += n.peekLength(), n.skip();

            return s;
          },
          enumerable: !0,
          configurable: !0
        }), r.prototype.getPoolCopy = function () {
          return this.pool.clone();
        }, r.prototype.map = function (t) {
          for (var n = [], s = this.getIter(); s.hasNext();) n.push(t(s.next()));

          return new r(n);
        }, r.prototype.compose = function (t, n) {
          var s = {};
          n && (typeof n == "boolean" ? s.isDocument = n : g(n) === "object" && (s = n));
          var u = this.getPoolCopy(),
              l = new h.default([]),
              v = this.getIter({
            sourcePool: u,
            targetPool: u
          });
          Array.isArray(t) && (t = new r(t, u));

          for (var w = t.getIter({
            targetPool: u
          });;) {
            var b = v.hasNext(),
                k = w.hasNext();
            if (!b && !k) break;
            if (!k) return new r(v.remainingStrs(l), u);
            if (!b && w.isPoolCompatible()) return new r(w.remainingStrs(l), u);
            if (w.peekAction() === "insert") l.push(w.next());else if (v.peekAction() === "remove") l.push(v.next());else {
              var A = Math.min(v.peekLength(), w.peekLength()),
                  D = b ? v.next(A) : null,
                  P = w.next(A);
              if (P.action === "retain") {
                if (D) {
                  var O = s.isDocument === void 0 ? D.action === "insert" : s.isDocument;
                  l.push(D.compose(P, y({}, s, {
                    isDocument: O
                  })));
                } else l.push(P);
              } else (!D || D.action === "retain") && l.push(P);
            }
          }

          return new r(l.stringify(!0), u);
        }, r.prototype.transform = function (t, n) {
          n === void 0 && (n = !1), Array.isArray(t) && (t = new r(t));

          for (var s = t.getPoolCopy(), u = new h.default(), l = this.getIter(), v = t.getIter({
            sourcePool: s,
            targetPool: s
          }); v.hasNext();) {
            if (!l.hasNext()) return new r(v.remainingStrs(u), s);
            if (l.peekAction() === "insert" && (n || v.peekAction() !== "insert")) u.push(new i.default({
              action: "retain",
              data: l.peekLength()
            })), l.skip();else if (v.peekAction() === "insert") u.push(v.next());else {
              var w = Math.min(l.peekLength(), v.peekLength()),
                  b = l.next(w),
                  k = v.next(w);

              if (b.action === "remove") {
                if (b.hasAttributes() || k.action === "remove" && k.hasAttributes()) throw new Error("Cannot transform removing operations with attributes");
                continue;
              }

              if (k.action === "remove") {
                if (k.hasAttributes()) throw new Error("Cannot transform removing operations with attributes");
                u.push(k);
              } else u.push(b.transform(k, n));
            }
          }

          return new r(u.stringify(!0), s);
        }, r.prototype.invert = function (t, n) {
          for (var s = new c.default(), u = new h.default(), l = this.getIter({
            targetPool: s
          }), v = (Array.isArray(t) ? new r(t, new c.default()) : t).getIter({
            targetPool: s
          }); v.hasNext();) if (v.peekAction() === "insert") u.push(new i.default({
            action: "remove",
            data: v.peekLength()
          })), v.skip();else {
            if (!l.hasNext()) throw new Error("Delta should be smaller than document");
            if (l.peekAction() !== "insert") throw new Error("Only document can invert deltas");
            var w = Math.min(l.peekLength(), v.peekLength()),
                b = v.next(w),
                k = l.next(w);
            b.action === "remove" ? u.push(k) : u.push(n ? n(k, b) : k.invert(b));
          }

          return new r(u.stringify(!0), s);
        }, r.prototype.stringify = function () {
          return this.isEmpty() ? "0:" : this.ops.length.toString(36) + ":" + this.ops + this.pool.stringify();
        }, r.prototype.toJSON = function () {
          for (var t = [], n = this.getIter(); n.hasNext();) t.push(n.next().toJSON());

          return t;
        }, r.prototype.explain = function (t) {
          t === void 0 && (t = "");

          for (var n = t + "Delta #" + this.ops.length + `:
`, s = this.getIter(); s.hasNext();) n += s.next().explain(t + "  ");

          return "" + n + t + "Pool: " + this.pool.stringify() + `
`;
        }, r.prototype.validateTable = function () {
          for (var t = {}, n = this.getIter(); n.hasNext();) {
            var s = n.next();

            if (s.dataType === "table") {
              var u = s.getAttributes().name;
              if (typeof u != "string") continue;
              var l = "$$" + u + "$$";
              if (t.hasOwnProperty(l)) throw new Error("Duplicated table name found: " + u);
              t[l] = !0;
            }
          }

          return this;
        }, r.isDelta = e.isDelta, r;
      }();

      p.default = o;
    },
    793: (m, p, d) => {
      "use strict";

      var g = d(125);
      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      var y = d(870),
          h = d(292),
          c = "0:",
          f = function () {
        function a(e, o) {
          this.action = e.action;
          var r = e.data;
          if (r == null) throw new Error('"data" is required for an Operator');

          if (g(r) === "object") {
            var t = Object.keys(r)[0],
                n = r[t];
            if (typeof n == "string") this.data = r, this.dataType = t;else {
              var s = a.dataHandlers[t];
              if (!s) throw new Error('Unsupported nested data type "' + t + '"');
              var u = s.pack(n, this.action === "insert");
              t === "delta" && u === c ? (this.data = 1, this.dataType = "number") : (this.data = (l = {}, l[t] = u, l), this.dataType = t);
            }
          } else this.data = r, this.dataType = g(r);

          this.packedAttributes = e.packedAttributes, this.pool = e.pool, this.packedData = o;
          var l;
        }

        return a.isOperator = function (e) {
          return typeof e.getAttributes == "function";
        }, a.registerDataHandler = function (e, o) {
          a.dataHandlers[e] = o;
        }, a.getUnresolvedAttributes = function (e) {
          return typeof e.getUnresolvedAttributes == "function" ? e.getUnresolvedAttributes() : e.getAttributes();
        }, Object.defineProperty(a.prototype, "length", {
          get: function () {
            return typeof this.data == "string" ? this.data.length : typeof this.data == "number" ? this.data : 1;
          },
          enumerable: !0,
          configurable: !0
        }), a.prototype.isEmpty = function () {
          return this.action === "retain" && (this.packedAttributes === "" || this.packedAttributes === void 0) && typeof this.data == "number";
        }, a.prototype.getParsedData = function () {
          var e = a.dataHandlers[this.dataType];
          if (e === void 0) throw new Error("Unknown handler for " + this.dataType);
          return e.parse(this.getCustomData());
        }, a.prototype.hasAttributes = function () {
          return this.packedAttributes !== void 0 && this.packedAttributes !== "";
        }, a.prototype.getAttributes = function () {
          return this.pool !== void 0 && this.hasAttributes() ? this.pool.unpackAttributes(this.packedAttributes) : {};
        }, a.prototype.stringify = function () {
          if (this.packedData === void 0) {
            var e = this.data,
                o = void 0;

            switch (this.action) {
              case "insert":
                o = "!";
                break;

              case "retain":
                o = "@";
                break;

              default:
                o = "#";
            }

            if (typeof e == "number") o += e.toString(36) + "#";else if (typeof e == "string") o += e.length.toString(36) + "!" + e;else {
              var r = this.dataType;
              if (r === void 0) throw new Error("ASSERT: dataType === undefined");
              var t = y.objectKeyToInitial[r];
              if (!t) throw new Error('Unsupport object data type: "' + r + '"');
              var n = "" + t + e[r];
              o += n.length.toString(36) + "@" + n;
            }
            this.packedData = o;
          }

          var s = this.packedAttributes === void 0 ? this.packedData : this.packedData + this.packedAttributes;
          return s.length.toString(36) + s;
        }, a.prototype.merge = function (e) {
          if (this.action !== e.action) return null;
          var o = g(this.data);
          if (o === "object" || o !== g(e.data)) return null;
          var r = this.hasAttributes();
          if (r !== e.hasAttributes()) return null;

          if (r) {
            if (this.pool === e.pool) {
              if (this.packedAttributes !== e.packedAttributes) return null;
            } else if (!i(this.getUnresolvedAttributes(), a.getUnresolvedAttributes(e))) return null;
          }

          return new a({
            action: this.action,
            data: this.data + e.data,
            pool: this.pool,
            packedAttributes: this.packedAttributes
          });
        }, a.prototype.getCustomData = function () {
          switch (g(this.data)) {
            case "string":
            case "number":
              throw new Error("Cannot get custom data from a simple operator");

            default:
              return this.data[this.dataType];
          }
        }, a.prototype.compose = function (e, o) {
          if (this.action === "remove") throw new Error("Removing operator cannot compose other operators");
          if (e.action !== "retain") throw new Error("Only retaining operator can be composed");
          var r,
              t = !1,
              n;

          if (e.hasAttributes()) {
            if (this.pool === void 0) throw new Error("this.pool is undefined when composing");
            var s = a.getUnresolvedAttributes(e);
            if (o.checkParaEnd && s.hasOwnProperty("paraEnd") && this.dataType === "string") throw new Error("ParaEnd overwrite string in document");
            t = s.hasOwnProperty("cellValue");
            var u = h.merge(this.getUnresolvedAttributes(), s, !o.isDocument);
            n = this.pool.packAttributes(u);
          } else n = this.packedAttributes;

          if (t) r = e.dataType === "number" ? e.data : 1;else if (this.dataType === "number") r = e.data;else if (e.dataType === "number") r = this.data;else {
            if (this.dataType === void 0) throw new Error('Missing "dataType" when composing ' + this.data + " with " + e.data);
            if (this.dataType !== e.dataType) throw new Error("Expect " + e.dataType + " to equal " + this.dataType + " when composing " + this.data + " with " + e.data);
            var l = a.dataHandlers[this.dataType];
            if (l === void 0) throw new Error("Unknown handler for " + this.dataType);
            var v = l.compose(this.getCustomData(), e.getCustomData(), o);
            r = this.dataType === "delta" && v === c ? 1 : (w = {}, w[this.dataType] = v, w);
          }
          return new a({
            action: this.action,
            data: r,
            pool: this.pool,
            packedAttributes: n
          });
          var w;
        }, a.prototype.transform = function (e, o) {
          if (o === void 0 && (o = !1), this.action !== "retain") throw new Error("Only retaining operator can transform other operators");
          if (e.action !== "retain") throw new Error("Only retaining operator can be transformed");
          var r;
          if (this.dataType === "number" || e.dataType === "number") r = e.data;else {
            if (this.dataType === void 0) throw new Error('Missing "dataType" when transforming ' + this.data + " with " + e.data);
            if (this.dataType !== e.dataType) throw new Error("Expect " + e.dataType + " to equal " + this.dataType + " when transforming " + this.data + " with " + e.data);
            var t = a.dataHandlers[this.dataType];
            if (t === void 0) throw new Error("Unknown handler for " + this.dataType);
            var n = t.transform(this.getCustomData(), e.getCustomData(), o);
            r = this.dataType === "delta" && n === c ? 1 : (l = {}, l[this.dataType] = n, l);
          }
          var s;

          if (e.hasAttributes()) {
            var u = h.transform(this.getUnresolvedAttributes(), a.getUnresolvedAttributes(e), o);
            if (e.pool === void 0) throw new Error("ASSERT: other.pool === undefined when transforming");
            s = e.pool.packAttributes(u);
          } else s = "";

          return new a({
            action: this.action,
            data: r,
            pool: e.pool,
            packedAttributes: s
          });
          var l;
        }, a.prototype.invert = function (e, o) {
          if (this.action !== "insert") throw new Error('invert should only apply to the "insert" action (' + this.dataType + ")");
          if (e.action !== "retain") throw new Error("Only retaining operator can be inverted");
          var r,
              t = !1,
              n;

          if (e.hasAttributes()) {
            var s = a.getUnresolvedAttributes(e);
            t = s.hasOwnProperty("cellValue");
            var u = h.invert(this.getUnresolvedAttributes(), s);
            if (e.pool === void 0) throw new Error("ASSERT: other.pool === undefined when inverting");
            n = e.pool.packAttributes(u);
          } else n = "";

          if (t) r = this.data;else if (o !== void 0) r = o;else if (typeof e.data == "number") r = e.data;else if (typeof this.data == "number" && e.dataType !== "delta") r = this.data;else {
            if (this.dataType === void 0) throw new Error('Missing "dataType" when inverting ' + this.data + " with " + e.data);

            if (this.dataType === "number" && e.dataType === "delta") {
              var l = a.dataHandlers.delta;
              r = {
                delta: l.invert(c, e.getCustomData())
              };
            } else {
              if (this.dataType !== e.dataType) throw new Error("Expect " + e.dataType + " to equal " + this.dataType + " when inverting " + this.data + " with " + e.data);
              var l = a.dataHandlers[this.dataType];
              if (l === void 0) throw new Error("Unknown handler for " + this.dataType);
              var v = l.invert(this.getCustomData(), e.getCustomData());
              r = this.dataType === "delta" && v === c ? 1 : (w = {}, w[this.dataType] = v, w);
            }
          }
          return new a({
            data: r,
            action: "retain",
            pool: e.pool,
            packedAttributes: n
          });
          var w;
        }, a.prototype.toJSON = function () {
          var e = {
            action: this.action
          };
          if (this.hasAttributes() && (e.attributes = this.getAttributes()), this.dataType === "number" || this.dataType === "string") e.data = this.data;else {
            var o = a.dataHandlers[this.dataType],
                r = this.getCustomData();
            e.data = (t = {}, t[this.dataType] = o ? o.toJSON(r) : r, t);
          }
          return e;
          var t;
        }, a.prototype.explain = function (e) {
          e === void 0 && (e = "");
          var o = this.stringify().length,
              r = e + "[" + this.dataType + "(" + this.action + ")] #" + (o - String(o).length) + `:
`;
          if (this.dataType === "number" || this.dataType === "string") r += e + this.data;else {
            var t = a.dataHandlers[this.dataType],
                n = this.getCustomData();
            r += t ? t.explain(e + "  ", n) : n;
          }
          return r + `
` + e + "PackedAttributes:" + this.packedAttributes + `
`;
        }, a.prototype.getUnresolvedAttributes = function () {
          return this.pool !== void 0 && this.hasAttributes() ? this.pool.unpackAttributes(this.packedAttributes, !1) : {};
        }, a.dataHandlers = {}, a;
      }();

      function i(a, e) {
        if (a === e) return !0;
        var o = Object.keys(a),
            r = o.length;
        if (r !== Object.keys(e).length) return !1;

        for (var t = 0; t < r; t++) if (a[o[t]] !== e[o[t]]) return !1;

        return !0;
      }

      p.default = f;
    },
    870: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      }), p.initialToObjectKey = {
        A: "attachment",
        C: "cell",
        I: "image",
        D: "delta",
        T: "table",
        P: "presentation",
        S: "slide",
        M: "mention",
        N: "inline-break",
        V: "divide",
        U: "upload-placeholder-attachment",
        G: "gallery",
        B: "gallery-block",
        Z: "mindmap",
        O: "object",
        F: "form"
      }, p.objectKeyToInitial = Object.keys(p.initialToObjectKey).reduce(function (d, g) {
        return d[p.initialToObjectKey[g]] = g, d;
      }, {});
    },
    397: (m, p, d) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var g = d(877);
      p.Delta = g.default;
      var y = d(793),
          h = d(202);
      h.registerDataHandlers(y.default, g.default, y.default);
    },
    292: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });

      function d(h, c, f) {
        var i = Object.keys(h);
        if (f && i.length === 0) return c;

        for (var a = Object.keys(c), e = 0, o = a; e < o.length; e++) {
          var r = o[e];
          h[r] = c[r], !f && h[r] === null && delete h[r];
        }

        return h;
      }

      p.merge = d;

      function g(h, c, f) {
        if (f) return c;
        var i = Object.keys(c);
        if (i.length === 0) return c;
        var a = Object.keys(h);
        if (a.length === 0) return c;
        if (a.length > i.length) for (var e = 0, o = i; e < o.length; e++) {
          var r = o[e];
          h.hasOwnProperty(r) && delete c[r];
        } else for (var t = 0, n = a; t < n.length; t++) {
          var s = n[t];
          delete c[s];
        }
        return c;
      }

      p.transform = g;

      function y(h, c) {
        var f = Object.keys(c);
        if (f.length === 0) return c;
        var i = Object.keys(h);
        if (i.length === 0) for (var a = 0, e = f; a < e.length; a++) {
          var o = e[a];
          c[o] = null;
        } else for (var r = 0, t = f; r < t.length; r++) {
          var o = t[r],
              n = h[o];
          n === void 0 ? c[o] = null : c[o] = n;
        }
        return c;
      }

      p.invert = y;
    },
    106: (m, p, d) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      });
      var g = d(397);
      p.Delta = g.Delta;
      var y = d(433);
      p.Pool = y.default;
    },
    511: (m, p) => {
      "use strict";

      Object.defineProperty(p, "__esModule", {
        value: !0
      }), p.TYPE_PROPERTY = "__MODOC_NAME__";
    },
    125: m => {
      function p(d) {
        return m.exports = p = typeof Symbol == "function" && typeof Symbol.iterator == "symbol" ? function (g) {
          return typeof g;
        } : function (g) {
          return g && typeof Symbol == "function" && g.constructor === Symbol && g !== Symbol.prototype ? "symbol" : typeof g;
        }, m.exports.__esModule = !0, m.exports.default = m.exports, p(d);
      }

      m.exports = p, m.exports.__esModule = !0, m.exports.default = m.exports;
    }
  },
      S = {};

  function T(m) {
    var p = S[m];
    if (p !== void 0) return p.exports;
    var d = S[m] = {
      exports: {}
    };
    return L[m].call(d.exports, d, d.exports, T), d.exports;
  }

  T.d = (m, p) => {
    for (var d in p) T.o(p, d) && !T.o(m, d) && Object.defineProperty(m, d, {
      enumerable: !0,
      get: p[d]
    });
  }, T.o = (m, p) => Object.prototype.hasOwnProperty.call(m, p), T.r = m => {
    typeof Symbol < "u" && Symbol.toStringTag && Object.defineProperty(m, Symbol.toStringTag, {
      value: "Module"
    }), Object.defineProperty(m, "__esModule", {
      value: !0
    });
  };
  var M = {};
  return (() => {
    "use strict";

    T.r(M), T.d(M, {
      default: () => p
    });
    var m = T(106);
    const p = m;
  })(), M;
})());