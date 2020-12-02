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


	using StructID = std::uint32_t;
	using FieldID = std::uint32_t;


	enum class TypeId : std::uint32_t {
		Int,
		Float,
		String,
		CustomBase,
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
		FieldID id;
		std::string name;
		std::string display_name;
		std::string description;
		FieldTypeInfo field_info;
	};

	struct StructDef {
		StructID id;
		std::string name;
		std::size_t size;

		std::vector<FieldID> fields;
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
		std::deque<FieldDef> fields;
		std::unordered_map<TypeId, StructID> type_id_to_struct;

		static std::atomic<StructID> s_property_struct_alloc;

		template<class S>
		static auto get_struct_id() -> StructID;

		template<class S>
		auto struct_def_for() const -> StructDef const*;

		auto struct_def_for(StructID) const -> StructDef const*;

		template<StandardLayout S>
		auto register_struct(std::string name) -> StructBuilder<S>;
	};




	struct FieldRef {
		StructID struct_id;
		FieldID field_id;
		std::byte const* field_ptr;
	};

	struct StructRef {
		StructID struct_id;
		std::byte const* struct_ptr;
	};

	template<class S>
	auto type_erase_struct(S const* s) -> StructRef {
		return StructRef {
			Kernel::get_struct_id<S>(),
			reinterpret_cast<std::byte const*>(s),
		};
	}



	void inspect(Kernel const& kernel, StructRef struct_ref);

	// std::optional<FieldRef> lookup_field(Kernel const& kernel, StructRef struct_ref, std::string_view field);




	template<class S>
	auto Kernel::get_struct_id() -> StructID {
		static StructID s_struct_id = ++s_property_struct_alloc;
		return s_struct_id;
	}

	template<class S>
	auto Kernel::struct_def_for() const -> StructDef const* {
		return this->struct_def_for(Kernel::get_struct_id<S>());
	}

	inline auto Kernel::struct_def_for(StructID id) const -> StructDef const* {
		auto it = std::find_if(
			this->structs.begin(), this->structs.end(),
			[id] (auto&& def) { return def.id == id; }
		);

		if (it != this->structs.end()) {
			return &*it;
		} else {
			return nullptr;
		}
	}

	template<StandardLayout S>
	auto Kernel::register_struct(std::string name) -> StructBuilder<S> {
		auto const struct_id = Kernel::get_struct_id<S>();

		this->structs.push_back(StructDef {
			struct_id,
			std::move(name),
			sizeof(S),
			{}
		});

		this->type_id_to_struct.insert({type_id<S>(), struct_id});

		return StructBuilder<S> {
			this,
			&this->structs.back()
		};
	}

	template<class S>
	template<class T>
	void StructBuilder<S>::add_field(T S::* field, std::string name, std::string display_name, std::string description) {
		FieldID const field_id = this->kernel->fields.size();

		this->kernel->fields.push_back(FieldDef {
			field_id,
			std::move(name),
			std::move(display_name),
			std::move(description),

			property_field_type_info(field),
		});

		this->struct_def->fields.push_back(field_id);
	}

} // property