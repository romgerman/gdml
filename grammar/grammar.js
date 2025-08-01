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

    node_definition: $ => seq(
      $.identifier,
      '{',
      repeat($.property_definition),
      repeat($.node_definition),
      '}'
    ),
    resource_definition: $ => seq(
      optional($.identifier),
      '{',
      repeat($.property_definition),
      '}'
    ),
    property_definition: $ => seq(
      choice($.identifier, $.member_expression),
      ':',
      choice($.expression, $.resource_definition),
      choice('\n', ';')
    ),

    expression: $ => choice(
      $.identifier,
      $.number,
      $.boolean,
      $.string,
      $.array_expression,
      $.member_expression
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

    // Primitives
    identifier: $ => /[A-Za-z_]+[A-Za-z_0-9]*/,
    number: $ => /\d+(.\d+)?/,
    boolean: $ => choice(token('true'), token('false')),
    string: $ => seq(
      '"',
      repeat(token.immediate(prec(1, /[^"\\\r\n]+/))),
      '"'
    ),
    comment: $ => token(seq(
      '#',
      /[^\r\n\u2028\u2029]*/
    )),
  },
});
