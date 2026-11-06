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
  precedences: $ => [
    [
      'member',
      'call',
      'binary_times',
      'binary_plus',
      'binary_relation',
      'binary_equality',
      'logical_and',
      'logical_or',
    ],
  ],
  conflicts: $ => [
    [$.resource_definition, $._callee],
    [$.resource_definition, $._primary_expression]
    //[$.node_definition, $._callee]
  ],
  rules: {
    source_file: $ => seq(
      optional(repeat($.import_statement)),
      repeat($.definition)
    ),
    definition: $ => choice(
      $.signal_definition,
      $.variable_definition,
      $.component_definition,
      $.node_definition,
      $.for_statement,
      $.if_statement
    ),

    // Statements
    node_definition: $ => seq(
      field('name', $.identifier),
      field('decorators', optional(seq(
        '(',
        optional(seq($.function_call, repeat(seq(',', $.function_call)))),
        ')'
      ))),
      '{',
      field('variables', repeat($.variable_definition)),
      field('properties', repeat(choice(
        $.property_definition,
        $.theme_property_definition
      ))),
      field('statements', repeat(
        choice($.node_definition, $.for_statement, $.if_statement)
      )),
      '}'
    ),
    resource_definition: $ => seq(
      field('shared', optional(token('&'))),
      field('name', $.identifier),
      field('decorators', optional(seq(
        '(',
        optional(seq($.function_call, repeat(seq(',', $.function_call)))),
        ')'
      ))),
      '{',
      field('properties', repeat($.property_definition)),
      '}'
    ),
    property_definition: $ => seq(
      field("name", choice($.signal_expression, $.identifier, $.property_path)),
      ':',
      field("value", $.expression),
      choice('\n', ';')
    ),
    property_path: $ => seq(
      $.identifier,
      repeat1(seq(
        '.',
        $.identifier
      ))
    ),
    theme_property_definition: $ => seq(
      '$',
      field("name", $.identifier),
      ':',
      field("value", $.expression),
      choice('\n', ';')
    ),
    signal_definition: $ => seq("signal", field('name', $.identifier), choice('\n', ';')),
    variable_definition: $ => seq(
      "let",
      field("name", $.identifier),
      optional(field("value", seq(
        '=',
        $.expression
      ))),
      choice('\n', ';')
    ),
    component_definition: $ => seq(
      "component",
      field("name", $.identifier),
      optional(field("params", seq(
        '(',
        seq(
          $.identifier,
          repeat(seq(
            ',',
            $.identifier
          ))
        ),
        ')'
      ))),
      field("value", seq(
        '=',
        $.node_definition
      )),
    ),
    for_statement: $ => seq(
      "@for",
      field("iter_name", $.identifier),
      optional(seq(',', field("iter_name2", $.identifier))),
      "in",
      field("range", choice($.identifier, $.number, $.array_expression, $.member_expression)),
      '{',
      field("statements", optional($.block)),
      '}'
    ),
    if_statement: $ => seq(
      "@if",
      field("condition", $.expression),
      '{',
      field("if", optional($.block)),
      '}',
      optional(seq(
        "@else",
        '{',
        field("else", optional($.block)),
        '}'
      ))
    ),
    block: $ => repeat1($.node_definition),
    import_statement: $ => seq(
      "import",
      $.identifier,
      $.from,
      $.string
    ),

    // Expressions
    expression: $ => choice(
      $._primary_expression,
      $.member_expression,
      $.binary_expression,
      $.resource_definition
    ),
    _primary_expression: $ => choice(
      $.identifier,
      $.self,
      $.number,
      $.boolean,
      $.string,
      $.array_expression,
      $.dictionary_expression,
      $.function_call,
      $.subscript_expression,
    ),
    member_expression: $ => prec.left('member', seq(
      field('object', $.expression),
      '.',
      field('property', $.identifier)
    )),
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
    function_call: $ => prec.left('call', seq(
      field('name', $._callee),
      '(',
      field('arguments', optional($.argument_list)),
      ')'
    )),
    _callee: $ => choice(
      $.identifier,
      $.member_expression,
      $.function_call,
      $.subscript_expression
    ),
    argument_list: $ => seq(
      $.expression,
      repeat(seq(
        ',',
        $.expression
      ))
    ),
    binary_expression: $ => choice(
      ...[
        ['&&', 'logical_and'],
        ['||', 'logical_or'],
        ['+', 'binary_plus'],
        ['-', 'binary_plus'],
        ['*', 'binary_times'],
        ['/', 'binary_times'],
        ['<', "binary_relation"],
        ['<=', "binary_relation"],
        ['==', "binary_equality"],
        ['!=', "binary_equality"],
        ['>=', "binary_relation"],
        ['>', "binary_relation"],
      ].map(([operator, precedence]) => prec.left(precedence, seq(
        field("left", $.expression),
        field("operator", operator),
        field("right", $.expression)
      )))
    ),
    subscript_expression: $ => prec.left('member', seq(
      field('object', choice($.expression)),
      '[',
      field('index', $.expression),
      ']'
    )),

    // Primitives
    identifier: $ => /[A-Za-z_]+[A-Za-z_0-9]*/,
    number: $ => /\d+(\.\d+)?/,
    boolean: $ => choice(token('true'), token('false')),
    self: $ => token('self'),
    from: $ => token('from'),
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
