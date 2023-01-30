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

// === tests/cases/compiler/enumLiteralsSubtypeReduction.ts ===
declare function AssertType(value:any, type:string):void;
enum E {
    E0,
    E1,
    E2,
    E3,
    E4,
    E5,
    E6,
    E7,
    E8,
    E9,
    E10,
    E11,
    E12,
    E13,
    E14,
    E15,
    E16,
    E17,
    E18,
    E19,
    E20,
    E21,
    E22,
    E23,
    E24,
    E25,
    E26,
    E27,
    E28,
    E29,
    E30,
    E31,
    E32,
    E33,
    E34,
    E35,
    E36,
    E37,
    E38,
    E39,
    E40,
    E41,
    E42,
    E43,
    E44,
    E45,
    E46,
    E47,
    E48,
    E49,
    E50,
    E51,
    E52,
    E53,
    E54,
    E55,
    E56,
    E57,
    E58,
    E59,
    E60,
    E61,
    E62,
    E63,
    E64,
    E65,
    E66,
    E67,
    E68,
    E69,
    E70,
    E71,
    E72,
    E73,
    E74,
    E75,
    E76,
    E77,
    E78,
    E79,
    E80,
    E81,
    E82,
    E83,
    E84,
    E85,
    E86,
    E87,
    E88,
    E89,
    E90,
    E91,
    E92,
    E93,
    E94,
    E95,
    E96,
    E97,
    E98,
    E99,
    E100,
    E101,
    E102,
    E103,
    E104,
    E105,
    E106,
    E107,
    E108,
    E109,
    E110,
    E111,
    E112,
    E113,
    E114,
    E115,
    E116,
    E117,
    E118,
    E119,
    E120,
    E121,
    E122,
    E123,
    E124,
    E125,
    E126,
    E127,
    E128,
    E129,
    E130,
    E131,
    E132,
    E133,
    E134,
    E135,
    E136,
    E137,
    E138,
    E139,
    E140,
    E141,
    E142,
    E143,
    E144,
    E145,
    E146,
    E147,
    E148,
    E149,
    E150,
    E151,
    E152,
    E153,
    E154,
    E155,
    E156,
    E157,
    E158,
    E159,
    E160,
    E161,
    E162,
    E163,
    E164,
    E165,
    E166,
    E167,
    E168,
    E169,
    E170,
    E171,
    E172,
    E173,
    E174,
    E175,
    E176,
    E177,
    E178,
    E179,
    E180,
    E181,
    E182,
    E183,
    E184,
    E185,
    E186,
    E187,
    E188,
    E189,
    E190,
    E191,
    E192,
    E193,
    E194,
    E195,
    E196,
    E197,
    E198,
    E199,
    E200,
    E201,
    E202,
    E203,
    E204,
    E205,
    E206,
    E207,
    E208,
    E209,
    E210,
    E211,
    E212,
    E213,
    E214,
    E215,
    E216,
    E217,
    E218,
    E219,
    E220,
    E221,
    E222,
    E223,
    E224,
    E225,
    E226,
    E227,
    E228,
    E229,
    E230,
    E231,
    E232,
    E233,
    E234,
    E235,
    E236,
    E237,
    E238,
    E239,
    E240,
    E241,
    E242,
    E243,
    E244,
    E245,
    E246,
    E247,
    E248,
    E249,
    E250,
    E251,
    E252,
    E253,
    E254,
    E255,
    E256,
    E257,
    E258,
    E259,
    E260,
    E261,
    E262,
    E263,
    E264,
    E265,
    E266,
    E267,
    E268,
    E269,
    E270,
    E271,
    E272,
    E273,
    E274,
    E275,
    E276,
    E277,
    E278,
    E279,
    E280,
    E281,
    E282,
    E283,
    E284,
    E285,
    E286,
    E287,
    E288,
    E289,
    E290,
    E291,
    E292,
    E293,
    E294,
    E295,
    E296,
    E297,
    E298,
    E299,
    E300,
    E301,
    E302,
    E303,
    E304,
    E305,
    E306,
    E307,
    E308,
    E309,
    E310,
    E311,
    E312,
    E313,
    E314,
    E315,
    E316,
    E317,
    E318,
    E319,
    E320,
    E321,
    E322,
    E323,
    E324,
    E325,
    E326,
    E327,
    E328,
    E329,
    E330,
    E331,
    E332,
    E333,
    E334,
    E335,
    E336,
    E337,
    E338,
    E339,
    E340,
    E341,
    E342,
    E343,
    E344,
    E345,
    E346,
    E347,
    E348,
    E349,
    E350,
    E351,
    E352,
    E353,
    E354,
    E355,
    E356,
    E357,
    E358,
    E359,
    E360,
    E361,
    E362,
    E363,
    E364,
    E365,
    E366,
    E367,
    E368,
    E369,
    E370,
    E371,
    E372,
    E373,
    E374,
    E375,
    E376,
    E377,
    E378,
    E379,
    E380,
    E381,
    E382,
    E383,
    E384,
    E385,
    E386,
    E387,
    E388,
    E389,
    E390,
    E391,
    E392,
    E393,
    E394,
    E395,
    E396,
    E397,
    E398,
    E399,
    E400,
    E401,
    E402,
    E403,
    E404,
    E405,
    E406,
    E407,
    E408,
    E409,
    E410,
    E411,
    E412,
    E413,
    E414,
    E415,
    E416,
    E417,
    E418,
    E419,
    E420,
    E421,
    E422,
    E423,
    E424,
    E425,
    E426,
    E427,
    E428,
    E429,
    E430,
    E431,
    E432,
    E433,
    E434,
    E435,
    E436,
    E437,
    E438,
    E439,
    E440,
    E441,
    E442,
    E443,
    E444,
    E445,
    E446,
    E447,
    E448,
    E449,
    E450,
    E451,
    E452,
    E453,
    E454,
    E455,
    E456,
    E457,
    E458,
    E459,
    E460,
    E461,
    E462,
    E463,
    E464,
    E465,
    E466,
    E467,
    E468,
    E469,
    E470,
    E471,
    E472,
    E473,
    E474,
    E475,
    E476,
    E477,
    E478,
    E479,
    E480,
    E481,
    E482,
    E483,
    E484,
    E485,
    E486,
    E487,
    E488,
    E489,
    E490,
    E491,
    E492,
    E493,
    E494,
    E495,
    E496,
    E497,
    E498,
    E499,
    E500,
    E501,
    E502,
    E503,
    E504,
    E505,
    E506,
    E507,
    E508,
    E509,
    E510,
    E511,
    E512,
    E513,
    E514,
    E515,
    E516,
    E517,
    E518,
    E519,
    E520,
    E521,
    E522,
    E523,
    E524,
    E525,
    E526,
    E527,
    E528,
    E529,
    E530,
    E531,
    E532,
    E533,
    E534,
    E535,
    E536,
    E537,
    E538,
    E539,
    E540,
    E541,
    E542,
    E543,
    E544,
    E545,
    E546,
    E547,
    E548,
    E549,
    E550,
    E551,
    E552,
    E553,
    E554,
    E555,
    E556,
    E557,
    E558,
    E559,
    E560,
    E561,
    E562,
    E563,
    E564,
    E565,
    E566,
    E567,
    E568,
    E569,
    E570,
    E571,
    E572,
    E573,
    E574,
    E575,
    E576,
    E577,
    E578,
    E579,
    E580,
    E581,
    E582,
    E583,
    E584,
    E585,
    E586,
    E587,
    E588,
    E589,
    E590,
    E591,
    E592,
    E593,
    E594,
    E595,
    E596,
    E597,
    E598,
    E599,
    E600,
    E601,
    E602,
    E603,
    E604,
    E605,
    E606,
    E607,
    E608,
    E609,
    E610,
    E611,
    E612,
    E613,
    E614,
    E615,
    E616,
    E617,
    E618,
    E619,
    E620,
    E621,
    E622,
    E623,
    E624,
    E625,
    E626,
    E627,
    E628,
    E629,
    E630,
    E631,
    E632,
    E633,
    E634,
    E635,
    E636,
    E637,
    E638,
    E639,
    E640,
    E641,
    E642,
    E643,
    E644,
    E645,
    E646,
    E647,
    E648,
    E649,
    E650,
    E651,
    E652,
    E653,
    E654,
    E655,
    E656,
    E657,
    E658,
    E659,
    E660,
    E661,
    E662,
    E663,
    E664,
    E665,
    E666,
    E667,
    E668,
    E669,
    E670,
    E671,
    E672,
    E673,
    E674,
    E675,
    E676,
    E677,
    E678,
    E679,
    E680,
    E681,
    E682,
    E683,
    E684,
    E685,
    E686,
    E687,
    E688,
    E689,
    E690,
    E691,
    E692,
    E693,
    E694,
    E695,
    E696,
    E697,
    E698,
    E699,
    E700,
    E701,
    E702,
    E703,
    E704,
    E705,
    E706,
    E707,
    E708,
    E709,
    E710,
    E711,
    E712,
    E713,
    E714,
    E715,
    E716,
    E717,
    E718,
    E719,
    E720,
    E721,
    E722,
    E723,
    E724,
    E725,
    E726,
    E727,
    E728,
    E729,
    E730,
    E731,
    E732,
    E733,
    E734,
    E735,
    E736,
    E737,
    E738,
    E739,
    E740,
    E741,
    E742,
    E743,
    E744,
    E745,
    E746,
    E747,
    E748,
    E749,
    E750,
    E751,
    E752,
    E753,
    E754,
    E755,
    E756,
    E757,
    E758,
    E759,
    E760,
    E761,
    E762,
    E763,
    E764,
    E765,
    E766,
    E767,
    E768,
    E769,
    E770,
    E771,
    E772,
    E773,
    E774,
    E775,
    E776,
    E777,
    E778,
    E779,
    E780,
    E781,
    E782,
    E783,
    E784,
    E785,
    E786,
    E787,
    E788,
    E789,
    E790,
    E791,
    E792,
    E793,
    E794,
    E795,
    E796,
    E797,
    E798,
    E799,
    E800,
    E801,
    E802,
    E803,
    E804,
    E805,
    E806,
    E807,
    E808,
    E809,
    E810,
    E811,
    E812,
    E813,
    E814,
    E815,
    E816,
    E817,
    E818,
    E819,
    E820,
    E821,
    E822,
    E823,
    E824,
    E825,
    E826,
    E827,
    E828,
    E829,
    E830,
    E831,
    E832,
    E833,
    E834,
    E835,
    E836,
    E837,
    E838,
    E839,
    E840,
    E841,
    E842,
    E843,
    E844,
    E845,
    E846,
    E847,
    E848,
    E849,
    E850,
    E851,
    E852,
    E853,
    E854,
    E855,
    E856,
    E857,
    E858,
    E859,
    E860,
    E861,
    E862,
    E863,
    E864,
    E865,
    E866,
    E867,
    E868,
    E869,
    E870,
    E871,
    E872,
    E873,
    E874,
    E875,
    E876,
    E877,
    E878,
    E879,
    E880,
    E881,
    E882,
    E883,
    E884,
    E885,
    E886,
    E887,
    E888,
    E889,
    E890,
    E891,
    E892,
    E893,
    E894,
    E895,
    E896,
    E897,
    E898,
    E899,
    E900,
    E901,
    E902,
    E903,
    E904,
    E905,
    E906,
    E907,
    E908,
    E909,
    E910,
    E911,
    E912,
    E913,
    E914,
    E915,
    E916,
    E917,
    E918,
    E919,
    E920,
    E921,
    E922,
    E923,
    E924,
    E925,
    E926,
    E927,
    E928,
    E929,
    E930,
    E931,
    E932,
    E933,
    E934,
    E935,
    E936,
    E937,
    E938,
    E939,
    E940,
    E941,
    E942,
    E943,
    E944,
    E945,
    E946,
    E947,
    E948,
    E949,
    E950,
    E951,
    E952,
    E953,
    E954,
    E955,
    E956,
    E957,
    E958,
    E959,
    E960,
    E961,
    E962,
    E963,
    E964,
    E965,
    E966,
    E967,
    E968,
    E969,
    E970,
    E971,
    E972,
    E973,
    E974,
    E975,
    E976,
    E977,
    E978,
    E979,
    E980,
    E981,
    E982,
    E983,
    E984,
    E985,
    E986,
    E987,
    E988,
    E989,
    E990,
    E991,
    E992,
    E993,
    E994,
    E995,
    E996,
    E997,
    E998,
    E999,
    E1000,
    E1001,
    E1002,
    E1003,
    E1004,
    E1005,
    E1006,
    E1007,
    E1008,
    E1009,
    E1010,
    E1011,
    E1012,
    E1013,
    E1014,
    E1015,
    E1016,
    E1017,
    E1018,
    E1019,
    E1020,
    E1021,
    E1022,
    E1023,
}
function run(a: number) {
    switch (a) {
AssertType(a, "number");

        case 0:
AssertType(0, "int");

AssertType([ E.E0, E.E1], "E[]");
AssertType(E.E0, "E.E0");
AssertType(E.E1, "E.E1");
            return [ E.E0, E.E1]

        case 2:
AssertType(2, "int");

AssertType([ E.E2, E.E3], "E[]");
AssertType(E.E2, "E.E2");
AssertType(E.E3, "E.E3");
            return [ E.E2, E.E3]

        case 4:
AssertType(4, "int");

AssertType([ E.E4, E.E5], "E[]");
AssertType(E.E4, "E.E4");
AssertType(E.E5, "E.E5");
            return [ E.E4, E.E5]

        case 6:
AssertType(6, "int");

AssertType([ E.E6, E.E7], "E[]");
AssertType(E.E6, "E.E6");
AssertType(E.E7, "E.E7");
            return [ E.E6, E.E7]

        case 8:
AssertType(8, "int");

AssertType([ E.E8, E.E9], "E[]");
AssertType(E.E8, "E.E8");
AssertType(E.E9, "E.E9");
            return [ E.E8, E.E9]

        case 10:
AssertType(10, "int");

AssertType([ E.E10, E.E11], "E[]");
AssertType(E.E10, "E.E10");
AssertType(E.E11, "E.E11");
            return [ E.E10, E.E11]

        case 12:
AssertType(12, "int");

AssertType([ E.E12, E.E13], "E[]");
AssertType(E.E12, "E.E12");
AssertType(E.E13, "E.E13");
            return [ E.E12, E.E13]

        case 14:
AssertType(14, "int");

AssertType([ E.E14, E.E15], "E[]");
AssertType(E.E14, "E.E14");
AssertType(E.E15, "E.E15");
            return [ E.E14, E.E15]

        case 16:
AssertType(16, "int");

AssertType([ E.E16, E.E17], "E[]");
AssertType(E.E16, "E.E16");
AssertType(E.E17, "E.E17");
            return [ E.E16, E.E17]

        case 18:
AssertType(18, "int");

AssertType([ E.E18, E.E19], "E[]");
AssertType(E.E18, "E.E18");
AssertType(E.E19, "E.E19");
            return [ E.E18, E.E19]

        case 20:
AssertType(20, "int");

AssertType([ E.E20, E.E21], "E[]");
AssertType(E.E20, "E.E20");
AssertType(E.E21, "E.E21");
            return [ E.E20, E.E21]

        case 22:
AssertType(22, "int");

AssertType([ E.E22, E.E23], "E[]");
AssertType(E.E22, "E.E22");
AssertType(E.E23, "E.E23");
            return [ E.E22, E.E23]

        case 24:
AssertType(24, "int");

AssertType([ E.E24, E.E25], "E[]");
AssertType(E.E24, "E.E24");
AssertType(E.E25, "E.E25");
            return [ E.E24, E.E25]

        case 26:
AssertType(26, "int");

AssertType([ E.E26, E.E27], "E[]");
AssertType(E.E26, "E.E26");
AssertType(E.E27, "E.E27");
            return [ E.E26, E.E27]

        case 28:
AssertType(28, "int");

AssertType([ E.E28, E.E29], "E[]");
AssertType(E.E28, "E.E28");
AssertType(E.E29, "E.E29");
            return [ E.E28, E.E29]

        case 30:
AssertType(30, "int");

AssertType([ E.E30, E.E31], "E[]");
AssertType(E.E30, "E.E30");
AssertType(E.E31, "E.E31");
            return [ E.E30, E.E31]

        case 32:
AssertType(32, "int");

AssertType([ E.E32, E.E33], "E[]");
AssertType(E.E32, "E.E32");
AssertType(E.E33, "E.E33");
            return [ E.E32, E.E33]

        case 34:
AssertType(34, "int");

AssertType([ E.E34, E.E35], "E[]");
AssertType(E.E34, "E.E34");
AssertType(E.E35, "E.E35");
            return [ E.E34, E.E35]

        case 36:
AssertType(36, "int");

AssertType([ E.E36, E.E37], "E[]");
AssertType(E.E36, "E.E36");
AssertType(E.E37, "E.E37");
            return [ E.E36, E.E37]

        case 38:
AssertType(38, "int");

AssertType([ E.E38, E.E39], "E[]");
AssertType(E.E38, "E.E38");
AssertType(E.E39, "E.E39");
            return [ E.E38, E.E39]

        case 40:
AssertType(40, "int");

AssertType([ E.E40, E.E41], "E[]");
AssertType(E.E40, "E.E40");
AssertType(E.E41, "E.E41");
            return [ E.E40, E.E41]

        case 42:
AssertType(42, "int");

AssertType([ E.E42, E.E43], "E[]");
AssertType(E.E42, "E.E42");
AssertType(E.E43, "E.E43");
            return [ E.E42, E.E43]

        case 44:
AssertType(44, "int");

AssertType([ E.E44, E.E45], "E[]");
AssertType(E.E44, "E.E44");
AssertType(E.E45, "E.E45");
            return [ E.E44, E.E45]

        case 46:
AssertType(46, "int");

AssertType([ E.E46, E.E47], "E[]");
AssertType(E.E46, "E.E46");
AssertType(E.E47, "E.E47");
            return [ E.E46, E.E47]

        case 48:
AssertType(48, "int");

AssertType([ E.E48, E.E49], "E[]");
AssertType(E.E48, "E.E48");
AssertType(E.E49, "E.E49");
            return [ E.E48, E.E49]

        case 50:
AssertType(50, "int");

AssertType([ E.E50, E.E51], "E[]");
AssertType(E.E50, "E.E50");
AssertType(E.E51, "E.E51");
            return [ E.E50, E.E51]

        case 52:
AssertType(52, "int");

AssertType([ E.E52, E.E53], "E[]");
AssertType(E.E52, "E.E52");
AssertType(E.E53, "E.E53");
            return [ E.E52, E.E53]

        case 54:
AssertType(54, "int");

AssertType([ E.E54, E.E55], "E[]");
AssertType(E.E54, "E.E54");
AssertType(E.E55, "E.E55");
            return [ E.E54, E.E55]

        case 56:
AssertType(56, "int");

AssertType([ E.E56, E.E57], "E[]");
AssertType(E.E56, "E.E56");
AssertType(E.E57, "E.E57");
            return [ E.E56, E.E57]

        case 58:
AssertType(58, "int");

AssertType([ E.E58, E.E59], "E[]");
AssertType(E.E58, "E.E58");
AssertType(E.E59, "E.E59");
            return [ E.E58, E.E59]

        case 60:
AssertType(60, "int");

AssertType([ E.E60, E.E61], "E[]");
AssertType(E.E60, "E.E60");
AssertType(E.E61, "E.E61");
            return [ E.E60, E.E61]

        case 62:
AssertType(62, "int");

AssertType([ E.E62, E.E63], "E[]");
AssertType(E.E62, "E.E62");
AssertType(E.E63, "E.E63");
            return [ E.E62, E.E63]

        case 64:
AssertType(64, "int");

AssertType([ E.E64, E.E65], "E[]");
AssertType(E.E64, "E.E64");
AssertType(E.E65, "E.E65");
            return [ E.E64, E.E65]

        case 66:
AssertType(66, "int");

AssertType([ E.E66, E.E67], "E[]");
AssertType(E.E66, "E.E66");
AssertType(E.E67, "E.E67");
            return [ E.E66, E.E67]

        case 68:
AssertType(68, "int");

AssertType([ E.E68, E.E69], "E[]");
AssertType(E.E68, "E.E68");
AssertType(E.E69, "E.E69");
            return [ E.E68, E.E69]

        case 70:
AssertType(70, "int");

AssertType([ E.E70, E.E71], "E[]");
AssertType(E.E70, "E.E70");
AssertType(E.E71, "E.E71");
            return [ E.E70, E.E71]

        case 72:
AssertType(72, "int");

AssertType([ E.E72, E.E73], "E[]");
AssertType(E.E72, "E.E72");
AssertType(E.E73, "E.E73");
            return [ E.E72, E.E73]

        case 74:
AssertType(74, "int");

AssertType([ E.E74, E.E75], "E[]");
AssertType(E.E74, "E.E74");
AssertType(E.E75, "E.E75");
            return [ E.E74, E.E75]

        case 76:
AssertType(76, "int");

AssertType([ E.E76, E.E77], "E[]");
AssertType(E.E76, "E.E76");
AssertType(E.E77, "E.E77");
            return [ E.E76, E.E77]

        case 78:
AssertType(78, "int");

AssertType([ E.E78, E.E79], "E[]");
AssertType(E.E78, "E.E78");
AssertType(E.E79, "E.E79");
            return [ E.E78, E.E79]

        case 80:
AssertType(80, "int");

AssertType([ E.E80, E.E81], "E[]");
AssertType(E.E80, "E.E80");
AssertType(E.E81, "E.E81");
            return [ E.E80, E.E81]

        case 82:
AssertType(82, "int");

AssertType([ E.E82, E.E83], "E[]");
AssertType(E.E82, "E.E82");
AssertType(E.E83, "E.E83");
            return [ E.E82, E.E83]

        case 84:
AssertType(84, "int");

AssertType([ E.E84, E.E85], "E[]");
AssertType(E.E84, "E.E84");
AssertType(E.E85, "E.E85");
            return [ E.E84, E.E85]

        case 86:
AssertType(86, "int");

AssertType([ E.E86, E.E87], "E[]");
AssertType(E.E86, "E.E86");
AssertType(E.E87, "E.E87");
            return [ E.E86, E.E87]

        case 88:
AssertType(88, "int");

AssertType([ E.E88, E.E89], "E[]");
AssertType(E.E88, "E.E88");
AssertType(E.E89, "E.E89");
            return [ E.E88, E.E89]

        case 90:
AssertType(90, "int");

AssertType([ E.E90, E.E91], "E[]");
AssertType(E.E90, "E.E90");
AssertType(E.E91, "E.E91");
            return [ E.E90, E.E91]

        case 92:
AssertType(92, "int");

AssertType([ E.E92, E.E93], "E[]");
AssertType(E.E92, "E.E92");
AssertType(E.E93, "E.E93");
            return [ E.E92, E.E93]

        case 94:
AssertType(94, "int");

AssertType([ E.E94, E.E95], "E[]");
AssertType(E.E94, "E.E94");
AssertType(E.E95, "E.E95");
            return [ E.E94, E.E95]

        case 96:
AssertType(96, "int");

AssertType([ E.E96, E.E97], "E[]");
AssertType(E.E96, "E.E96");
AssertType(E.E97, "E.E97");
            return [ E.E96, E.E97]

        case 98:
AssertType(98, "int");

AssertType([ E.E98, E.E99], "E[]");
AssertType(E.E98, "E.E98");
AssertType(E.E99, "E.E99");
            return [ E.E98, E.E99]

        case 100:
AssertType(100, "int");

AssertType([ E.E100, E.E101], "E[]");
AssertType(E.E100, "E.E100");
AssertType(E.E101, "E.E101");
            return [ E.E100, E.E101]

        case 102:
AssertType(102, "int");

AssertType([ E.E102, E.E103], "E[]");
AssertType(E.E102, "E.E102");
AssertType(E.E103, "E.E103");
            return [ E.E102, E.E103]

        case 104:
AssertType(104, "int");

AssertType([ E.E104, E.E105], "E[]");
AssertType(E.E104, "E.E104");
AssertType(E.E105, "E.E105");
            return [ E.E104, E.E105]

        case 106:
AssertType(106, "int");

AssertType([ E.E106, E.E107], "E[]");
AssertType(E.E106, "E.E106");
AssertType(E.E107, "E.E107");
            return [ E.E106, E.E107]

        case 108:
AssertType(108, "int");

AssertType([ E.E108, E.E109], "E[]");
AssertType(E.E108, "E.E108");
AssertType(E.E109, "E.E109");
            return [ E.E108, E.E109]

        case 110:
AssertType(110, "int");

AssertType([ E.E110, E.E111], "E[]");
AssertType(E.E110, "E.E110");
AssertType(E.E111, "E.E111");
            return [ E.E110, E.E111]

        case 112:
AssertType(112, "int");

AssertType([ E.E112, E.E113], "E[]");
AssertType(E.E112, "E.E112");
AssertType(E.E113, "E.E113");
            return [ E.E112, E.E113]

        case 114:
AssertType(114, "int");

AssertType([ E.E114, E.E115], "E[]");
AssertType(E.E114, "E.E114");
AssertType(E.E115, "E.E115");
            return [ E.E114, E.E115]

        case 116:
AssertType(116, "int");

AssertType([ E.E116, E.E117], "E[]");
AssertType(E.E116, "E.E116");
AssertType(E.E117, "E.E117");
            return [ E.E116, E.E117]

        case 118:
AssertType(118, "int");

AssertType([ E.E118, E.E119], "E[]");
AssertType(E.E118, "E.E118");
AssertType(E.E119, "E.E119");
            return [ E.E118, E.E119]

        case 120:
AssertType(120, "int");

AssertType([ E.E120, E.E121], "E[]");
AssertType(E.E120, "E.E120");
AssertType(E.E121, "E.E121");
            return [ E.E120, E.E121]

        case 122:
AssertType(122, "int");

AssertType([ E.E122, E.E123], "E[]");
AssertType(E.E122, "E.E122");
AssertType(E.E123, "E.E123");
            return [ E.E122, E.E123]

        case 124:
AssertType(124, "int");

AssertType([ E.E124, E.E125], "E[]");
AssertType(E.E124, "E.E124");
AssertType(E.E125, "E.E125");
            return [ E.E124, E.E125]

        case 126:
AssertType(126, "int");

AssertType([ E.E126, E.E127], "E[]");
AssertType(E.E126, "E.E126");
AssertType(E.E127, "E.E127");
            return [ E.E126, E.E127]

        case 128:
AssertType(128, "int");

AssertType([ E.E128, E.E129], "E[]");
AssertType(E.E128, "E.E128");
AssertType(E.E129, "E.E129");
            return [ E.E128, E.E129]

        case 130:
AssertType(130, "int");

AssertType([ E.E130, E.E131], "E[]");
AssertType(E.E130, "E.E130");
AssertType(E.E131, "E.E131");
            return [ E.E130, E.E131]

        case 132:
AssertType(132, "int");

AssertType([ E.E132, E.E133], "E[]");
AssertType(E.E132, "E.E132");
AssertType(E.E133, "E.E133");
            return [ E.E132, E.E133]

        case 134:
AssertType(134, "int");

AssertType([ E.E134, E.E135], "E[]");
AssertType(E.E134, "E.E134");
AssertType(E.E135, "E.E135");
            return [ E.E134, E.E135]

        case 136:
AssertType(136, "int");

AssertType([ E.E136, E.E137], "E[]");
AssertType(E.E136, "E.E136");
AssertType(E.E137, "E.E137");
            return [ E.E136, E.E137]

        case 138:
AssertType(138, "int");

AssertType([ E.E138, E.E139], "E[]");
AssertType(E.E138, "E.E138");
AssertType(E.E139, "E.E139");
            return [ E.E138, E.E139]

        case 140:
AssertType(140, "int");

AssertType([ E.E140, E.E141], "E[]");
AssertType(E.E140, "E.E140");
AssertType(E.E141, "E.E141");
            return [ E.E140, E.E141]

        case 142:
AssertType(142, "int");

AssertType([ E.E142, E.E143], "E[]");
AssertType(E.E142, "E.E142");
AssertType(E.E143, "E.E143");
            return [ E.E142, E.E143]

        case 144:
AssertType(144, "int");

AssertType([ E.E144, E.E145], "E[]");
AssertType(E.E144, "E.E144");
AssertType(E.E145, "E.E145");
            return [ E.E144, E.E145]

        case 146:
AssertType(146, "int");

AssertType([ E.E146, E.E147], "E[]");
AssertType(E.E146, "E.E146");
AssertType(E.E147, "E.E147");
            return [ E.E146, E.E147]

        case 148:
AssertType(148, "int");

AssertType([ E.E148, E.E149], "E[]");
AssertType(E.E148, "E.E148");
AssertType(E.E149, "E.E149");
            return [ E.E148, E.E149]

        case 150:
AssertType(150, "int");

AssertType([ E.E150, E.E151], "E[]");
AssertType(E.E150, "E.E150");
AssertType(E.E151, "E.E151");
            return [ E.E150, E.E151]

        case 152:
AssertType(152, "int");

AssertType([ E.E152, E.E153], "E[]");
AssertType(E.E152, "E.E152");
AssertType(E.E153, "E.E153");
            return [ E.E152, E.E153]

        case 154:
AssertType(154, "int");

AssertType([ E.E154, E.E155], "E[]");
AssertType(E.E154, "E.E154");
AssertType(E.E155, "E.E155");
            return [ E.E154, E.E155]

        case 156:
AssertType(156, "int");

AssertType([ E.E156, E.E157], "E[]");
AssertType(E.E156, "E.E156");
AssertType(E.E157, "E.E157");
            return [ E.E156, E.E157]

        case 158:
AssertType(158, "int");

AssertType([ E.E158, E.E159], "E[]");
AssertType(E.E158, "E.E158");
AssertType(E.E159, "E.E159");
            return [ E.E158, E.E159]

        case 160:
AssertType(160, "int");

AssertType([ E.E160, E.E161], "E[]");
AssertType(E.E160, "E.E160");
AssertType(E.E161, "E.E161");
            return [ E.E160, E.E161]

        case 162:
AssertType(162, "int");

AssertType([ E.E162, E.E163], "E[]");
AssertType(E.E162, "E.E162");
AssertType(E.E163, "E.E163");
            return [ E.E162, E.E163]

        case 164:
AssertType(164, "int");

AssertType([ E.E164, E.E165], "E[]");
AssertType(E.E164, "E.E164");
AssertType(E.E165, "E.E165");
            return [ E.E164, E.E165]

        case 166:
AssertType(166, "int");

AssertType([ E.E166, E.E167], "E[]");
AssertType(E.E166, "E.E166");
AssertType(E.E167, "E.E167");
            return [ E.E166, E.E167]

        case 168:
AssertType(168, "int");

AssertType([ E.E168, E.E169], "E[]");
AssertType(E.E168, "E.E168");
AssertType(E.E169, "E.E169");
            return [ E.E168, E.E169]

        case 170:
AssertType(170, "int");

AssertType([ E.E170, E.E171], "E[]");
AssertType(E.E170, "E.E170");
AssertType(E.E171, "E.E171");
            return [ E.E170, E.E171]

        case 172:
AssertType(172, "int");

AssertType([ E.E172, E.E173], "E[]");
AssertType(E.E172, "E.E172");
AssertType(E.E173, "E.E173");
            return [ E.E172, E.E173]

        case 174:
AssertType(174, "int");

AssertType([ E.E174, E.E175], "E[]");
AssertType(E.E174, "E.E174");
AssertType(E.E175, "E.E175");
            return [ E.E174, E.E175]

        case 176:
AssertType(176, "int");

AssertType([ E.E176, E.E177], "E[]");
AssertType(E.E176, "E.E176");
AssertType(E.E177, "E.E177");
            return [ E.E176, E.E177]

        case 178:
AssertType(178, "int");

AssertType([ E.E178, E.E179], "E[]");
AssertType(E.E178, "E.E178");
AssertType(E.E179, "E.E179");
            return [ E.E178, E.E179]

        case 180:
AssertType(180, "int");

AssertType([ E.E180, E.E181], "E[]");
AssertType(E.E180, "E.E180");
AssertType(E.E181, "E.E181");
            return [ E.E180, E.E181]

        case 182:
AssertType(182, "int");

AssertType([ E.E182, E.E183], "E[]");
AssertType(E.E182, "E.E182");
AssertType(E.E183, "E.E183");
            return [ E.E182, E.E183]

        case 184:
AssertType(184, "int");

AssertType([ E.E184, E.E185], "E[]");
AssertType(E.E184, "E.E184");
AssertType(E.E185, "E.E185");
            return [ E.E184, E.E185]

        case 186:
AssertType(186, "int");

AssertType([ E.E186, E.E187], "E[]");
AssertType(E.E186, "E.E186");
AssertType(E.E187, "E.E187");
            return [ E.E186, E.E187]

        case 188:
AssertType(188, "int");

AssertType([ E.E188, E.E189], "E[]");
AssertType(E.E188, "E.E188");
AssertType(E.E189, "E.E189");
            return [ E.E188, E.E189]

        case 190:
AssertType(190, "int");

AssertType([ E.E190, E.E191], "E[]");
AssertType(E.E190, "E.E190");
AssertType(E.E191, "E.E191");
            return [ E.E190, E.E191]

        case 192:
AssertType(192, "int");

AssertType([ E.E192, E.E193], "E[]");
AssertType(E.E192, "E.E192");
AssertType(E.E193, "E.E193");
            return [ E.E192, E.E193]

        case 194:
AssertType(194, "int");

AssertType([ E.E194, E.E195], "E[]");
AssertType(E.E194, "E.E194");
AssertType(E.E195, "E.E195");
            return [ E.E194, E.E195]

        case 196:
AssertType(196, "int");

AssertType([ E.E196, E.E197], "E[]");
AssertType(E.E196, "E.E196");
AssertType(E.E197, "E.E197");
            return [ E.E196, E.E197]

        case 198:
AssertType(198, "int");

AssertType([ E.E198, E.E199], "E[]");
AssertType(E.E198, "E.E198");
AssertType(E.E199, "E.E199");
            return [ E.E198, E.E199]

        case 200:
AssertType(200, "int");

AssertType([ E.E200, E.E201], "E[]");
AssertType(E.E200, "E.E200");
AssertType(E.E201, "E.E201");
            return [ E.E200, E.E201]

        case 202:
AssertType(202, "int");

AssertType([ E.E202, E.E203], "E[]");
AssertType(E.E202, "E.E202");
AssertType(E.E203, "E.E203");
            return [ E.E202, E.E203]

        case 204:
AssertType(204, "int");

AssertType([ E.E204, E.E205], "E[]");
AssertType(E.E204, "E.E204");
AssertType(E.E205, "E.E205");
            return [ E.E204, E.E205]

        case 206:
AssertType(206, "int");

AssertType([ E.E206, E.E207], "E[]");
AssertType(E.E206, "E.E206");
AssertType(E.E207, "E.E207");
            return [ E.E206, E.E207]

        case 208:
AssertType(208, "int");

AssertType([ E.E208, E.E209], "E[]");
AssertType(E.E208, "E.E208");
AssertType(E.E209, "E.E209");
            return [ E.E208, E.E209]

        case 210:
AssertType(210, "int");

AssertType([ E.E210, E.E211], "E[]");
AssertType(E.E210, "E.E210");
AssertType(E.E211, "E.E211");
            return [ E.E210, E.E211]

        case 212:
AssertType(212, "int");

AssertType([ E.E212, E.E213], "E[]");
AssertType(E.E212, "E.E212");
AssertType(E.E213, "E.E213");
            return [ E.E212, E.E213]

        case 214:
AssertType(214, "int");

AssertType([ E.E214, E.E215], "E[]");
AssertType(E.E214, "E.E214");
AssertType(E.E215, "E.E215");
            return [ E.E214, E.E215]

        case 216:
AssertType(216, "int");

AssertType([ E.E216, E.E217], "E[]");
AssertType(E.E216, "E.E216");
AssertType(E.E217, "E.E217");
            return [ E.E216, E.E217]

        case 218:
AssertType(218, "int");

AssertType([ E.E218, E.E219], "E[]");
AssertType(E.E218, "E.E218");
AssertType(E.E219, "E.E219");
            return [ E.E218, E.E219]

        case 220:
AssertType(220, "int");

AssertType([ E.E220, E.E221], "E[]");
AssertType(E.E220, "E.E220");
AssertType(E.E221, "E.E221");
            return [ E.E220, E.E221]

        case 222:
AssertType(222, "int");

AssertType([ E.E222, E.E223], "E[]");
AssertType(E.E222, "E.E222");
AssertType(E.E223, "E.E223");
            return [ E.E222, E.E223]

        case 224:
AssertType(224, "int");

AssertType([ E.E224, E.E225], "E[]");
AssertType(E.E224, "E.E224");
AssertType(E.E225, "E.E225");
            return [ E.E224, E.E225]

        case 226:
AssertType(226, "int");

AssertType([ E.E226, E.E227], "E[]");
AssertType(E.E226, "E.E226");
AssertType(E.E227, "E.E227");
            return [ E.E226, E.E227]

        case 228:
AssertType(228, "int");

AssertType([ E.E228, E.E229], "E[]");
AssertType(E.E228, "E.E228");
AssertType(E.E229, "E.E229");
            return [ E.E228, E.E229]

        case 230:
AssertType(230, "int");

AssertType([ E.E230, E.E231], "E[]");
AssertType(E.E230, "E.E230");
AssertType(E.E231, "E.E231");
            return [ E.E230, E.E231]

        case 232:
AssertType(232, "int");

AssertType([ E.E232, E.E233], "E[]");
AssertType(E.E232, "E.E232");
AssertType(E.E233, "E.E233");
            return [ E.E232, E.E233]

        case 234:
AssertType(234, "int");

AssertType([ E.E234, E.E235], "E[]");
AssertType(E.E234, "E.E234");
AssertType(E.E235, "E.E235");
            return [ E.E234, E.E235]

        case 236:
AssertType(236, "int");

AssertType([ E.E236, E.E237], "E[]");
AssertType(E.E236, "E.E236");
AssertType(E.E237, "E.E237");
            return [ E.E236, E.E237]

        case 238:
AssertType(238, "int");

AssertType([ E.E238, E.E239], "E[]");
AssertType(E.E238, "E.E238");
AssertType(E.E239, "E.E239");
            return [ E.E238, E.E239]

        case 240:
AssertType(240, "int");

AssertType([ E.E240, E.E241], "E[]");
AssertType(E.E240, "E.E240");
AssertType(E.E241, "E.E241");
            return [ E.E240, E.E241]

        case 242:
AssertType(242, "int");

AssertType([ E.E242, E.E243], "E[]");
AssertType(E.E242, "E.E242");
AssertType(E.E243, "E.E243");
            return [ E.E242, E.E243]

        case 244:
AssertType(244, "int");

AssertType([ E.E244, E.E245], "E[]");
AssertType(E.E244, "E.E244");
AssertType(E.E245, "E.E245");
            return [ E.E244, E.E245]

        case 246:
AssertType(246, "int");

AssertType([ E.E246, E.E247], "E[]");
AssertType(E.E246, "E.E246");
AssertType(E.E247, "E.E247");
            return [ E.E246, E.E247]

        case 248:
AssertType(248, "int");

AssertType([ E.E248, E.E249], "E[]");
AssertType(E.E248, "E.E248");
AssertType(E.E249, "E.E249");
            return [ E.E248, E.E249]

        case 250:
AssertType(250, "int");

AssertType([ E.E250, E.E251], "E[]");
AssertType(E.E250, "E.E250");
AssertType(E.E251, "E.E251");
            return [ E.E250, E.E251]

        case 252:
AssertType(252, "int");

AssertType([ E.E252, E.E253], "E[]");
AssertType(E.E252, "E.E252");
AssertType(E.E253, "E.E253");
            return [ E.E252, E.E253]

        case 254:
AssertType(254, "int");

AssertType([ E.E254, E.E255], "E[]");
AssertType(E.E254, "E.E254");
AssertType(E.E255, "E.E255");
            return [ E.E254, E.E255]

        case 256:
AssertType(256, "int");

AssertType([ E.E256, E.E257], "E[]");
AssertType(E.E256, "E.E256");
AssertType(E.E257, "E.E257");
            return [ E.E256, E.E257]

        case 258:
AssertType(258, "int");

AssertType([ E.E258, E.E259], "E[]");
AssertType(E.E258, "E.E258");
AssertType(E.E259, "E.E259");
            return [ E.E258, E.E259]

        case 260:
AssertType(260, "int");

AssertType([ E.E260, E.E261], "E[]");
AssertType(E.E260, "E.E260");
AssertType(E.E261, "E.E261");
            return [ E.E260, E.E261]

        case 262:
AssertType(262, "int");

AssertType([ E.E262, E.E263], "E[]");
AssertType(E.E262, "E.E262");
AssertType(E.E263, "E.E263");
            return [ E.E262, E.E263]

        case 264:
AssertType(264, "int");

AssertType([ E.E264, E.E265], "E[]");
AssertType(E.E264, "E.E264");
AssertType(E.E265, "E.E265");
            return [ E.E264, E.E265]

        case 266:
AssertType(266, "int");

AssertType([ E.E266, E.E267], "E[]");
AssertType(E.E266, "E.E266");
AssertType(E.E267, "E.E267");
            return [ E.E266, E.E267]

        case 268:
AssertType(268, "int");

AssertType([ E.E268, E.E269], "E[]");
AssertType(E.E268, "E.E268");
AssertType(E.E269, "E.E269");
            return [ E.E268, E.E269]

        case 270:
AssertType(270, "int");

AssertType([ E.E270, E.E271], "E[]");
AssertType(E.E270, "E.E270");
AssertType(E.E271, "E.E271");
            return [ E.E270, E.E271]

        case 272:
AssertType(272, "int");

AssertType([ E.E272, E.E273], "E[]");
AssertType(E.E272, "E.E272");
AssertType(E.E273, "E.E273");
            return [ E.E272, E.E273]

        case 274:
AssertType(274, "int");

AssertType([ E.E274, E.E275], "E[]");
AssertType(E.E274, "E.E274");
AssertType(E.E275, "E.E275");
            return [ E.E274, E.E275]

        case 276:
AssertType(276, "int");

AssertType([ E.E276, E.E277], "E[]");
AssertType(E.E276, "E.E276");
AssertType(E.E277, "E.E277");
            return [ E.E276, E.E277]

        case 278:
AssertType(278, "int");

AssertType([ E.E278, E.E279], "E[]");
AssertType(E.E278, "E.E278");
AssertType(E.E279, "E.E279");
            return [ E.E278, E.E279]

        case 280:
AssertType(280, "int");

AssertType([ E.E280, E.E281], "E[]");
AssertType(E.E280, "E.E280");
AssertType(E.E281, "E.E281");
            return [ E.E280, E.E281]

        case 282:
AssertType(282, "int");

AssertType([ E.E282, E.E283], "E[]");
AssertType(E.E282, "E.E282");
AssertType(E.E283, "E.E283");
            return [ E.E282, E.E283]

        case 284:
AssertType(284, "int");

AssertType([ E.E284, E.E285], "E[]");
AssertType(E.E284, "E.E284");
AssertType(E.E285, "E.E285");
            return [ E.E284, E.E285]

        case 286:
AssertType(286, "int");

AssertType([ E.E286, E.E287], "E[]");
AssertType(E.E286, "E.E286");
AssertType(E.E287, "E.E287");
            return [ E.E286, E.E287]

        case 288:
AssertType(288, "int");

AssertType([ E.E288, E.E289], "E[]");
AssertType(E.E288, "E.E288");
AssertType(E.E289, "E.E289");
            return [ E.E288, E.E289]

        case 290:
AssertType(290, "int");

AssertType([ E.E290, E.E291], "E[]");
AssertType(E.E290, "E.E290");
AssertType(E.E291, "E.E291");
            return [ E.E290, E.E291]

        case 292:
AssertType(292, "int");

AssertType([ E.E292, E.E293], "E[]");
AssertType(E.E292, "E.E292");
AssertType(E.E293, "E.E293");
            return [ E.E292, E.E293]

        case 294:
AssertType(294, "int");

AssertType([ E.E294, E.E295], "E[]");
AssertType(E.E294, "E.E294");
AssertType(E.E295, "E.E295");
            return [ E.E294, E.E295]

        case 296:
AssertType(296, "int");

AssertType([ E.E296, E.E297], "E[]");
AssertType(E.E296, "E.E296");
AssertType(E.E297, "E.E297");
            return [ E.E296, E.E297]

        case 298:
AssertType(298, "int");

AssertType([ E.E298, E.E299], "E[]");
AssertType(E.E298, "E.E298");
AssertType(E.E299, "E.E299");
            return [ E.E298, E.E299]

        case 300:
AssertType(300, "int");

AssertType([ E.E300, E.E301], "E[]");
AssertType(E.E300, "E.E300");
AssertType(E.E301, "E.E301");
            return [ E.E300, E.E301]

        case 302:
AssertType(302, "int");

AssertType([ E.E302, E.E303], "E[]");
AssertType(E.E302, "E.E302");
AssertType(E.E303, "E.E303");
            return [ E.E302, E.E303]

        case 304:
AssertType(304, "int");

AssertType([ E.E304, E.E305], "E[]");
AssertType(E.E304, "E.E304");
AssertType(E.E305, "E.E305");
            return [ E.E304, E.E305]

        case 306:
AssertType(306, "int");

AssertType([ E.E306, E.E307], "E[]");
AssertType(E.E306, "E.E306");
AssertType(E.E307, "E.E307");
            return [ E.E306, E.E307]

        case 308:
AssertType(308, "int");

AssertType([ E.E308, E.E309], "E[]");
AssertType(E.E308, "E.E308");
AssertType(E.E309, "E.E309");
            return [ E.E308, E.E309]

        case 310:
AssertType(310, "int");

AssertType([ E.E310, E.E311], "E[]");
AssertType(E.E310, "E.E310");
AssertType(E.E311, "E.E311");
            return [ E.E310, E.E311]

        case 312:
AssertType(312, "int");

AssertType([ E.E312, E.E313], "E[]");
AssertType(E.E312, "E.E312");
AssertType(E.E313, "E.E313");
            return [ E.E312, E.E313]

        case 314:
AssertType(314, "int");

AssertType([ E.E314, E.E315], "E[]");
AssertType(E.E314, "E.E314");
AssertType(E.E315, "E.E315");
            return [ E.E314, E.E315]

        case 316:
AssertType(316, "int");

AssertType([ E.E316, E.E317], "E[]");
AssertType(E.E316, "E.E316");
AssertType(E.E317, "E.E317");
            return [ E.E316, E.E317]

        case 318:
AssertType(318, "int");

AssertType([ E.E318, E.E319], "E[]");
AssertType(E.E318, "E.E318");
AssertType(E.E319, "E.E319");
            return [ E.E318, E.E319]

        case 320:
AssertType(320, "int");

AssertType([ E.E320, E.E321], "E[]");
AssertType(E.E320, "E.E320");
AssertType(E.E321, "E.E321");
            return [ E.E320, E.E321]

        case 322:
AssertType(322, "int");

AssertType([ E.E322, E.E323], "E[]");
AssertType(E.E322, "E.E322");
AssertType(E.E323, "E.E323");
            return [ E.E322, E.E323]

        case 324:
AssertType(324, "int");

AssertType([ E.E324, E.E325], "E[]");
AssertType(E.E324, "E.E324");
AssertType(E.E325, "E.E325");
            return [ E.E324, E.E325]

        case 326:
AssertType(326, "int");

AssertType([ E.E326, E.E327], "E[]");
AssertType(E.E326, "E.E326");
AssertType(E.E327, "E.E327");
            return [ E.E326, E.E327]

        case 328:
AssertType(328, "int");

AssertType([ E.E328, E.E329], "E[]");
AssertType(E.E328, "E.E328");
AssertType(E.E329, "E.E329");
            return [ E.E328, E.E329]

        case 330:
AssertType(330, "int");

AssertType([ E.E330, E.E331], "E[]");
AssertType(E.E330, "E.E330");
AssertType(E.E331, "E.E331");
            return [ E.E330, E.E331]

        case 332:
AssertType(332, "int");

AssertType([ E.E332, E.E333], "E[]");
AssertType(E.E332, "E.E332");
AssertType(E.E333, "E.E333");
            return [ E.E332, E.E333]

        case 334:
AssertType(334, "int");

AssertType([ E.E334, E.E335], "E[]");
AssertType(E.E334, "E.E334");
AssertType(E.E335, "E.E335");
            return [ E.E334, E.E335]

        case 336:
AssertType(336, "int");

AssertType([ E.E336, E.E337], "E[]");
AssertType(E.E336, "E.E336");
AssertType(E.E337, "E.E337");
            return [ E.E336, E.E337]

        case 338:
AssertType(338, "int");

AssertType([ E.E338, E.E339], "E[]");
AssertType(E.E338, "E.E338");
AssertType(E.E339, "E.E339");
            return [ E.E338, E.E339]

        case 340:
AssertType(340, "int");

AssertType([ E.E340, E.E341], "E[]");
AssertType(E.E340, "E.E340");
AssertType(E.E341, "E.E341");
            return [ E.E340, E.E341]

        case 342:
AssertType(342, "int");

AssertType([ E.E342, E.E343], "E[]");
AssertType(E.E342, "E.E342");
AssertType(E.E343, "E.E343");
            return [ E.E342, E.E343]

        case 344:
AssertType(344, "int");

AssertType([ E.E344, E.E345], "E[]");
AssertType(E.E344, "E.E344");
AssertType(E.E345, "E.E345");
            return [ E.E344, E.E345]

        case 346:
AssertType(346, "int");

AssertType([ E.E346, E.E347], "E[]");
AssertType(E.E346, "E.E346");
AssertType(E.E347, "E.E347");
            return [ E.E346, E.E347]

        case 348:
AssertType(348, "int");

AssertType([ E.E348, E.E349], "E[]");
AssertType(E.E348, "E.E348");
AssertType(E.E349, "E.E349");
            return [ E.E348, E.E349]

        case 350:
AssertType(350, "int");

AssertType([ E.E350, E.E351], "E[]");
AssertType(E.E350, "E.E350");
AssertType(E.E351, "E.E351");
            return [ E.E350, E.E351]

        case 352:
AssertType(352, "int");

AssertType([ E.E352, E.E353], "E[]");
AssertType(E.E352, "E.E352");
AssertType(E.E353, "E.E353");
            return [ E.E352, E.E353]

        case 354:
AssertType(354, "int");

AssertType([ E.E354, E.E355], "E[]");
AssertType(E.E354, "E.E354");
AssertType(E.E355, "E.E355");
            return [ E.E354, E.E355]

        case 356:
AssertType(356, "int");

AssertType([ E.E356, E.E357], "E[]");
AssertType(E.E356, "E.E356");
AssertType(E.E357, "E.E357");
            return [ E.E356, E.E357]

        case 358:
AssertType(358, "int");

AssertType([ E.E358, E.E359], "E[]");
AssertType(E.E358, "E.E358");
AssertType(E.E359, "E.E359");
            return [ E.E358, E.E359]

        case 360:
AssertType(360, "int");

AssertType([ E.E360, E.E361], "E[]");
AssertType(E.E360, "E.E360");
AssertType(E.E361, "E.E361");
            return [ E.E360, E.E361]

        case 362:
AssertType(362, "int");

AssertType([ E.E362, E.E363], "E[]");
AssertType(E.E362, "E.E362");
AssertType(E.E363, "E.E363");
            return [ E.E362, E.E363]

        case 364:
AssertType(364, "int");

AssertType([ E.E364, E.E365], "E[]");
AssertType(E.E364, "E.E364");
AssertType(E.E365, "E.E365");
            return [ E.E364, E.E365]

        case 366:
AssertType(366, "int");

AssertType([ E.E366, E.E367], "E[]");
AssertType(E.E366, "E.E366");
AssertType(E.E367, "E.E367");
            return [ E.E366, E.E367]

        case 368:
AssertType(368, "int");

AssertType([ E.E368, E.E369], "E[]");
AssertType(E.E368, "E.E368");
AssertType(E.E369, "E.E369");
            return [ E.E368, E.E369]

        case 370:
AssertType(370, "int");

AssertType([ E.E370, E.E371], "E[]");
AssertType(E.E370, "E.E370");
AssertType(E.E371, "E.E371");
            return [ E.E370, E.E371]

        case 372:
AssertType(372, "int");

AssertType([ E.E372, E.E373], "E[]");
AssertType(E.E372, "E.E372");
AssertType(E.E373, "E.E373");
            return [ E.E372, E.E373]

        case 374:
AssertType(374, "int");

AssertType([ E.E374, E.E375], "E[]");
AssertType(E.E374, "E.E374");
AssertType(E.E375, "E.E375");
            return [ E.E374, E.E375]

        case 376:
AssertType(376, "int");

AssertType([ E.E376, E.E377], "E[]");
AssertType(E.E376, "E.E376");
AssertType(E.E377, "E.E377");
            return [ E.E376, E.E377]

        case 378:
AssertType(378, "int");

AssertType([ E.E378, E.E379], "E[]");
AssertType(E.E378, "E.E378");
AssertType(E.E379, "E.E379");
            return [ E.E378, E.E379]

        case 380:
AssertType(380, "int");

AssertType([ E.E380, E.E381], "E[]");
AssertType(E.E380, "E.E380");
AssertType(E.E381, "E.E381");
            return [ E.E380, E.E381]

        case 382:
AssertType(382, "int");

AssertType([ E.E382, E.E383], "E[]");
AssertType(E.E382, "E.E382");
AssertType(E.E383, "E.E383");
            return [ E.E382, E.E383]

        case 384:
AssertType(384, "int");

AssertType([ E.E384, E.E385], "E[]");
AssertType(E.E384, "E.E384");
AssertType(E.E385, "E.E385");
            return [ E.E384, E.E385]

        case 386:
AssertType(386, "int");

AssertType([ E.E386, E.E387], "E[]");
AssertType(E.E386, "E.E386");
AssertType(E.E387, "E.E387");
            return [ E.E386, E.E387]

        case 388:
AssertType(388, "int");

AssertType([ E.E388, E.E389], "E[]");
AssertType(E.E388, "E.E388");
AssertType(E.E389, "E.E389");
            return [ E.E388, E.E389]

        case 390:
AssertType(390, "int");

AssertType([ E.E390, E.E391], "E[]");
AssertType(E.E390, "E.E390");
AssertType(E.E391, "E.E391");
            return [ E.E390, E.E391]

        case 392:
AssertType(392, "int");

AssertType([ E.E392, E.E393], "E[]");
AssertType(E.E392, "E.E392");
AssertType(E.E393, "E.E393");
            return [ E.E392, E.E393]

        case 394:
AssertType(394, "int");

AssertType([ E.E394, E.E395], "E[]");
AssertType(E.E394, "E.E394");
AssertType(E.E395, "E.E395");
            return [ E.E394, E.E395]

        case 396:
AssertType(396, "int");

AssertType([ E.E396, E.E397], "E[]");
AssertType(E.E396, "E.E396");
AssertType(E.E397, "E.E397");
            return [ E.E396, E.E397]

        case 398:
AssertType(398, "int");

AssertType([ E.E398, E.E399], "E[]");
AssertType(E.E398, "E.E398");
AssertType(E.E399, "E.E399");
            return [ E.E398, E.E399]

        case 400:
AssertType(400, "int");

AssertType([ E.E400, E.E401], "E[]");
AssertType(E.E400, "E.E400");
AssertType(E.E401, "E.E401");
            return [ E.E400, E.E401]

        case 402:
AssertType(402, "int");

AssertType([ E.E402, E.E403], "E[]");
AssertType(E.E402, "E.E402");
AssertType(E.E403, "E.E403");
            return [ E.E402, E.E403]

        case 404:
AssertType(404, "int");

AssertType([ E.E404, E.E405], "E[]");
AssertType(E.E404, "E.E404");
AssertType(E.E405, "E.E405");
            return [ E.E404, E.E405]

        case 406:
AssertType(406, "int");

AssertType([ E.E406, E.E407], "E[]");
AssertType(E.E406, "E.E406");
AssertType(E.E407, "E.E407");
            return [ E.E406, E.E407]

        case 408:
AssertType(408, "int");

AssertType([ E.E408, E.E409], "E[]");
AssertType(E.E408, "E.E408");
AssertType(E.E409, "E.E409");
            return [ E.E408, E.E409]

        case 410:
AssertType(410, "int");

AssertType([ E.E410, E.E411], "E[]");
AssertType(E.E410, "E.E410");
AssertType(E.E411, "E.E411");
            return [ E.E410, E.E411]

        case 412:
AssertType(412, "int");

AssertType([ E.E412, E.E413], "E[]");
AssertType(E.E412, "E.E412");
AssertType(E.E413, "E.E413");
            return [ E.E412, E.E413]

        case 414:
AssertType(414, "int");

AssertType([ E.E414, E.E415], "E[]");
AssertType(E.E414, "E.E414");
AssertType(E.E415, "E.E415");
            return [ E.E414, E.E415]

        case 416:
AssertType(416, "int");

AssertType([ E.E416, E.E417], "E[]");
AssertType(E.E416, "E.E416");
AssertType(E.E417, "E.E417");
            return [ E.E416, E.E417]

        case 418:
AssertType(418, "int");

AssertType([ E.E418, E.E419], "E[]");
AssertType(E.E418, "E.E418");
AssertType(E.E419, "E.E419");
            return [ E.E418, E.E419]

        case 420:
AssertType(420, "int");

AssertType([ E.E420, E.E421], "E[]");
AssertType(E.E420, "E.E420");
AssertType(E.E421, "E.E421");
            return [ E.E420, E.E421]

        case 422:
AssertType(422, "int");

AssertType([ E.E422, E.E423], "E[]");
AssertType(E.E422, "E.E422");
AssertType(E.E423, "E.E423");
            return [ E.E422, E.E423]

        case 424:
AssertType(424, "int");

AssertType([ E.E424, E.E425], "E[]");
AssertType(E.E424, "E.E424");
AssertType(E.E425, "E.E425");
            return [ E.E424, E.E425]

        case 426:
AssertType(426, "int");

AssertType([ E.E426, E.E427], "E[]");
AssertType(E.E426, "E.E426");
AssertType(E.E427, "E.E427");
            return [ E.E426, E.E427]

        case 428:
AssertType(428, "int");

AssertType([ E.E428, E.E429], "E[]");
AssertType(E.E428, "E.E428");
AssertType(E.E429, "E.E429");
            return [ E.E428, E.E429]

        case 430:
AssertType(430, "int");

AssertType([ E.E430, E.E431], "E[]");
AssertType(E.E430, "E.E430");
AssertType(E.E431, "E.E431");
            return [ E.E430, E.E431]

        case 432:
AssertType(432, "int");

AssertType([ E.E432, E.E433], "E[]");
AssertType(E.E432, "E.E432");
AssertType(E.E433, "E.E433");
            return [ E.E432, E.E433]

        case 434:
AssertType(434, "int");

AssertType([ E.E434, E.E435], "E[]");
AssertType(E.E434, "E.E434");
AssertType(E.E435, "E.E435");
            return [ E.E434, E.E435]

        case 436:
AssertType(436, "int");

AssertType([ E.E436, E.E437], "E[]");
AssertType(E.E436, "E.E436");
AssertType(E.E437, "E.E437");
            return [ E.E436, E.E437]

        case 438:
AssertType(438, "int");

AssertType([ E.E438, E.E439], "E[]");
AssertType(E.E438, "E.E438");
AssertType(E.E439, "E.E439");
            return [ E.E438, E.E439]

        case 440:
AssertType(440, "int");

AssertType([ E.E440, E.E441], "E[]");
AssertType(E.E440, "E.E440");
AssertType(E.E441, "E.E441");
            return [ E.E440, E.E441]

        case 442:
AssertType(442, "int");

AssertType([ E.E442, E.E443], "E[]");
AssertType(E.E442, "E.E442");
AssertType(E.E443, "E.E443");
            return [ E.E442, E.E443]

        case 444:
AssertType(444, "int");

AssertType([ E.E444, E.E445], "E[]");
AssertType(E.E444, "E.E444");
AssertType(E.E445, "E.E445");
            return [ E.E444, E.E445]

        case 446:
AssertType(446, "int");

AssertType([ E.E446, E.E447], "E[]");
AssertType(E.E446, "E.E446");
AssertType(E.E447, "E.E447");
            return [ E.E446, E.E447]

        case 448:
AssertType(448, "int");

AssertType([ E.E448, E.E449], "E[]");
AssertType(E.E448, "E.E448");
AssertType(E.E449, "E.E449");
            return [ E.E448, E.E449]

        case 450:
AssertType(450, "int");

AssertType([ E.E450, E.E451], "E[]");
AssertType(E.E450, "E.E450");
AssertType(E.E451, "E.E451");
            return [ E.E450, E.E451]

        case 452:
AssertType(452, "int");

AssertType([ E.E452, E.E453], "E[]");
AssertType(E.E452, "E.E452");
AssertType(E.E453, "E.E453");
            return [ E.E452, E.E453]

        case 454:
AssertType(454, "int");

AssertType([ E.E454, E.E455], "E[]");
AssertType(E.E454, "E.E454");
AssertType(E.E455, "E.E455");
            return [ E.E454, E.E455]

        case 456:
AssertType(456, "int");

AssertType([ E.E456, E.E457], "E[]");
AssertType(E.E456, "E.E456");
AssertType(E.E457, "E.E457");
            return [ E.E456, E.E457]

        case 458:
AssertType(458, "int");

AssertType([ E.E458, E.E459], "E[]");
AssertType(E.E458, "E.E458");
AssertType(E.E459, "E.E459");
            return [ E.E458, E.E459]

        case 460:
AssertType(460, "int");

AssertType([ E.E460, E.E461], "E[]");
AssertType(E.E460, "E.E460");
AssertType(E.E461, "E.E461");
            return [ E.E460, E.E461]

        case 462:
AssertType(462, "int");

AssertType([ E.E462, E.E463], "E[]");
AssertType(E.E462, "E.E462");
AssertType(E.E463, "E.E463");
            return [ E.E462, E.E463]

        case 464:
AssertType(464, "int");

AssertType([ E.E464, E.E465], "E[]");
AssertType(E.E464, "E.E464");
AssertType(E.E465, "E.E465");
            return [ E.E464, E.E465]

        case 466:
AssertType(466, "int");

AssertType([ E.E466, E.E467], "E[]");
AssertType(E.E466, "E.E466");
AssertType(E.E467, "E.E467");
            return [ E.E466, E.E467]

        case 468:
AssertType(468, "int");

AssertType([ E.E468, E.E469], "E[]");
AssertType(E.E468, "E.E468");
AssertType(E.E469, "E.E469");
            return [ E.E468, E.E469]

        case 470:
AssertType(470, "int");

AssertType([ E.E470, E.E471], "E[]");
AssertType(E.E470, "E.E470");
AssertType(E.E471, "E.E471");
            return [ E.E470, E.E471]

        case 472:
AssertType(472, "int");

AssertType([ E.E472, E.E473], "E[]");
AssertType(E.E472, "E.E472");
AssertType(E.E473, "E.E473");
            return [ E.E472, E.E473]

        case 474:
AssertType(474, "int");

AssertType([ E.E474, E.E475], "E[]");
AssertType(E.E474, "E.E474");
AssertType(E.E475, "E.E475");
            return [ E.E474, E.E475]

        case 476:
AssertType(476, "int");

AssertType([ E.E476, E.E477], "E[]");
AssertType(E.E476, "E.E476");
AssertType(E.E477, "E.E477");
            return [ E.E476, E.E477]

        case 478:
AssertType(478, "int");

AssertType([ E.E478, E.E479], "E[]");
AssertType(E.E478, "E.E478");
AssertType(E.E479, "E.E479");
            return [ E.E478, E.E479]

        case 480:
AssertType(480, "int");

AssertType([ E.E480, E.E481], "E[]");
AssertType(E.E480, "E.E480");
AssertType(E.E481, "E.E481");
            return [ E.E480, E.E481]

        case 482:
AssertType(482, "int");

AssertType([ E.E482, E.E483], "E[]");
AssertType(E.E482, "E.E482");
AssertType(E.E483, "E.E483");
            return [ E.E482, E.E483]

        case 484:
AssertType(484, "int");

AssertType([ E.E484, E.E485], "E[]");
AssertType(E.E484, "E.E484");
AssertType(E.E485, "E.E485");
            return [ E.E484, E.E485]

        case 486:
AssertType(486, "int");

AssertType([ E.E486, E.E487], "E[]");
AssertType(E.E486, "E.E486");
AssertType(E.E487, "E.E487");
            return [ E.E486, E.E487]

        case 488:
AssertType(488, "int");

AssertType([ E.E488, E.E489], "E[]");
AssertType(E.E488, "E.E488");
AssertType(E.E489, "E.E489");
            return [ E.E488, E.E489]

        case 490:
AssertType(490, "int");

AssertType([ E.E490, E.E491], "E[]");
AssertType(E.E490, "E.E490");
AssertType(E.E491, "E.E491");
            return [ E.E490, E.E491]

        case 492:
AssertType(492, "int");

AssertType([ E.E492, E.E493], "E[]");
AssertType(E.E492, "E.E492");
AssertType(E.E493, "E.E493");
            return [ E.E492, E.E493]

        case 494:
AssertType(494, "int");

AssertType([ E.E494, E.E495], "E[]");
AssertType(E.E494, "E.E494");
AssertType(E.E495, "E.E495");
            return [ E.E494, E.E495]

        case 496:
AssertType(496, "int");

AssertType([ E.E496, E.E497], "E[]");
AssertType(E.E496, "E.E496");
AssertType(E.E497, "E.E497");
            return [ E.E496, E.E497]

        case 498:
AssertType(498, "int");

AssertType([ E.E498, E.E499], "E[]");
AssertType(E.E498, "E.E498");
AssertType(E.E499, "E.E499");
            return [ E.E498, E.E499]

        case 500:
AssertType(500, "int");

AssertType([ E.E500, E.E501], "E[]");
AssertType(E.E500, "E.E500");
AssertType(E.E501, "E.E501");
            return [ E.E500, E.E501]

        case 502:
AssertType(502, "int");

AssertType([ E.E502, E.E503], "E[]");
AssertType(E.E502, "E.E502");
AssertType(E.E503, "E.E503");
            return [ E.E502, E.E503]

        case 504:
AssertType(504, "int");

AssertType([ E.E504, E.E505], "E[]");
AssertType(E.E504, "E.E504");
AssertType(E.E505, "E.E505");
            return [ E.E504, E.E505]

        case 506:
AssertType(506, "int");

AssertType([ E.E506, E.E507], "E[]");
AssertType(E.E506, "E.E506");
AssertType(E.E507, "E.E507");
            return [ E.E506, E.E507]

        case 508:
AssertType(508, "int");

AssertType([ E.E508, E.E509], "E[]");
AssertType(E.E508, "E.E508");
AssertType(E.E509, "E.E509");
            return [ E.E508, E.E509]

        case 510:
AssertType(510, "int");

AssertType([ E.E510, E.E511], "E[]");
AssertType(E.E510, "E.E510");
AssertType(E.E511, "E.E511");
            return [ E.E510, E.E511]

        case 512:
AssertType(512, "int");

AssertType([ E.E512, E.E513], "E[]");
AssertType(E.E512, "E.E512");
AssertType(E.E513, "E.E513");
            return [ E.E512, E.E513]

        case 514:
AssertType(514, "int");

AssertType([ E.E514, E.E515], "E[]");
AssertType(E.E514, "E.E514");
AssertType(E.E515, "E.E515");
            return [ E.E514, E.E515]

        case 516:
AssertType(516, "int");

AssertType([ E.E516, E.E517], "E[]");
AssertType(E.E516, "E.E516");
AssertType(E.E517, "E.E517");
            return [ E.E516, E.E517]

        case 518:
AssertType(518, "int");

AssertType([ E.E518, E.E519], "E[]");
AssertType(E.E518, "E.E518");
AssertType(E.E519, "E.E519");
            return [ E.E518, E.E519]

        case 520:
AssertType(520, "int");

AssertType([ E.E520, E.E521], "E[]");
AssertType(E.E520, "E.E520");
AssertType(E.E521, "E.E521");
            return [ E.E520, E.E521]

        case 522:
AssertType(522, "int");

AssertType([ E.E522, E.E523], "E[]");
AssertType(E.E522, "E.E522");
AssertType(E.E523, "E.E523");
            return [ E.E522, E.E523]

        case 524:
AssertType(524, "int");

AssertType([ E.E524, E.E525], "E[]");
AssertType(E.E524, "E.E524");
AssertType(E.E525, "E.E525");
            return [ E.E524, E.E525]

        case 526:
AssertType(526, "int");

AssertType([ E.E526, E.E527], "E[]");
AssertType(E.E526, "E.E526");
AssertType(E.E527, "E.E527");
            return [ E.E526, E.E527]

        case 528:
AssertType(528, "int");

AssertType([ E.E528, E.E529], "E[]");
AssertType(E.E528, "E.E528");
AssertType(E.E529, "E.E529");
            return [ E.E528, E.E529]

        case 530:
AssertType(530, "int");

AssertType([ E.E530, E.E531], "E[]");
AssertType(E.E530, "E.E530");
AssertType(E.E531, "E.E531");
            return [ E.E530, E.E531]

        case 532:
AssertType(532, "int");

AssertType([ E.E532, E.E533], "E[]");
AssertType(E.E532, "E.E532");
AssertType(E.E533, "E.E533");
            return [ E.E532, E.E533]

        case 534:
AssertType(534, "int");

AssertType([ E.E534, E.E535], "E[]");
AssertType(E.E534, "E.E534");
AssertType(E.E535, "E.E535");
            return [ E.E534, E.E535]

        case 536:
AssertType(536, "int");

AssertType([ E.E536, E.E537], "E[]");
AssertType(E.E536, "E.E536");
AssertType(E.E537, "E.E537");
            return [ E.E536, E.E537]

        case 538:
AssertType(538, "int");

AssertType([ E.E538, E.E539], "E[]");
AssertType(E.E538, "E.E538");
AssertType(E.E539, "E.E539");
            return [ E.E538, E.E539]

        case 540:
AssertType(540, "int");

AssertType([ E.E540, E.E541], "E[]");
AssertType(E.E540, "E.E540");
AssertType(E.E541, "E.E541");
            return [ E.E540, E.E541]

        case 542:
AssertType(542, "int");

AssertType([ E.E542, E.E543], "E[]");
AssertType(E.E542, "E.E542");
AssertType(E.E543, "E.E543");
            return [ E.E542, E.E543]

        case 544:
AssertType(544, "int");

AssertType([ E.E544, E.E545], "E[]");
AssertType(E.E544, "E.E544");
AssertType(E.E545, "E.E545");
            return [ E.E544, E.E545]

        case 546:
AssertType(546, "int");

AssertType([ E.E546, E.E547], "E[]");
AssertType(E.E546, "E.E546");
AssertType(E.E547, "E.E547");
            return [ E.E546, E.E547]

        case 548:
AssertType(548, "int");

AssertType([ E.E548, E.E549], "E[]");
AssertType(E.E548, "E.E548");
AssertType(E.E549, "E.E549");
            return [ E.E548, E.E549]

        case 550:
AssertType(550, "int");

AssertType([ E.E550, E.E551], "E[]");
AssertType(E.E550, "E.E550");
AssertType(E.E551, "E.E551");
            return [ E.E550, E.E551]

        case 552:
AssertType(552, "int");

AssertType([ E.E552, E.E553], "E[]");
AssertType(E.E552, "E.E552");
AssertType(E.E553, "E.E553");
            return [ E.E552, E.E553]

        case 554:
AssertType(554, "int");

AssertType([ E.E554, E.E555], "E[]");
AssertType(E.E554, "E.E554");
AssertType(E.E555, "E.E555");
            return [ E.E554, E.E555]

        case 556:
AssertType(556, "int");

AssertType([ E.E556, E.E557], "E[]");
AssertType(E.E556, "E.E556");
AssertType(E.E557, "E.E557");
            return [ E.E556, E.E557]

        case 558:
AssertType(558, "int");

AssertType([ E.E558, E.E559], "E[]");
AssertType(E.E558, "E.E558");
AssertType(E.E559, "E.E559");
            return [ E.E558, E.E559]

        case 560:
AssertType(560, "int");

AssertType([ E.E560, E.E561], "E[]");
AssertType(E.E560, "E.E560");
AssertType(E.E561, "E.E561");
            return [ E.E560, E.E561]

        case 562:
AssertType(562, "int");

AssertType([ E.E562, E.E563], "E[]");
AssertType(E.E562, "E.E562");
AssertType(E.E563, "E.E563");
            return [ E.E562, E.E563]

        case 564:
AssertType(564, "int");

AssertType([ E.E564, E.E565], "E[]");
AssertType(E.E564, "E.E564");
AssertType(E.E565, "E.E565");
            return [ E.E564, E.E565]

        case 566:
AssertType(566, "int");

AssertType([ E.E566, E.E567], "E[]");
AssertType(E.E566, "E.E566");
AssertType(E.E567, "E.E567");
            return [ E.E566, E.E567]

        case 568:
AssertType(568, "int");

AssertType([ E.E568, E.E569], "E[]");
AssertType(E.E568, "E.E568");
AssertType(E.E569, "E.E569");
            return [ E.E568, E.E569]

        case 570:
AssertType(570, "int");

AssertType([ E.E570, E.E571], "E[]");
AssertType(E.E570, "E.E570");
AssertType(E.E571, "E.E571");
            return [ E.E570, E.E571]

        case 572:
AssertType(572, "int");

AssertType([ E.E572, E.E573], "E[]");
AssertType(E.E572, "E.E572");
AssertType(E.E573, "E.E573");
            return [ E.E572, E.E573]

        case 574:
AssertType(574, "int");

AssertType([ E.E574, E.E575], "E[]");
AssertType(E.E574, "E.E574");
AssertType(E.E575, "E.E575");
            return [ E.E574, E.E575]

        case 576:
AssertType(576, "int");

AssertType([ E.E576, E.E577], "E[]");
AssertType(E.E576, "E.E576");
AssertType(E.E577, "E.E577");
            return [ E.E576, E.E577]

        case 578:
AssertType(578, "int");

AssertType([ E.E578, E.E579], "E[]");
AssertType(E.E578, "E.E578");
AssertType(E.E579, "E.E579");
            return [ E.E578, E.E579]

        case 580:
AssertType(580, "int");

AssertType([ E.E580, E.E581], "E[]");
AssertType(E.E580, "E.E580");
AssertType(E.E581, "E.E581");
            return [ E.E580, E.E581]

        case 582:
AssertType(582, "int");

AssertType([ E.E582, E.E583], "E[]");
AssertType(E.E582, "E.E582");
AssertType(E.E583, "E.E583");
            return [ E.E582, E.E583]

        case 584:
AssertType(584, "int");

AssertType([ E.E584, E.E585], "E[]");
AssertType(E.E584, "E.E584");
AssertType(E.E585, "E.E585");
            return [ E.E584, E.E585]

        case 586:
AssertType(586, "int");

AssertType([ E.E586, E.E587], "E[]");
AssertType(E.E586, "E.E586");
AssertType(E.E587, "E.E587");
            return [ E.E586, E.E587]

        case 588:
AssertType(588, "int");

AssertType([ E.E588, E.E589], "E[]");
AssertType(E.E588, "E.E588");
AssertType(E.E589, "E.E589");
            return [ E.E588, E.E589]

        case 590:
AssertType(590, "int");

AssertType([ E.E590, E.E591], "E[]");
AssertType(E.E590, "E.E590");
AssertType(E.E591, "E.E591");
            return [ E.E590, E.E591]

        case 592:
AssertType(592, "int");

AssertType([ E.E592, E.E593], "E[]");
AssertType(E.E592, "E.E592");
AssertType(E.E593, "E.E593");
            return [ E.E592, E.E593]

        case 594:
AssertType(594, "int");

AssertType([ E.E594, E.E595], "E[]");
AssertType(E.E594, "E.E594");
AssertType(E.E595, "E.E595");
            return [ E.E594, E.E595]

        case 596:
AssertType(596, "int");

AssertType([ E.E596, E.E597], "E[]");
AssertType(E.E596, "E.E596");
AssertType(E.E597, "E.E597");
            return [ E.E596, E.E597]

        case 598:
AssertType(598, "int");

AssertType([ E.E598, E.E599], "E[]");
AssertType(E.E598, "E.E598");
AssertType(E.E599, "E.E599");
            return [ E.E598, E.E599]

        case 600:
AssertType(600, "int");

AssertType([ E.E600, E.E601], "E[]");
AssertType(E.E600, "E.E600");
AssertType(E.E601, "E.E601");
            return [ E.E600, E.E601]

        case 602:
AssertType(602, "int");

AssertType([ E.E602, E.E603], "E[]");
AssertType(E.E602, "E.E602");
AssertType(E.E603, "E.E603");
            return [ E.E602, E.E603]

        case 604:
AssertType(604, "int");

AssertType([ E.E604, E.E605], "E[]");
AssertType(E.E604, "E.E604");
AssertType(E.E605, "E.E605");
            return [ E.E604, E.E605]

        case 606:
AssertType(606, "int");

AssertType([ E.E606, E.E607], "E[]");
AssertType(E.E606, "E.E606");
AssertType(E.E607, "E.E607");
            return [ E.E606, E.E607]

        case 608:
AssertType(608, "int");

AssertType([ E.E608, E.E609], "E[]");
AssertType(E.E608, "E.E608");
AssertType(E.E609, "E.E609");
            return [ E.E608, E.E609]

        case 610:
AssertType(610, "int");

AssertType([ E.E610, E.E611], "E[]");
AssertType(E.E610, "E.E610");
AssertType(E.E611, "E.E611");
            return [ E.E610, E.E611]

        case 612:
AssertType(612, "int");

AssertType([ E.E612, E.E613], "E[]");
AssertType(E.E612, "E.E612");
AssertType(E.E613, "E.E613");
            return [ E.E612, E.E613]

        case 614:
AssertType(614, "int");

AssertType([ E.E614, E.E615], "E[]");
AssertType(E.E614, "E.E614");
AssertType(E.E615, "E.E615");
            return [ E.E614, E.E615]

        case 616:
AssertType(616, "int");

AssertType([ E.E616, E.E617], "E[]");
AssertType(E.E616, "E.E616");
AssertType(E.E617, "E.E617");
            return [ E.E616, E.E617]

        case 618:
AssertType(618, "int");

AssertType([ E.E618, E.E619], "E[]");
AssertType(E.E618, "E.E618");
AssertType(E.E619, "E.E619");
            return [ E.E618, E.E619]

        case 620:
AssertType(620, "int");

AssertType([ E.E620, E.E621], "E[]");
AssertType(E.E620, "E.E620");
AssertType(E.E621, "E.E621");
            return [ E.E620, E.E621]

        case 622:
AssertType(622, "int");

AssertType([ E.E622, E.E623], "E[]");
AssertType(E.E622, "E.E622");
AssertType(E.E623, "E.E623");
            return [ E.E622, E.E623]

        case 624:
AssertType(624, "int");

AssertType([ E.E624, E.E625], "E[]");
AssertType(E.E624, "E.E624");
AssertType(E.E625, "E.E625");
            return [ E.E624, E.E625]

        case 626:
AssertType(626, "int");

AssertType([ E.E626, E.E627], "E[]");
AssertType(E.E626, "E.E626");
AssertType(E.E627, "E.E627");
            return [ E.E626, E.E627]

        case 628:
AssertType(628, "int");

AssertType([ E.E628, E.E629], "E[]");
AssertType(E.E628, "E.E628");
AssertType(E.E629, "E.E629");
            return [ E.E628, E.E629]

        case 630:
AssertType(630, "int");

AssertType([ E.E630, E.E631], "E[]");
AssertType(E.E630, "E.E630");
AssertType(E.E631, "E.E631");
            return [ E.E630, E.E631]

        case 632:
AssertType(632, "int");

AssertType([ E.E632, E.E633], "E[]");
AssertType(E.E632, "E.E632");
AssertType(E.E633, "E.E633");
            return [ E.E632, E.E633]

        case 634:
AssertType(634, "int");

AssertType([ E.E634, E.E635], "E[]");
AssertType(E.E634, "E.E634");
AssertType(E.E635, "E.E635");
            return [ E.E634, E.E635]

        case 636:
AssertType(636, "int");

AssertType([ E.E636, E.E637], "E[]");
AssertType(E.E636, "E.E636");
AssertType(E.E637, "E.E637");
            return [ E.E636, E.E637]

        case 638:
AssertType(638, "int");

AssertType([ E.E638, E.E639], "E[]");
AssertType(E.E638, "E.E638");
AssertType(E.E639, "E.E639");
            return [ E.E638, E.E639]

        case 640:
AssertType(640, "int");

AssertType([ E.E640, E.E641], "E[]");
AssertType(E.E640, "E.E640");
AssertType(E.E641, "E.E641");
            return [ E.E640, E.E641]

        case 642:
AssertType(642, "int");

AssertType([ E.E642, E.E643], "E[]");
AssertType(E.E642, "E.E642");
AssertType(E.E643, "E.E643");
            return [ E.E642, E.E643]

        case 644:
AssertType(644, "int");

AssertType([ E.E644, E.E645], "E[]");
AssertType(E.E644, "E.E644");
AssertType(E.E645, "E.E645");
            return [ E.E644, E.E645]

        case 646:
AssertType(646, "int");

AssertType([ E.E646, E.E647], "E[]");
AssertType(E.E646, "E.E646");
AssertType(E.E647, "E.E647");
            return [ E.E646, E.E647]

        case 648:
AssertType(648, "int");

AssertType([ E.E648, E.E649], "E[]");
AssertType(E.E648, "E.E648");
AssertType(E.E649, "E.E649");
            return [ E.E648, E.E649]

        case 650:
AssertType(650, "int");

AssertType([ E.E650, E.E651], "E[]");
AssertType(E.E650, "E.E650");
AssertType(E.E651, "E.E651");
            return [ E.E650, E.E651]

        case 652:
AssertType(652, "int");

AssertType([ E.E652, E.E653], "E[]");
AssertType(E.E652, "E.E652");
AssertType(E.E653, "E.E653");
            return [ E.E652, E.E653]

        case 654:
AssertType(654, "int");

AssertType([ E.E654, E.E655], "E[]");
AssertType(E.E654, "E.E654");
AssertType(E.E655, "E.E655");
            return [ E.E654, E.E655]

        case 656:
AssertType(656, "int");

AssertType([ E.E656, E.E657], "E[]");
AssertType(E.E656, "E.E656");
AssertType(E.E657, "E.E657");
            return [ E.E656, E.E657]

        case 658:
AssertType(658, "int");

AssertType([ E.E658, E.E659], "E[]");
AssertType(E.E658, "E.E658");
AssertType(E.E659, "E.E659");
            return [ E.E658, E.E659]

        case 660:
AssertType(660, "int");

AssertType([ E.E660, E.E661], "E[]");
AssertType(E.E660, "E.E660");
AssertType(E.E661, "E.E661");
            return [ E.E660, E.E661]

        case 662:
AssertType(662, "int");

AssertType([ E.E662, E.E663], "E[]");
AssertType(E.E662, "E.E662");
AssertType(E.E663, "E.E663");
            return [ E.E662, E.E663]

        case 664:
AssertType(664, "int");

AssertType([ E.E664, E.E665], "E[]");
AssertType(E.E664, "E.E664");
AssertType(E.E665, "E.E665");
            return [ E.E664, E.E665]

        case 666:
AssertType(666, "int");

AssertType([ E.E666, E.E667], "E[]");
AssertType(E.E666, "E.E666");
AssertType(E.E667, "E.E667");
            return [ E.E666, E.E667]

        case 668:
AssertType(668, "int");

AssertType([ E.E668, E.E669], "E[]");
AssertType(E.E668, "E.E668");
AssertType(E.E669, "E.E669");
            return [ E.E668, E.E669]

        case 670:
AssertType(670, "int");

AssertType([ E.E670, E.E671], "E[]");
AssertType(E.E670, "E.E670");
AssertType(E.E671, "E.E671");
            return [ E.E670, E.E671]

        case 672:
AssertType(672, "int");

AssertType([ E.E672, E.E673], "E[]");
AssertType(E.E672, "E.E672");
AssertType(E.E673, "E.E673");
            return [ E.E672, E.E673]

        case 674:
AssertType(674, "int");

AssertType([ E.E674, E.E675], "E[]");
AssertType(E.E674, "E.E674");
AssertType(E.E675, "E.E675");
            return [ E.E674, E.E675]

        case 676:
AssertType(676, "int");

AssertType([ E.E676, E.E677], "E[]");
AssertType(E.E676, "E.E676");
AssertType(E.E677, "E.E677");
            return [ E.E676, E.E677]

        case 678:
AssertType(678, "int");

AssertType([ E.E678, E.E679], "E[]");
AssertType(E.E678, "E.E678");
AssertType(E.E679, "E.E679");
            return [ E.E678, E.E679]

        case 680:
AssertType(680, "int");

AssertType([ E.E680, E.E681], "E[]");
AssertType(E.E680, "E.E680");
AssertType(E.E681, "E.E681");
            return [ E.E680, E.E681]

        case 682:
AssertType(682, "int");

AssertType([ E.E682, E.E683], "E[]");
AssertType(E.E682, "E.E682");
AssertType(E.E683, "E.E683");
            return [ E.E682, E.E683]

        case 684:
AssertType(684, "int");

AssertType([ E.E684, E.E685], "E[]");
AssertType(E.E684, "E.E684");
AssertType(E.E685, "E.E685");
            return [ E.E684, E.E685]

        case 686:
AssertType(686, "int");

AssertType([ E.E686, E.E687], "E[]");
AssertType(E.E686, "E.E686");
AssertType(E.E687, "E.E687");
            return [ E.E686, E.E687]

        case 688:
AssertType(688, "int");

AssertType([ E.E688, E.E689], "E[]");
AssertType(E.E688, "E.E688");
AssertType(E.E689, "E.E689");
            return [ E.E688, E.E689]

        case 690:
AssertType(690, "int");

AssertType([ E.E690, E.E691], "E[]");
AssertType(E.E690, "E.E690");
AssertType(E.E691, "E.E691");
            return [ E.E690, E.E691]

        case 692:
AssertType(692, "int");

AssertType([ E.E692, E.E693], "E[]");
AssertType(E.E692, "E.E692");
AssertType(E.E693, "E.E693");
            return [ E.E692, E.E693]

        case 694:
AssertType(694, "int");

AssertType([ E.E694, E.E695], "E[]");
AssertType(E.E694, "E.E694");
AssertType(E.E695, "E.E695");
            return [ E.E694, E.E695]

        case 696:
AssertType(696, "int");

AssertType([ E.E696, E.E697], "E[]");
AssertType(E.E696, "E.E696");
AssertType(E.E697, "E.E697");
            return [ E.E696, E.E697]

        case 698:
AssertType(698, "int");

AssertType([ E.E698, E.E699], "E[]");
AssertType(E.E698, "E.E698");
AssertType(E.E699, "E.E699");
            return [ E.E698, E.E699]

        case 700:
AssertType(700, "int");

AssertType([ E.E700, E.E701], "E[]");
AssertType(E.E700, "E.E700");
AssertType(E.E701, "E.E701");
            return [ E.E700, E.E701]

        case 702:
AssertType(702, "int");

AssertType([ E.E702, E.E703], "E[]");
AssertType(E.E702, "E.E702");
AssertType(E.E703, "E.E703");
            return [ E.E702, E.E703]

        case 704:
AssertType(704, "int");

AssertType([ E.E704, E.E705], "E[]");
AssertType(E.E704, "E.E704");
AssertType(E.E705, "E.E705");
            return [ E.E704, E.E705]

        case 706:
AssertType(706, "int");

AssertType([ E.E706, E.E707], "E[]");
AssertType(E.E706, "E.E706");
AssertType(E.E707, "E.E707");
            return [ E.E706, E.E707]

        case 708:
AssertType(708, "int");

AssertType([ E.E708, E.E709], "E[]");
AssertType(E.E708, "E.E708");
AssertType(E.E709, "E.E709");
            return [ E.E708, E.E709]

        case 710:
AssertType(710, "int");

AssertType([ E.E710, E.E711], "E[]");
AssertType(E.E710, "E.E710");
AssertType(E.E711, "E.E711");
            return [ E.E710, E.E711]

        case 712:
AssertType(712, "int");

AssertType([ E.E712, E.E713], "E[]");
AssertType(E.E712, "E.E712");
AssertType(E.E713, "E.E713");
            return [ E.E712, E.E713]

        case 714:
AssertType(714, "int");

AssertType([ E.E714, E.E715], "E[]");
AssertType(E.E714, "E.E714");
AssertType(E.E715, "E.E715");
            return [ E.E714, E.E715]

        case 716:
AssertType(716, "int");

AssertType([ E.E716, E.E717], "E[]");
AssertType(E.E716, "E.E716");
AssertType(E.E717, "E.E717");
            return [ E.E716, E.E717]

        case 718:
AssertType(718, "int");

AssertType([ E.E718, E.E719], "E[]");
AssertType(E.E718, "E.E718");
AssertType(E.E719, "E.E719");
            return [ E.E718, E.E719]

        case 720:
AssertType(720, "int");

AssertType([ E.E720, E.E721], "E[]");
AssertType(E.E720, "E.E720");
AssertType(E.E721, "E.E721");
            return [ E.E720, E.E721]

        case 722:
AssertType(722, "int");

AssertType([ E.E722, E.E723], "E[]");
AssertType(E.E722, "E.E722");
AssertType(E.E723, "E.E723");
            return [ E.E722, E.E723]

        case 724:
AssertType(724, "int");

AssertType([ E.E724, E.E725], "E[]");
AssertType(E.E724, "E.E724");
AssertType(E.E725, "E.E725");
            return [ E.E724, E.E725]

        case 726:
AssertType(726, "int");

AssertType([ E.E726, E.E727], "E[]");
AssertType(E.E726, "E.E726");
AssertType(E.E727, "E.E727");
            return [ E.E726, E.E727]

        case 728:
AssertType(728, "int");

AssertType([ E.E728, E.E729], "E[]");
AssertType(E.E728, "E.E728");
AssertType(E.E729, "E.E729");
            return [ E.E728, E.E729]

        case 730:
AssertType(730, "int");

AssertType([ E.E730, E.E731], "E[]");
AssertType(E.E730, "E.E730");
AssertType(E.E731, "E.E731");
            return [ E.E730, E.E731]

        case 732:
AssertType(732, "int");

AssertType([ E.E732, E.E733], "E[]");
AssertType(E.E732, "E.E732");
AssertType(E.E733, "E.E733");
            return [ E.E732, E.E733]

        case 734:
AssertType(734, "int");

AssertType([ E.E734, E.E735], "E[]");
AssertType(E.E734, "E.E734");
AssertType(E.E735, "E.E735");
            return [ E.E734, E.E735]

        case 736:
AssertType(736, "int");

AssertType([ E.E736, E.E737], "E[]");
AssertType(E.E736, "E.E736");
AssertType(E.E737, "E.E737");
            return [ E.E736, E.E737]

        case 738:
AssertType(738, "int");

AssertType([ E.E738, E.E739], "E[]");
AssertType(E.E738, "E.E738");
AssertType(E.E739, "E.E739");
            return [ E.E738, E.E739]

        case 740:
AssertType(740, "int");

AssertType([ E.E740, E.E741], "E[]");
AssertType(E.E740, "E.E740");
AssertType(E.E741, "E.E741");
            return [ E.E740, E.E741]

        case 742:
AssertType(742, "int");

AssertType([ E.E742, E.E743], "E[]");
AssertType(E.E742, "E.E742");
AssertType(E.E743, "E.E743");
            return [ E.E742, E.E743]

        case 744:
AssertType(744, "int");

AssertType([ E.E744, E.E745], "E[]");
AssertType(E.E744, "E.E744");
AssertType(E.E745, "E.E745");
            return [ E.E744, E.E745]

        case 746:
AssertType(746, "int");

AssertType([ E.E746, E.E747], "E[]");
AssertType(E.E746, "E.E746");
AssertType(E.E747, "E.E747");
            return [ E.E746, E.E747]

        case 748:
AssertType(748, "int");

AssertType([ E.E748, E.E749], "E[]");
AssertType(E.E748, "E.E748");
AssertType(E.E749, "E.E749");
            return [ E.E748, E.E749]

        case 750:
AssertType(750, "int");

AssertType([ E.E750, E.E751], "E[]");
AssertType(E.E750, "E.E750");
AssertType(E.E751, "E.E751");
            return [ E.E750, E.E751]

        case 752:
AssertType(752, "int");

AssertType([ E.E752, E.E753], "E[]");
AssertType(E.E752, "E.E752");
AssertType(E.E753, "E.E753");
            return [ E.E752, E.E753]

        case 754:
AssertType(754, "int");

AssertType([ E.E754, E.E755], "E[]");
AssertType(E.E754, "E.E754");
AssertType(E.E755, "E.E755");
            return [ E.E754, E.E755]

        case 756:
AssertType(756, "int");

AssertType([ E.E756, E.E757], "E[]");
AssertType(E.E756, "E.E756");
AssertType(E.E757, "E.E757");
            return [ E.E756, E.E757]

        case 758:
AssertType(758, "int");

AssertType([ E.E758, E.E759], "E[]");
AssertType(E.E758, "E.E758");
AssertType(E.E759, "E.E759");
            return [ E.E758, E.E759]

        case 760:
AssertType(760, "int");

AssertType([ E.E760, E.E761], "E[]");
AssertType(E.E760, "E.E760");
AssertType(E.E761, "E.E761");
            return [ E.E760, E.E761]

        case 762:
AssertType(762, "int");

AssertType([ E.E762, E.E763], "E[]");
AssertType(E.E762, "E.E762");
AssertType(E.E763, "E.E763");
            return [ E.E762, E.E763]

        case 764:
AssertType(764, "int");

AssertType([ E.E764, E.E765], "E[]");
AssertType(E.E764, "E.E764");
AssertType(E.E765, "E.E765");
            return [ E.E764, E.E765]

        case 766:
AssertType(766, "int");

AssertType([ E.E766, E.E767], "E[]");
AssertType(E.E766, "E.E766");
AssertType(E.E767, "E.E767");
            return [ E.E766, E.E767]

        case 768:
AssertType(768, "int");

AssertType([ E.E768, E.E769], "E[]");
AssertType(E.E768, "E.E768");
AssertType(E.E769, "E.E769");
            return [ E.E768, E.E769]

        case 770:
AssertType(770, "int");

AssertType([ E.E770, E.E771], "E[]");
AssertType(E.E770, "E.E770");
AssertType(E.E771, "E.E771");
            return [ E.E770, E.E771]

        case 772:
AssertType(772, "int");

AssertType([ E.E772, E.E773], "E[]");
AssertType(E.E772, "E.E772");
AssertType(E.E773, "E.E773");
            return [ E.E772, E.E773]

        case 774:
AssertType(774, "int");

AssertType([ E.E774, E.E775], "E[]");
AssertType(E.E774, "E.E774");
AssertType(E.E775, "E.E775");
            return [ E.E774, E.E775]

        case 776:
AssertType(776, "int");

AssertType([ E.E776, E.E777], "E[]");
AssertType(E.E776, "E.E776");
AssertType(E.E777, "E.E777");
            return [ E.E776, E.E777]

        case 778:
AssertType(778, "int");

AssertType([ E.E778, E.E779], "E[]");
AssertType(E.E778, "E.E778");
AssertType(E.E779, "E.E779");
            return [ E.E778, E.E779]

        case 780:
AssertType(780, "int");

AssertType([ E.E780, E.E781], "E[]");
AssertType(E.E780, "E.E780");
AssertType(E.E781, "E.E781");
            return [ E.E780, E.E781]

        case 782:
AssertType(782, "int");

AssertType([ E.E782, E.E783], "E[]");
AssertType(E.E782, "E.E782");
AssertType(E.E783, "E.E783");
            return [ E.E782, E.E783]

        case 784:
AssertType(784, "int");

AssertType([ E.E784, E.E785], "E[]");
AssertType(E.E784, "E.E784");
AssertType(E.E785, "E.E785");
            return [ E.E784, E.E785]

        case 786:
AssertType(786, "int");

AssertType([ E.E786, E.E787], "E[]");
AssertType(E.E786, "E.E786");
AssertType(E.E787, "E.E787");
            return [ E.E786, E.E787]

        case 788:
AssertType(788, "int");

AssertType([ E.E788, E.E789], "E[]");
AssertType(E.E788, "E.E788");
AssertType(E.E789, "E.E789");
            return [ E.E788, E.E789]

        case 790:
AssertType(790, "int");

AssertType([ E.E790, E.E791], "E[]");
AssertType(E.E790, "E.E790");
AssertType(E.E791, "E.E791");
            return [ E.E790, E.E791]

        case 792:
AssertType(792, "int");

AssertType([ E.E792, E.E793], "E[]");
AssertType(E.E792, "E.E792");
AssertType(E.E793, "E.E793");
            return [ E.E792, E.E793]

        case 794:
AssertType(794, "int");

AssertType([ E.E794, E.E795], "E[]");
AssertType(E.E794, "E.E794");
AssertType(E.E795, "E.E795");
            return [ E.E794, E.E795]

        case 796:
AssertType(796, "int");

AssertType([ E.E796, E.E797], "E[]");
AssertType(E.E796, "E.E796");
AssertType(E.E797, "E.E797");
            return [ E.E796, E.E797]

        case 798:
AssertType(798, "int");

AssertType([ E.E798, E.E799], "E[]");
AssertType(E.E798, "E.E798");
AssertType(E.E799, "E.E799");
            return [ E.E798, E.E799]

        case 800:
AssertType(800, "int");

AssertType([ E.E800, E.E801], "E[]");
AssertType(E.E800, "E.E800");
AssertType(E.E801, "E.E801");
            return [ E.E800, E.E801]

        case 802:
AssertType(802, "int");

AssertType([ E.E802, E.E803], "E[]");
AssertType(E.E802, "E.E802");
AssertType(E.E803, "E.E803");
            return [ E.E802, E.E803]

        case 804:
AssertType(804, "int");

AssertType([ E.E804, E.E805], "E[]");
AssertType(E.E804, "E.E804");
AssertType(E.E805, "E.E805");
            return [ E.E804, E.E805]

        case 806:
AssertType(806, "int");

AssertType([ E.E806, E.E807], "E[]");
AssertType(E.E806, "E.E806");
AssertType(E.E807, "E.E807");
            return [ E.E806, E.E807]

        case 808:
AssertType(808, "int");

AssertType([ E.E808, E.E809], "E[]");
AssertType(E.E808, "E.E808");
AssertType(E.E809, "E.E809");
            return [ E.E808, E.E809]

        case 810:
AssertType(810, "int");

AssertType([ E.E810, E.E811], "E[]");
AssertType(E.E810, "E.E810");
AssertType(E.E811, "E.E811");
            return [ E.E810, E.E811]

        case 812:
AssertType(812, "int");

AssertType([ E.E812, E.E813], "E[]");
AssertType(E.E812, "E.E812");
AssertType(E.E813, "E.E813");
            return [ E.E812, E.E813]

        case 814:
AssertType(814, "int");

AssertType([ E.E814, E.E815], "E[]");
AssertType(E.E814, "E.E814");
AssertType(E.E815, "E.E815");
            return [ E.E814, E.E815]

        case 816:
AssertType(816, "int");

AssertType([ E.E816, E.E817], "E[]");
AssertType(E.E816, "E.E816");
AssertType(E.E817, "E.E817");
            return [ E.E816, E.E817]

        case 818:
AssertType(818, "int");

AssertType([ E.E818, E.E819], "E[]");
AssertType(E.E818, "E.E818");
AssertType(E.E819, "E.E819");
            return [ E.E818, E.E819]

        case 820:
AssertType(820, "int");

AssertType([ E.E820, E.E821], "E[]");
AssertType(E.E820, "E.E820");
AssertType(E.E821, "E.E821");
            return [ E.E820, E.E821]

        case 822:
AssertType(822, "int");

AssertType([ E.E822, E.E823], "E[]");
AssertType(E.E822, "E.E822");
AssertType(E.E823, "E.E823");
            return [ E.E822, E.E823]

        case 824:
AssertType(824, "int");

AssertType([ E.E824, E.E825], "E[]");
AssertType(E.E824, "E.E824");
AssertType(E.E825, "E.E825");
            return [ E.E824, E.E825]

        case 826:
AssertType(826, "int");

AssertType([ E.E826, E.E827], "E[]");
AssertType(E.E826, "E.E826");
AssertType(E.E827, "E.E827");
            return [ E.E826, E.E827]

        case 828:
AssertType(828, "int");

AssertType([ E.E828, E.E829], "E[]");
AssertType(E.E828, "E.E828");
AssertType(E.E829, "E.E829");
            return [ E.E828, E.E829]

        case 830:
AssertType(830, "int");

AssertType([ E.E830, E.E831], "E[]");
AssertType(E.E830, "E.E830");
AssertType(E.E831, "E.E831");
            return [ E.E830, E.E831]

        case 832:
AssertType(832, "int");

AssertType([ E.E832, E.E833], "E[]");
AssertType(E.E832, "E.E832");
AssertType(E.E833, "E.E833");
            return [ E.E832, E.E833]

        case 834:
AssertType(834, "int");

AssertType([ E.E834, E.E835], "E[]");
AssertType(E.E834, "E.E834");
AssertType(E.E835, "E.E835");
            return [ E.E834, E.E835]

        case 836:
AssertType(836, "int");

AssertType([ E.E836, E.E837], "E[]");
AssertType(E.E836, "E.E836");
AssertType(E.E837, "E.E837");
            return [ E.E836, E.E837]

        case 838:
AssertType(838, "int");

AssertType([ E.E838, E.E839], "E[]");
AssertType(E.E838, "E.E838");
AssertType(E.E839, "E.E839");
            return [ E.E838, E.E839]

        case 840:
AssertType(840, "int");

AssertType([ E.E840, E.E841], "E[]");
AssertType(E.E840, "E.E840");
AssertType(E.E841, "E.E841");
            return [ E.E840, E.E841]

        case 842:
AssertType(842, "int");

AssertType([ E.E842, E.E843], "E[]");
AssertType(E.E842, "E.E842");
AssertType(E.E843, "E.E843");
            return [ E.E842, E.E843]

        case 844:
AssertType(844, "int");

AssertType([ E.E844, E.E845], "E[]");
AssertType(E.E844, "E.E844");
AssertType(E.E845, "E.E845");
            return [ E.E844, E.E845]

        case 846:
AssertType(846, "int");

AssertType([ E.E846, E.E847], "E[]");
AssertType(E.E846, "E.E846");
AssertType(E.E847, "E.E847");
            return [ E.E846, E.E847]

        case 848:
AssertType(848, "int");

AssertType([ E.E848, E.E849], "E[]");
AssertType(E.E848, "E.E848");
AssertType(E.E849, "E.E849");
            return [ E.E848, E.E849]

        case 850:
AssertType(850, "int");

AssertType([ E.E850, E.E851], "E[]");
AssertType(E.E850, "E.E850");
AssertType(E.E851, "E.E851");
            return [ E.E850, E.E851]

        case 852:
AssertType(852, "int");

AssertType([ E.E852, E.E853], "E[]");
AssertType(E.E852, "E.E852");
AssertType(E.E853, "E.E853");
            return [ E.E852, E.E853]

        case 854:
AssertType(854, "int");

AssertType([ E.E854, E.E855], "E[]");
AssertType(E.E854, "E.E854");
AssertType(E.E855, "E.E855");
            return [ E.E854, E.E855]

        case 856:
AssertType(856, "int");

AssertType([ E.E856, E.E857], "E[]");
AssertType(E.E856, "E.E856");
AssertType(E.E857, "E.E857");
            return [ E.E856, E.E857]

        case 858:
AssertType(858, "int");

AssertType([ E.E858, E.E859], "E[]");
AssertType(E.E858, "E.E858");
AssertType(E.E859, "E.E859");
            return [ E.E858, E.E859]

        case 860:
AssertType(860, "int");

AssertType([ E.E860, E.E861], "E[]");
AssertType(E.E860, "E.E860");
AssertType(E.E861, "E.E861");
            return [ E.E860, E.E861]

        case 862:
AssertType(862, "int");

AssertType([ E.E862, E.E863], "E[]");
AssertType(E.E862, "E.E862");
AssertType(E.E863, "E.E863");
            return [ E.E862, E.E863]

        case 864:
AssertType(864, "int");

AssertType([ E.E864, E.E865], "E[]");
AssertType(E.E864, "E.E864");
AssertType(E.E865, "E.E865");
            return [ E.E864, E.E865]

        case 866:
AssertType(866, "int");

AssertType([ E.E866, E.E867], "E[]");
AssertType(E.E866, "E.E866");
AssertType(E.E867, "E.E867");
            return [ E.E866, E.E867]

        case 868:
AssertType(868, "int");

AssertType([ E.E868, E.E869], "E[]");
AssertType(E.E868, "E.E868");
AssertType(E.E869, "E.E869");
            return [ E.E868, E.E869]

        case 870:
AssertType(870, "int");

AssertType([ E.E870, E.E871], "E[]");
AssertType(E.E870, "E.E870");
AssertType(E.E871, "E.E871");
            return [ E.E870, E.E871]

        case 872:
AssertType(872, "int");

AssertType([ E.E872, E.E873], "E[]");
AssertType(E.E872, "E.E872");
AssertType(E.E873, "E.E873");
            return [ E.E872, E.E873]

        case 874:
AssertType(874, "int");

AssertType([ E.E874, E.E875], "E[]");
AssertType(E.E874, "E.E874");
AssertType(E.E875, "E.E875");
            return [ E.E874, E.E875]

        case 876:
AssertType(876, "int");

AssertType([ E.E876, E.E877], "E[]");
AssertType(E.E876, "E.E876");
AssertType(E.E877, "E.E877");
            return [ E.E876, E.E877]

        case 878:
AssertType(878, "int");

AssertType([ E.E878, E.E879], "E[]");
AssertType(E.E878, "E.E878");
AssertType(E.E879, "E.E879");
            return [ E.E878, E.E879]

        case 880:
AssertType(880, "int");

AssertType([ E.E880, E.E881], "E[]");
AssertType(E.E880, "E.E880");
AssertType(E.E881, "E.E881");
            return [ E.E880, E.E881]

        case 882:
AssertType(882, "int");

AssertType([ E.E882, E.E883], "E[]");
AssertType(E.E882, "E.E882");
AssertType(E.E883, "E.E883");
            return [ E.E882, E.E883]

        case 884:
AssertType(884, "int");

AssertType([ E.E884, E.E885], "E[]");
AssertType(E.E884, "E.E884");
AssertType(E.E885, "E.E885");
            return [ E.E884, E.E885]

        case 886:
AssertType(886, "int");

AssertType([ E.E886, E.E887], "E[]");
AssertType(E.E886, "E.E886");
AssertType(E.E887, "E.E887");
            return [ E.E886, E.E887]

        case 888:
AssertType(888, "int");

AssertType([ E.E888, E.E889], "E[]");
AssertType(E.E888, "E.E888");
AssertType(E.E889, "E.E889");
            return [ E.E888, E.E889]

        case 890:
AssertType(890, "int");

AssertType([ E.E890, E.E891], "E[]");
AssertType(E.E890, "E.E890");
AssertType(E.E891, "E.E891");
            return [ E.E890, E.E891]

        case 892:
AssertType(892, "int");

AssertType([ E.E892, E.E893], "E[]");
AssertType(E.E892, "E.E892");
AssertType(E.E893, "E.E893");
            return [ E.E892, E.E893]

        case 894:
AssertType(894, "int");

AssertType([ E.E894, E.E895], "E[]");
AssertType(E.E894, "E.E894");
AssertType(E.E895, "E.E895");
            return [ E.E894, E.E895]

        case 896:
AssertType(896, "int");

AssertType([ E.E896, E.E897], "E[]");
AssertType(E.E896, "E.E896");
AssertType(E.E897, "E.E897");
            return [ E.E896, E.E897]

        case 898:
AssertType(898, "int");

AssertType([ E.E898, E.E899], "E[]");
AssertType(E.E898, "E.E898");
AssertType(E.E899, "E.E899");
            return [ E.E898, E.E899]

        case 900:
AssertType(900, "int");

AssertType([ E.E900, E.E901], "E[]");
AssertType(E.E900, "E.E900");
AssertType(E.E901, "E.E901");
            return [ E.E900, E.E901]

        case 902:
AssertType(902, "int");

AssertType([ E.E902, E.E903], "E[]");
AssertType(E.E902, "E.E902");
AssertType(E.E903, "E.E903");
            return [ E.E902, E.E903]

        case 904:
AssertType(904, "int");

AssertType([ E.E904, E.E905], "E[]");
AssertType(E.E904, "E.E904");
AssertType(E.E905, "E.E905");
            return [ E.E904, E.E905]

        case 906:
AssertType(906, "int");

AssertType([ E.E906, E.E907], "E[]");
AssertType(E.E906, "E.E906");
AssertType(E.E907, "E.E907");
            return [ E.E906, E.E907]

        case 908:
AssertType(908, "int");

AssertType([ E.E908, E.E909], "E[]");
AssertType(E.E908, "E.E908");
AssertType(E.E909, "E.E909");
            return [ E.E908, E.E909]

        case 910:
AssertType(910, "int");

AssertType([ E.E910, E.E911], "E[]");
AssertType(E.E910, "E.E910");
AssertType(E.E911, "E.E911");
            return [ E.E910, E.E911]

        case 912:
AssertType(912, "int");

AssertType([ E.E912, E.E913], "E[]");
AssertType(E.E912, "E.E912");
AssertType(E.E913, "E.E913");
            return [ E.E912, E.E913]

        case 914:
AssertType(914, "int");

AssertType([ E.E914, E.E915], "E[]");
AssertType(E.E914, "E.E914");
AssertType(E.E915, "E.E915");
            return [ E.E914, E.E915]

        case 916:
AssertType(916, "int");

AssertType([ E.E916, E.E917], "E[]");
AssertType(E.E916, "E.E916");
AssertType(E.E917, "E.E917");
            return [ E.E916, E.E917]

        case 918:
AssertType(918, "int");

AssertType([ E.E918, E.E919], "E[]");
AssertType(E.E918, "E.E918");
AssertType(E.E919, "E.E919");
            return [ E.E918, E.E919]

        case 920:
AssertType(920, "int");

AssertType([ E.E920, E.E921], "E[]");
AssertType(E.E920, "E.E920");
AssertType(E.E921, "E.E921");
            return [ E.E920, E.E921]

        case 922:
AssertType(922, "int");

AssertType([ E.E922, E.E923], "E[]");
AssertType(E.E922, "E.E922");
AssertType(E.E923, "E.E923");
            return [ E.E922, E.E923]

        case 924:
AssertType(924, "int");

AssertType([ E.E924, E.E925], "E[]");
AssertType(E.E924, "E.E924");
AssertType(E.E925, "E.E925");
            return [ E.E924, E.E925]

        case 926:
AssertType(926, "int");

AssertType([ E.E926, E.E927], "E[]");
AssertType(E.E926, "E.E926");
AssertType(E.E927, "E.E927");
            return [ E.E926, E.E927]

        case 928:
AssertType(928, "int");

AssertType([ E.E928, E.E929], "E[]");
AssertType(E.E928, "E.E928");
AssertType(E.E929, "E.E929");
            return [ E.E928, E.E929]

        case 930:
AssertType(930, "int");

AssertType([ E.E930, E.E931], "E[]");
AssertType(E.E930, "E.E930");
AssertType(E.E931, "E.E931");
            return [ E.E930, E.E931]

        case 932:
AssertType(932, "int");

AssertType([ E.E932, E.E933], "E[]");
AssertType(E.E932, "E.E932");
AssertType(E.E933, "E.E933");
            return [ E.E932, E.E933]

        case 934:
AssertType(934, "int");

AssertType([ E.E934, E.E935], "E[]");
AssertType(E.E934, "E.E934");
AssertType(E.E935, "E.E935");
            return [ E.E934, E.E935]

        case 936:
AssertType(936, "int");

AssertType([ E.E936, E.E937], "E[]");
AssertType(E.E936, "E.E936");
AssertType(E.E937, "E.E937");
            return [ E.E936, E.E937]

        case 938:
AssertType(938, "int");

AssertType([ E.E938, E.E939], "E[]");
AssertType(E.E938, "E.E938");
AssertType(E.E939, "E.E939");
            return [ E.E938, E.E939]

        case 940:
AssertType(940, "int");

AssertType([ E.E940, E.E941], "E[]");
AssertType(E.E940, "E.E940");
AssertType(E.E941, "E.E941");
            return [ E.E940, E.E941]

        case 942:
AssertType(942, "int");

AssertType([ E.E942, E.E943], "E[]");
AssertType(E.E942, "E.E942");
AssertType(E.E943, "E.E943");
            return [ E.E942, E.E943]

        case 944:
AssertType(944, "int");

AssertType([ E.E944, E.E945], "E[]");
AssertType(E.E944, "E.E944");
AssertType(E.E945, "E.E945");
            return [ E.E944, E.E945]

        case 946:
AssertType(946, "int");

AssertType([ E.E946, E.E947], "E[]");
AssertType(E.E946, "E.E946");
AssertType(E.E947, "E.E947");
            return [ E.E946, E.E947]

        case 948:
AssertType(948, "int");

AssertType([ E.E948, E.E949], "E[]");
AssertType(E.E948, "E.E948");
AssertType(E.E949, "E.E949");
            return [ E.E948, E.E949]

        case 950:
AssertType(950, "int");

AssertType([ E.E950, E.E951], "E[]");
AssertType(E.E950, "E.E950");
AssertType(E.E951, "E.E951");
            return [ E.E950, E.E951]

        case 952:
AssertType(952, "int");

AssertType([ E.E952, E.E953], "E[]");
AssertType(E.E952, "E.E952");
AssertType(E.E953, "E.E953");
            return [ E.E952, E.E953]

        case 954:
AssertType(954, "int");

AssertType([ E.E954, E.E955], "E[]");
AssertType(E.E954, "E.E954");
AssertType(E.E955, "E.E955");
            return [ E.E954, E.E955]

        case 956:
AssertType(956, "int");

AssertType([ E.E956, E.E957], "E[]");
AssertType(E.E956, "E.E956");
AssertType(E.E957, "E.E957");
            return [ E.E956, E.E957]

        case 958:
AssertType(958, "int");

AssertType([ E.E958, E.E959], "E[]");
AssertType(E.E958, "E.E958");
AssertType(E.E959, "E.E959");
            return [ E.E958, E.E959]

        case 960:
AssertType(960, "int");

AssertType([ E.E960, E.E961], "E[]");
AssertType(E.E960, "E.E960");
AssertType(E.E961, "E.E961");
            return [ E.E960, E.E961]

        case 962:
AssertType(962, "int");

AssertType([ E.E962, E.E963], "E[]");
AssertType(E.E962, "E.E962");
AssertType(E.E963, "E.E963");
            return [ E.E962, E.E963]

        case 964:
AssertType(964, "int");

AssertType([ E.E964, E.E965], "E[]");
AssertType(E.E964, "E.E964");
AssertType(E.E965, "E.E965");
            return [ E.E964, E.E965]

        case 966:
AssertType(966, "int");

AssertType([ E.E966, E.E967], "E[]");
AssertType(E.E966, "E.E966");
AssertType(E.E967, "E.E967");
            return [ E.E966, E.E967]

        case 968:
AssertType(968, "int");

AssertType([ E.E968, E.E969], "E[]");
AssertType(E.E968, "E.E968");
AssertType(E.E969, "E.E969");
            return [ E.E968, E.E969]

        case 970:
AssertType(970, "int");

AssertType([ E.E970, E.E971], "E[]");
AssertType(E.E970, "E.E970");
AssertType(E.E971, "E.E971");
            return [ E.E970, E.E971]

        case 972:
AssertType(972, "int");

AssertType([ E.E972, E.E973], "E[]");
AssertType(E.E972, "E.E972");
AssertType(E.E973, "E.E973");
            return [ E.E972, E.E973]

        case 974:
AssertType(974, "int");

AssertType([ E.E974, E.E975], "E[]");
AssertType(E.E974, "E.E974");
AssertType(E.E975, "E.E975");
            return [ E.E974, E.E975]

        case 976:
AssertType(976, "int");

AssertType([ E.E976, E.E977], "E[]");
AssertType(E.E976, "E.E976");
AssertType(E.E977, "E.E977");
            return [ E.E976, E.E977]

        case 978:
AssertType(978, "int");

AssertType([ E.E978, E.E979], "E[]");
AssertType(E.E978, "E.E978");
AssertType(E.E979, "E.E979");
            return [ E.E978, E.E979]

        case 980:
AssertType(980, "int");

AssertType([ E.E980, E.E981], "E[]");
AssertType(E.E980, "E.E980");
AssertType(E.E981, "E.E981");
            return [ E.E980, E.E981]

        case 982:
AssertType(982, "int");

AssertType([ E.E982, E.E983], "E[]");
AssertType(E.E982, "E.E982");
AssertType(E.E983, "E.E983");
            return [ E.E982, E.E983]

        case 984:
AssertType(984, "int");

AssertType([ E.E984, E.E985], "E[]");
AssertType(E.E984, "E.E984");
AssertType(E.E985, "E.E985");
            return [ E.E984, E.E985]

        case 986:
AssertType(986, "int");

AssertType([ E.E986, E.E987], "E[]");
AssertType(E.E986, "E.E986");
AssertType(E.E987, "E.E987");
            return [ E.E986, E.E987]

        case 988:
AssertType(988, "int");

AssertType([ E.E988, E.E989], "E[]");
AssertType(E.E988, "E.E988");
AssertType(E.E989, "E.E989");
            return [ E.E988, E.E989]

        case 990:
AssertType(990, "int");

AssertType([ E.E990, E.E991], "E[]");
AssertType(E.E990, "E.E990");
AssertType(E.E991, "E.E991");
            return [ E.E990, E.E991]

        case 992:
AssertType(992, "int");

AssertType([ E.E992, E.E993], "E[]");
AssertType(E.E992, "E.E992");
AssertType(E.E993, "E.E993");
            return [ E.E992, E.E993]

        case 994:
AssertType(994, "int");

AssertType([ E.E994, E.E995], "E[]");
AssertType(E.E994, "E.E994");
AssertType(E.E995, "E.E995");
            return [ E.E994, E.E995]

        case 996:
AssertType(996, "int");

AssertType([ E.E996, E.E997], "E[]");
AssertType(E.E996, "E.E996");
AssertType(E.E997, "E.E997");
            return [ E.E996, E.E997]

        case 998:
AssertType(998, "int");

AssertType([ E.E998, E.E999], "E[]");
AssertType(E.E998, "E.E998");
AssertType(E.E999, "E.E999");
            return [ E.E998, E.E999]

        case 1000:
AssertType(1000, "int");

AssertType([ E.E1000, E.E1001], "E[]");
AssertType(E.E1000, "E.E1000");
AssertType(E.E1001, "E.E1001");
            return [ E.E1000, E.E1001]

        case 1002:
AssertType(1002, "int");

AssertType([ E.E1002, E.E1003], "E[]");
AssertType(E.E1002, "E.E1002");
AssertType(E.E1003, "E.E1003");
            return [ E.E1002, E.E1003]

        case 1004:
AssertType(1004, "int");

AssertType([ E.E1004, E.E1005], "E[]");
AssertType(E.E1004, "E.E1004");
AssertType(E.E1005, "E.E1005");
            return [ E.E1004, E.E1005]

        case 1006:
AssertType(1006, "int");

AssertType([ E.E1006, E.E1007], "E[]");
AssertType(E.E1006, "E.E1006");
AssertType(E.E1007, "E.E1007");
            return [ E.E1006, E.E1007]

        case 1008:
AssertType(1008, "int");

AssertType([ E.E1008, E.E1009], "E[]");
AssertType(E.E1008, "E.E1008");
AssertType(E.E1009, "E.E1009");
            return [ E.E1008, E.E1009]

        case 1010:
AssertType(1010, "int");

AssertType([ E.E1010, E.E1011], "E[]");
AssertType(E.E1010, "E.E1010");
AssertType(E.E1011, "E.E1011");
            return [ E.E1010, E.E1011]

        case 1012:
AssertType(1012, "int");

AssertType([ E.E1012, E.E1013], "E[]");
AssertType(E.E1012, "E.E1012");
AssertType(E.E1013, "E.E1013");
            return [ E.E1012, E.E1013]

        case 1014:
AssertType(1014, "int");

AssertType([ E.E1014, E.E1015], "E[]");
AssertType(E.E1014, "E.E1014");
AssertType(E.E1015, "E.E1015");
            return [ E.E1014, E.E1015]

        case 1016:
AssertType(1016, "int");

AssertType([ E.E1016, E.E1017], "E[]");
AssertType(E.E1016, "E.E1016");
AssertType(E.E1017, "E.E1017");
            return [ E.E1016, E.E1017]

        case 1018:
AssertType(1018, "int");

AssertType([ E.E1018, E.E1019], "E[]");
AssertType(E.E1018, "E.E1018");
AssertType(E.E1019, "E.E1019");
            return [ E.E1018, E.E1019]

        case 1020:
AssertType(1020, "int");

AssertType([ E.E1020, E.E1021], "E[]");
AssertType(E.E1020, "E.E1020");
AssertType(E.E1021, "E.E1021");
            return [ E.E1020, E.E1021]

        case 1022:
AssertType(1022, "int");

AssertType([ E.E1022, E.E1023], "E[]");
AssertType(E.E1022, "E.E1022");
AssertType(E.E1023, "E.E1023");
            return [ E.E1022, E.E1023]
    }
}


