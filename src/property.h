#pragma once

#include "util.h"
#include "type_id.h"
#include "attribute.h"

#include <string>
#include <cstdint>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <vector>
#include <optional>
#include <string_view>
#include <new>
#include <fmt/format.h>


namespace property {


	using StructId = std::uint32_t;
	using FieldIdx = std::uint32_t;

	using EnumId = std::uint32_t;
	using EnumVariantIdx = std::uint32_t;


	namespace internal {
		struct FieldTypeInfoErased {
			virtual std::byte const* adjust_struct_ptr(std::byte const* struct_ptr) const = 0;
			virtual std::string format(std::byte const* struct_ptr) const = 0;
		};

		template<class S, class F>
		struct FieldTypeInfoErasedImpl : FieldTypeInfoErased {
			F S::* field_offset;

			FieldTypeInfoErasedImpl(F S::* p) : field_offset{p} {}

			std::byte const* adjust_struct_ptr(std::byte const* struct_ptr_raw) const final {
				auto const struct_ptr = reinterpret_cast<S const*>(struct_ptr_raw);
				auto const field_ptr = &(struct_ptr->*field_offset);
				return std::launder(reinterpret_cast<std::byte const*>(field_ptr));
			}

			std::string format(std::byte const* field_ptr_raw) const final {
				auto const field_ptr = reinterpret_cast<F const*>(field_ptr_raw);
				auto const& field_data = *std::launder(field_ptr);
				return fmt::format("{}", field_data);
			}
		};
	}


	struct FieldTypeInfo {
		template<StandardLayout S, class F>
		FieldTypeInfo(F S::* field)
			: type_id {property::type_id<F>()}
		{
			static_assert(sizeof(internal::FieldTypeInfoErasedImpl<S, F>) <= sizeof(this->offset_storage));
			new (this->offset_storage) internal::FieldTypeInfoErasedImpl<S, F> {field};
		}

		std::byte const* adjust_struct_ptr(std::byte const* struct_ptr) const {
			return get_base()->adjust_struct_ptr(struct_ptr);
		}

		// TODO: its kinda weird that this takes field_ptr but nothing else does?
		std::string format(std::byte const* field_ptr) const {
			return get_base()->format(field_ptr);
		}

		template<class F>
		F const* try_read(std::byte const* struct_ptr) {
			if (this->type_id != property::type_id<F>()) {
				return nullptr;
			}

			auto field_ptr = this->adjust_struct_ptr(struct_ptr);
			return std::launder(reinterpret_cast<F const*>(field_ptr));
		}

	private:
		alignas(internal::FieldTypeInfoErased)
		std::byte offset_storage [sizeof(internal::FieldTypeInfoErased) + sizeof(uintptr_t)];

		internal::FieldTypeInfoErased const* get_base() const {
			return std::launder(reinterpret_cast<internal::FieldTypeInfoErased const*>(this->offset_storage));
		}

	public:
		TypeId type_id;
	};



	struct FieldDef {
		FieldIdx idx;
		std::string name;
		std::string display_name;
		std::string description;
		AttributeList attributes;
		FieldTypeInfo field_info;
	};

	struct StructDef {
		StructId id;
		std::string name;
		std::size_t size;

		std::vector<FieldDef> fields;
	};

	struct EnumVariantDef {
		std::string display_name;
		int value;
	};

	struct EnumDef {
		EnumId id;
		std::string name;

		std::vector<EnumVariantDef> variants;
	};

	template<class Property>
	struct FieldBuilder {
		struct Kernel* kernel;
		FieldDef* field_def;

		template<AttributeCompatible<Property> A>
		void add_attribute(A attribute);
	};

	template<class S>
	struct StructBuilder {
		struct Kernel* kernel;
		StructDef* struct_def;

		template<class Property>
		auto add_field(Property S::*, std::string name, std::string display_name, std::string description)
			-> FieldBuilder<Property>;
	};

	template<Enum S>
	struct EnumBuilder {
		struct Kernel* kernel;
		EnumDef* enum_def;

		void add_variant(S value, std::string display_name);
	};



	struct Kernel {
		std::deque<StructDef> structs;
		std::deque<EnumDef> enums;
		std::unordered_map<TypeId, StructId> type_id_to_struct;
		std::unordered_map<TypeId, StructId> type_id_to_enum;

		static std::atomic<StructId> s_registered_type_id_alloc;

		template<class S>
		static auto get_struct_id() -> StructId;
		template<Enum E>
		static auto get_enum_id() -> EnumId;

		template<class S>
		auto struct_def_for() const -> StructDef const*;

		auto struct_def_for(StructId) const -> StructDef const*;

		auto struct_id_from_type_id(TypeId) const -> std::optional<StructId>;


		template<Enum E>
		auto enum_def_for() const -> EnumDef const*;

		auto enum_def_for(EnumId) const -> EnumDef const*;

		auto enum_id_from_type_id(TypeId) const -> std::optional<EnumId>;

		template<StandardLayout S>
		auto register_struct(std::string name) -> StructBuilder<S>;

		template<Enum E>
		auto register_enum(std::string name) -> EnumBuilder<E>;
	};




	struct FieldRef {
		StructId struct_id;
		FieldDef const* field_def;
		std::byte const* field_ptr;

		template<class A> 
		bool has_attribute() const;

		template<class A> 
		A const* get_attribute() const;
	};

	struct StructRef {
		StructDef const* struct_def;
		std::byte const* struct_ptr;

		// FieldRef
	};


	template<class S>
	auto type_erase_struct(Kernel const& kernel, S const* s) -> StructRef;


	void inspect(Kernel const& kernel, StructRef struct_ref);
	void inspect(Kernel const& kernel, FieldRef field_ref);

	std::optional<FieldRef> resolve_field_path(Kernel const& kernel, StructRef struct_ref, std::string_view field_path);

} // property


#include "property.inl"