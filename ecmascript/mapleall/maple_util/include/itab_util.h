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

#ifndef MRT_ITAB_HOT_METHOD_H
#define MRT_ITAB_HOT_METHOD_H

namespace maple {
constexpr int kHashSize = 23;
constexpr int kHeadSizeOfSecondHash = 3;
constexpr int kFlagAgInHeadOfSecondHash = 1;
constexpr unsigned int kItabSecondHashSize = 0x1fff;
constexpr int kFlagFirstHashConflict = 0;
constexpr int kFlagSecondHashConflict = 1;
#define ITAB_HOTMETHOD_SIZE (sizeof(itabHotMethod) / sizeof(itabHotMethod[0]))

constexpr int kItabFirstHashSize = kHashSize;

unsigned int DJBHash(const char *str);
unsigned int GetHashIndex(const char *name);
unsigned int GetSecondHashIndex(const char *name);

static const char *itabHotMethod[] = {
    "hasNext_7C_28_29Z",
    "next_7C_28_29Ljava_2Flang_2FObject_3B",
    "asBinder_7C_28_29Landroid_2Fos_2FIBinder_3B",
    "gettid_7C_28_29I",
    "get_7C_28I_29Ljava_2Flang_2FObject_3B",
    "size_7C_28_29I",
    "next_7C_28_29I",
    "get_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B",
    "length_7C_28_29I",
    "isUidPrivileged_7C_28I_29Z",
    "sendBehavior_7C_28Lhuawei_2Fandroid_2Fsecurity_2FIHwBehaviorCollectManager_24BehaviorId_3BALjava_2Flang_2FObject_"
    "3B_29V",
    "sendBehavior_7C_28Lhuawei_2Fandroid_2Fsecurity_2FIHwBehaviorCollectManager_24BehaviorId_3B_29V",
    "iterator_7C_28_29Ljava_2Futil_2FIterator_3B",
    "attribute_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Lorg_2Fxmlpull_2Fv1_"
    "2FXmlSerializer_3B",
    "getAttributeValue_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Ljava_2Flang_2FString_3B",
    "index_7C_28_29I",
    "toString_7C_28_29Ljava_2Flang_2FString_3B",
    "print_7C_28Ljava_2Flang_2FObject_3BLjava_2Futil_2FLocale_3B_29V",
    "append_7C_28Ljava_2Flang_2FCharSequence_3B_29Ljava_2Flang_2FAppendable_3B",
    "add_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "containsKey_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "onReadFromDisk_7C_28_29V",
    "apply_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "getDepth_7C_28_29I",
    "accept_7C_28Ljava_2Flang_2FObject_3B_29V",
    "getName_7C_28_29Ljava_2Flang_2FString_3B",
    "writeToParcel_7C_28Landroid_2Fos_2FParcel_3BI_29V",
    "getuid_7C_28_29I",
    "charAt_7C_28I_29C",
    "read_7C_28Ljava_2Fio_2FFileDescriptor_3BABII_29I",
    "test_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "contains_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "isEmpty_7C_28_29Z",
    "createFromParcel_7C_28Landroid_2Fos_2FParcel_3B_29Ljava_2Flang_2FObject_3B",
    "toJavaFormatString_7C_28_29Ljava_2Flang_2FString_3B",
    "endTag_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Lorg_2Fxmlpull_2Fv1_2FXmlSerializer_3B",
    "getHwHandler_7C_28_29Landroid_2Fos_2FIHwHandler_3B",
    "startTag_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Lorg_2Fxmlpull_2Fv1_2FXmlSerializer_3B",
    "resetMessageDelayMillis_7C_28Landroid_2Fos_2FMessage_3BJ_29J",
    "close_7C_28Ljava_2Fio_2FFileDescriptor_3B_29V",
    "iterator_7C_28_29Ljava_2Futil_2FIterator_3B",
    "transact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z",
    "getValue_7C_28_29Ljava_2Flang_2FObject_3B",
    "checkPermission_7C_28Ljava_2Flang_2FString_3BII_29I",
    "getKey_7C_28_29Ljava_2Flang_2FObject_3B",
    "compare_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29I",
    "getAttrs_7C_28_29Landroid_2Fview_2FWindowManager_24LayoutParams_3B",
    "run_7C_28_29V",
    "getpid_7C_28_29I",
    "isEmpty_7C_28_29Z",
    "checkUidPermission_7C_28Ljava_2Flang_2FString_3BI_29I",
    "close_7C_28Ljava_2Flang_2FThrowable_3B_29V",
    "queryLocalInterface_7C_28Ljava_2Flang_2FString_3B_29Landroid_2Fos_2FIInterface_3B",
    "getService_7C_28Landroid_2Fapp_2FContextImpl_3B_29Ljava_2Flang_2FObject_3B",
    "getInt_7C_28I_29I",
    "stat_7C_28Ljava_2Flang_2FString_3B_29Landroid_2Fsystem_2FStructStat_3B",
    "getHwConfiguration_7C_28_29Landroid_2Fcontent_2Fres_2FIHwConfiguration_3B",
    "process_7C_28Lcom_2Fandroid_2Fserver_2Fjob_2Fcontrollers_2FJobStatus_3B_29V",
    "put_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B",
    "getWinFreezeScreenMonitor_7C_28_29Lcom_2Fandroid_2Fserver_2Fos_2FIFreezeScreenWindowMonitor_3B",
    "setTo_7C_28Landroid_2Fcontent_2Fres_2FIHwConfiguration_3B_29V",
    "getWindowLayerFromTypeLw_7C_28IZ_29I",
    "getString_7C_28I_29Ljava_2Flang_2FString_3B",
    "isVisibleLw_7C_28_29Z",
    "toArray_7C_28_29ALjava_2Flang_2FObject_3B",
    "getPolicyMask_7C_28_29I",
    "close_7C_28_29V",
    "compareTo_7C_28Landroid_2Fcontent_2Fres_2FIHwConfiguration_3B_29I",
    "setToDefaults_7C_28_29V",
    "getEventType_7C_28_29I",
    "contains_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "remove_7C_28I_29Ljava_2Flang_2FObject_3B",
    "checkIntent_7C_28ILjava_2Flang_2FString_3BIILjava_2Flang_2FString_3BI_29Z",
    "getStackId_7C_28_29I",
    "hashCode_7C_28_29I",
    "applyPostLayoutPolicyLw_7C_28Landroid_2Fview_2FWindowManagerPolicy_24WindowState_3BLandroid_2Fview_"
    "2FWindowManager_24LayoutParams_3BLandroid_2Fview_2FWindowManagerPolicy_24WindowState_3BLandroid_2Fview_"
    "2FWindowManagerPolicy_24WindowState_3B_29V",
    "getHwFlogManager_7C_28_29Landroid_2Fcommon_2FHwFlogManager_3B",
    "canAffectSystemUiFlags_7C_28_29Z",
    "isKeyguardLocked_7C_28_29Z",
    "slog_7C_28IILjava_2Flang_2FString_3B_29I",
    "values_7C_28_29Ljava_2Futil_2FCollection_3B",
    "checkFreezeScreen_7C_28Landroid_2Futil_2FArrayMap_3B_29V",
    "moveToNext_7C_28_29Z",
    "getResolution_7C_28Ljava_2Flang_2FString_3B_29F",
    "resolveContentProvider_7C_28Ljava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_2FProviderInfo_3B",
    "uptimeMillis_7C_28_29J",
    "getApplicationInfo_7C_28Ljava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_2FApplicationInfo_3B",
    "canAddInternalSystemWindow_7C_28_29Z",
    "append_7C_28Ljava_2Flang_2FCharSequence_3B_29Ljava_2Flang_2FAppendable_3B",
    "getBaseType_7C_28_29I",
    "onWriteToDisk_7C_28_29V",
    "isCustedCouldStopped_7C_28Ljava_2Flang_2FString_3BZZ_29Z",
    "size_7C_28_29I",
    "write_7C_28Ljava_2Fio_2FFileDescriptor_3BABII_29I",
    "getIntentForIntentSender_7C_28Landroid_2Fcontent_2FIIntentSender_3B_29Landroid_2Fcontent_2FIntent_3B",
    "getApsManager_7C_28_29Landroid_2Faps_2FIApsManager_3B",
    "getConfigItem_7C_28I_29I",
    "getAppToken_7C_28_29Landroid_2Fview_2FIApplicationToken_3B",
    "linkToDeath_7C_28Landroid_2Fos_2FIBinder_24DeathRecipient_3BI_29V",
    "packageHasActiveAdmins_7C_28Ljava_2Flang_2FString_3BI_29Z",
    "getPackagesForUid_7C_28I_29ALjava_2Flang_2FString_3B",
    "elapsedRealtime_7C_28_29J",
    "isBoundWidgetPackage_7C_28Ljava_2Flang_2FString_3BI_29Z",
    "getIntentForIntentSender_7C_28Landroid_2Fcontent_2FIIntentSender_3B_29Landroid_2Fcontent_2FIntent_3B",
    "getPackageUid_7C_28Ljava_2Flang_2FString_3BII_29I",
    "getWinFreezeScreenMonitor_7C_28_29Lcom_2Fandroid_2Fserver_2Fos_2FIFreezeScreenWindowMonitor_3B",
    "stat_7C_28Ljava_2Flang_2FString_3B_29Landroid_2Fsystem_2FStructStat_3B",
    "isKeyguardLocked_7C_28_29Z",
    "applyPostLayoutPolicyLw_7C_28Landroid_2Fview_2FWindowManagerPolicy_24WindowState_3BLandroid_2Fview_"
    "2FWindowManager_24LayoutParams_3BLandroid_2Fview_2FWindowManagerPolicy_24WindowState_3BLandroid_2Fview_"
    "2FWindowManagerPolicy_24WindowState_3B_29V",
    "getPackagesForUid_7C_28I_29ALjava_2Flang_2FString_3B",
    "compare_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29I",
    "next_7C_28_29Ljava_2Flang_2FObject_3B",
    "getApplicationInfo_7C_28Ljava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_2FApplicationInfo_3B",
    "registerReceiver_7C_28Landroid_2Fapp_2FIApplicationThread_3BLjava_2Flang_2FString_3BLandroid_2Fcontent_"
    "2FIIntentReceiver_3BLandroid_2Fcontent_2FIntentFilter_3BLjava_2Flang_2FString_3BII_29Landroid_2Fcontent_2FIntent_"
    "3B",
    "iterator_7C_28_29Ljava_2Futil_2FIterator_3B",
    "getHwFlogManager_7C_28_29Landroid_2Fcommon_2FHwFlogManager_3B",
    "getString_7C_28I_29Ljava_2Flang_2FString_3B",
    "getppid_7C_28_29I",
    "getNameForUid_7C_28I_29Ljava_2Flang_2FString_3B",
    "isProtectedBroadcast_7C_28Ljava_2Flang_2FString_3B_29Z",
    "getEventType_7C_28_29I",
    "packageHasActiveAdmins_7C_28Ljava_2Flang_2FString_3BI_29Z",
    "getApplicationEnabledSetting_7C_28Ljava_2Flang_2FString_3BI_29I",
    "handleIncomingUser_7C_28IIIZZLjava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29I",
    "write_7C_28Ljava_2Fio_2FFileDescriptor_3BABII_29I",
    "getApsManager_7C_28_29Landroid_2Faps_2FIApsManager_3B",
    "getBaseType_7C_28_29I",
    "size_7C_28_29I",
    "getPackageInfo_7C_28Ljava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_2FPackageInfo_3B",
    "remove_7C_28I_29Ljava_2Flang_2FObject_3B",
    "resolveContentProvider_7C_28Ljava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_2FProviderInfo_3B",
    "fstat_7C_28Ljava_2Fio_2FFileDescriptor_3B_29Landroid_2Fsystem_2FStructStat_3B",
    "getName_7C_28_29Ljava_2Flang_2FString_3B",
    "isEmpty_7C_28_29Z",
    "queryIntentReceivers_7C_28Landroid_2Fcontent_2FIntent_3BLjava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_"
    "2FParceledListSlice_3B",
    "print_7C_28Ljava_2Flang_2FObject_3BLjava_2Futil_2FLocale_3B_29V",
    "getStackId_7C_28_29I",
    "iterator_7C_28_29Ljava_2Futil_2FIterator_3B",
    "get_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B",
    "checkUidSignatures_7C_28II_29I",
    "add_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "containsKey_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "endTag_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Lorg_2Fxmlpull_2Fv1_2FXmlSerializer_3B",
    "getResolution_7C_28Ljava_2Flang_2FString_3B_29F",
    "getDepth_7C_28_29I",
    "canAddInternalSystemWindow_7C_28_29Z",
    "getResolution_7C_28Ljava_2Flang_2FString_3B_29F",
    "values_7C_28_29Ljava_2Futil_2FCollection_3B",
    "moveToNext_7C_28_29Z",
    "getCurrentUser_7C_28_29Landroid_2Fcontent_2Fpm_2FUserInfo_3B",
    "close_7C_28_29V",
    "getInt_7C_28I_29I",
    "charAt_7C_28I_29C",
    "read_7C_28Ljava_2Fio_2FFileDescriptor_3BABII_29I",
    "getRunningAppProcesses_7C_28_29Ljava_2Futil_2FList_3B",
    "setPackageStoppedState_7C_28Ljava_2Flang_2FString_3BZI_29V",
    "getNonDecorDisplayWidth_7C_28IIIII_29I",
    "startTag_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Lorg_2Fxmlpull_2Fv1_2FXmlSerializer_3B",
    "finishLayoutLw_7C_28_29V",
    "contains_7C_28Ljava_2Flang_2FObject_3B_29Z",
    "setFirewallUidRule_7C_28III_29V",
    "getForceRotationManager_7C_28_29Lcom_2Fhuawei_2Fforcerotation_2FIForceRotationManager_3B",
    "ioctlInt_7C_28Ljava_2Fio_2FFileDescriptor_3BILandroid_2Futil_2FMutableInt_3B_29I",
    "close_7C_28Ljava_2Fio_2FFileDescriptor_3B_29V",
    "isResourceNeeded_7C_28I_29Z",
    "isAppStartModeDisabled_7C_28ILjava_2Flang_2FString_3B_29Z",
    "getpid_7C_28_29I",
    "beginPostLayoutPolicyLw_7C_28II_29V",
    "getAttributeValue_7C_28Ljava_2Flang_2FString_3BLjava_2Flang_2FString_3B_29Ljava_2Flang_2FString_3B",
    "getUserInfo_7C_28I_29Landroid_2Fcontent_2Fpm_2FUserInfo_3B",
    "getActiveScorerPackage_7C_28_29Ljava_2Flang_2FString_3B",
    "isPowerSaveWhitelistExceptIdleApp_7C_28Ljava_2Flang_2FString_3B_29Z",
    "open_7C_28Ljava_2Flang_2FString_3BII_29Ljava_2Fio_2FFileDescriptor_3B",
    "index_7C_28_29I",
    "toString_7C_28_29Ljava_2Flang_2FString_3B",
    "isPackageAvailable_7C_28Ljava_2Flang_2FString_3BI_29Z",
    "append_7C_28Ljava_2Flang_2FCharSequence_3B_29Ljava_2Flang_2FAppendable_3B",
    "scheduleRegisteredReceiver_7C_28Landroid_2Fcontent_2FIIntentReceiver_3BLandroid_2Fcontent_2FIntent_3BILjava_"
    "2Flang_2FString_3BLandroid_2Fos_2FBundle_3BZZII_29V",
    "getPackageForIntentSender_7C_28Landroid_2Fcontent_2FIIntentSender_3B_29Ljava_2Flang_2FString_3B",
    "toArray_7C_28_29ALjava_2Flang_2FObject_3B",
    "setToDefaults_7C_28_29V",
    "isEmpty_7C_28_29Z",
    "put_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B",
    "getStackBounds_7C_28ILandroid_2Fgraphics_2FRect_3B_29V",
    "checkPackageStartable_7C_28Ljava_2Flang_2FString_3BI_29V",
    "checkPackage_7C_28ILjava_2Flang_2FString_3B_29I",
    "notifyPackageUse_7C_28Ljava_2Flang_2FString_3BI_29V",
    "getAppToken_7C_28_29Landroid_2Fview_2FIApplicationToken_3B",
    "isScreenOn_7C_28_29Z",
    "computeFrameLw_7C_28Landroid_2Fgraphics_2FRect_3BLandroid_2Fgraphics_2FRect_3BLandroid_2Fgraphics_2FRect_"
    "3BLandroid_2Fgraphics_2FRect_3BLandroid_2Fgraphics_2FRect_3BLandroid_2Fgraphics_2FRect_3BLandroid_2Fgraphics_"
    "2FRect_3BLandroid_2Fgraphics_2FRect_3B_29V",
    "getPolicy_7C_28Landroid_2Fcontent_2FComponentName_3BLjava_2Flang_2FString_3BI_29Landroid_2Fos_2FBundle_3B",
    "getAttributeNameResource_7C_28I_29I",
    "getuid_7C_28_29I",
    "getConfigDisplayHeight_7C_28IIIII_29I",
    "set_7C_28ILjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B",
    "finishPostLayoutPolicyLw_7C_28_29I",
    "isNetworkMetered_7C_28Landroid_2Fnet_2FNetworkState_3B_29Z",
    "getResources_7C_28Ljava_2Flang_2FClassLoader_3B_29Landroid_2Fcontent_2Fres_2FResources_3B",
    "refContentProvider_7C_28Landroid_2Fos_2FIBinder_3BII_29Z",
    "queryIntentServices_7C_28Landroid_2Fcontent_2FIntent_3BLjava_2Flang_2FString_3BII_29Landroid_2Fcontent_2Fpm_"
    "2FParceledListSlice_3B",
    "bindService_7C_28Landroid_2Fapp_2FIApplicationThread_3BLandroid_2Fos_2FIBinder_3BLandroid_2Fcontent_2FIntent_"
    "3BLjava_2Flang_2FString_3BLandroid_2Fapp_2FIServiceConnection_3BILjava_2Flang_2FString_3BI_29I",
    "appendChild_7C_28Lorg_2Fw3c_2Fdom_2FNode_3B_29Lorg_2Fw3c_2Fdom_2FNode_3B",
    "getHwNsdImpl_7C_28_29Landroid_2Fview_2FIHwNsdImpl_3B",
    "getenv_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2FString_3B"};
}  // namespace maple
#endif
