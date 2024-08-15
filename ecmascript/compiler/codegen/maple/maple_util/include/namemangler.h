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

#ifndef NAMEMANGLER_H
#define NAMEMANGLER_H
#include <string>
#include <vector>

// This is a general name mangler which is shared between maple compiler and runtime.
// maple-compiler-specific data structure may NOT be used here.
namespace namemangler {
#define TO_STR(s) TO_STR2(s)
#define TO_STR2(s) #s

#define VTAB_PREFIX __vtb_
#define ITAB_PREFIX __itb_
#define VTAB_AND_ITAB_PREFIX __vtb_and_itb_
#define ITAB_CONFLICT_PREFIX __itbC_
#define CLASSINFO_PREFIX __cinf_
#define CLASSINFO_RO_PREFIX __classinforo__
#define SUPERCLASSINFO_PREFIX __superclasses__
#define PRIMITIVECLASSINFO_PREFIX __pinf_
#define CLASS_INIT_BRIDGE_PREFIX __ClassInitBridge__
#define GCTIB_PREFIX MCC_GCTIB__
#define REF_PREFIX REF_
#define JARRAY_PREFIX A

#define VTAB_PREFIX_STR TO_STR(VTAB_PREFIX)
#define ITAB_PREFIX_STR TO_STR(ITAB_PREFIX)
#define VTAB_AND_ITAB_PREFIX_STR TO_STR(VTAB_AND_ITAB_PREFIX)
#define ITAB_CONFLICT_PREFIX_STR TO_STR(ITAB_CONFLICT_PREFIX)
#define CLASSINFO_PREFIX_STR TO_STR(CLASSINFO_PREFIX)
#define CLASSINFO_RO_PREFIX_STR TO_STR(CLASSINFO_RO_PREFIX)
#define SUPERCLASSINFO_PREFIX_STR TO_STR(SUPERCLASSINFO_PREFIX)
#define PRIMITIVECLASSINFO_PREFIX_STR TO_STR(PRIMITIVECLASSINFO_PREFIX)
#define CLASS_INIT_BRIDGE_PREFIX_STR TO_STR(CLASS_INIT_BRIDGE_PREFIX)
#define GCTIB_PREFIX_STR TO_STR(GCTIB_PREFIX)
#define REF_PREFIX_STR TO_STR(REF_PREFIX)
#define JARRAY_PREFIX_STR TO_STR(JARRAY_PREFIX)

// Names of all compiler-generated tables and accessed by runtime
static constexpr const char kMuidPrefixStr[] = "__muid_";
static constexpr const char kMuidRoPrefixStr[] = "__muid_ro";
static constexpr const char kMuidFuncDefTabPrefixStr[] = "__muid_func_def_tab";
static constexpr const char kMuidFuncDefOrigTabPrefixStr[] = "__muid_ro_func_def_orig_tab";
static constexpr const char kMuidFuncInfTabPrefixStr[] = "__muid_ro_func_inf_tab";
static constexpr const char kMuidFuncMuidIdxTabPrefixStr[] = "__muid_ro_func_muid_idx_tab";
static constexpr const char kMuidDataDefTabPrefixStr[] = "__muid_data_def_tab";
static constexpr const char kMuidDataDefOrigTabPrefixStr[] = "__muid_ro_data_def_orig_tab";
static constexpr const char kMuidFuncUndefTabPrefixStr[] = "__muid_func_undef_tab";
static constexpr const char kMuidDataUndefTabPrefixStr[] = "__muid_data_undef_tab";
static constexpr const char kMuidFuncDefMuidTabPrefixStr[] = "__muid_ro_func_def_muid_tab";
static constexpr const char kMuidDataDefMuidTabPrefixStr[] = "__muid_ro_data_def_muid_tab";
static constexpr const char kMuidFuncUndefMuidTabPrefixStr[] = "__muid_ro_func_undef_muid_tab";
static constexpr const char kMuidDataUndefMuidTabPrefixStr[] = "__muid_ro_data_undef_muid_tab";
static constexpr const char kMuidVtabAndItabPrefixStr[] = "__muid_vtab_and_itab";
static constexpr const char kMuidItabConflictPrefixStr[] = "__muid_itab_conflict";
static constexpr const char kMuidColdVtabAndItabPrefixStr[] = "__muid_cold_vtab_and_itab";
static constexpr const char kMuidColdItabConflictPrefixStr[] = "__muid_cold_itab_conflict";
static constexpr const char kMuidVtabOffsetPrefixStr[] = "__muid_vtab_offset_tab";
static constexpr const char kMuidFieldOffsetPrefixStr[] = "__muid_field_offset_tab";
static constexpr const char kMuidVtabOffsetKeyPrefixStr[] = "__muid_vtable_offset_key_tab";
static constexpr const char kMuidFieldOffsetKeyPrefixStr[] = "__muid_field_offset_key_tab";
static constexpr const char kMuidValueOffsetPrefixStr[] = "__muid_offset_value_table";
static constexpr const char kMuidLocalClassInfoStr[] = "__muid_local_classinfo_tab";
static constexpr const char kMuidSuperclassPrefixStr[] = "__muid_superclass";
static constexpr const char kMuidGlobalRootlistPrefixStr[] = "__muid_globalrootlist";
static constexpr const char kMuidClassMetadataPrefixStr[] = "__muid_classmetadata";
static constexpr const char kMuidClassMetadataBucketPrefixStr[] = "__muid_classmetadata_bucket";
static constexpr const char kMuidDataSectionStr[] = "__data_section";
static constexpr const char kMuidRangeTabPrefixStr[] = "__muid_range_tab";
static constexpr const char kMuidConststrPrefixStr[] = "__muid_conststr";
static constexpr const char kVtabOffsetTabStr[] = "__vtable_offset_table";
static constexpr const char kFieldOffsetTabKeyStr[] = "__field_offset_key_table";
static constexpr const char kFieldOffsetTabStr[] = "__field_offset_table";
static constexpr const char kVtableKeyOffsetTabStr[] = "__vtable_offset_key_table";
static constexpr const char kVtableOffsetTabKeyStr[] = "__vtable_offset_key_table";
static constexpr const char kFieldKeyOffsetTabStr[] = "__field_offset_table";
static constexpr const char kOffsetTabStr[] = "__offset_value_table";
static constexpr const char kInlineCacheTabStr[] = "__inline_cache_table";
static constexpr const char kLocalClassInfoStr[] = "__local_classinfo_table";
static constexpr const char kMethodsInfoPrefixStr[] = "__methods_info__";
static constexpr const char kMethodsInfoCompactPrefixStr[] = "__methods_infocompact__";
static constexpr const char kFieldsInfoPrefixStr[] = "__fields_info__";
static constexpr const char kFieldsInfoCompactPrefixStr[] = "__fields_infocompact__";
static constexpr const char kFieldOffsetDataPrefixStr[] = "__fieldOffsetData__";
static constexpr const char kMethodAddrDataPrefixStr[] = "__methodAddrData__";
static constexpr const char kMethodSignaturePrefixStr[] = "__methodSignature__";
static constexpr const char kParameterTypesPrefixStr[] = "__parameterTypes__";
static constexpr const char kRegJNITabPrefixStr[] = "__reg_jni_tab";
static constexpr const char kRegJNIFuncTabPrefixStr[] = "__reg_jni_func_tab";
static constexpr const char kReflectionStrtabPrefixStr[] = "__reflection_strtab";
static constexpr const char kReflectionStartHotStrtabPrefixStr[] = "__reflection_start_hot_strtab";
static constexpr const char kReflectionBothHotStrTabPrefixStr[] = "__reflection_both_hot_strtab";
static constexpr const char kReflectionRunHotStrtabPrefixStr[] = "__reflection_run_hot_strtab";
static constexpr const char kReflectionNoEmitStrtabPrefixStr[] = "__reflection_no_emit_strtab";
static constexpr const char kMarkMuidFuncDefStr[] = "muid_func_def:";
static constexpr const char kMarkMuidFuncUndefStr[] = "muid_func_undef:";
static constexpr const char kGcRootList[] = "gcRootNewList";
static constexpr const char kDecoupleOption[] = "__decouple_option";
static constexpr const char kDecoupleStr[] = "__decouple";
static constexpr const char kCompilerVersionNum[] = "__compilerVersionNum";
static constexpr const char kCompilerVersionNumStr[] = "__compilerVersionNumTab";
static constexpr const char kCompilerMfileStatus[] = "__compiler_mfile_status";
static constexpr const char kMapleGlobalVariable[] = "maple_global_variable";
static constexpr const char kMapleLiteralString[] = "maple_literal_string";

static constexpr const char kSourceMuid[] = "__sourceMuid";
static constexpr const char kSourceMuidSectionStr[] = "__sourceMuidTab";
static constexpr const char kDecoupleStaticKeyStr[] = "__staticDecoupleKeyOffset";
static constexpr const char kDecoupleStaticValueStr[] = "__staticDecoupleValueOffset";
static constexpr const char kMarkDecoupleStaticStr[] = "decouple_static:";
static constexpr const char kClassInfoPrefix[] = "__cinf";
static constexpr const char kBssSectionStr[] = "__bss_section";
static constexpr const char kLinkerHashSoStr[] = "__linkerHashSo";

static constexpr const char kStaticFieldNamePrefixStr[] = "__static_field_name";
static constexpr const char kMplSuffix[] = ".mpl";
static constexpr const char kClinvocation[] = ".clinvocation";
static constexpr const char kPackageNameSplitterStr[] = "_2F";
static constexpr const char kFileNameSplitterStr[] = "$$";
static constexpr const char kNameSplitterStr[] = "_7C";  // 7C is the ascii code for |
static constexpr const char kRightBracketStr[] = "_29";  // 29 is the ascii code for )
static constexpr const char kClassNameSplitterStr[] = "_3B_7C";
static constexpr const char kClassMetadataTypeName[] = "__class_meta__";
static constexpr const char kPtrPrefixStr[] = "_PTR";
static constexpr const char kClassINfoPtrPrefixStr[] = "_PTR__cinf_";
static constexpr const char kArrayClassInfoPrefixStr[] = "__cinf_A";
static constexpr const char kShadowClassName[] = "shadow_24__klass__";
static constexpr const char kClinitSuffix[] = "_7C_3Cclinit_3E_7C_28_29V";
static constexpr const char kCinitStr[] = "_7C_3Cinit_3E_7C_28";
static constexpr const char kClinitSubStr[] = "7C_3Cinit_3E_7C";

static constexpr const char kPreNativeFunc[] = "MCC_PreNativeCall";
static constexpr const char kPostNativeFunc[] = "MCC_PostNativeCall";
static constexpr const char kDecodeRefFunc[] = "MCC_DecodeReference";
static constexpr const char kFindNativeFunc[] = "MCC_FindNativeMethodPtr";
static constexpr const char kFindNativeFuncNoeh[] = "MCC_FindNativeMethodPtrWithoutException";
static constexpr const char kDummyNativeFunc[] = "MCC_DummyNativeMethodPtr";
static constexpr const char kCheckThrowPendingExceptionFunc[] = "MCC_CheckThrowPendingException";
static constexpr const char kCallFastNative[] = "MCC_CallFastNative";
static constexpr const char kCallFastNativeExt[] = "MCC_CallFastNativeExt";
static constexpr const char kCallSlowNativeExt[] = "MCC_CallSlowNativeExt";
static constexpr const char kSetReliableUnwindContextFunc[] = "MCC_SetReliableUnwindContext";

static constexpr const char kArrayClassCacheTable[] = "__arrayClassCacheTable";
static constexpr const char kArrayClassCacheNameTable[] = "__muid_ro_arrayClassCacheNameTable";
static constexpr const char kFunctionLayoutStr[] = "__func_layout__";

static constexpr const char kFunctionProfileTabPrefixStr[] = "__muid_profile_func_tab";

static constexpr const char kBBProfileTabPrefixStr[] = "__muid_prof_counter_tab";
static constexpr const char kFuncIRProfInfTabPrefixStr[] = "__muid_prof_ir_desc_tab";

static constexpr const char kprefixProfModDesc[] = "__mpl_prof_moddesc_";
static constexpr const char kprefixProfCtrTbl[] = "__mpl_prof_ctrtbl_";
static constexpr const char kprefixProfFuncDesc[] = "__mpl_prof_funcdesc_";
static constexpr const char kprefixProfFuncDescTbl[] = "__mpl_func_prof_desc_tbl_";
static constexpr const char kprefixProfInit[] = "__mpl_prof_init_";
static constexpr const char kprefixProfExit[] = "__mpl_prof_exit_";
static constexpr const char kGCCProfInit[] = "__gcov_init";
static constexpr const char kGCCProfExit[] = "__gcov_exit";
static constexpr const char kMplMergeFuncAdd[] = "__gcov_merge_add";
static constexpr const char kProfFileNameExt[] = ".gcda";

static constexpr const char kBindingProtectedRegionStr[] = "__BindingProtectRegion__";

static constexpr const char kClassNamePrefixStr[] = "L";
static constexpr const char kClassMethodSplitterStr[] = "_3B";
static constexpr const char kFuncGetCurrentCl[] = "MCC_GetCurrentClassLoader";

#ifdef ARK_LITECG_DEBUG
std::string DecodeName(const std::string &name);
void DecodeMapleNameToJDescriptor(const std::string &nameIn, std::string &nameOut);

__attribute__((visibility("default"))) unsigned UTF16ToUTF8(std::string &str, const std::u16string &str16,
                                                            unsigned short num = 0, bool isBigEndian = false);
size_t GetUleb128Size(uint64_t val);
size_t GetSleb128Size(int32_t val);
#endif
}  // namespace namemangler

#endif
