
namespace property {

	template<class S>
	auto type_erase_struct(Kernel const& kernel, S const* s) -> StructRef {
		return StructRef {
			kernel.struct_def_for<S>(),
			reinterpret_cast<std::byte const*>(s),
		};
	}

	template<class S>
	auto Kernel::get_struct_id() -> StructId {
		static StructId s_struct_id = ++s_registered_type_id_alloc;
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
	auto Kernel::get_enum_id() -> EnumId {
		static EnumId s_enum_id = ++s_registered_type_id_alloc;
		return s_enum_id;
	}

	template<Enum E>
	auto Kernel::enum_def_for() const -> EnumDef const* {
		return this->enum_def_for(Kernel::get_enum_id<E>());
	}


	template<Enum E>
	auto Kernel::register_enum(std::string name) -> EnumBuilder<E> {
		auto const enum_id = Kernel::get_enum_id<E>();

		this->enums.push_back(EnumDef {
			enum_id,
			std::move(name),
			{}
		});

		this->type_id_to_enum.insert({type_id<E>(), enum_id});

		return EnumBuilder<E> {
			this,
			&this->enums.back()
		};
	}

	template<Enum E>
	void EnumBuilder<E>::add_variant(E value, std::string display_name) {
		// TODO: static_assert value can be stored in an int

		this->enum_def->variants.push_back(EnumVariantDef {
			std::move(display_name),
			static_cast<int>(value),
		});
	}


	template<class A>
	bool FieldRef::has_attribute() const { return this->field_def->attributes.has_attribute<A>(); }

	template<class A>
	A const* FieldRef::get_attribute() const { return this->field_def->attributes.get_attribute<A>(); }

} // property