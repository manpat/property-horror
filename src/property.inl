
namespace property {

	template<class S>
	auto type_erase_struct(S const* s) -> StructRef {
		return StructRef {
			Kernel::get_struct_id<S>(),
			reinterpret_cast<std::byte const*>(s),
		};
	}

	template<class S>
	auto Kernel::get_struct_id() -> StructId {
		static StructId s_struct_id = ++s_property_struct_alloc;
		return s_struct_id;
	}

	template<class S>
	auto Kernel::struct_def_for() const -> StructDef const* {
		return this->struct_def_for(Kernel::get_struct_id<S>());
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
		FieldIdx const field_idx = this->struct_def->fields.size();

		this->struct_def->fields.push_back(FieldDef {
			field_idx,
			std::move(name),
			std::move(display_name),
			std::move(description),

			property_field_type_info(field),
		});
	}

} // property