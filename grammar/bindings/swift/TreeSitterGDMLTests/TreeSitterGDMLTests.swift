import XCTest
import SwiftTreeSitter
import TreeSitterGdml

final class TreeSitterGdmlTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_gdml())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Godot Markup Language grammar")
    }
}
