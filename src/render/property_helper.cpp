#include "property_helper.h"

#include "gdml_atom.h"
#include "gdml_decorator.h"
#include "tree_renderer_context.h"
#include "variant_registry.h"
#include "decorator_cache.h"
#include "function_cache.h"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace traversal {

PropertyHelper::PropertyHelper(TreeRendererVisitorContext *ctx, gdml::DecoratorCache *dec_cache) : m_context(ctx), m_decorator_cache(dec_cache) {
	m_function_cache = std::make_shared<gdml::FunctionCache>();
}

bool traversal::PropertyHelper::get_property(godot::Variant &root, godot::String &prop_key, godot::Variant &out) {
	if (prop_key == "self") {
		out = root;
		return true;
	}

	// If it's a variable in the current scope
	if (m_context->get_scope()->has_variable(prop_key)) {
		out = m_context->get_scope()->get_variable(prop_key);
		return true;
	}

	// If it's a top-level reactive variable
	if (m_context->has_reactive_variable(prop_key)) {
		godot::Ref<reactive::GDMLAtom> atom = m_context->get_reactive_variable(prop_key);
		out = atom->get_value();
		m_context->set_active_variable(prop_key);
		return true;
	}

	bool is_valid = false;

	// If it's a property of the current variant
	const godot::Variant prop_value = root.get(prop_key, &is_valid);
	if (is_valid) {
		out = prop_value;
		return true;
	}

	// Check if it's a global script name
	godot::Dictionary class_info;
	for (const godot::Dictionary &ci : godot::ProjectSettings::get_singleton()->get_global_class_list()) {
		if (godot::String(ci.get("class", "")) == prop_key) {
			class_info = ci;
			break;
		}
	}

	if (!class_info.is_empty()) {
		godot::String path = class_info.get("path", "");
		godot::Ref<godot::Script> script = godot::ResourceLoader::get_singleton()->load(path, "Script");
		if (script.is_valid()) {
			out = script;
			return true;
		}
	}

	// Check if it's a singleton
	if (godot::Engine::get_singleton()->has_singleton(prop_key)) {
		out = godot::Engine::get_singleton()->get_singleton(prop_key);
		return true;
	}

	// Check if it's a global function
	godot::Ref<gdml::GDMLFunction> fn = m_function_cache->get(prop_key);
	if (!fn.is_null()) {
		out = fn;
		return true;
	}

	return false;
}

bool PropertyHelper::get_deep_property(godot::Variant &root, godot::MemberExpressionNode &member_expression, godot::Variant &out) {
	std::unique_ptr<godot::ExpressionNode> &expression = member_expression.expression;
	std::unique_ptr<godot::IdentifierNode> &prop_name = member_expression.identifier;
	
	if (expression->type == godot::GDMLNodeType::IDENTIFIER_EXPRESSION) {
		godot::IdentifierNode *id_expr = static_cast<godot::IdentifierNode *>(expression.get());
		const godot::StringName &value_name = prop_name->name;

		// Check if it's a constant of a basic Variant type
		if (VariantRegistry::is_builtin(id_expr->name)) {
			godot::Variant ret;
			if (VariantRegistry::get_constant(id_expr->name, value_name, ret)) {
				out = ret;
				return true;
			}
		}

		// Check if it's a class and has the next path segment as a constant
		if (godot::ClassDB::class_exists(id_expr->name)) {
			if (godot::ClassDB::class_has_integer_constant(id_expr->name, value_name)) {
				out = godot::ClassDB::class_get_integer_constant(id_expr->name, value_name);
				return true;
			}
		}
	}

	godot::Variant base_variant;

	if (!resolve(root, *expression, base_variant)) {
		return false;
	}

	bool is_valid = false;
	
	// We have a value go check next
	godot::Variant next_value = base_variant.get(prop_name->name, &is_valid);

	if (is_valid) {
		out = next_value;
		return true;
	}

	godot::Object *obj = base_variant.get_validated_object();
	if (obj) {
		const godot::String class_name = obj->get_class();

		if (godot::ClassDB::class_has_integer_constant(class_name, prop_name->name)) {
			out = godot::ClassDB::class_get_integer_constant(class_name, prop_name->name);
			return true;
		}
	}

	return false;
}

bool PropertyHelper::set_property(godot::Variant &root, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &expr) {
	godot::Variant out_value;
	if (resolve(root, expr, out_value)) {
		variant.set(prop_key, out_value);
		return true;
	} else {
		return false;
	}
}

bool PropertyHelper::set_deep_property(godot::Variant &root, godot::Variant &variant, godot::MemberExpressionNode &member_expression, godot::ExpressionNode &expr) {
	godot::Variant previous_variant;
	godot::Variant current_variant = variant;

	// TODO:
	bool is_valid = false;
	// for (auto it = path.begin(); it != path.end() - 1; ++it) {
	// 	previous_variant = current_variant;
	// 	current_variant = current_variant.get(it->get()->name, &is_valid);

	// 	if (!is_valid) {
	// 		print_error("Cannot get property " + it->get()->name + " in path");
	// 		return false;
	// 	}
	// }

	// String &variant_property = path.back()->name;
	// if (setVariantProperty(root_variant, current_variant, variant_property, value_node)) {
	// 	const String &previous_property = (path.end() - 2)->get()->name;
	// 	previous_variant.set(previous_property, current_variant, &is_valid);
	// }

	return is_valid;
}

bool PropertyHelper::resolve(godot::Variant &root, godot::ExpressionNode &expr, godot::Variant &out) {
	switch (expr.type) {
		case godot::GDMLNodeType::STRING_EXPRESSION: {
			godot::StringNode &node = static_cast<godot::StringNode &>(expr);
			out = node.value;
		} break;
		case godot::GDMLNodeType::NUMBER_EXPRESSION: {
			godot::NumberNode node = static_cast<godot::NumberNode &>(expr);
			out = godot::Variant(node.isFloat ? node.getFloat() : node.getInteger());
		} break;
		case godot::GDMLNodeType::BOOLEAN_EXPRESSION: {
			godot::BooleanNode &node = static_cast<godot::BooleanNode &>(expr);
			out = node.value;
		} break;
		case godot::GDMLNodeType::IDENTIFIER_EXPRESSION: {
			godot::IdentifierNode &node = static_cast<godot::IdentifierNode &>(expr);

			godot::Variant out_value;
			if (get_property(root, node.name, out_value)) {
				out = out_value;
			} else {
				m_context->push_error("Cannot access property '" + node.name + "'", node);
				return false;
			}
		} break;
		case godot::GDMLNodeType::ARRAY_EXPRESSION: {
			godot::ArrayExpressionNode &node = static_cast<godot::ArrayExpressionNode &>(expr);
			godot::Array out_arr;

			for (const std::unique_ptr<godot::ExpressionNode> &item_node : node.values) {
				godot::Variant out_value;
				if (resolve(root, *item_node, out_value)) {
					out_arr.push_back(out_value);
				}
			}

			out = out_arr;
		} break;
		case godot::GDMLNodeType::DICTIONARY_EXPRESSION: {
			godot::DictionaryExpressionNode &node = static_cast<godot::DictionaryExpressionNode &>(expr);
			godot::Dictionary out_dict;

			for (const std::unique_ptr<godot::PropertyDefinitionNode> &item_node : node.values) {
				if (item_node->name->type == godot::GDMLNodeType::IDENTIFIER_EXPRESSION) {
					godot::String &name = static_cast<godot::IdentifierNode *>(item_node->name.get())->name;
					godot::Variant out_value;
					if (resolve(root, *item_node->value.get(), out_value)) {
						out_dict[name] = out_value;
					}
				}
			}

			out = out_dict;
		} break;
		case godot::GDMLNodeType::MEMBER_ACCESS_EXPRESSION: {
			godot::MemberExpressionNode &node = static_cast<godot::MemberExpressionNode &>(expr);
			godot::Variant out_value;
			if (get_deep_property(root, node, out_value)) {
				out = out_value;
			} else {
				return false;
			}
		} break;
		case godot::GDMLNodeType::RESOURCE_DEFINITION: {
			godot::ResourceDefinitionNode &node = static_cast<godot::ResourceDefinitionNode &>(expr);
			godot::Variant resource_variant;

			if (VariantRegistry::is_builtin(node.name->name)) {
				if (!VariantRegistry::construct(node.name->name, resource_variant)) {
					m_context->push_error("Error constructing variant of type " + node.name->name, node);
					return false;
				}
			} else {
				if (godot::ClassDB::class_exists(node.name->name)) {
					resource_variant = godot::ClassDB::instantiate(node.name->name);
				} else {
					m_context->push_error("Error instantiating class of type " + node.name->name, node);
					return false;
				}
			}

			for (const std::unique_ptr<godot::PropertyDefinitionNode> &property : node.properties) {
				if (property->name->type == godot::GDMLNodeType::IDENTIFIER_EXPRESSION) {
					godot::IdentifierNode *identifier = static_cast<godot::IdentifierNode *>(property->name.get());
					set_property(root, resource_variant, identifier->name, *property->value.get());
				} else if (property->name->type == godot::GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
					godot::MemberExpressionNode *member_access = static_cast<godot::MemberExpressionNode *>(property->name.get());
					set_deep_property(root, resource_variant, *member_access, *property->value.get());
				}
			}

			for (const std::unique_ptr<godot::FunctionCallExpressionNode> &dec_fn : node.decorators) {
				if (dec_fn->name->type == godot::GDMLNodeType::IDENTIFIER_EXPRESSION) {
					godot::IdentifierNode *dec_fn_name = static_cast<godot::IdentifierNode *>(dec_fn->name.get());
					godot::Ref<gdml::GDMLDecorator> decorator = m_decorator_cache->store(dec_fn_name->name);

					godot::Array call_args;
					call_args.push_back(resource_variant);
					
					godot::Array args;
					for (std::unique_ptr<godot::ExpressionNode> &arg_node : dec_fn->arguments) {
						godot::Variant arg_value;
						if (resolve(root, *arg_node, arg_value)) {
							args.push_back(arg_value);
						}
					}

					call_args.push_back(args);

					if (!decorator->has_method(godot::StringName("modify_resource"))) {
						continue;
					} else {
						decorator->callv(godot::StringName("modify_resource"), call_args);
					}
				}
			}

			out = resource_variant;
		} break;
		case godot::GDMLNodeType::FUNCTION_CALL: {
			const godot::FunctionCallExpressionNode &node = (godot::FunctionCallExpressionNode &)expr;
			
			godot::Array args;
	
			for (const std::unique_ptr<godot::ExpressionNode> &e : node.arguments) {
				godot::Variant arg_value;
				if (resolve(root, *e.get(), arg_value)) {
					args.push_back(arg_value);
				} else {
					m_context->push_error("Unsupported argument type '" + godot::String::num_int64((int)e->type) + "' in function call", expr);
				}
			}

			if (node.name->type == godot::GDMLNodeType::IDENTIFIER_EXPRESSION) {
				godot::IdentifierNode *name_node = static_cast<godot::IdentifierNode *>(node.name.get());

				godot::Variant target;
				if (!resolve(root, *node.name, target)) {
					return false;
				}

				if (target.get_type() == godot::Variant::OBJECT) {
					godot::Object *target_obj = target.get_validated_object();
					if (target_obj && target_obj->is_class("GDMLFunction")) {
						out = target_obj->call(godot::StringName("execute"), args);
						return true;
					}
				}
				
				if (target.get_type() != godot::Variant::CALLABLE) {
					m_context->push_error("Cannot call non-Callable", node);
					return false;
				}

				godot::Callable callable = target;
				out = callable.callv(args);
			} else if (node.name->type == godot::GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
				godot::MemberExpressionNode *member_node = static_cast<godot::MemberExpressionNode *>(node.name.get());

				godot::Variant target;
				if (!resolve(root, *member_node->expression, target)) {
					m_context->push_error("Cannot resolve function call", node);
					return false;
				}

				godot::Object *obj = target.get_validated_object();
				if (obj) {
					out = obj->callv(member_node->identifier->name, args);
				}
			} else {
				m_context->push_error("Unsupported call target", node);
				return false;
			}
		} break;
		case godot::GDMLNodeType::BINARY_EXPRESSION: {
			godot::BinaryExpressionNode &node = static_cast<godot::BinaryExpressionNode &>(expr);
			
			godot::Variant left;
			if (resolve(root, *node.left, left)) {
				godot::Variant right;
				if (resolve(root, *node.right, right)) {
					switch (node.op)
					{
					case godot::GDMLOperator::AND:
						out = left && right;
						break;
					case godot::GDMLOperator::OR:
						out = left || right;
						break;
					case godot::GDMLOperator::PLUS:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left + (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left + (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left + (float_t)right;
						} else if (left.get_type() == godot::Variant::STRING || right.get_type() == godot::Variant::STRING) {
							out = godot::String(left) + godot::String(right);
						}
						break;
					case godot::GDMLOperator::MINUS:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left - (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left - (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left - (float_t)right;
						}
						break;
					case godot::GDMLOperator::MUL:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left * (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left * (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left * (float_t)right;
						}
						break;
					case godot::GDMLOperator::DIV:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left / (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left / (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left / (float_t)right;
						}
						break;
					case godot::GDMLOperator::LESS_THAN:
						out = left < right;
						break;
					case godot::GDMLOperator::LESS_OR_EQUAL_THAN:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left <= (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left <= (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left <= (float_t)right;
						}
						break;
					case godot::GDMLOperator::GREATER_THAN:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left > (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left > (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left > (float_t)right;
						}
						break;
					case godot::GDMLOperator::GREATER_OR_EQUAL_THAN:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left >= (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left >= (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left >= (float_t)right;
						}
						break;
					case godot::GDMLOperator::EQUAL:
						if (left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::INT)  {
							out = (int64_t)left == (int64_t)right;
						} else if (left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::FLOAT) {
							out = (float_t)left == (float_t)right;
						} else if ((left.get_type() == godot::Variant::INT && right.get_type() == godot::Variant::FLOAT) ||
									(left.get_type() == godot::Variant::FLOAT && right.get_type() == godot::Variant::INT)) {
							out = (float_t)left == (float_t)right;
						} else {
							out = left == right;
						}
						break;
					case godot::GDMLOperator::NOT_EQUAL:
						out = left != right;
						break;
					}
				} else {
					m_context->push_error("Cannot resolve right part of binary expression", node);
				}
			} else {
				m_context->push_error("Cannot resolve left part of binary expression", node);
			}

		} break;
		case godot::GDMLNodeType::SUBSCRIPT_EXPRESSION: {
			const godot::SubscriptExpressionNode &node = (godot::SubscriptExpressionNode &)expr;

			godot::Variant v_object;
			if (!resolve(root, *node.object, v_object)) {
				m_context->push_error("Cannot resolve object", node);
				return false;
			}

			godot::Variant v_index;
			if (!resolve(root, *node.index, v_index)) {
				m_context->push_error("Cannot resolve index", node);
				return false;
			}
			
			if (v_object.get_type() == godot::Variant::ARRAY) {
				godot::Array obj = v_object;
				int64_t index = v_index;
				out = obj[index];
			} else if (v_object.get_type() == godot::Variant::DICTIONARY) {
				godot::Dictionary obj = v_object;
				out = obj[v_index];
			}
		} break;
		default: {
			m_context->push_error("Cannot resolve unknown expression type", expr);
			return false;
		}
	}

	return true;
}

}
