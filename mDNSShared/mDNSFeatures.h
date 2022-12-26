/*
 * Copyright (c) 2019-2022 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __mDNSFeatures_h
#define __mDNSFeatures_h

#define HAS_FEATURE_CAT(A, B)       A ## B
#define HAS_FEATURE_CHECK_0         1
#define HAS_FEATURE_CHECK_1         1
#define HAS_FEATURE(X)              ((X) / HAS_FEATURE_CAT(HAS_FEATURE_CHECK_, X))

#define MDNSRESPONDER_SUPPORTS(PLATFORM, FEATURE) \
    (MDNSRESPONDER_PLATFORM_ ## PLATFORM && \
    HAS_FEATURE(MDNSRESPONDER_SUPPORTS_ ## PLATFORM ## _ ## FEATURE))

#ifndef MDNSRESPONDER_PLATFORM_APPLE
#define MDNSRESPONDER_PLATFORM_APPLE        0
#endif

#if MDNSRESPONDER_PLATFORM_APPLE
#include "ApplePlatformFeatures.h"
#endif

// Common Features

#undef MDNSRESPONDER_PLATFORM_COMMON
#define MDNSRESPONDER_PLATFORM_COMMON       1

// Feature: DNS Push
// Radar:   <rdar://problem/23226275>
// Enabled: Yes, for Apple.

#if !defined(MDNSRESPONDER_SUPPORTS_COMMON_DNS_PUSH)
    #if MDNSRESPONDER_PLATFORM_APPLE
        #define MDNSRESPONDER_SUPPORTS_COMMON_DNS_PUSH      1
    #else
        #define MDNSRESPONDER_SUPPORTS_COMMON_DNS_PUSH      0
    #endif
#endif

// Feature: DNS LLQ
// Radar:   <rdar://problem/83483790>
// Enabled: No.

#if !defined(MDNSRESPONDER_SUPPORTS_COMMON_DNS_LLQ)
    #if MDNSRESPONDER_PLATFORM_APPLE
        #define MDNSRESPONDER_SUPPORTS_COMMON_DNS_LLQ 0
    #else
        #define MDNSRESPONDER_SUPPORTS_COMMON_DNS_LLQ 0
    #endif
#endif

// Feature: Use Multicast DNS to discover the local DNS server that is authoritative for a given domain.
// Radar:   <rdar://69957139>
// Enabled: No

#if !defined(MDNSRESPONDER_SUPPORTS_COMMON_LOCAL_DNS_RESOLVER_DISCOVERY)
    #if MDNSRESPONDER_PLATFORM_APPLE
        #define MDNSRESPONDER_SUPPORTS_COMMON_LOCAL_DNS_RESOLVER_DISCOVERY 1
    #else
        #define MDNSRESPONDER_SUPPORTS_COMMON_LOCAL_DNS_RESOLVER_DISCOVERY 0
    #endif
#endif

#endif  // __mDNSFeatures_h
