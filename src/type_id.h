#pragma once

#include <cstdint>
#include <string>

namespace property {

	enum class TypeId : std::uint32_t {
		Int,
		Float,
		String,
		CustomBase,
		// TODO: it would be useful to encode struct/arrayness
	};

	namespace internal {
		TypeId new_custom_type_id();
	}

	template<class T>
	TypeId type_id() {
		static TypeId s_type_id = internal::new_custom_type_id();
		return s_type_id;
	}

	template<> inline TypeId type_id<int>() { return TypeId::Int; }
	template<> inline TypeId type_id<float>() { return TypeId::Float; }
	template<> inline TypeId type_id<std::string>() { return TypeId::String; }

	std::string format_debug(TypeId id);


} // property