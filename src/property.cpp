#include "property.h"

namespace property {

	std::atomic<StructID> Kernel::s_property_struct_alloc {};


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


	static void inspect_impl(Kernel const& kernel, StructRef struct_ref, int indent) {
		auto const [struct_id, struct_ptr] = struct_ref;
		auto const struct_def = kernel.struct_def_for(struct_id);
		if (!struct_def) {
			fmt::print("struct <unregistered type>\n");
			return;
		}

		fmt::print("{:{}}struct \"{}\" (id: {}, size: {}):\n", "", indent*4, struct_def->name, struct_def->id, struct_def->size);

		for (auto field_id : struct_def->fields) {
			auto const& field_def = kernel.fields[field_id];

			fmt::print("{:{}}  - field \"{}\" ({})\n", "", indent*4, field_def.display_name, field_def.name);
			fmt::print("{:{}}    description: {}\n", "", indent*4, field_def.description);
			fmt::print("{:{}}    type: {}\n", "", indent*4, field_def.field_info.type_id);
			fmt::print("{:{}}    offset: {}B\n", "", indent*4, field_def.field_info.offset);

			auto const field_ptr = field_def.field_info.adjust_struct_ptr(struct_ptr);
			auto const field_str = field_def.field_info.format(field_ptr);

			fmt::print("{:{}}    contents: {}\n\n", "", indent*4, field_str);

			auto child_struct_id = kernel.type_id_to_struct.find(field_def.field_info.type_id);
			if (child_struct_id != kernel.type_id_to_struct.end()) {
				inspect_impl(kernel, StructRef{child_struct_id->second, field_ptr}, indent+1);
			}
		}
	}

	void inspect(Kernel const& kernel, StructRef struct_ref) {
		inspect_impl(kernel, struct_ref, 0);
	}



	// std::optional<FieldRef> lookup_field(Kernel const& kernel, StructRef struct_ref, std::string_view field) {
	// 	auto const [struct_id, struct_ptr] = struct_ref;
	// 	auto const struct_def = kernel.struct_def_for(struct_id);
	// }

}