// Copyright © Laura Andelare
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "UE5Coro/AsyncAwaiters.h"

using namespace UE5Coro;
using namespace UE5Coro::Private;

namespace
{
template<typename H>
struct TAutoStartResumeRunnable : FRunnable
{
	H& Promise;

	explicit TAutoStartResumeRunnable(stdcoro::coroutine_handle<H> Handle,
	                                  EThreadPriority Priority, uint64 Affinity,
	                                  EThreadCreateFlags Flags)
		: Promise(Handle.promise())
	{
		FRunnableThread::Create(this, TEXT("UE5Coro::Async::MoveToNewThread"),
		                        0, Priority, Affinity, Flags);
	}

	virtual uint32 Run() override
	{
		Promise.Resume();
		return 0;
	}

	virtual void Exit() override
	{
		delete this;
	}
};
}

FAsyncAwaiter Async::MoveToThread(ENamedThreads::Type Thread)
{
	return FAsyncAwaiter(Thread);
}

FAsyncAwaiter Async::MoveToGameThread()
{
	return FAsyncAwaiter(ENamedThreads::GameThread);
}

FNewThreadAwaiter Async::MoveToNewThread(
	EThreadPriority Priority, uint64 Affinity, EThreadCreateFlags Flags)
{
	return FNewThreadAwaiter(Priority, Affinity, Flags);
}

void FNewThreadAwaiter::await_suspend(FAsyncHandle Handle)
{
	new TAutoStartResumeRunnable(Handle, Priority, Affinity, Flags);
}

void FNewThreadAwaiter::await_suspend(FLatentHandle Handle)
{
	Handle.promise().DetachFromGameThread();
	new TAutoStartResumeRunnable(Handle, Priority, Affinity, Flags);
}
