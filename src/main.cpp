#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <deque>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>

using PropertyStructID = std::uint32_t;
using PropertyFieldID = std::uint32_t;

// static constexpr PropertyStructID INVALID_STRUCT_ID = ~PropertyStructID(0);

template<class...>
[[maybe_unused]] static constexpr bool false_v = false;

// template<class T>
// struct PropertyFieldTypeTrait {
// 	static_assert(false_v<T>, "trying to use unregistered type");
// };


template <typename S, typename T>
constexpr uintptr_t member_offsetof(T S::* member) {
	static_assert(std::is_standard_layout_v<S>, "only standard layout types are supported");

	alignas(S) char container_space[sizeof(S)] = {};
	const auto fake_container = reinterpret_cast<S*>(container_space);
	return reinterpret_cast<uintptr_t>(&(fake_container->*member)) - reinterpret_cast<uintptr_t>(fake_container);
}



struct FieldTypeInfo {
	void (*format_to) (fmt::memory_buffer&, char const*);
};

template<class T>
FieldTypeInfo property_field_type_info() {
	// static_assert(false_v<T>, "trying to use unregistered type");
	return {
		[] (fmt::memory_buffer& out, char const* data) {
			fmt::format_to(out, "{}", *reinterpret_cast<T const*>(data));
		}
	};
}

// template<>
// FieldTypeInfo property_field_type_info<int>() { return {}; }
// template<>
// FieldTypeInfo property_field_type_info<float>() { return {}; }
// template<>
// FieldTypeInfo property_field_type_info<std::string>() {
// 	return default_property_field_type_info<std::string>();
// }


struct PropertyFieldDef {
	PropertyFieldID id;
	std::string name;
	std::string display_name;
	std::string description;
	FieldTypeInfo field_info;
	uintptr_t offset;
};

struct PropertyStructDef {
	PropertyStructID id;
	std::string name;
	std::size_t size;

	std::vector<PropertyFieldID> fields;
};

template<class S>
struct PropertyStructBuilder {
	struct PropertyKernel* kernel;
	PropertyStructDef* struct_def;

	template<class T>
	void add_field(T S::*, std::string name, std::string display_name, std::string description);
};

struct PropertyKernel {
	std::deque<PropertyStructDef> structs;
	std::deque<PropertyFieldDef> fields;

	static std::atomic<PropertyStructID> s_property_struct_alloc;

	template<class S>
	static auto get_struct_id() -> PropertyStructID;

	auto struct_def_for(PropertyStructID) const -> PropertyStructDef const*;

	template<class S>
	auto register_struct(std::string name) -> PropertyStructBuilder<S>;
};

std::atomic<PropertyStructID> PropertyKernel::s_property_struct_alloc {};

template<class S>
auto PropertyKernel::get_struct_id() -> PropertyStructID {
	static PropertyStructID s_struct_id = ++s_property_struct_alloc;
	return s_struct_id;
}

auto PropertyKernel::struct_def_for(PropertyStructID id) const -> PropertyStructDef const* {
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

template<class S>
auto PropertyKernel::register_struct(std::string name) -> PropertyStructBuilder<S> {
	static_assert(std::is_standard_layout_v<S>, "only standard layout types are supported");

	this->structs.push_back(PropertyStructDef {
		PropertyKernel::get_struct_id<S>(),
		std::move(name),
		sizeof(S),
		{}
	});

	return PropertyStructBuilder<S> {
		this,
		&this->structs.back()
	};
}

template<class S>
template<class T>
void PropertyStructBuilder<S>::add_field(T S::* field, std::string name, std::string display_name, std::string description) {
	PropertyFieldID const field_id = this->kernel->fields.size();

	this->kernel->fields.push_back(PropertyFieldDef {
		field_id,
		std::move(name),
		std::move(display_name),
		std::move(description),

		property_field_type_info<T>(),
		member_offsetof(field),
	});

	this->struct_def->fields.push_back(field_id);
}




struct Foo {
	std::string whatever;
	int a_field;
	float blah;
};

struct Blah {
	int whatever;
};



template<class S>
void inspect(PropertyKernel const& kernel, S const& data) {
	auto const struct_def = kernel.struct_def_for(kernel.get_struct_id<S>());
	if (!struct_def) {
		fmt::print("<unregistered type>\n");
		return;
	}

	fmt::print("struct \"{}\" (id: {}, size: {}):\n", struct_def->name, struct_def->id, struct_def->size);
	for (auto field_id : struct_def->fields) {
		auto const& field_def = kernel.fields[field_id];

		fmt::print("  - field \"{}\" ({})\n", field_def.display_name, field_def.name);
		fmt::print("    offset: {}B\n", field_def.offset);

		fmt::memory_buffer buf {};

		auto const field_ptr = reinterpret_cast<char const*>(&data) + field_def.offset;
		field_def.field_info.format_to(buf, field_ptr);

		fmt::print("    contents: {}\n", to_string(buf));
	}
}



int main() {
	PropertyKernel kernel {};

	auto struct_foo = kernel.register_struct<Foo>("Foo");
	struct_foo.add_field(&Foo::whatever, "whatever", "Whatever", "It's a whatever");
	struct_foo.add_field(&Foo::a_field, "a_field", "A Field", "It's a field");
	struct_foo.add_field(&Foo::blah, "blah", "Blah", "You know ;)");

	auto struct_blah = kernel.register_struct<Blah>("Blah");
	struct_blah.add_field(&Blah::whatever, "whatever", "Whatever", "Thingo the doohicky");

	// for (auto const& struct_def : kernel.structs) {
	// 	fmt::print("struct \"{}\" (id: {}, size: {}):\n", struct_def.name, struct_def.id, struct_def.size);
	// 	for (auto field_id : struct_def.fields) {
	// 		auto const& field_def = kernel.fields[field_id];

	// 		fmt::print("  - field \"{}\" ({})\n", field_def.display_name, field_def.name);
	// 		fmt::print("    offset: {}B\n", field_def.offset);
	// 	}
	// }

	Foo foo {"aaa", 123, 5.0f};
	inspect(kernel, foo);
}