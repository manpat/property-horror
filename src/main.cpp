#include "util.h"
#include "property.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <cstdint>
#include <memory>
#include <algorithm>



struct Blah {
	float meh;
};

struct Foo {
	std::string whatever;
	int a_field;
	Blah blah;
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




int main() {
	property::Kernel kernel {};

	auto struct_foo = kernel.register_struct<Foo>("Foo");
	struct_foo.add_field(&Foo::whatever, "whatever", "Whatever", "It's a whatever");
	struct_foo.add_field(&Foo::a_field, "a_field", "A Field", "It's a field");
	struct_foo.add_field(&Foo::blah, "a_blah", "The Blah", "You know ;)");

	auto struct_blah = kernel.register_struct<Blah>("Blah");
	struct_blah.add_field(&Blah::meh, "meh", "Meh", "Thingo the doohicky");

	Foo foo {"aaa", 123, Blah{5.0f}};

	property::StructRef foo_ref = property::type_erase_struct(&foo);
	inspect(kernel, foo_ref);

	fmt::print("\n--- property_search ---\n");

	if (auto field_ref = resolve_field_path(kernel, foo_ref, "a_blah/meh")) {
		inspect(kernel, *field_ref);
	} else {
		fmt::print("no :(\n");
	}
}

