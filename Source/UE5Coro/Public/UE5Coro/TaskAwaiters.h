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

#pragma once

#include "CoreMinimal.h"
#include "UE5Coro/Definitions.h"
#include "Tasks/Task.h"
#include "UE5Coro/AsyncCoroutine.h"

namespace UE5Coro::Private
{
class FTaskAwaiter;
}

namespace UE5Coro::Tasks
{
/** Suspends the coroutine and resumes it in a UE::Tasks::TTask. */
UE5CORO_API Private::FTaskAwaiter MoveToTask(const TCHAR* DebugName = nullptr);
}

namespace UE5Coro::Private
{
class [[nodiscard]] FTaskAwaiter
{
protected:
	const TCHAR* DebugName;

	template<typename P>
	decltype(auto) SuspendBase(stdcoro::coroutine_handle<P> Handle)
	{
		if constexpr (std::is_same_v<P, FLatentPromise>)
			Handle.promise().DetachFromGameThread();
		return [Handle] { Handle.promise().Resume(); };
	}

public:
	explicit FTaskAwaiter(const TCHAR* DebugName) : DebugName(DebugName) { }
	UE_NONCOPYABLE(FTaskAwaiter);

	bool await_ready() { return false; }
	void await_resume() { }

	template<typename P>
	void await_suspend(stdcoro::coroutine_handle<P> Handle)
	{
		UE::Tasks::Launch(DebugName, SuspendBase(Handle));
	}
};

template<typename T>
class [[nodiscard]] TTaskAwaiter : public FTaskAwaiter
{
	UE::Tasks::TTask<T> Task;

public:
	explicit TTaskAwaiter(UE::Tasks::TTask<T> Task, const TCHAR* DebugName)
		: FTaskAwaiter(DebugName), Task(Task) { }

	bool await_ready() { return Task.IsCompleted(); }
	auto await_resume()
	{
		if constexpr (!std::is_void_v<T>)
			return Task.GetResult();
	}

	template<typename P>
	void await_suspend(stdcoro::coroutine_handle<P> Handle)
	{
		UE::Tasks::Launch(DebugName, SuspendBase(Handle), Task);
	}
};

template<typename P, typename T>
struct TAwaitTransform<P, UE::Tasks::TTask<T>>
{
	TTaskAwaiter<T> operator()(UE::Tasks::TTask<T> Task)
	{
		return TTaskAwaiter<T>(Task, TEXT("UE5Coro automatic co_await wrapper"));
	}
};
}
