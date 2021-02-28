#include "property/type_id.h"

#include <atomic>
#include <fmt/format.h>

namespace property {
	static std::atomic<std::uint32_t> s_type_id_alloc { std::uint32_t(TypeId::CustomBase)-1 };


	TypeId internal::new_custom_type_id() {
		return TypeId{++s_type_id_alloc};
	}


	std::string format_debug(TypeId id) {
		switch(id) {
			case TypeId::Int: return "Int";
			case TypeId::Float: return "Float";
			case TypeId::String: return "String";
			default:
				return fmt::format(
					"Custom({})",
					std::uint32_t(id) - std::uint32_t(TypeId::CustomBase)
				);
		}
	}
}