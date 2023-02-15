/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2023 Apple, Inc. All rights reserved.
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

#include "Poll.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#include "GenLinkedList.h"
#include "DebugServices.h"

#define MAX_SOURCES MAXIMUM_WAIT_OBJECTS


typedef struct PollSource_struct
{
	SOCKET socket;
	HANDLE handle;
	void   *context;

	union
	{
		mDNSPollSocketCallback	socket;
		mDNSPollEventCallback	event;
	} callback;

	struct Worker_struct		*worker;
	struct PollSource_struct	*next;
} PollSource;


typedef struct Worker_struct
{
	HANDLE					thread;		// NULL for main worker
	unsigned				id;			// 0 for main worker

	HANDLE					start;		// NULL for main worker
	HANDLE					stop;		// NULL for main worker
	BOOL					done;		// Not used for main worker

	DWORD					numSources;
	PollSource				*sources[ MAX_SOURCES ];
	HANDLE					handles[ MAX_SOURCES ];
	DWORD					result;
	DWORD					error;  // store detailed error code associated to result when result is WAIT_FAILED, 0 otherwise
	struct Worker_struct	*next;
} Worker;


typedef struct Poll_struct
{
	mDNSBool		setup;   // true if poll mechanism already initialized
	HANDLE			wakeup;
	GenLinkedList	sources;
	DWORD			numSources;
	Worker			main;
	GenLinkedList	workers;
	HANDLE			workerHandles[ MAX_SOURCES ]; // store handles for stop event
	DWORD			numWorkers;
} Poll;


/*
 * Poll Methods
 */

mDNSlocal mStatus			PollRegisterSource( PollSource *source );
mDNSlocal mStatus			PollUnregisterSource( PollSource *source );
mDNSlocal mStatus			PollStartWorkers( void );
mDNSlocal mStatus			PollStopWorkers( void );
mDNSlocal mStatus			PollRemoveWorker( Worker *worker );


/*
 * Worker Methods
 */

mDNSlocal mStatus			WorkerInit( Worker *worker );
mDNSlocal void				WorkerFree( Worker *worker );
mDNSlocal void				WorkerRegisterSource( Worker *worker, PollSource *source );
mDNSlocal int				WorkerSourceToIndex( Worker *worker, PollSource *source );
mDNSlocal void				WorkerUnregisterSource( Worker *worker, PollSource *source );
mDNSlocal void				WorkerDispatch( Worker *worker);
mDNSlocal void CALLBACK		WorkerWakeupNotification( HANDLE event, void *context );
mDNSlocal unsigned WINAPI	WorkerMain( LPVOID inParam );


#define	DEBUG_NAME	"[mDNSWin32poll] "

mDNSlocal Poll gPoll = { mDNSfalse, NULL };


static char *win32_strerror(DWORD inErrorCode)
{
    static char buffer[1024];
    DWORD n;

    memset(buffer, 0, sizeof(buffer));
    n = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        inErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        sizeof(buffer),
        NULL);
    if (n > 0)
    {
        // Remove any trailing CR's or LF's since some messages have them.
        while ((n > 0) && isspace(((unsigned char *) buffer)[n - 1]))
            buffer[--n] = '\0';
    }
    return buffer;
}


mDNSlocal void ShiftDown( void * arr, size_t arraySize, size_t itemSize, int index )
{
    memmove( ( ( unsigned char* ) arr ) + ( ( index - 1 ) * itemSize ), ( ( unsigned char* ) arr ) + ( index * itemSize ), ( arraySize - index ) * itemSize );
}

// Add socket to poll list
mDNSexport mStatus mDNSPollRegisterSocket( SOCKET socket, int networkEvents, mDNSPollSocketCallback callback, void *context )
{
	PollSource	*source = NULL;
	HANDLE		event = WSA_INVALID_EVENT;
	mStatus		err = mStatus_UnknownErr;
	DWORD		ret;

	if ( socket == INVALID_SOCKET )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterSocket: socket is not valid, exiting !\n" );
		err = mStatus_BadParamErr;
		goto exit;
	}

	source = (PollSource *) malloc( sizeof( PollSource ) );
	if ( source == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterSocket: malloc error\n" );
		err = mStatus_NoMemoryErr;
		goto exit;
	}

	event = WSACreateEvent();
	if ( event == WSA_INVALID_EVENT )
	{
		ret = (DWORD)WSAGetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterSocket: WSACreateEvent error %u %s\n", ret, win32_strerror( ret ));
		err = mStatus_UnknownErr;
		goto exit;
	}

	ret = WSAEventSelect( socket, event, networkEvents );
	if ( ret == SOCKET_ERROR )
	{
		ret = (DWORD)WSAGetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterSocket: WSAEventSelect error %u %s\n", ret, win32_strerror( ret ));
		err = mStatus_UnknownErr;
		goto exit;
	}

	source->socket = socket;
	source->handle = event;
	source->callback.socket = callback;
	source->context = context;

	err = PollRegisterSource( source );
	if ( err != mStatus_NoError )
		goto exit;
	
	err = mStatus_NoError;
	
exit:

	if ( err != mStatus_NoError )
	{
		if ( event != WSA_INVALID_EVENT )
		{
			BOOL ok;
			
			ok = WSACloseEvent( event );
			if ( !ok )
			{
				ret = (DWORD)WSAGetLastError();
				dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterSocket: WSACloseEvent failed %u %s\n", ret, win32_strerror( ret ) );
			}
		}

		if ( source != NULL )
			free( source );
	}

	return err;
}

// Remove socket from poll list
mDNSexport mStatus mDNSPollUnregisterSocket( SOCKET socket )
{
	PollSource	*source;
	mStatus		err = mStatus_UnknownErr;

	if ( socket == INVALID_SOCKET )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterSocket: socket is not valid, exiting !\n" );
		return mStatus_BadParamErr;
	}

	// Look for socket in the list
	for ( source = (PollSource *)gPoll.sources.Head; source; source = source->next )
		if ( source->socket == socket )
			break;

	// socket found in list ?
	if ( source != NULL )
	{
		BOOL ok;
		
		ok = WSACloseEvent( source->handle );
		if ( !ok )
		{
			DWORD ret = (DWORD)WSAGetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterSocket: WSACloseEvent failed %u %s\n", ret, win32_strerror( ret ) );
		}

		(void)PollUnregisterSource( source );

		free( source );

		err = mStatus_NoError;
	}
	else
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterSocket: source for socket %u not found \n", socket );
		err = mStatus_BadParamErr;
	}

	return err;
}

// Add event to poll list
mDNSexport mStatus mDNSPollRegisterEvent( HANDLE event, mDNSPollEventCallback callback, void *context )
{
	PollSource	*source = NULL;
	mStatus		err = mStatus_UnknownErr;

	if ( event == WSA_INVALID_EVENT || event == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterEvent: invalid event, exiting !\n" );
		err = mStatus_BadParamErr;
		goto exit;
	}
	
	source = (PollSource *) malloc( sizeof( PollSource ) );
	if ( source == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollRegisterEvent: malloc error\n" );
		err = mStatus_NoMemoryErr;
		goto exit;
	}

	source->socket = INVALID_SOCKET;
	source->handle = event;
	source->callback.event = callback;
	source->context = context;

	err = PollRegisterSource( source );
	if ( err != mStatus_NoError )
		goto exit;
	
	err = mStatus_NoError;
	
exit:

	if ( err != mStatus_NoError )
		if ( source != NULL )
			free( source );

	return err;
}

// Remove event from poll list
mDNSexport mStatus mDNSPollUnregisterEvent( HANDLE event )
{
	PollSource	*source;
	mStatus		err = mStatus_UnknownErr;

	if ( event == WSA_INVALID_EVENT || event == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterEvent: invalid event, exiting !\n" );
		err = mStatus_BadParamErr;
		return err;
	}

	// Look for event in the list
	for ( source = (PollSource *)gPoll.sources.Head; source; source = source->next )
		if ( source->handle == event )
			break;

	// event found in list ?
	if ( source != NULL )
	{
		(void)PollUnregisterSource( source );

		free( source );

		err = mStatus_NoError;
	}
	else
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPollUnregisterEvent: source for handle %p not found \n", event );
		err = mStatus_BadParamErr;
	}

	return err;
}

// Poll for event with msec time-out interval, in milliseconds.
// If a nonzero value is specified, the function waits until the specified objects are signaled or the interval elapses.
// If dwMilliseconds is zero, the function does not enter a wait state if the specified objects are not signaled; it always returns immediately.
// If dwMilliseconds is INFINITE, the function will return only when the specified objects are signaled.
mDNSexport mStatus mDNSPoll( DWORD msec )
{
	mStatus err = mStatus_NoError;

	if ( gPoll.numWorkers > 0 )
	{	
		err = PollStartWorkers();
		if ( err != mStatus_NoError )
		{
			goto exit;
		}
	}

	gPoll.main.result = WaitForMultipleObjects( gPoll.main.numSources, gPoll.main.handles, FALSE, msec );
	if ( gPoll.main.result == WAIT_FAILED )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "mDNSPoll: WaitForMultipleObjects WAIT_FAILED %u %s\n", ret, win32_strerror( ret ) );
		gPoll.main.error = ret;
		err = mStatus_UnknownErr;
	}
	else
		gPoll.main.error = 0;

	if ( gPoll.numWorkers > 0 )
	{
		err = PollStopWorkers();
		if ( err != mStatus_NoError )
		{
			goto exit;
		}
	}

	WorkerDispatch( &gPoll.main );

exit:

	return err;
}

mDNSexport mStatus PollSetup( void )
{
	mStatus err = mStatus_UnknownErr;

	verbosedebugf(DEBUG_NAME "%s ", __FUNCTION__);

	// Setup already done ?
	if ( !gPoll.setup )
	{
		memset( &gPoll, 0, sizeof( gPoll ) );

		InitLinkedList( &gPoll.sources, offsetof( PollSource, next ) );
		InitLinkedList( &gPoll.workers, offsetof( Worker, next ) );

		gPoll.wakeup = CreateEvent( NULL, TRUE, FALSE, NULL );
		if ( gPoll.wakeup == INVALID_HANDLE_VALUE)
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "PollSetup: CreateEvent error %u %s\n", ret, win32_strerror( ret ) );
			err = mStatus_UnknownErr;
			goto exit;
		}

		err = WorkerInit( &gPoll.main );
		if ( err != mStatus_NoError )
			goto exit;
		
		gPoll.setup = mDNStrue; // Init complete
		
		err = mStatus_NoError;
	}
	else
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollSetup: setup already done !\n" );
		err = mStatus_BadParamErr;
	}

exit:

	return err;
}

mDNSexport mStatus PollCleanup( void )
{
	mStatus err = mStatus_UnknownErr;
	
	verbosedebugf(DEBUG_NAME "%s gPoll.setup %d", __FUNCTION__, gPoll.setup);

	// Setup already done ?
	if ( gPoll.setup )
	{
		BOOL ok;
		
		ok = CloseHandle( gPoll.wakeup );
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "PollCleanup: CloseHandle failed %u %s\n", ret, win32_strerror( ret ) );
		}		
		gPoll.wakeup = NULL;

		if ( gPoll.workers.Head )
		{
			Worker *worker;

			dlog( kDebugLevelVerbose, DEBUG_NAME "PollCleanup: Poll list not empty, leak(s) will happen, did you forget to unregister some event(s)/sockets(s) ?\n" );
#ifdef DEBUG
			if ( gPoll.numSources )
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollCleanup: Poll leaking, numSources not 0 : %u\n", gPoll.numSources );
			if ( gPoll.main.numSources > 1 )
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollCleanup: Poll leaking, main numSources not 0 : %u\n", gPoll.main.numSources );
#endif
			
			worker = ( Worker *)gPoll.workers.Head;
			while ( worker )
			{
				Worker *tmp;

				tmp = worker;
				worker = worker->next;

				WorkerFree( tmp );
			}
		}

		WorkerFree( &gPoll.main );

		memset( &gPoll, 0, sizeof( gPoll ) );  // Erase everything

		gPoll.setup = mDNSfalse;               // Cleanup complete
		
		err = mStatus_NoError;
	}
	else
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollCleanup: setup never done before !\n" );
		err = mStatus_BadParamErr;
	}

	return err;
}

mDNSlocal mStatus PollRegisterSource( PollSource *source )
{
	Worker	*worker = NULL;
	mStatus err = mStatus_UnknownErr;

	AddToTail( &gPoll.sources, source );
	gPoll.numSources++;

	// First check our main worker. In most cases, we won't have to worry about threads
	if ( gPoll.main.numSources < MAX_SOURCES )
	{
		WorkerRegisterSource( &gPoll.main, source );
	}
	else
	{
		// Try to find a thread to use that we've already created
		for ( worker = (Worker *)gPoll.workers.Head; worker; worker = worker->next )
		{
			if ( worker->numSources < MAX_SOURCES )
			{
				WorkerRegisterSource( worker, source );
				break;
			}
		}

		// If not (no worker or full), then create a new worker and make a thread to run it in
		if ( worker == NULL )
		{
			DWORD ret;
			uintptr_t th;

			worker = ( Worker *) malloc( sizeof( Worker ) );
			if ( worker == NULL )
			{
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollRegisterSource: malloc error\n" );
				err = mStatus_NoMemoryErr;
				goto exit;
			}
	
			memset( worker, 0, sizeof( Worker ) );

			worker->start = CreateEvent( NULL, FALSE, FALSE, NULL );
			if ( worker->start == INVALID_HANDLE_VALUE)
			{
				ret = GetLastError();
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollRegisterSource: CreateEvent (start) error %u %s\n", ret, win32_strerror( ret ) );
				err = mStatus_UnknownErr;
				goto exit;
			}

			worker->stop = CreateEvent( NULL, FALSE, FALSE, NULL );
			if ( worker->stop == INVALID_HANDLE_VALUE)
			{
				ret = GetLastError();
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollRegisterSource: CreateEvent (stop) error %u %s\n", ret, win32_strerror( ret ) );
				err = mStatus_UnknownErr;
				goto exit;
			}

			err = WorkerInit( worker );
			if ( err != mStatus_NoError )
			{
				goto exit;
			}

			// Create thread with _beginthreadex() instead of CreateThread() to avoid
			// memory leaks when using static run-time libraries.
			// See <http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/createthread.asp>.
			th = _beginthreadex( NULL, 0, WorkerMain, worker, 0, &worker->id );
			if ( th == 0 )
			{
				int eno = errno;
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollRegisterSource: _beginthreadex failed %d %s\n", eno, strerror( eno ) );
				err = mStatus_UnknownErr;
				goto exit;
			}
			else
				worker->thread = ( HANDLE ) th;

			AddToTail( &gPoll.workers, worker );
			gPoll.workerHandles[ gPoll.numWorkers++ ] = worker->stop;
			
			WorkerRegisterSource( worker, source );
		}
	}
	
	err = mStatus_NoError;

exit:

	if ( ( err != mStatus_NoError ) && worker )
		WorkerFree( worker );

	return err;
}

mDNSlocal mStatus PollUnregisterSource( PollSource *source )
{
	int ret = RemoveFromList( &gPoll.sources, source );
	if ( ret == 0 )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollUnregisterSource: source %p not found\n", source );
		return mStatus_BadParamErr;
	}
	
	gPoll.numSources--;

	WorkerUnregisterSource( source->worker, source );
	
	return mStatus_NoError;
}

mDNSlocal mStatus PollStartWorkers( void )
{
	Worker	*worker;
	Worker	*next;
	mStatus	err = mStatus_NoError;
	BOOL	ok;

	dlog( kDebugLevelChatty, DEBUG_NAME "PollStartWorkers: starting workers\n" );
	
	worker = (Worker *)gPoll.workers.Head;
	while ( worker )
	{
		next = worker->next;

		if ( worker->numSources == 1 )
		{
			(void)PollRemoveWorker( worker );
		}
		else
		{
			dlog( kDebugLevelChatty, DEBUG_NAME "PollStartWorkers: waking up worker\n" );

			ok = SetEvent( worker->start );
			if ( !ok )
			{
				DWORD ret = GetLastError();
				dlog( kDebugLevelVerbose, DEBUG_NAME "PollStartWorkers: SetEvent failed %u %s\n", ret, win32_strerror( ret ) );
				err = mStatus_UnknownErr;
			}

			if ( err != mStatus_NoError )
				(void)PollRemoveWorker( worker );
		}

		worker = next;
	}

	return err;
}

mDNSlocal mStatus PollStopWorkers( void )
{
	DWORD	result;
	Worker	*worker;
	BOOL	ok;
	mStatus	err = mStatus_NoError;

	dlog( kDebugLevelChatty, DEBUG_NAME "PollStopWorkers: stopping workers\n" );
	
	ok = SetEvent( gPoll.wakeup );
	if ( !ok )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollStopWorkers: SetEvent failed %u %s\n", ret, win32_strerror( ret ) );
		err = mStatus_UnknownErr;
	}

	// Wait For 5 seconds for all the workers to wake up
	result = WaitForMultipleObjects( gPoll.numWorkers, gPoll.workerHandles, TRUE, 5000 );
	if ( result == WAIT_FAILED )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollStopWorkers: WaitForSingleObject failed WAIT_FAILED %u %s\n", ret, win32_strerror( ret ) );
		err = mStatus_UnknownErr;
	}

	ok = ResetEvent( gPoll.wakeup );
	if ( !ok )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollStopWorkers: ResetEvent failed %u %s\n", ret, win32_strerror( ret ) );
		err = mStatus_UnknownErr;
	}

	for ( worker = (Worker *)gPoll.workers.Head; worker; worker = worker->next )
		WorkerDispatch( worker );

	return err;
}

mDNSlocal mStatus PollRemoveWorker( Worker *worker )
{
	DWORD	result;
	mStatus	err = mStatus_UnknownErr;
	BOOL	ok;
	DWORD	i;
	int		rc;

	dlog( kDebugLevelChatty, DEBUG_NAME "PollRemoveWorker: removing worker %u\n", worker->id );
	
	rc = RemoveFromList( &gPoll.workers, worker );
	if ( rc == 0 )
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollRemoveWorker: worker %u not found\n", worker->id );

	// Remove handle from gPoll.workerHandles
	for ( i = 0; i < gPoll.numWorkers; i++ )
		if ( gPoll.workerHandles[ i ] == worker->stop )
		{
			ShiftDown( gPoll.workerHandles, gPoll.numWorkers, sizeof( gPoll.workerHandles[ 0 ] ), i + 1 );
			break;
		}

	worker->done = TRUE;
	gPoll.numWorkers--;

	// Cause the thread to exit.
	ok = SetEvent( worker->start );
	if ( !ok )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollRemoveWorker: SetEvent failed %u %s\n", ret, win32_strerror( ret ) );
		err = mStatus_UnknownErr;
	}
			
	result = WaitForSingleObject( worker->thread, 5000 );
	if ( result == WAIT_FAILED )
	{
		DWORD ret = GetLastError();
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollRemoveWorker: WaitForSingleObject failed WAIT_FAILED %u %s\n", ret, win32_strerror( ret ) );
		err = mStatus_UnknownErr;
	}
	else if ( result == WAIT_TIMEOUT )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollRemoveWorker: timeout\n" );
	}
	else if ( result == WAIT_ABANDONED )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "PollRemoveWorker: WAIT_ABANDONED\n" );
	}
	
	WorkerFree( worker );
	
	err = mStatus_NoError;

	return err;
}

mDNSlocal void WorkerRegisterSource( Worker *worker, PollSource *source )
{
	source->worker = worker;
	worker->sources[ worker->numSources ] = source;
	worker->handles[ worker->numSources ] = source->handle;
	worker->numSources++;
}

mDNSlocal int WorkerSourceToIndex( Worker *worker, PollSource *source )
{
	int index;

	for ( index = 0; index < ( int ) worker->numSources; index++ )
		if ( worker->sources[ index ] == source )
			break;

	if ( index == ( int ) worker->numSources )
		index = -1;

	return index;
}

mDNSlocal void WorkerUnregisterSource( Worker *worker, PollSource *source )
{
	int   sourceIndex;
	DWORD delta;

	sourceIndex = WorkerSourceToIndex( worker, source );
	if ( sourceIndex == -1 )
	{
		LogMsg( "WorkerUnregisterSource: source not found in list" );
		goto exit;
	}

	delta = ( worker->numSources - sourceIndex - 1 );

	// If this source is not at the end of the list, then move memory
	if ( delta > 0 )
	{
		ShiftDown( worker->sources, worker->numSources, sizeof( worker->sources[ 0 ] ), sourceIndex + 1 );
		ShiftDown( worker->handles, worker->numSources, sizeof( worker->handles[ 0 ] ), sourceIndex + 1 );
	}

	worker->numSources--;

exit:

	return;
}

mDNSlocal void CALLBACK WorkerWakeupNotification( HANDLE event, void *context )
{
	(void) event;
	(void) context;

	dlog( kDebugLevelChatty, DEBUG_NAME "WorkerWakeupNotification: Worker thread wakeup\n" );
}

mDNSlocal void WorkerDispatch( Worker *worker )
{
	if ( worker->result == WAIT_FAILED )
	{
		/* What should we do here ? */
		
		// Log something, even if it has already been done !!
		dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerDispatch: worker WAIT_FAILED error %u %s\n", worker->error, win32_strerror( worker->error ) );
	}
	else if ( worker->result == WAIT_TIMEOUT )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "WorkerDispatch: timeout\n" );
	}
	else if ( worker->result == WAIT_ABANDONED ) // That one should not happen, but in case
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerDispatch: WAIT_ABANDONED\n" );
	}
	else
	{
		DWORD		waitItemIndex = ( DWORD )( ( ( int ) worker->result ) - WAIT_OBJECT_0 ); // get index of signaled source
		PollSource	*source = NULL;

		// Sanity check
		if ( waitItemIndex >= worker->numSources )
		{
			LogMsg( "WorkerDispatch: waitItemIndex (%u) is >= numSources (%u)", waitItemIndex, worker->numSources );
			goto exit;
		}

		source = worker->sources[ waitItemIndex ];  // get signaled source from his index

		if ( source->socket != INVALID_SOCKET )     // event for a socket, call socket callback
		{
			WSANETWORKEVENTS event;
			int ret;
	
			ret = WSAEnumNetworkEvents( source->socket, source->handle, &event );
			if ( ret == SOCKET_ERROR )
			{
				DWORD err = (DWORD)WSAGetLastError();
				dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerDispatch: WSAEnumNetworkEvents error %u %s\n", err, win32_strerror( err ));
				source->callback.socket( source->socket, NULL, source->context );
			}
			else
				source->callback.socket( source->socket, &event, source->context );
		}
		else // event for handle, call event callback
		{
			source->callback.event( source->handle, source->context );
		}
	}

exit:

	return;
}

mDNSlocal mStatus WorkerInit( Worker *worker )
{
	PollSource	*source = NULL;
	mStatus		err = mStatus_NoError;

	if ( worker == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerInit: worker should not be null, exiting !\n" );
		err = mStatus_BadParamErr;
		goto exit;
	}

	source = (PollSource *) malloc( sizeof( PollSource ) );
	if ( source == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerInit: malloc error\n" );
		err = mStatus_NoMemoryErr;
		goto exit;
	}

	source->socket = INVALID_SOCKET;
	source->handle = gPoll.wakeup;
	source->callback.event = WorkerWakeupNotification;
	source->context = NULL;
	
	WorkerRegisterSource( worker, source );

exit:

	return err;
}

mDNSlocal void WorkerFree( Worker *worker )
{
	BOOL ok;
	int i;
	PollSource	*source = NULL;

	if ( worker->start )
	{
		ok = CloseHandle( worker->start );
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerFree: CloseHandle failed %u %s\n", ret, win32_strerror(ret) );
		}
		worker->start = NULL;
	}

	if ( worker->stop )
	{
		ok = CloseHandle( worker->stop );
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerFree: CloseHandle failed %u %s\n", ret, win32_strerror(ret) );
		}
		worker->stop = NULL;
	}
	
	if ( worker->thread )
	{
		ok = CloseHandle( worker->thread ); // Don't forget to close thread handle !
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerFree: CloseHandle failed %u %s\n", ret, win32_strerror(ret) );
		}
		worker->thread = NULL;
	}
	
	for (i = 0; i < ( int ) worker->numSources; i++)
		if (worker->sources[i]->worker == worker)
			source = worker->sources[i];
	if ( source != NULL )
	{
		WorkerUnregisterSource( worker, source );
		mDNSPlatformMemFree( source );
	}

	if (worker != &gPoll.main)        // Free worker only if it is not the main one that is static !
	{
		if (worker->numSources != 0)
		{
			dlog(kDebugLevelVerbose, DEBUG_NAME "WorkerFree: Poll leaking, numSources not 0 : %u\n", worker->numSources);
#ifdef DEBUG

#endif
		}

		free( worker );
	}
}

mDNSlocal unsigned WINAPI WorkerMain( LPVOID inParam )
{
	Worker	*worker = ( Worker * ) inParam;
	DWORD	result;
	BOOL	ok;

	dlog( kDebugLevelVerbose, DEBUG_NAME "entering WorkerMain()\n" );
	
	if ( worker == NULL )
	{
		dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerMain: worker should not be null, exiting !\n" );
		goto exit;
	}

	while ( TRUE )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "WorkerMain: worker thread %u will wait on main loop\n", worker->id );
		
		result = WaitForSingleObject( worker->start, INFINITE );	
		if ( result == WAIT_FAILED )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerMain: WaitForSingleObject failed WAIT_FAILED %u %s\n", ret, win32_strerror( ret ) );
			break;
		}

		if ( worker->done )
			break;

		dlog( kDebugLevelChatty, DEBUG_NAME "WorkerMain: worker thread %u will wait on sockets\n", worker->id );

		worker->result = WaitForMultipleObjects( worker->numSources, worker->handles, FALSE, INFINITE );
		if ( worker->result == WAIT_FAILED )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerMain: WaitForMultipleObjects failed WAIT_FAILED %d %s\n", ret, win32_strerror( ret ) );
			worker->error = ret;
			break;
		}
		else
			worker->error = 0;

		dlog( kDebugLevelChatty, DEBUG_NAME "WorkerMain: worker thread %u did wait on sockets: %u\n", worker->id, worker->result );

		ok = SetEvent( gPoll.wakeup );
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerMain: SetEvent failed %u %s\n", ret, win32_strerror( ret ) );
			break;
		}

		dlog( kDebugLevelChatty, DEBUG_NAME "WorkerMain: worker thread %u preparing to sleep\n", worker->id );

		ok = SetEvent( worker->stop );
		if ( !ok )
		{
			DWORD ret = GetLastError();
			dlog( kDebugLevelVerbose, DEBUG_NAME "WorkerMain: SetEvent failed %u %s\n", ret, win32_strerror( ret ) );
			break;
		}
	}

	dlog( kDebugLevelVerbose, DEBUG_NAME "exiting WorkerMain()\n" );

exit:

	return 0;
}
