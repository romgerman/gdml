/**
 * @file Gdml grammar for tree-sitter
 * @author romgerman
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "gdml",
  extras: $ => [
    $.comment,
    /[\s\p{Zs}\uFEFF\u2028\u2029\u2060\u200B]/u
  ],
  word: $ => $.identifier,
  rules: {
    source_file: $ => repeat($.definition),
    definition: $ => choice(
      $.node_definition
    ),

    // Statements
    node_definition: $ => seq(
      $.identifier,
      '{',
      repeat($.property_definition),
      repeat($.node_definition),
      '}'
    ),
    resource_definition: $ => seq(
      $.identifier,
      '{',
      repeat($.property_definition),
      '}'
    ),
    property_definition: $ => seq(
      field("name", choice($.identifier, $.member_expression)),
      ':',
      field("value", choice($.expression, $.resource_definition)),
      choice('\n', ';')
    ),

    // Expressions
    expression: $ => choice(
      $.identifier,
      $.number,
      $.boolean,
      $.string,
      $.array_expression,
      $.member_expression,
      $.dictionary_expression
    ),
    member_expression: $ => seq(
      $.identifier,
      repeat1(seq('.', $.identifier))
    ),
    array_expression: $ => seq(
      '[',
      optional(seq(
        $.expression,
        repeat(seq(
          ',',
          $.expression
        ))
      )),
      ']'
    ),
    dictionary_expression: $ => seq(
      '{',
      optional(repeat($.property_definition)),
      '}'
    ),

    // Primitives
    identifier: $ => /[A-Za-z_]+[A-Za-z_0-9]*/,
    number: $ => /\d+(.\d+)?/,
    boolean: $ => choice(token('true'), token('false')),
    string: $ => seq(
      '"',
      repeat(choice(
        $.string_fragment,
        $.escape_sequence
      )),
      '"'
    ),
    string_fragment: $ => token.immediate(prec(1, /[^"\\\r\n]+/)),
    comment: $ => token(seq(
      '#',
      /[^\r\n\u2028\u2029]*/
    )),
    escape_sequence: $ => token.immediate(seq(
      '\\',
      choice(
        /[^xu0-7]/,
        /[0-7]{1,3}/,
        /x[0-9a-fA-F]{2}/,
        /u[0-9a-fA-F]{4}/,
        /u\{[0-9a-fA-F]+\}/,
        /[\r?][\n\u2028\u2029]/,
      ),
    ))
  },
});
