/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2023 Apple Inc. All rights reserved.
 *
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

 //----------------------------------------------------------------------------------------
 //	Registry Constants
 //----------------------------------------------------------------------------------------

#ifdef WIN32_CENTENNIAL
#define BONJOUR_HKEY	HKEY_CURRENT_USER
#else
#define BONJOUR_HKEY	HKEY_LOCAL_MACHINE
#endif


#if defined(UNICODE)

#	define kServiceParametersSoftware			L"SOFTWARE"
#	define kServiceParametersBonjour			L"Bonjour"
#	define kServiceParametersNode				L"SOFTWARE\\Apple Inc.\\Bonjour"
#	define kServiceName							L"Bonjour Service"
#	define kServiceDynDNSBrowseDomains			L"BrowseDomains"
#	define kServiceDynDNSHostNames				L"HostNames"
#	define kServiceDynDNSRegistrationDomains	L"RegistrationDomains"
#	define kServiceDynDNSDomains				L"Domains"
#	define kServiceDynDNSEnabled				L"Enabled"
#	define kServiceDynDNSStatus					L"Status"
#	define kServiceManageLLRouting				L"ManageLLRouting"
#	define kServiceCacheEntryCount				L"CacheEntryCount"
#	define kServiceManageFirewall				L"ManageFirewall"
#	define kServiceAdvertisedServices			L"Services"

#else

Unicode support is mandatory!!

#endif
