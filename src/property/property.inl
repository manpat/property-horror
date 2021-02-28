
namespace property {
	template<class S>
	auto Kernel::get_struct_id() const -> StructId {
		static StructId s_struct_id = ++this->registered_type_id_alloc;
		return s_struct_id;
	}


	template<Enum E>
	auto Kernel::get_enum_id() const -> EnumId {
		static EnumId s_enum_id = ++this->registered_type_id_alloc;
		return s_enum_id;
	}


	template<class S>
	auto Kernel::struct_def_for() const -> StructDef const* {
		return this->struct_def_for(Kernel::get_struct_id<S>());
	}


	template<Enum E>
	auto Kernel::enum_def_for() const -> EnumDef const* {
		return this->enum_def_for(Kernel::get_enum_id<E>());
	}


	template<class A>
	bool FieldRef::has_attribute() const { return this->field_def->attributes.has_attribute<A>(); }

	template<class A>
	A const* FieldRef::get_attribute() const { return this->field_def->attributes.get_attribute<A>(); }


	
	template<class S>
	auto type_erase_struct(Kernel const& kernel, S const* s) -> StructRef {
		return StructRef {
			kernel.struct_def_for<S>(),
			reinterpret_cast<std::byte const*>(s),
		};
	}

} // property