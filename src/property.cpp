#include "property.h"

#include <fmt/core.h>

namespace property {

	std::atomic<StructId> Kernel::s_property_struct_alloc {};


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

	static void inspect_impl(Kernel const& kernel, StructRef struct_ref, int indent);

	static void inspect_impl(Kernel const& kernel, FieldRef field_ref, int indent) {
		auto const [struct_id, field_def, field_ptr] = field_ref;
		auto const field_str = field_def->field_info.format(field_ptr);

		fmt::print("{:{}}field \"{}\" ({})\n", "", indent*4, field_def->display_name, field_def->name);
		fmt::print("{:{}}description: {}\n", "", indent*4, field_def->description);
		fmt::print("{:{}}type: {}\n", "", indent*4, field_def->field_info.type_id);
		fmt::print("{:{}}offset: {}B\n", "", indent*4, field_def->field_info.offset);
		fmt::print("{:{}}contents: {}\n\n", "", indent*4, field_str);

		if (auto child_struct_id = kernel.struct_id_from_type_id(field_def->field_info.type_id)) {
			inspect_impl(kernel, StructRef{*child_struct_id, field_ptr}, indent+1);
		}
	}

	static void inspect_impl(Kernel const& kernel, StructRef struct_ref, int indent) {
		auto const [struct_id, struct_ptr] = struct_ref;
		auto const struct_def = kernel.struct_def_for(struct_id);
		if (!struct_def) {
			fmt::print("{:{}}struct <unregistered type>\n", "", indent*4);
			return;
		}

		fmt::print("{:{}}struct \"{}\" (id: {}, size: {}):\n", "", indent*4, struct_def->name, struct_def->id, struct_def->size);

		for (FieldIdx field_idx = 0; field_idx < struct_def->fields.size(); field_idx++) {
			auto const& field_def = struct_def->fields[field_idx];
			auto const field_data = field_def.field_info.adjust_struct_ptr(struct_ptr);
			inspect_impl(kernel, FieldRef{struct_id, &field_def, field_data}, indent+1);
		}
	}

	void inspect(Kernel const& kernel, StructRef struct_ref) {
		inspect_impl(kernel, struct_ref, 0);
	}

	void inspect(Kernel const& kernel, FieldRef field_ref) {
		inspect_impl(kernel, field_ref, 0);
	}




	std::optional<FieldRef> resolve_field_path(Kernel const& kernel, StructRef struct_ref, std::string_view field_path) {
		if (field_path.empty()) {
			return std::nullopt;
		}

		auto const [struct_id, struct_ptr] = struct_ref;
		StructId owning_struct_id = struct_id;
		FieldDef const* field_def = nullptr;
		std::byte const* field_ptr = struct_ptr;

		while (true) {
			auto const struct_def = kernel.struct_def_for(owning_struct_id);
			if (!struct_def) {
				return std::nullopt;
			}

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
					owning_struct_id,
					field_def,
					field_ptr,
				};
			}

			if (auto struct_id = kernel.struct_id_from_type_id(field_def->field_info.type_id)) {
				owning_struct_id = *struct_id;
			} else {
				return std::nullopt;
			}
		}
	}

}