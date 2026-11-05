#pragma once

#include "gdml_parser.h"

#include <godot_cpp/variant/variant.hpp>

namespace gdml {
class DecoratorCache;
class FunctionCache;
}

namespace traversal {

class TreeRendererVisitorContext;

struct UntrackReactiveVariable {
	bool &m_is_untracked;
	bool m_previous;
	UntrackReactiveVariable(bool &is_untracked): m_is_untracked(is_untracked), m_previous(is_untracked) {
		is_untracked = true;
	}
	~UntrackReactiveVariable() {
		m_is_untracked = m_previous;
	}
};

class PropertyHelper {
	TreeRendererVisitorContext *m_context;
	gdml::DecoratorCache *m_decorator_cache;
	std::shared_ptr<gdml::FunctionCache> m_function_cache;
	bool m_is_untracked = false;
public:
	PropertyHelper(TreeRendererVisitorContext *ctx, gdml::DecoratorCache *dec_cache);
	bool get_property(godot::Variant &root, godot::String &prop_key, godot::Variant &out);
	bool get_deep_property(godot::Variant &root, godot::MemberExpressionNode &member_expression, godot::Variant &out);
	bool set_property(godot::Variant &root, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &expr);
	bool set_deep_property(godot::Variant &root, godot::Variant &variant, godot::PropertyPathNode &path_expr, godot::ExpressionNode &expr);
	bool set_control_theme_override(godot::Variant &root, godot::Variant &variant, godot::String &prop_key, const godot::ExpressionNode &expr);
	bool resolve(godot::Variant &root, godot::ExpressionNode &expr, godot::Variant &out);

	static bool references_self(godot::ExpressionNode &expr);
};

}
