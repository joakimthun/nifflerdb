#pragma once

#include <memory>

namespace niffler {

	template<class T>
	struct result {
		result() = delete;
		inline result(bool ok) : ok(ok), value(nullptr) {}
		inline result(bool ok, std::unique_ptr<T> value) : ok(ok), value(std::move(value)) {}

		std::unique_ptr<T> value;
		const bool ok;
	};

}
