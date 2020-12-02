#pragma once

#include "util.h"

#include <string>
#include <cstdint>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <vector>
#include <optional>
#include <string_view>
#include <fmt/format.h>


namespace property {


	using StructId = std::uint32_t;
	using FieldIdx = std::uint32_t;


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



	struct FieldTypeInfo {
		uintptr_t offset;
		TypeId type_id;

		std::string (*format) (std::byte const*);

		std::byte const* adjust_struct_ptr(std::byte const* struct_ptr) const {
			return struct_ptr + this->offset;
		}

		template<class F>
		F const* try_read(std::byte const* struct_ptr) {
			if (this->type_id != property::type_id<F>()) {
				return nullptr;
			}

			auto field_ptr = this->adjust_struct_ptr(struct_ptr);
			return reinterpret_cast<F const*>(field_ptr);
		}
	};

	template<StandardLayout S, class F>
	FieldTypeInfo property_field_type_info(F S::* field) {
		return {
			member_offsetof(field),
			type_id<F>(),

			[] (std::byte const* field_data) {
				return fmt::format("{}", *reinterpret_cast<F const*>(field_data));
			}
		};
	}



	struct FieldDef {
		FieldIdx idx;
		std::string name;
		std::string display_name;
		std::string description;
		FieldTypeInfo field_info;
	};

	struct StructDef {
		StructId id;
		std::string name;
		std::size_t size;

		std::vector<FieldDef> fields;
	};

	template<class S>
	struct StructBuilder {
		struct Kernel* kernel;
		StructDef* struct_def;

		template<class T>
		void add_field(T S::*, std::string name, std::string display_name, std::string description);
	};



	struct Kernel {
		std::deque<StructDef> structs;
		std::unordered_map<TypeId, StructId> type_id_to_struct;

		static std::atomic<StructId> s_property_struct_alloc;

		template<class S>
		static auto get_struct_id() -> StructId;

		template<class S>
		auto struct_def_for() const -> StructDef const*;

		auto struct_def_for(StructId) const -> StructDef const*;

		auto struct_id_from_type_id(TypeId) const -> std::optional<StructId>;

		template<StandardLayout S>
		auto register_struct(std::string name) -> StructBuilder<S>;
	};




	struct FieldRef {
		StructId struct_id;
		FieldDef const* field_def;
		std::byte const* field_ptr;
	};

	struct StructRef {
		StructId struct_id;
		std::byte const* struct_ptr;
	};


	template<class S>
	auto type_erase_struct(S const* s) -> StructRef;


	void inspect(Kernel const& kernel, StructRef struct_ref);
	void inspect(Kernel const& kernel, FieldRef field_ref);

	std::optional<FieldRef> resolve_field_path(Kernel const& kernel, StructRef struct_ref, std::string_view field_path);

} // property


#include "property.inl"