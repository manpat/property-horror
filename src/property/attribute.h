#pragma once

#include "property/type_id.h"
#include "property/util.h"

#include <vector>
#include <memory>

namespace property {

	template<class Attribute, class Property> 
	struct AttributeCompatibleTrait : std::false_type {};

	template<class Attribute, class Property>
	concept AttributeCompatible = AttributeCompatibleTrait<Attribute, Property>::value
		&& DebugFormattable<Attribute>;


	namespace internal {
		struct AttributeBase {
			TypeId type_id;

			AttributeBase(TypeId tid) : type_id{tid} {}

			virtual ~AttributeBase() {}
			virtual std::string format() const = 0;
		};

		template<class A>
		struct AttributeImpl : AttributeBase {
			A attribute;

			AttributeImpl(A a)
				: AttributeBase{property::type_id<A>()}
				, attribute{std::move(a)}
			{}

			std::string format() const final {
				return format_debug(attribute);
			}
		};
	}

	struct AttributeRef {
		TypeId type_id;
		internal::AttributeBase const* data;
	};


	struct AttributeIterator : std::vector<std::unique_ptr<internal::AttributeBase>>::const_iterator {
		using base = std::vector<std::unique_ptr<internal::AttributeBase>>::const_iterator;
		using value_type = AttributeRef;

		AttributeRef operator*() const {
			auto base_ptr = base::operator*().get();

			return AttributeRef {
				base_ptr->type_id,
				base_ptr,
			};
		}
	};


	struct AttributeList {
		std::vector<std::unique_ptr<internal::AttributeBase>> attributes;

		template<class A>
		void add_attribute(A a) {
			internal::AttributeImpl<A> impl {std::move(a)};

			auto attr = std::make_unique<internal::AttributeImpl<A>>(std::move(impl));
			attributes.push_back(std::move(attr));
		}

		template<class A>
		A const* get_attribute() const {
			auto const type_id = property::type_id<A>();

			auto it = std::find_if(
				this->attributes.begin(), this->attributes.end(),
				[type_id] (auto&& attribute) {
					return type_id == attribute->type_id;
				}
			);

			if (it == this->attributes.end()) {
				return nullptr;
			}

			auto const impl_ptr = static_cast<internal::AttributeImpl<A> const*>(it->get());
			return &impl_ptr->attribute;
		}

		template<class A>
		bool has_attribute() const {
			return (this->get_attribute<A>() != nullptr);
		}


		auto size() const { return attributes.size(); }
		auto empty() const { return attributes.empty(); }

		auto begin() const { return AttributeIterator{attributes.begin()}; }
		auto end() const { return AttributeIterator{attributes.end()}; }
	};

} // property