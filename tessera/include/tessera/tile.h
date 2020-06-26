#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <variant>
#include <optional>

namespace tess {
	class tile_impl;

	class nil_property
	{};

	using property_value = std::variant<double, std::string, bool, nil_property>;

	namespace details {
		template<typename U>
		class property_container {
		public:
			template<typename T>
			std::optional<T> get_prop(const std::string& prop) const {
				auto p = static_cast<const U*>(this)->get_property_variant(prop);
				if (std::holds_alternative<T>(p))
					return std::get<T>(p);
				else
					return std::nullopt;
			}
		};
	}

	class vertex : public details::property_container<vertex> {
		friend class tessera_impl;
	public:
		std::tuple<double, double> pos() const;
		property_value get_property_variant(const std::string& prop) const;
		class impl_type;
	private:
		impl_type* impl_;
	};

	class edge : public details::property_container<edge>  {
		friend class tessera_impl;
	public:
		const vertex& u() const;
		const vertex& v() const; 
		property_value get_property_variant(const std::string& prop) const;
		class impl_type;
	private:
		impl_type* impl_;
	};

	class tile : public details::property_container<tile> {
		friend class tessera_impl;
	public:
		const std::vector<vertex>& vertices() const;
		const std::vector<edge>& edges() const;
		property_value get_property_variant(const std::string& prop) const;
		class impl_type;
	private:
		impl_type* impl_;
	};

}