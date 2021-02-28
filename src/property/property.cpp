#include "property/property.h"

#include <fmt/core.h>

namespace property {

	std::byte const* FieldTypeInfo::adjust_struct_ptr(std::byte const* struct_ptr) const {
		return get_base()->adjust_struct_ptr(struct_ptr);
	}


	std::string FieldTypeInfo::format(std::byte const* field_ptr) const {
		return get_base()->format(field_ptr);
	}


	template<class F>
	F const* FieldTypeInfo::try_read(std::byte const* struct_ptr) {
		if (this->type_id != property::type_id<F>()) {
			return nullptr;
		}

		auto field_ptr = this->adjust_struct_ptr(struct_ptr);
		return std::launder(reinterpret_cast<F const*>(field_ptr));
	}
	

	internal::FieldTypeInfoErased const* FieldTypeInfo::get_base() const {
		return std::launder(reinterpret_cast<internal::FieldTypeInfoErased const*>(this->offset_storage));
	}



	auto Kernel::struct_def_for(StructId id) const -> StructDef const* {
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


	auto Kernel::struct_id_from_type_id(TypeId id) const -> std::optional<StructId> {
		auto struct_id_it = this->type_id_to_struct.find(id);
		if (struct_id_it != this->type_id_to_struct.end()) {
			return struct_id_it->second;
		} else {
			return std::nullopt;
		}
	}


	auto Kernel::enum_def_for(EnumId id) const -> EnumDef const* {
		auto it = std::find_if(
			this->enums.begin(), this->enums.end(),
			[id] (auto&& def) { return def.id == id; }
		);

		if (it != this->enums.end()) {
			return &*it;
		} else {
			return nullptr;
		}
	}


	auto Kernel::enum_id_from_type_id(TypeId id) const -> std::optional<EnumId> {
		auto enum_id_it = this->type_id_to_enum.find(id);
		if (enum_id_it != this->type_id_to_enum.end()) {
			return enum_id_it->second;
		} else {
			return std::nullopt;
		}
	}



	static void inspect_impl(Kernel const& kernel, StructRef struct_ref, int indent);

	static void inspect_impl(Kernel const& kernel, FieldRef field_ref, int indent) {
		auto const [struct_id, field_def, field_ptr] = field_ref;
		auto const field_str = field_def->field_info.format(field_ptr);

		fmt::print("{:{}}field \"{}\" ({})\n", "", indent*4, field_def->display_name, field_def->name);
		fmt::print("{:{}}description: {}\n", "", indent*4, field_def->description);
		fmt::print("{:{}}type: {}\n", "", indent*4, field_def->field_info.type_id);
		fmt::print("{:{}}contents: {}\n", "", indent*4, field_str);


		if (!field_def->attributes.empty()) {
			fmt::print("{:{}}attributes: ", "", indent*4);

			for (auto attribute_ref : field_def->attributes) {
				fmt::print("{}, ", attribute_ref.data->format());
			}

			fmt::print("\n");
		}

		if (auto child_struct_id = kernel.struct_id_from_type_id(field_def->field_info.type_id)) {
			auto const struct_def = kernel.struct_def_for(*child_struct_id);
			inspect_impl(kernel, StructRef{struct_def, field_ptr}, indent+1);
		}
		if (auto child_enum_id = kernel.enum_id_from_type_id(field_def->field_info.type_id)) {
			auto const enum_def = kernel.enum_def_for(*child_enum_id);
			fmt::print("{:{}}enum \"{}\" ", "", (indent+1)*4, enum_def->name);

			for (auto const& variant_def : enum_def->variants) {
				fmt::print("(\"{}\", {}) ", variant_def.display_name, variant_def.value);
			}

			fmt::print("\n");
		}

		fmt::print("\n");
	}

	static void inspect_impl(Kernel const& kernel, StructRef struct_ref, int indent) {
		auto const [struct_def, struct_ptr] = struct_ref;

		fmt::print("{:{}}struct \"{}\" (id: {}, size: {}):\n", "", indent*4, struct_def->name, struct_def->id, struct_def->size);

		for (FieldIdx field_idx = 0; field_idx < struct_def->fields.size(); field_idx++) {
			auto const& field_def = struct_def->fields[field_idx];
			auto const field_data = field_def.field_info.adjust_struct_ptr(struct_ptr);
			inspect_impl(kernel, FieldRef{struct_def->id, &field_def, field_data}, indent+1);
		}
	}

	void inspect(Kernel const& kernel, StructRef struct_ref) {
		inspect_impl(kernel, struct_ref, 0);
	}

	void inspect(Kernel const& kernel, FieldRef field_ref) {
		inspect_impl(kernel, field_ref, 0);
	}



	// TODO: does this need to take a StructRef? could this instead take a StructDef?
	std::optional<FieldRef> resolve_field_path(Kernel const& kernel, StructRef struct_ref, std::string_view field_path) {
		if (field_path.empty()) {
			return std::nullopt;
		}

		auto const [original_struct_def, struct_ptr] = struct_ref;
		StructDef const* struct_def = original_struct_def;
		FieldDef const* field_def = nullptr;
		std::byte const* field_ptr = struct_ptr;

		while (true) {
			auto const [path_segment, tail] = split(field_path, '/');
			field_path = tail;

			auto const field_it = std::find_if(
				struct_def->fields.begin(), struct_def->fields.end(),
				[path_segment=path_segment] (auto&& field_def) {
					return field_def.name == path_segment;
				}
			);

			if (field_it == struct_def->fields.end()) {
				return std::nullopt;
			}


			field_def = &*field_it;
			field_ptr = field_it->field_info.adjust_struct_ptr(field_ptr);

			if (field_path.empty()) {
				return FieldRef {
					struct_def->id,
					field_def,
					field_ptr,
				};
			}

			if (auto struct_id = kernel.struct_id_from_type_id(field_def->field_info.type_id)) {
				struct_def = kernel.struct_def_for(*struct_id);
			} else {
				return std::nullopt;
			}
		}
	}

}