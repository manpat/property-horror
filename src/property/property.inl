
namespace property {
	template<class F>
	bool FieldTypeInfo::matches_type() const {
		return this->type_id == property::type_id<F>();
	}

	// template<class F>
	// F const* FieldTypeInfo::try_read(std::byte const* struct_ptr) const {
	// 	if (this->matches_type<F>()) {
	// 		return nullptr;
	// 	}

	// 	auto field_ptr = this->adjust_struct_ptr(struct_ptr);
	// 	return std::launder(reinterpret_cast<F const*>(field_ptr));
	// }


	// template<class F>
	// bool FieldTypeInfo::try_write(std::byte* struct_ptr, F value) const {
	// 	if (this->matches_type<F>()) {
	// 		return false;
	// 	}

	// 	auto field_ptr = this->adjust_struct_ptr(struct_ptr);
	// 	auto field = std::launder(reinterpret_cast<F*>(field_ptr));
	// 	*field = std::move(value);

	// 	return true;
	// }


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


	template<class F>
	F const* FieldRef::try_read() const {
		if (!this->field_def->field_info.matches_type<F>()) {
			return nullptr;
		}

		return std::launder(reinterpret_cast<F const*>(this->field_ptr));
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