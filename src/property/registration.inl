namespace property {
	template<StandardLayout S>
	auto register_struct(Kernel& kernel, std::string name) -> StructBuilder<S> {
		auto const struct_id = kernel.get_struct_id<S>();

		kernel.structs.push_back(StructDef {
			struct_id,
			std::move(name),
			sizeof(S),
			{}
		});

		kernel.type_id_to_struct.insert({type_id<S>(), struct_id});

		return StructBuilder<S> {
			&kernel,
			&kernel.structs.back()
		};
	}


	template<Enum E>
	auto register_enum(Kernel& kernel, std::string name) -> EnumBuilder<E> {
		auto const enum_id = kernel.get_enum_id<E>();

		kernel.enums.push_back(EnumDef {
			enum_id,
			std::move(name),
			{}
		});

		kernel.type_id_to_enum.insert({type_id<E>(), enum_id});

		return EnumBuilder<E> {
			&kernel,
			&kernel.enums.back()
		};
	}


	template<class S>
	template<class Property>
	auto StructBuilder<S>::add_field(Property S::* field, std::string name, std::string display_name,
		std::string description) -> FieldBuilder<Property>
	{
		FieldIdx const field_idx = this->struct_def->fields.size();

		this->struct_def->fields.push_back(FieldDef {
			field_idx,
			std::move(name),
			std::move(display_name),
			std::move(description),

			{},
			FieldTypeInfo { field },
		});

		return FieldBuilder<Property> {
			this->kernel,
			&this->struct_def->fields.back(),
		};
	}


	template<class Property>
	template<AttributeCompatible<Property> A>
	void FieldBuilder<Property>::add_attribute(A attribute) {
		this->field_def->attributes.add_attribute(std::move(attribute));
	}


	template<Enum E>
	void EnumBuilder<E>::add_variant(E value, std::string display_name) {
		// TODO: static_assert value can be stored in an int

		this->enum_def->variants.push_back(EnumVariantDef {
			std::move(display_name),
			static_cast<int>(value),
		});
	}
}