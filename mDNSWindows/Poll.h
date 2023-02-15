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

#pragma once


#include	<winsock2.h>

#include	"mDNSEmbeddedAPI.h"


#if defined(__cplusplus )
extern "C" {
#endif


typedef void ( CALLBACK *mDNSPollSocketCallback )( SOCKET socket, LPWSANETWORKEVENTS event, void *context );
typedef void ( CALLBACK *mDNSPollEventCallback ) ( HANDLE event, void *context );


extern mStatus	mDNSPollRegisterSocket( SOCKET socket, int networkEvents, mDNSPollSocketCallback callback, void *context );
extern mStatus	mDNSPollUnregisterSocket( SOCKET socket );
extern mStatus	mDNSPollRegisterEvent( HANDLE event, mDNSPollEventCallback callback, void *context );
extern mStatus	mDNSPollUnregisterEvent( HANDLE event );
extern mStatus	mDNSPoll( DWORD msec );
extern mStatus	PollSetup( void );
extern mStatus	PollCleanup( void );

#if defined(__cplusplus)
}
#endif

