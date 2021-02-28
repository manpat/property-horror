#include "property/property.h"

namespace property {

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


	template<StandardLayout S>
	auto register_struct(Kernel&, std::string name) -> StructBuilder<S>;

	template<Enum E>
	auto register_enum(Kernel&, std::string name) -> EnumBuilder<E>;
}

#include "property/registration.inl"