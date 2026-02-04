#pragma once

// Simple exception-based error handling for HRESULT and NVENCSTATUS.
// IMPORTANT: Do not add external library dependencies to this file.
// Keep this header dependency-free (only C++ standard library features allowed).

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
	template <typename T>
	Try_t operator|(T v) const {
		if (failed(v))
			throw;
		return *this;
	}
};

inline constexpr Try_t Try{};
