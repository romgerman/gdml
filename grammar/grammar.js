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
      $.signal_definition,
      $.variable_definition,
      $.node_definition,
      $.for_statement
    ),

    // Statements
    node_definition: $ => seq(
      field('name', $.identifier),
      optional(seq(
        '(',
        optional(seq($.function_call, repeat(seq(',', $.function_call)))),
        ')'
      )),
      '{',
      field('variables', repeat($.variable_definition)),
      field('properties', repeat(choice(
        $.property_definition,
        $.theme_property_definition
      ))),
      field('statements', repeat(
        choice($.node_definition, $.for_statement)
      )),
      '}'
    ),
    resource_definition: $ => seq(
      field('name', $.identifier),
      '{',
      field('properties', repeat($.property_definition)),
      '}'
    ),
    property_definition: $ => seq(
      field("name", choice($.signal_expression, $.identifier, $.member_expression)),
      ':',
      field("value", choice($.expression, $.resource_definition)),
      choice('\n', ';')
    ),
    theme_property_definition: $ => seq(
      '$',
      field("name", choice($.signal_expression, $.identifier, $.member_expression)),
      ':',
      field("value", choice($.expression, $.resource_definition)),
      choice('\n', ';')
    ),
    signal_definition: $ => seq("signal", field('name', $.identifier), choice('\n', ';')),
    variable_definition: $ => seq(
      "let",
      field("name", $.identifier),
      optional(field("value", seq(
        '=',
        choice($.expression, $.resource_definition)
      ))),
      choice('\n', ';')
    ),
    for_statement: $ => seq(
      "@for",
      field("iter_name", $.identifier),
      optional(seq(',', field("iter_name2", $.identifier))),
      "in",
      field("range", choice($.identifier, $.number, $.array_expression, $.member_expression)),
      '{',
      repeat($.node_definition),
      '}'
    ),

    // Expressions
    expression: $ => choice(
      $.identifier,
      $.self,
      $.number,
      $.boolean,
      $.string,
      $.array_expression,
      $.member_expression,
      $.dictionary_expression,
      $.function_call
    ),
    member_expression: $ => seq(
      choice($.self, $.identifier),
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
    signal_expression: $ => seq(
      '@',
      $.identifier
    ),
    function_call: $ => seq(
      field('name', choice(
        $.member_expression,
        $.identifier
      )),
      '(',
      field('arguments', optional($.argument_list)),
      ')'
    ),
    argument_list: $ => seq(
      seq(
        $.expression,
        repeat(seq(
          ',',
          $.expression
        ))
      )
    ),

    // Primitives
    identifier: $ => /[A-Za-z_]+[A-Za-z_0-9]*/,
    number: $ => /\d+(\.\d+)?/,
    boolean: $ => choice(token('true'), token('false')),
    self: $ => token('self'),
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
