#pragma once

#include "property/util.h"
#include "property/type_id.h"
#include "property/attribute.h"

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


	template<class Property> 
	struct ListLikePropertyTrait : std::false_type {};

	template<class Property>
	concept ListLikeProperty = ListLikePropertyTrait<Property>::value;


	namespace internal {
		struct FieldTypeInfoErased {
			virtual std::byte const* adjust_struct_ptr(std::byte const* struct_ptr) const = 0;
			virtual std::string format(std::byte const* struct_ptr) const = 0;
		};

		template<class S, class F>
		struct FieldTypeInfoErasedImpl final : FieldTypeInfoErased {
			F S::* field_offset;

			FieldTypeInfoErasedImpl(F S::* p) : field_offset{p} {}

			std::byte const* adjust_struct_ptr(std::byte const* struct_ptr_raw) const final {
				auto const struct_ptr = std::launder(reinterpret_cast<S const*>(struct_ptr_raw));
				auto const field_ptr = &(struct_ptr->*field_offset);
				return reinterpret_cast<std::byte const*>(field_ptr);
			}

			std::string format(std::byte const* field_ptr_raw) const final {
				auto const field_ptr = reinterpret_cast<F const*>(field_ptr_raw);
				auto const& field_data = *std::launder(field_ptr);
				return fmt::format("{}", field_data);
			}
		};

		template<class S, ListLikeProperty F>
		struct FieldTypeInfoErasedImpl<S, F> final : FieldTypeInfoErased {
			F S::* field_offset;

			FieldTypeInfoErasedImpl(F S::* p) : field_offset{p} {}

			std::byte const* adjust_struct_ptr(std::byte const* struct_ptr_raw) const final {
				auto const struct_ptr = std::launder(reinterpret_cast<S const*>(struct_ptr_raw));
				auto const field_ptr = &(struct_ptr->*field_offset);
				return reinterpret_cast<std::byte const*>(field_ptr);
			}

			std::string format(std::byte const* /*field_ptr_raw*/) const final {
				// auto const field_ptr = reinterpret_cast<F const*>(field_ptr_raw);
				// auto const& field_data = *std::launder(field_ptr);
				return fmt::format("<some list>");
			}
		};
	}


	struct FieldTypeInfo {
		template<StandardLayout S, class F>
		FieldTypeInfo(F S::* field)
			: type_id {property::type_id<F>()}
		{
			static_assert(sizeof(internal::FieldTypeInfoErasedImpl<S, F>) <= sizeof(this->offset_storage));
			static_assert(alignof(internal::FieldTypeInfoErasedImpl<S, F>) <= alignof(internal::FieldTypeInfoErased));
			new (this->offset_storage) internal::FieldTypeInfoErasedImpl<S, F> {field};
		}

		std::byte const* adjust_struct_ptr(std::byte const* struct_ptr) const;
		std::byte* adjust_struct_ptr(std::byte* struct_ptr) const;

		// TODO: its kinda weird that this takes field_ptr but nothing else does?
		std::string format(std::byte const* field_ptr) const;

		template<class F>
		bool matches_type() const;

	private:
		alignas(internal::FieldTypeInfoErased)
		std::byte offset_storage [sizeof(internal::FieldTypeInfoErased) + sizeof(uintptr_t)];

		internal::FieldTypeInfoErased const* get_base() const;

	public:
		TypeId type_id;
	};



	struct ListFieldInfo {
		TypeId element_type_id;

		std::byte const* get_element_ptr(std::byte const* field_ptr, std::size_t) const;
		std::size_t get_size(std::byte const* field_ptr) const;

		// TODO: how to modify?
	};



	// Core types
	struct FieldDef {
		FieldIdx idx;
		std::string name;
		std::string display_name;
		std::string description;
		AttributeList attributes;
		FieldTypeInfo field_info;
		// TODO: std::optional<ListFieldInfo> list_info;
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

	
	struct Kernel {
		std::deque<StructDef> structs;
		std::deque<EnumDef> enums;
		std::unordered_map<TypeId, StructId> type_id_to_struct;
		std::unordered_map<TypeId, StructId> type_id_to_enum;

		mutable std::atomic<StructId> registered_type_id_alloc;

		template<class S>
		auto get_struct_id() const -> StructId;
		template<Enum E>
		auto get_enum_id() const -> EnumId;

		template<class S>
		auto struct_def_for() const -> StructDef const*;
		auto struct_def_for(StructId) const -> StructDef const*;
		auto struct_id_from_type_id(TypeId) const -> std::optional<StructId>;


		template<Enum E>
		auto enum_def_for() const -> EnumDef const*;
		auto enum_def_for(EnumId) const -> EnumDef const*;
		auto enum_id_from_type_id(TypeId) const -> std::optional<EnumId>;
	};




	struct FieldRef {
		StructId struct_id;
		FieldDef const* field_def;
		std::byte const* field_ptr;


		template<class F>
		F const* try_read() const;


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


#include "property/property.inl"