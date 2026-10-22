#pragma once

#include "gdml_parser.h"

#include <map>

#include <godot_cpp/variant/string.hpp>

namespace gdml {

class GDMLDocument {
	std::map<godot::String, std::unique_ptr<godot::SourceFileNode>> m_files;

public:
	const godot::SourceFileNode &load(const godot::String &p_path, const godot::String p_source) {
		auto it = m_files.find(p_path);

		if (it == m_files.end()) {
			godot::GdmlParser parser;
			it = m_files.emplace(p_path, parser.parse(p_source)).first;
		}
		
		return *it->second;
	}
};

}