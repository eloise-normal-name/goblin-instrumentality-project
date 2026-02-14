#pragma once

#include "crash_diagnostics.h"

#ifdef _HRESULT_DEFINED
constexpr bool failed(HRESULT hr) noexcept {
	return FAILED(hr);
}
#endif

#ifdef _NV_ENCODEAPI_H_
constexpr bool failed(NVENCSTATUS status) noexcept {
	return status != NV_ENC_SUCCESS;
}
#endif

#include <type_traits>

template <typename T>
struct always_false : std::false_type {};

template <typename T>
constexpr bool failed(T) {
	static_assert(always_false<T>::value,
				  "Unsupported status type passed to Try |. Provide tv_failed(T).");
	return true;
}

struct Try_t {
	CrashDiagnostics* diagnostics = nullptr;
	const char* context			  = nullptr;

	Try_t() = default;
	Try_t(CrashDiagnostics* diag, const char* ctx) : diagnostics(diag), context(ctx) {}

	template <typename T>
	Try_t operator|(T v) const {
		if (failed(v)) {
			if (diagnostics && context) {
#ifdef _HRESULT_DEFINED
				if constexpr (std::is_same_v<T, HRESULT>) {
					diagnostics->LogError(context, v);
				} else
#endif
				{
					diagnostics->LogException(context);
				}
			}
			throw;
		}
		return *this;
	}
};

inline constexpr Try_t Try{};
