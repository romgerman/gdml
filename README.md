# GDML - GooD Markup Language

> [!WARNING]
> This is a hobby project

A simple markup language for Godot 4+.

## Features:
- ### Reactive

    By defining top-level variables you can then re-render your UI just by changing their values
    ```coffee
    let items = [1, 2, 3, 4, 5] # Use get_variable in a script to access it

    @for i, value in items { # This will automatically re-render when "items" variable changes
        Label {
            text: value
        }
    }
    ```
- ### Components

    Define reusable parts of your UI
    ```coffee
    # Components can optionally have parameters
    component MyItem(title) = HBoxContainer {
        Label {
            text: title
        }
        Button {
            text: "OK"
        }
    }
    ```
- ### Imports

    Import your components, custom scenes or GDScript Controls in your templates
    ```coffee
    import ItemCmp from "res://ui_examples/inventory/item_component.gdml"

    # ...

    ItemCmp {}
    ```
- ### Decorators

    You can change behavior of any node by providing a custom decorator to it. This can be used to simplify templates
    ```coffee
    # Without decorator
    VBoxContainer {
        anchor_left: 0
        anchor_top: 0
        anchor_right: 1
        anchor_bottom: 1
    }

    # "anchors" is a decorator defined in GDScript
    VBoxContainer(anchors(0, 0, 1, 1)) {}
    ```
- ### Custom functions

    You can define custom functions to be called from a template
    ```coffee
    TextureRect {
        texture: load("res://icon.svg") # load is a built-in function
    }
    ```
- ### UI transitions based on Tweens

    Built-in "transition" decorator will help you to define enter/exit transitions
    ```coffee
    let trans_in

    # ...

    PanelContainer(
        transition(GDMLTransitionEnter {
            tween_factory: trans_in
        })
    ) {}
    ```
- ### Control flow

    Built-in `for` and `if` statements
    ```coffee
    let arr = [1, 3, 5]
    let dict = { "hello": "world"; }
    let enabled = true

    @for i, value in arr {
        Label {
            text: value
        }
    }

    @for key, value in dict {
        Label {
            text: key + ": " + value
        }
    }

    @if enabled {
        Button {
            text: "Hello"
        }
    }
    ```
- ### Renders in the editor

    All markup can be rendered in the editor and hot-reloaded when any of it changes when you use built-in GDMLNode
