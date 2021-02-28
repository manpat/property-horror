#include "property/util.h"
#include "property/property.h"
#include "property/registration.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <cstdint>
#include <memory>
#include <algorithm>



struct Blah {
	float meh;
};

enum class Wamp {A, B, C};

struct Foo {
	std::string whatever;
	int a_field;
	Blah blah;
	Wamp womp;
};


std::string format_debug(Foo const& foo) {
	return fmt::format(
		"Foo<whatever: '{}', a_field: {}, blah: {}>",
		foo.whatever,
		foo.a_field,
		foo.blah
	);
};

std::string format_debug(Blah const& blah) {
	return fmt::format(
		"Blah<meh: {}>",
		blah.meh
	);
};

std::string format_debug(Wamp wamp) {
	switch (wamp) {
		case Wamp::A: return "Wamp::A";
		case Wamp::B: return "Wamp::B";
		case Wamp::C: return "Wamp::C";
		default: return "<unknown wamp>";
	}
};



struct HiddenAttribute {};

template<class T>
struct RangeAttribute { T min; T max; };

std::string format_debug(HiddenAttribute) { return "HiddenAttribute"; };

template<class T>
std::string format_debug(RangeAttribute<T> attr) {
	return fmt::format("RangeAttribute[{}, {}]", attr.min, attr.max);
};


template<class T>
struct property::AttributeCompatibleTrait<HiddenAttribute, T> : std::true_type {};

template<class T>
struct property::AttributeCompatibleTrait<RangeAttribute<T>, T> : std::true_type {};



int main() {
	property::Kernel kernel {};

	auto struct_foo = register_struct<Foo>(kernel, "Foo");
	struct_foo.add_field(&Foo::whatever, "whatever", "Whatever", "It's a whatever");
	struct_foo.add_field(&Foo::a_field, "a_field", "A Field", "It's a field")
		.add_attribute(RangeAttribute<int> {-3, 7});
	struct_foo.add_field(&Foo::blah, "a_blah", "The Blah", "You know ;)");
	struct_foo.add_field(&Foo::womp, "womp", "Some kinda thing", "Womp womp");

	auto struct_blah = register_struct<Blah>(kernel, "Blah");
	auto field = struct_blah.add_field(&Blah::meh, "meh", "Meh", "Thingo the doohicky");
	field.add_attribute(HiddenAttribute {});
	field.add_attribute(RangeAttribute<float> {1.0f, 4.0f});

	// field.add_attribute(FilepathAttribute {
	// 	default_directory,
	// 	OpenMode::Saving,
	// 	"DOOM WAD (*.wad)"
	// });

	// field.add_attribute(HiddenAttribute {});
	// field.add_attribute(IntAttribute {3, 5});

	// // ....

	// field_def.get_attribute<FilepathAttribute>();

	auto enum_wamp = register_enum<Wamp>(kernel, "Wamp");
	enum_wamp.add_variant(Wamp::A, "A");
	enum_wamp.add_variant(Wamp::B, "B");
	enum_wamp.add_variant(Wamp::C, "C");



	Foo foo {"aaa", 123, Blah{5.0f}, Wamp::B};

	property::StructRef foo_ref = property::type_erase_struct(kernel, &foo);
	inspect(kernel, foo_ref);

	fmt::print("\n--- property_search ---\n");

	if (auto field_ref = resolve_field_path(kernel, foo_ref, "a_blah/meh")) {

		if (field_ref->has_attribute<HiddenAttribute>()) {
			fmt::print("has HiddenAttribute!\n");
		}

		if (auto attr = field_ref->get_attribute<RangeAttribute<float>>()) {
			fmt::print("has RangeAttribute! {}, {}\n", attr->min, attr->max);
		}

		inspect(kernel, *field_ref);
	} else {
		fmt::print("no :(\n");
	}
}

